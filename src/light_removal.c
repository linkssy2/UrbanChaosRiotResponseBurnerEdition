/* Separate, default-off experiment. The derived original light begin routine
 * reaches this gate only after its original transform/debug/eligibility work,
 * before any successful-begin texture/state edits. Its own false epilogue and
 * caller's next-light branch remain responsible for control flow. */
#include "light_removal.h"
#include "effect_diagnostics.h"

#if UCRR_LIGHT_REMOVAL
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "recomp_types.h"
#include "xbox_memory_layout.h"

typedef struct {
    uint64_t calls, suppressed, forwarded, rejected;
} UCLightRemovalCounts;
typedef struct { uint64_t qpc; unsigned mask; } UCLightSettingEvent;
static struct {
    int enabled;
    unsigned mask;
    DWORD owner_tid;
    int live_setting;
    LONG requested_lighting, active_lighting;
#if !UCRR_PLAY_ONLY
    LONG owner_error, requests;
    uint64_t birth, frequency, initialized_qpc, capture_start, capture_end;
    UCLightRemovalCounts counts, start_counts, end_counts;
    uint64_t changes;
    unsigned event_count, event_overflow;
    UCLightSettingEvent events[32];
#endif
} uc_light;

static int uc_light_owner(void) {
    if (GetCurrentThreadId() == uc_light.owner_tid) return 1;
    UC_EFFECT_DIAGNOSTIC(InterlockedIncrement(&uc_light.owner_error));
    return 0;
}

void uc_light_removal_init(void) {
    if (uc_light.enabled) return;
    const char *enabled = getenv("UCRR_LIGHT_REMOVAL");
    const char *mask = getenv("UCRR_LIGHT_REMOVAL_MASK");
    if (!enabled || strcmp(enabled, "1") || !mask ||
        (mask[0] != '0' && mask[0] != '1') || mask[1]) return;
    uc_light.enabled = 1;
    uc_light.mask = (unsigned)(mask[0] - '0');
    const char *live = getenv("UCRR_LIGHT_REMOVAL_LIVE");
    uc_light.live_setting = live && !strcmp(live, "1");
    uc_light.requested_lighting = uc_light.active_lighting = !uc_light.mask;
    uc_light.owner_tid = GetCurrentThreadId();
#if !UCRR_PLAY_ONLY
    FILETIME born, dead, kernel, user;
    memset(&born, 0, sizeof(born));
    GetProcessTimes(GetCurrentProcess(), &born, &dead, &kernel, &user);
    uc_light.birth = ((uint64_t)born.dwHighDateTime << 32) | born.dwLowDateTime;
    LARGE_INTEGER value;
    value.QuadPart = 0; QueryPerformanceFrequency(&value); uc_light.frequency = (uint64_t)value.QuadPart;
    value.QuadPart = 0; QueryPerformanceCounter(&value); uc_light.initialized_qpc = (uint64_t)value.QuadPart;
#endif
}

int uc_light_removal_enabled(void) { return uc_light.enabled; }

int uc_light_removal_live_toggle_enabled(void) {
    return uc_light.enabled && uc_light.live_setting;
}
int uc_light_removal_requested_lighting(void) {
    return !uc_light.enabled || InterlockedCompareExchange(&uc_light.requested_lighting, 0, 0) != 0;
}
int uc_light_removal_active_lighting(void) {
    return !uc_light.enabled || InterlockedCompareExchange(&uc_light.active_lighting, 0, 0) != 0;
}
int uc_light_removal_request_lighting(int on) {
    if (!uc_light_removal_live_toggle_enabled() || (on != 0 && on != 1)) return 0;
    InterlockedExchange(&uc_light.requested_lighting, on);
    UC_EFFECT_DIAGNOSTIC(InterlockedIncrement(&uc_light.requests));
    return 1;
}
void uc_light_removal_apply_pending(void) {
    if (!uc_light_removal_live_toggle_enabled() || !uc_light_owner()) return;
    unsigned mask = !InterlockedCompareExchange(&uc_light.requested_lighting, 0, 0);
    if (mask == uc_light.mask) return;
    /* The owner is between complete guest frames. Every Begin within the next
     * frame sees this fixed mask; request_lighting never edits it directly. */
    uc_light.mask = mask;
    InterlockedExchange(&uc_light.active_lighting, !mask);
#if !UCRR_PLAY_ONLY
    ++uc_light.changes;
    if (uc_light.event_count < 32) {
        LARGE_INTEGER now; now.QuadPart = 0; QueryPerformanceCounter(&now);
        uc_light.events[uc_light.event_count++] = (UCLightSettingEvent){(uint64_t)now.QuadPart, mask};
    } else ++uc_light.event_overflow;
#endif
}

