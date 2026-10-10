/* Separate default-OFF glow blur/composite omission. See the scoped contract
 * in analysis/system_audit/20260928-glow-removal-boundary.md. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "glow_removal.h"
#include "effect_diagnostics.h"

#if !UCRR_PLAY_ONLY
static const char *const uc_glow_reasons[] = {
    "producer", "graphics_owner", "post_owner", "active_callbacks", "viewport_manager", "tree_sentinel", "tree_walk", "viewport_key", "multiple_viewports", "viewport_type_or_mode", "dimensions_or_exclusive", "scissor", "write_mask"
};
#endif
#define UC_GLOW_REASON_COUNT 13
typedef struct {
    uint64_t calls, eligible, suppressed, forwarded, rejected;
    uint64_t rgb_eligible;
    uint64_t rejection_reason[UC_GLOW_REASON_COUNT];
} UCGlowRemovalCounts;
static struct {
    int enabled, two_views, rgb_only;
    unsigned mask;
    DWORD owner_tid;
#if !UCRR_PLAY_ONLY
    LONG owner_error;
    uint64_t birth, frequency, initialized_qpc, capture_start, capture_end;
    UCGlowRemovalCounts counts, start_counts, end_counts;
#endif
} uc_glow;

static int uc_glow_owner(void) {
    if (GetCurrentThreadId() == uc_glow.owner_tid) return 1;
    UC_EFFECT_DIAGNOSTIC(InterlockedIncrement(&uc_glow.owner_error));
    return 0;
}

void uc_glow_removal_init(void) {
#if defined(UCRR_GLOW_REMOVAL)
    if (uc_glow.enabled) return;
    const char *enabled = getenv("UCRR_GLOW_REMOVAL");
    const char *mask = getenv("UCRR_GLOW_REMOVAL_MASK");
    if (!enabled || strcmp(enabled, "1") || !mask ||
        (mask[0] != '0' && mask[0] != '1') || mask[1]) return;
    uc_glow.enabled = 1;
    uc_glow.mask = (unsigned)(mask[0] - '0');
    const char *two = getenv("UCRR_GLOW_TWO_VIEW_EXPERIMENT");
    uc_glow.two_views = two && !strcmp(two, "1");
    const char *rgb = getenv("UCRR_GLOW_RGB_EXPERIMENT");
    uc_glow.rgb_only = rgb && !strcmp(rgb, "1");
    uc_glow.owner_tid = GetCurrentThreadId();
#if !UCRR_PLAY_ONLY
    FILETIME born = {0}, dead = {0}, kernel = {0}, user = {0};
    GetProcessTimes(GetCurrentProcess(), &born, &dead, &kernel, &user);
    uc_glow.birth = ((uint64_t)born.dwHighDateTime << 32) | born.dwLowDateTime;
    LARGE_INTEGER value = {0};
    QueryPerformanceFrequency(&value); uc_glow.frequency = (uint64_t)value.QuadPart;
    value.QuadPart = 0;
    QueryPerformanceCounter(&value); uc_glow.initialized_qpc = (uint64_t)value.QuadPart;
#endif
#endif
}

int uc_glow_removal_enabled(void) { return uc_glow.enabled; }

static int uc_glow_ram(uint32_t va, uint32_t size) {
    return va >= 0x10000u && !(va & 3u) && (uint64_t)va + size <= (uint64_t)g_xbox_total_ram;
}

/* Narrow first implementation: retain native EC downsample, omit its private
 * blur and scene additive composite only. Keep conservative single-view and
 * inactive-callback scope. Scratch coverage is established by native target
 * binding, which resets viewport/clip before each retained scratch producer. */
