/* Exact, bounded memoization of the original receiver-matrix inverse. This
 * preserves lighting and the original order of every guest callback/command.
 * Cache values are the actual original leaf's complete changed state, not a
 * replacement matrix algorithm. No saved guest pointer is dereferenced later. */
#include "light_restoration.h"
#include "recomp_types.h"
void sub_002D4360(void);

#if UCRR_LIGHT_RESTORATION
#include <windows.h>
#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "xbox_memory_layout.h"

#define UC_LR_ENTRIES 64u
typedef struct {
    uint32_t valid, mxcsr_before, constant_bits, mxcsr_after;
    uint32_t input[16], output[16], scratch[48];
    RecompXmm vectors[8];
} UCLightInverseEntry;
static struct {
    LONG initialized, owner_error;
    int enabled;
    unsigned mask;
    DWORD owner_tid;
    uint64_t birth, frequency, initialized_qpc, capture_start, capture_end;
    UCLightRestorationCounts counts, start_counts, end_counts;
    UCLightInverseEntry entries[UC_LR_ENTRIES];
} uc_lr;

static int uc_lr_owner(void) {
    if (GetCurrentThreadId() == uc_lr.owner_tid) return 1;
    InterlockedIncrement(&uc_lr.owner_error); return 0;
}
static int uc_lr_span(uint32_t address, uint32_t length) {
    return address >= 0x10000u && !(address & 3u) &&
        (uint64_t)address + length <= (uint64_t)g_xbox_total_ram;
}
static int uc_lr_overlap(uint32_t a, uint32_t an, uint32_t b, uint32_t bn) {
    return (uint64_t)a < (uint64_t)b + bn && (uint64_t)b < (uint64_t)a + an;
}
static int uc_lr_finite_words(const uint32_t *words, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        uint32_t magnitude = words[i] & 0x7FFFFFFFu;
        /* Admit zeros and bounded normals only, never NaN/Inf/subnormal input. */
        if (magnitude && (magnitude < 0x35800000u || magnitude > 0x49800000u)) return 0;
    }
    return 1;
}
static int uc_lr_finite_result(const uint32_t *words, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        uint32_t magnitude = words[i] & 0x7FFFFFFFu;
        if (magnitude >= 0x7F800000u || (magnitude && magnitude < 0x00800000u)) return 0;
    }
    return 1;
}
static uint32_t uc_lr_hash(const uint32_t *words, uint32_t csr, uint32_t constant) {
    uint32_t hash = 0x811C9DC5u ^ csr ^ constant;
    for (unsigned i = 0; i < 16; ++i) hash = (hash ^ words[i]) * 0x01000193u;
    return (hash ^ (hash >> 16)) & (UC_LR_ENTRIES - 1u);
}
void uc_light_restoration_init(void) {
    if (InterlockedCompareExchange(&uc_lr.initialized, 1, 0)) return;
    const char *enable = getenv("UCRR_LIGHT_RESTORATION");
    const char *mask = getenv("UCRR_LIGHT_RESTORATION_MASK");
    if (!enable || strcmp(enable, "1") || !mask ||
        (mask[0] != '0' && mask[0] != '1') || mask[1]) return;
    uc_lr.mask = (unsigned)(mask[0] - '0');
    uc_lr.owner_tid = GetCurrentThreadId();
    FILETIME born, dead, kernel, user; memset(&born, 0, sizeof(born));
    GetProcessTimes(GetCurrentProcess(), &born, &dead, &kernel, &user);
    uc_lr.birth = ((uint64_t)born.dwHighDateTime << 32) | born.dwLowDateTime;
    LARGE_INTEGER value; value.QuadPart = 0;
    QueryPerformanceFrequency(&value); uc_lr.frequency = (uint64_t)value.QuadPart;
    value.QuadPart = 0; QueryPerformanceCounter(&value);
    uc_lr.initialized_qpc = (uint64_t)value.QuadPart;
    uc_lr.enabled = 1;
}
int uc_light_restoration_enabled(void) { return uc_lr.enabled; }