int uc_light_removal_admit(uint32_t object_va, uint32_t caller_return_va) {
    if (!uc_light.enabled || !uc_light_owner()) return 0;
#if UCRR_PLAY_ONLY
    if (!uc_light.mask) return 0;
#endif
    UC_EFFECT_DIAGNOSTIC(++uc_light.counts.calls);
    if (caller_return_va != 0x00098E6Du || object_va < 0x10000u ||
        (object_va & 3u) || (uint64_t)object_va + 0x214u > (uint64_t)g_xbox_total_ram ||
        MEM32(object_va) != 0x004573F0u) {
        UC_EFFECT_DIAGNOSTIC(++uc_light.counts.rejected);
        UC_EFFECT_DIAGNOSTIC(++uc_light.counts.forwarded);
        return 0;
    }
    if (!uc_light.mask) {
        UC_EFFECT_DIAGNOSTIC(++uc_light.counts.forwarded);
        return 0;
    }
    UC_EFFECT_DIAGNOSTIC(++uc_light.counts.suppressed);
    return 1;
}

#if !UCRR_PLAY_ONLY
static void uc_light_write_counts(FILE *file, const UCLightRemovalCounts *c) {
    fprintf(file, "{\"calls\":%llu,\"suppressed\":%llu,\"forwarded\":%llu,\"rejected\":%llu}",
        (unsigned long long)c->calls, (unsigned long long)c->suppressed,
        (unsigned long long)c->forwarded, (unsigned long long)c->rejected);
}

/* Legacy experiments remain immutable. Opt-in user settings are explicitly
 * marked mutable so strict comparison analyzers cannot accept them by mistake.
 * Export only after the recorder has stopped, before its completion sentinel. */
static void uc_light_snapshot(void) {
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return;
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!slash || slash - path > MAX_PATH - 65) return;
    swprintf(slash + 1, MAX_PATH - (slash + 1 - path), L"light-removal-%lu.json", GetCurrentProcessId());
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    FILE *file = _wfopen(path, L"wb");
    if (!file) return;
    fprintf(file, "{\"schema\":1,\"experiment\":\"dynamic-light-begin\",\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"mask\":%u,\"immutable_mode\":%s,\"owner_tid\":%lu,\"owner_error\":%ld,\"qpc_frequency\":%llu,\"initialized_qpc\":%llu,\"snapshot_qpc\":%llu,\"counters\":",
        GetCurrentProcessId(), (unsigned long long)uc_light.birth, uc_light.mask,
        uc_light.live_setting ? "false" : "true", uc_light.owner_tid, uc_light.owner_error, (unsigned long long)uc_light.frequency,
        (unsigned long long)uc_light.initialized_qpc, (unsigned long long)now.QuadPart);
    uc_light_write_counts(file, &uc_light.counts);
    fprintf(file, ",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",
        (unsigned long long)uc_light.capture_start, (unsigned long long)uc_light.capture_end);
    uc_light_write_counts(file, &uc_light.start_counts);
    fputs(",\"end_counts\":", file); uc_light_write_counts(file, &uc_light.end_counts);
    fputs("}", file);
    if (uc_light.live_setting) {
        fprintf(file, ",\"live_setting\":{\"requested_lighting\":%s,\"active_lighting\":%s,\"requests\":%lu,\"changes\":%llu,\"event_overflow\":%u,\"applied_events\":[",
            uc_light_removal_requested_lighting() ? "true" : "false",
            uc_light_removal_active_lighting() ? "true" : "false",
            (unsigned long)InterlockedCompareExchange(&uc_light.requests,0,0),
            (unsigned long long)uc_light.changes, uc_light.event_overflow);
        for (unsigned i=0;i<uc_light.event_count;++i) {
            fprintf(file, "%s{\"qpc\":%llu,\"mask\":%u}", i ? "," : "",
                (unsigned long long)uc_light.events[i].qpc, uc_light.events[i].mask);
        }
        fputs("]}",file);
    }
    fputs("}\n", file); fclose(file);
}

void uc_light_removal_capture_boundary(int start, uint64_t qpc) {
    if (!uc_light.enabled || !uc_light_owner()) return;
    if (start) {
        uc_light.capture_start = qpc;
        uc_light.start_counts = uc_light.counts;
    } else {
        uc_light.capture_end = qpc;
        uc_light.end_counts = uc_light.counts;
        uc_light_snapshot();
    }
}
#else
void uc_light_removal_capture_boundary(int start, uint64_t qpc) {
    (void)start; (void)qpc;
}
#endif
#else
void uc_light_removal_init(void) {}
int uc_light_removal_enabled(void) { return 0; }
int uc_light_removal_live_toggle_enabled(void) { return 0; }
int uc_light_removal_request_lighting(int on) { (void)on; return 0; }
int uc_light_removal_requested_lighting(void) { return 1; }
int uc_light_removal_active_lighting(void) { return 1; }
void uc_light_removal_apply_pending(void) {}
int uc_light_removal_admit(uint32_t object, uint32_t caller) {
    (void)object; (void)caller; return 0;
}
void uc_light_removal_capture_boundary(int start, uint64_t qpc) {
    (void)start; (void)qpc;
}
#endif