int uc_glow_removal_filter(uint32_t object_va, uint32_t caller_return_va,
                          uint32_t scissor_count, uint32_t exclusive,
                          uint32_t rect_va, uint32_t width, uint32_t height) {
#if defined(UCRR_GLOW_REMOVAL)
    if (!uc_glow.enabled || !uc_glow_owner()) return 0;
#if UCRR_PLAY_ONLY
    if (!uc_glow.mask) return 0;
#endif
    UC_EFFECT_DIAGNOSTIC(++uc_glow.counts.calls);
#if !UCRR_PLAY_ONLY
    unsigned reason = 0;
#endif
    if (caller_return_va != 0x000997EBu || !uc_glow_ram(object_va, 0x50u) ||
        MEM32(object_va) != 0x00456E30u) goto rejected;
    UC_EFFECT_DIAGNOSTIC(reason = 1);
    uint32_t graphics = MEM32(0x519F60u);
    if (!uc_glow_ram(graphics, 0x4Cu) || MEM32(graphics) != 0x004561F8u ||
        MEM32(graphics + 0x2Cu) != object_va) goto rejected;
    UC_EFFECT_DIAGNOSTIC(reason = 2);
    uint32_t post = MEM32(graphics + 0x28u);
    if (!uc_glow_ram(post, 0x14u) || MEM32(post) != 0x004561D4u) goto rejected;
    UC_EFFECT_DIAGNOSTIC(reason = 3);
    uint32_t head = MEM32(post + 8u);
    if (!uc_glow_ram(head, 0x18u) || MEM32(head) != head || MEM32(post + 0xCu)) goto rejected;
    UC_EFFECT_DIAGNOSTIC(reason = 4);
    uint32_t manager = MEM32(graphics + 0x48u);
    if (!uc_glow_ram(manager, 0x90u) || MEM32(manager) != 0x00456308u) goto rejected;
    UC_EFFECT_DIAGNOSTIC(reason = 5);
    uint32_t sentinel = MEM32(manager + 0x88u);
    if (!uc_glow_ram(sentinel, 0x18u) || !MEM8(sentinel + 0x15u)) goto rejected;
    UC_EFFECT_DIAGNOSTIC(reason = 6);
    uint32_t node = MEM32(sentinel + 4u), found = sentinel;
    unsigned depth = 0;
    while (1) {
        if (!uc_glow_ram(node, 0x18u) || ++depth > 64u) goto rejected;
        if (MEM8(node + 0x15u)) break;
        /* Original 1F9400 unsigned lower_bound for key zero always goes left. */
        found = node;
        node = MEM32(node);
    }
    UC_EFFECT_DIAGNOSTIC(reason = 7);
    if (found == sentinel || MEM32(found + 0xCu) != 0) goto rejected;
    UC_EFFECT_DIAGNOSTIC(reason = 8);
    /* The original continuation looks up key zero, not every stored viewport.
     * Default keeps the old singleton contract. The separate experiment accepts
     * only the observed two-node topology and known key-2 interface; no arbitrary
     * tree, node mutation, viewport unregistering or parameter edits. */
    if (MEM32(sentinel) != found || MEM32(sentinel + 4u) != found ||
        MEM32(sentinel + 8u) != found || MEM32(found) != sentinel ||
        MEM32(found + 4u) != sentinel || MEM32(found + 8u) != sentinel) {
        uint32_t other = MEM32(found + 8u);
        if (!uc_glow.two_views || !uc_glow_ram(other, 0x18u) || other == found || other == sentinel ||
            MEM32(sentinel) != found || MEM32(sentinel + 4u) != found ||
            MEM32(sentinel + 8u) != other || MEM32(found) != sentinel ||
            MEM32(found + 4u) != sentinel || MEM32(other) != sentinel ||
            MEM32(other + 4u) != found || MEM32(other + 8u) != sentinel ||
            MEM32(other + 0xCu) != 2u || MEM8(other + 0x15u)) goto rejected;
        uint32_t other_view = MEM32(other + 0x10u);
        if (!uc_glow_ram(other_view, 0x60u) ||
            MEM32(other_view + 8u) != 0x00456750u || MEM8(other_view + 0x5Du)) goto rejected;
    }
    UC_EFFECT_DIAGNOSTIC(reason = 9);
    uint32_t viewport = MEM32(found + 0x10u);
    if (!uc_glow_ram(viewport, 0x60u) || MEM32(viewport + 8u) != 0x00456750u ||
        MEM8(viewport + 8u + 0x55u)) goto rejected;
    UC_EFFECT_DIAGNOSTIC(reason = 10);
    if (!width || !height || width > 4096u || height > 4096u || exclusive) goto rejected;
    UC_EFFECT_DIAGNOSTIC(reason = 11);
    if (scissor_count) {
        if (scissor_count != 1u || !uc_glow_ram(rect_va, 16u) ||
            (int32_t)MEM32(rect_va) > 0 || (int32_t)MEM32(rect_va + 4u) > 0 ||
            (int32_t)MEM32(rect_va + 8u) < (int32_t)width ||
            (int32_t)MEM32(rect_va + 12u) < (int32_t)height) goto rejected;
    }
    UC_EFFECT_DIAGNOSTIC(reason = 12);
    uint32_t write_mask = MEM32(0x3F40ACu);
    /* RGB-only preserves EC alpha throughout native threshold/filter draws.
     * F0 is fully redefined from EC by the next filter before being sampled.
     * This remains a separate omission experiment, never a mask override. */
    if (write_mask != 0x01010101u && !(uc_glow.rgb_only && write_mask == 0x00010101u)) goto rejected;
    if (write_mask == 0x00010101u) UC_EFFECT_DIAGNOSTIC(++uc_glow.counts.rgb_eligible);
    UC_EFFECT_DIAGNOSTIC(++uc_glow.counts.eligible);
    if (uc_glow.mask) { UC_EFFECT_DIAGNOSTIC(++uc_glow.counts.suppressed); return 1; }
    UC_EFFECT_DIAGNOSTIC(++uc_glow.counts.forwarded);
    return 0;
rejected:
    UC_EFFECT_DIAGNOSTIC(++uc_glow.counts.rejection_reason[reason]);
    UC_EFFECT_DIAGNOSTIC(++uc_glow.counts.rejected);
    UC_EFFECT_DIAGNOSTIC(++uc_glow.counts.forwarded);
#else
    (void)object_va; (void)caller_return_va; (void)scissor_count; (void)exclusive;
    (void)rect_va; (void)width; (void)height;
#endif
    return 0;
}