void uc_light_restoration_inverse(void) {
    if (!uc_lr.enabled || !uc_lr_owner()) { sub_002D4360(); return; }
    ++uc_lr.counts.calls;
    uint32_t call_sp = g_esp, outer_bp = g_ebp, input_va = g_ecx;
    uint32_t scratch_va = ((call_sp - 4u) & ~15u) - 0xC0u;
    /* Exact B3370 call shape: its owned matrix copy is call-SP+94, with no
     * callback inside the inverse. Reject ordinary aliases and foreign seams. */
    if (!uc_lr_span(call_sp - 4u, 8u) || !uc_lr_span(outer_bp, 16u) ||
        !uc_lr_span(g_esi, 0x214u) || !uc_lr_span(g_edi, 4u) ||
        MEM32(call_sp) != 0x000B3404u || MEM32(outer_bp + 4u) != 0x00099128u ||
        MEM32(outer_bp + 8u) != g_edi || MEM32(g_esi) != 0x004573F0u ||
        (uint64_t)call_sp + 0x144u != (uint64_t)(outer_bp & ~15u) ||
        (uint64_t)call_sp + 0x94u != (uint64_t)input_va) {
        ++uc_lr.counts.rejected_site; goto reject;
    }
    if (!uc_lr_span(input_va, 64u) || !uc_lr_span(scratch_va, 0xC0u) ||
        !uc_lr_span(0x0044C174u, 4u) ||
        uc_lr_overlap(scratch_va, 0xC0u, input_va, 64u) ||
        uc_lr_overlap(scratch_va, 0xC0u, call_sp - 4u, 8u) ||
        uc_lr_overlap(g_esi, 0x214u, scratch_va, outer_bp + 16u - scratch_va) ||
        uc_lr_overlap(g_edi, 4u, scratch_va, outer_bp + 16u - scratch_va) ||
        uc_lr_overlap(0x0044C174u, 4u, scratch_va, outer_bp + 16u - scratch_va)) {
        ++uc_lr.counts.rejected_range; goto reject;
    }
    uint32_t csr_before = _mm_getcsr(), constant_bits = MEM32(0x0044C174u);
    uint32_t input[16]; memcpy(input, (const void *)XBOX_PTR(input_va), sizeof(input));
    if ((csr_before & 0x1F80u) != 0x1F80u ||
        !uc_lr_finite_words(input, 16) || !uc_lr_finite_words(&constant_bits, 1)) {
        ++uc_lr.counts.rejected_fp; goto reject;
    }
    ++uc_lr.counts.eligible;
    if (!uc_lr.mask) { ++uc_lr.counts.forwarded; sub_002D4360(); return; }
    UCLightInverseEntry *entry = &uc_lr.entries[uc_lr_hash(input, csr_before, constant_bits)];
    if (entry->valid && entry->mxcsr_before == csr_before && entry->constant_bits == constant_bits &&
        !memcmp(entry->input, input, sizeof(input))) {
        ++uc_lr.counts.hits;
        MEM32(call_sp - 4u) = outer_bp;
        memcpy((void *)XBOX_PTR(scratch_va), entry->scratch, sizeof(entry->scratch));
        memcpy((void *)XBOX_PTR(input_va), entry->output, sizeof(entry->output));
        g_xmm0 = entry->vectors[0]; g_xmm1 = entry->vectors[1];
        g_xmm2 = entry->vectors[2]; g_xmm3 = entry->vectors[3];
        g_xmm4 = entry->vectors[4]; g_xmm5 = entry->vectors[5];
        g_xmm6 = entry->vectors[6]; g_xmm7 = entry->vectors[7];
        g_eax = input_va; g_ebp = call_sp - 4u; g_esp = call_sp + 4u;
        g_recomp_zf = 0; /* Original tests call_sp==0; admission excludes zero. */
        _mm_setcsr(entry->mxcsr_after);
        return;
    }
    ++uc_lr.counts.misses; ++uc_lr.counts.forwarded;
    sub_002D4360();
    /* Capture only actual original outputs; singular/exceptional results are
     * never cached. Existing FP sticky bits are part of the exact input key. */
    RecompXmm vectors[8] = {g_xmm0,g_xmm1,g_xmm2,g_xmm3,g_xmm4,g_xmm5,g_xmm6,g_xmm7};
    if (!uc_lr_finite_result((const uint32_t *)XBOX_PTR(input_va), 16) ||
        !uc_lr_finite_result((const uint32_t *)XBOX_PTR(scratch_va), 48) ||
        !uc_lr_finite_result((const uint32_t *)vectors, 32)) {
        ++uc_lr.counts.uncacheable; return;
    }
    if (entry->valid) ++uc_lr.counts.evictions;
    entry->valid = 0;
    memcpy(entry->input, input, sizeof(input));
    memcpy(entry->output, (const void *)XBOX_PTR(input_va), sizeof(entry->output));
    memcpy(entry->scratch, (const void *)XBOX_PTR(scratch_va), sizeof(entry->scratch));
    memcpy(entry->vectors, vectors, sizeof(vectors));
    entry->mxcsr_before = csr_before; entry->constant_bits = constant_bits;
    entry->mxcsr_after = _mm_getcsr(); entry->valid = 1; ++uc_lr.counts.stores;
    return;
reject:
    ++uc_lr.counts.rejected; ++uc_lr.counts.forwarded; sub_002D4360();
}

