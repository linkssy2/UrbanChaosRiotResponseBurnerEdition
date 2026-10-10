/* Separate default-OFF experiment. Filter only the selected light's render-time
 * UseSpecular load. Preserve its authored +D4 byte, all geometry, and the full
 * original material/shader/state transaction including native cleanup. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "specular_removal.h"
#include "effect_diagnostics.h"

typedef struct {
    uint64_t calls, eligible, suppressed, forwarded, rejected;
} UCSpecularRemovalCounts;
static struct {
    int enabled;
    unsigned mask;
    DWORD owner_tid;
#if !UCRR_PLAY_ONLY
    LONG owner_error;
    uint64_t birth, frequency, initialized_qpc, capture_start, capture_end;
    UCSpecularRemovalCounts counts, start_counts, end_counts;
#endif
} uc_specular;

static int uc_specular_owner(void) {
    if (GetCurrentThreadId() == uc_specular.owner_tid) return 1;
    UC_EFFECT_DIAGNOSTIC(InterlockedIncrement(&uc_specular.owner_error));
    return 0;
}

void uc_specular_removal_init(void) {
#if defined(UCRR_SPECULAR_REMOVAL)
    if (uc_specular.enabled) return;
    const char *enabled = getenv("UCRR_SPECULAR_REMOVAL");
    const char *mask = getenv("UCRR_SPECULAR_REMOVAL_MASK");
    if (!enabled || strcmp(enabled, "1") || !mask ||
        (mask[0] != '0' && mask[0] != '1') || mask[1]) return;
    uc_specular.enabled = 1;
    uc_specular.mask = (unsigned)(mask[0] - '0');
    uc_specular.owner_tid = GetCurrentThreadId();
#if !UCRR_PLAY_ONLY
    FILETIME born = {0}, dead = {0}, kernel = {0}, user = {0};
    GetProcessTimes(GetCurrentProcess(), &born, &dead, &kernel, &user);
    uc_specular.birth = ((uint64_t)born.dwHighDateTime << 32) | born.dwLowDateTime;
    LARGE_INTEGER value = {0};
    QueryPerformanceFrequency(&value); uc_specular.frequency = (uint64_t)value.QuadPart;
    value.QuadPart = 0;
    QueryPerformanceCounter(&value); uc_specular.initialized_qpc = (uint64_t)value.QuadPart;
#endif
#endif
}

int uc_specular_removal_enabled(void) { return uc_specular.enabled; }

uint8_t uc_specular_removal_filter(uint8_t authored, uint32_t object_va, uint32_t caller_return_va) {
#if defined(UCRR_SPECULAR_REMOVAL)
    if (!uc_specular.enabled || !uc_specular_owner()) return authored;
#if UCRR_PLAY_ONLY
    if (!uc_specular.mask || !authored) return authored;
#endif
    UC_EFFECT_DIAGNOSTIC(++uc_specular.counts.calls);
    if (caller_return_va != 0x00090630u || object_va < 0x10000u ||
        (object_va & 3u) || (uint64_t)object_va + 0x220u > (uint64_t)g_xbox_total_ram ||
        MEM32(object_va) != 0x004573F0u) {
        UC_EFFECT_DIAGNOSTIC(++uc_specular.counts.rejected);
        UC_EFFECT_DIAGNOSTIC(++uc_specular.counts.forwarded);
        return authored;
    }
    if (authored) UC_EFFECT_DIAGNOSTIC(++uc_specular.counts.eligible);
    if (authored && uc_specular.mask) {
        UC_EFFECT_DIAGNOSTIC(++uc_specular.counts.suppressed);
        return 0;
    }
    UC_EFFECT_DIAGNOSTIC(++uc_specular.counts.forwarded);
#else
    (void)object_va; (void)caller_return_va;
#endif
    return authored;
}

#if !UCRR_PLAY_ONLY
static void uc_specular_write_counts(FILE *file, const UCSpecularRemovalCounts *c) {
    fprintf(file, "{\"calls\":%llu,\"eligible\":%llu,\"suppressed\":%llu,\"forwarded\":%llu,\"rejected\":%llu}",
        (unsigned long long)c->calls, (unsigned long long)c->eligible,
        (unsigned long long)c->suppressed, (unsigned long long)c->forwarded,
        (unsigned long long)c->rejected);
}

/* Immutable mode. Called after quiet recorder completion, before its sentinel. */
static void uc_specular_snapshot(void) {
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!length || length >= MAX_PATH || !slash || slash - path > MAX_PATH - 65) return;
    swprintf(slash + 1, MAX_PATH - (slash + 1 - path), L"specular-removal-%lu.json", GetCurrentProcessId());
    LARGE_INTEGER now = {0}; QueryPerformanceCounter(&now);
    FILE *file = _wfopen(path, L"wb");
    if (!file) return;
    fprintf(file, "{\"schema\":1,\"experiment\":\"point-light-use-specular\",\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"mask\":%u,\"immutable_mode\":true,\"owner_tid\":%lu,\"owner_error\":%ld,\"qpc_frequency\":%llu,\"initialized_qpc\":%llu,\"snapshot_qpc\":%llu,\"counters\":",
        GetCurrentProcessId(), (unsigned long long)uc_specular.birth, uc_specular.mask,
        uc_specular.owner_tid, uc_specular.owner_error, (unsigned long long)uc_specular.frequency,
        (unsigned long long)uc_specular.initialized_qpc, (unsigned long long)now.QuadPart);
    uc_specular_write_counts(file, &uc_specular.counts);
    fprintf(file, ",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",
        (unsigned long long)uc_specular.capture_start, (unsigned long long)uc_specular.capture_end);
    uc_specular_write_counts(file, &uc_specular.start_counts);
    fputs(",\"end_counts\":", file); uc_specular_write_counts(file, &uc_specular.end_counts);
    fputs("}}\n", file); fclose(file);
}

void uc_specular_removal_capture_boundary(int start, uint64_t qpc) {
    if (!uc_specular.enabled || !uc_specular_owner()) return;
    if (start) {
        uc_specular.capture_start = qpc;
        uc_specular.start_counts = uc_specular.counts;
    } else {
        uc_specular.capture_end = qpc;
        uc_specular.end_counts = uc_specular.counts;
        uc_specular_snapshot();
    }
}
#else
void uc_specular_removal_capture_boundary(int start, uint64_t qpc) {
    (void)start; (void)qpc;
}
#endif