#if !UCRR_PLAY_ONLY
static void uc_glow_write_counts(FILE *file, const UCGlowRemovalCounts *c) {
    fprintf(file, "{\"calls\":%llu,\"eligible\":%llu,\"suppressed\":%llu,\"forwarded\":%llu,\"rejected\":%llu",
        (unsigned long long)c->calls, (unsigned long long)c->eligible,
        (unsigned long long)c->suppressed, (unsigned long long)c->forwarded,
        (unsigned long long)c->rejected);
    fprintf(file, ",\"rgb_eligible\":%llu", (unsigned long long)c->rgb_eligible);
    fputs(",\"rejection_reasons\":{", file);
    for (unsigned i = 0; i < UC_GLOW_REASON_COUNT; ++i)
        fprintf(file, "%s\"%s\":%llu", i ? "," : "", uc_glow_reasons[i],
            (unsigned long long)c->rejection_reason[i]);
    fputs("}}", file);
}

/* Immutable mode. Called after quiet recorder completion, before its sentinel. */
static void uc_glow_snapshot(void) {
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!length || length >= MAX_PATH || !slash || slash - path > MAX_PATH - 65) return;
    swprintf(slash + 1, MAX_PATH - (slash + 1 - path), L"glow-removal-%lu.json", GetCurrentProcessId());
    LARGE_INTEGER now = {0}; QueryPerformanceCounter(&now);
    FILE *file = _wfopen(path, L"wb");
    if (!file) return;
    fprintf(file, "{\"schema\":2,\"experiment\":\"glow-filter-and-composite\",\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"mask\":%u,\"two_view_experiment\":%d,\"immutable_mode\":true,\"owner_tid\":%lu,\"owner_error\":%ld,\"qpc_frequency\":%llu,\"initialized_qpc\":%llu,\"snapshot_qpc\":%llu,\"counters\":",
        GetCurrentProcessId(), (unsigned long long)uc_glow.birth, uc_glow.mask, uc_glow.two_views,
        uc_glow.owner_tid, uc_glow.owner_error, (unsigned long long)uc_glow.frequency,
        (unsigned long long)uc_glow.initialized_qpc, (unsigned long long)now.QuadPart);
    uc_glow_write_counts(file, &uc_glow.counts);
    fprintf(file, ",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",
        (unsigned long long)uc_glow.capture_start, (unsigned long long)uc_glow.capture_end);
    uc_glow_write_counts(file, &uc_glow.start_counts);
    fputs(",\"end_counts\":", file); uc_glow_write_counts(file, &uc_glow.end_counts);
    fputs("}}\n", file); fclose(file);
}

void uc_glow_removal_capture_boundary(int start, uint64_t qpc) {
    if (!uc_glow.enabled || !uc_glow_owner()) return;
    if (start) {
        uc_glow.capture_start = qpc;
        uc_glow.start_counts = uc_glow.counts;
    } else {
        uc_glow.capture_end = qpc;
        uc_glow.end_counts = uc_glow.counts;
        uc_glow_snapshot();
    }
}
#else
void uc_glow_removal_capture_boundary(int start, uint64_t qpc) {
    (void)start; (void)qpc;
}
#endif