void uc_light_restoration_read_counts(UCLightRestorationCounts *out) {
    if (out) *out = uc_lr.counts;
}
static void uc_lr_write_counts(FILE *file, const UCLightRestorationCounts *c) {
    fprintf(file,"{\"calls\":%llu,\"eligible\":%llu,\"hits\":%llu,\"misses\":%llu,\"forwarded\":%llu,\"rejected\":%llu,\"stores\":%llu,\"evictions\":%llu,\"rejected_site\":%llu,\"rejected_range\":%llu,\"rejected_fp\":%llu,\"uncacheable\":%llu}",
        (unsigned long long)c->calls,(unsigned long long)c->eligible,(unsigned long long)c->hits,
        (unsigned long long)c->misses,(unsigned long long)c->forwarded,(unsigned long long)c->rejected,
        (unsigned long long)c->stores,(unsigned long long)c->evictions,(unsigned long long)c->rejected_site,
        (unsigned long long)c->rejected_range,(unsigned long long)c->rejected_fp,(unsigned long long)c->uncacheable);
}
static void uc_lr_snapshot(void) {
    wchar_t path[MAX_PATH]; DWORD length = GetModuleFileNameW(NULL,path,MAX_PATH);
    if (!length || length >= MAX_PATH) return;
    wchar_t *slash = wcsrchr(path,L'\\'); if (!slash || slash-path > MAX_PATH-72) return;
    swprintf(slash+1,MAX_PATH-(slash+1-path),L"light-restoration-%lu.json",GetCurrentProcessId());
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    FILE *file = _wfopen(path,L"wb"); if (!file) return;
    fprintf(file,"{\"schema\":1,\"experiment\":\"light-receiver-inverse-reuse\",\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"mask\":%u,\"immutable_mode\":true,\"owner_tid\":%lu,\"owner_error\":%ld,\"cache_entries\":%u,\"qpc_frequency\":%llu,\"initialized_qpc\":%llu,\"snapshot_qpc\":%llu,\"counters\":",
        GetCurrentProcessId(),(unsigned long long)uc_lr.birth,uc_lr.mask,uc_lr.owner_tid,uc_lr.owner_error,
        UC_LR_ENTRIES,(unsigned long long)uc_lr.frequency,(unsigned long long)uc_lr.initialized_qpc,(unsigned long long)now.QuadPart);
    uc_lr_write_counts(file,&uc_lr.counts);
    fprintf(file,",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",(unsigned long long)uc_lr.capture_start,(unsigned long long)uc_lr.capture_end);
    uc_lr_write_counts(file,&uc_lr.start_counts); fputs(",\"end_counts\":",file);
    uc_lr_write_counts(file,&uc_lr.end_counts); fputs("}}\n",file); fclose(file);
}
void uc_light_restoration_capture_boundary(int start, uint64_t qpc) {
    if (!uc_lr.enabled || !uc_lr_owner()) return;
    if (start) { uc_lr.capture_start=qpc; uc_lr.start_counts=uc_lr.counts; }
    else { uc_lr.capture_end=qpc; uc_lr.end_counts=uc_lr.counts; uc_lr_snapshot(); }
}
#else
void uc_light_restoration_init(void) {}
int uc_light_restoration_enabled(void) { return 0; }
void uc_light_restoration_inverse(void) { sub_002D4360(); }
void uc_light_restoration_capture_boundary(int start,uint64_t qpc) {(void)start;(void)qpc;}
void uc_light_restoration_read_counts(UCLightRestorationCounts *out) { if(out)memset(out,0,sizeof(*out)); }
#endif
