/* Separate, default-OFF feature-removal experiment. Run the original casts-
 * shadows getter first. Its original caller owns the empty-tree branch, normal
 * lighting, stencil cleanup and tree release. No particle-removal state changes. */
#include "shadow_removal.h"
#include "effect_diagnostics.h"

#if UCRR_SHADOW_REMOVAL
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "graphics_features.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"

typedef struct {
    uint64_t calls, eligible, suppressed, forwarded, rejected;
    uint64_t caster_calls, caster_rejected;
} UCShadowRemovalCounts;

static struct {
    int enabled;
    unsigned mask;
    DWORD owner_tid;
#if !UCRR_PLAY_ONLY
    LONG owner_error;
    uint64_t birth, frequency, initialized_qpc, capture_start, capture_end;
    UCShadowRemovalCounts counts, start_counts, end_counts;
#endif
} uc_shadow;

static int uc_shadow_owner(void) {
    if (GetCurrentThreadId() == uc_shadow.owner_tid) return 1;
    UC_EFFECT_DIAGNOSTIC(InterlockedIncrement(&uc_shadow.owner_error));
    return 0;
}

/* The original getter reads object+75h. Admission requires its vtable and the
 * whole prefix to be in initialized ordinary RAM, with no wrapped guest span. */
static int uc_shadow_object(uint32_t object) {
    return object >= 0x10000u && !(object & 3u) &&
        (uint64_t)object + 0x76u <= (uint64_t)g_xbox_total_ram &&
        MEM32(object) == 0x004573F0u;
}

void uc_shadow_removal_init(void) {
    const char *enabled = getenv("UCRR_SHADOW_REMOVAL");
    const char *mask = getenv("UCRR_SHADOW_REMOVAL_MASK");
    if (!enabled || strcmp(enabled, "1") || !mask ||
        (mask[0] != '0' && mask[0] != '1') || mask[1]) return;
    /* Initialization happens once, before guest execution. No runtime setter. */
    if (uc_shadow.enabled) return;
    uc_shadow.enabled = 1;
    uc_shadow.mask = (unsigned)(mask[0] - '0');
    if(uc_graphics_features_enabled())uc_shadow.mask=!(uc_graphics_features_load()&16);
    uc_shadow.owner_tid = GetCurrentThreadId();
#if !UCRR_PLAY_ONLY
    FILETIME born, dead, kernel, user;
    memset(&born, 0, sizeof(born));
    GetProcessTimes(GetCurrentProcess(), &born, &dead, &kernel, &user);
    uc_shadow.birth = ((uint64_t)born.dwHighDateTime << 32) | born.dwLowDateTime;
    LARGE_INTEGER value;
    value.QuadPart = 0; QueryPerformanceFrequency(&value);
    uc_shadow.frequency = (uint64_t)value.QuadPart;
    value.QuadPart = 0; QueryPerformanceCounter(&value);
    uc_shadow.initialized_qpc = (uint64_t)value.QuadPart;
#endif
}

int uc_shadow_removal_enabled(void) { return uc_shadow.enabled; }
int uc_shadow_visibility_ready(void){return uc_shadow.enabled&&uc_shadow_owner();}
void uc_shadow_visibility_apply(unsigned value){uc_shadow.mask=!(value&16u);}

uint8_t uc_shadow_removal_filter(uint8_t original, uint32_t object, uint32_t caller) {
    if (!uc_shadow.enabled || !uc_shadow_owner()) return original;
#if UCRR_PLAY_ONLY
    if (!uc_shadow.mask || !original) return original;
#endif
    UC_EFFECT_DIAGNOSTIC(++uc_shadow.counts.calls);
    if (caller != 0x00098EB1u || !uc_shadow_object(object)) {
        UC_EFFECT_DIAGNOSTIC(++uc_shadow.counts.rejected);
        UC_EFFECT_DIAGNOSTIC(++uc_shadow.counts.forwarded);
        return original;
    }
    if (original) UC_EFFECT_DIAGNOSTIC(++uc_shadow.counts.eligible);
    if (!uc_shadow.mask || !original) {
        UC_EFFECT_DIAGNOSTIC(++uc_shadow.counts.forwarded);
        return original;
    }
    UC_EFFECT_DIAGNOSTIC(++uc_shadow.counts.suppressed);
    return 0;
}

void uc_shadow_removal_caster(uint32_t object, uint32_t caller) {
#if !UCRR_PLAY_ONLY

    if (!uc_shadow.enabled || !uc_shadow_owner()) return;
    if (caller != 0x0009902Cu || !uc_shadow_object(object)) {
        UC_EFFECT_DIAGNOSTIC(++uc_shadow.counts.caster_rejected);
        return;
    }
    UC_EFFECT_DIAGNOSTIC(++uc_shadow.counts.caster_calls);

#else
    (void)object; (void)caller;
#endif
}

#if !UCRR_PLAY_ONLY
static void uc_shadow_write_counts(FILE *file, const UCShadowRemovalCounts *c) {
    fprintf(file, "{\"calls\":%llu,\"eligible\":%llu,\"suppressed\":%llu,\"forwarded\":%llu,\"rejected\":%llu,\"caster_calls\":%llu,\"caster_rejected\":%llu}",
        (unsigned long long)c->calls, (unsigned long long)c->eligible,
        (unsigned long long)c->suppressed, (unsigned long long)c->forwarded,
        (unsigned long long)c->rejected, (unsigned long long)c->caster_calls,
        (unsigned long long)c->caster_rejected);
}

/* Export only at the recorder's safe end boundary, before its completion
 * sentinel. No per-call clock, log, mode change, or guest-memory mutation. */
static void uc_shadow_snapshot(void) {
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return;
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!slash || slash - path > MAX_PATH - 65) return;
    swprintf(slash + 1, MAX_PATH - (slash + 1 - path), L"shadow-removal-%lu.json", GetCurrentProcessId());
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    FILE *file = _wfopen(path, L"wb");
    if (!file) return;
    fprintf(file, "{\"schema\":1,\"experiment\":\"stencil-shadow-casts\",\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"mask\":%u,\"immutable_mode\":true,\"owner_tid\":%lu,\"owner_error\":%ld,\"qpc_frequency\":%llu,\"initialized_qpc\":%llu,\"snapshot_qpc\":%llu,\"counters\":",
        GetCurrentProcessId(), (unsigned long long)uc_shadow.birth, uc_shadow.mask,
        uc_shadow.owner_tid, uc_shadow.owner_error, (unsigned long long)uc_shadow.frequency,
        (unsigned long long)uc_shadow.initialized_qpc, (unsigned long long)now.QuadPart);
    uc_shadow_write_counts(file, &uc_shadow.counts);
    fprintf(file, ",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",
        (unsigned long long)uc_shadow.capture_start, (unsigned long long)uc_shadow.capture_end);
    uc_shadow_write_counts(file, &uc_shadow.start_counts);
    fputs(",\"end_counts\":", file); uc_shadow_write_counts(file, &uc_shadow.end_counts);
    fputs("}}\n", file); fclose(file);
}

void uc_shadow_removal_capture_boundary(int start, uint64_t qpc) {
    if (!uc_shadow.enabled || !uc_shadow_owner()) return;
    if (start) {
        uc_shadow.capture_start = qpc;
        uc_shadow.start_counts = uc_shadow.counts;
    } else {
        uc_shadow.capture_end = qpc;
        uc_shadow.end_counts = uc_shadow.counts;
        uc_shadow_snapshot();
    }
}
#else
void uc_shadow_removal_capture_boundary(int start, uint64_t qpc) {
    (void)start; (void)qpc;
}
#endif
#else
void uc_shadow_removal_init(void) {}
int uc_shadow_visibility_ready(void){return 0;}
void uc_shadow_visibility_apply(unsigned value){(void)value;}
int uc_shadow_removal_enabled(void) { return 0; }
uint8_t uc_shadow_removal_filter(uint8_t original, uint32_t object, uint32_t caller) {
    (void)object; (void)caller; return original;
}
void uc_shadow_removal_caster(uint32_t object, uint32_t caller) {
    (void)object; (void)caller;
}
void uc_shadow_removal_capture_boundary(int start, uint64_t qpc) {
    (void)start; (void)qpc;
}
#endif
