/* Exact visible-particle command specialization. No particle count, geometry,
 * texture, material, topology, Begin/End or simulation change. Default OFF. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "particle_restoration.h"

typedef struct {
    uint64_t calls, eligible, optimized, forwarded, rejected;
    uint64_t eligible_items, optimized_items, optimized_packet_words;
    uint64_t class_calls[3], class_eligible[3], class_optimized[3];
    uint64_t reasons[8];
} UCParticleRestorationCounts;
static struct {
    LONG initialized;
    int enabled;
    unsigned mask;
    DWORD owner_tid;
    LONG owner_error;
    uint64_t birth, frequency, initialized_qpc, capture_start, capture_end;
    UCParticleRestorationCounts counts, start_counts, end_counts;
} uc_particle;

static int uc_particle_owner(void) {
    if (GetCurrentThreadId() == uc_particle.owner_tid) return 1;
    InterlockedIncrement(&uc_particle.owner_error);
    return 0;
}
void uc_particle_restoration_init(void) {
#if defined(UCRR_PARTICLE_RESTORATION)
    if (InterlockedCompareExchange(&uc_particle.initialized, 1, 0)) return;
    const char *enabled = getenv("UCRR_PARTICLE_RESTORATION");
    const char *mask = getenv("UCRR_PARTICLE_RESTORATION_MASK");
    if (!enabled || strcmp(enabled, "1") || !mask ||
        (mask[0] != '0' && mask[0] != '1') || mask[1]) return;
    uc_particle.enabled = 1;
    uc_particle.mask = (unsigned)(mask[0] - '0');
    uc_particle.owner_tid = GetCurrentThreadId();
    FILETIME born = {0}, dead = {0}, kernel = {0}, user = {0};
    GetProcessTimes(GetCurrentProcess(), &born, &dead, &kernel, &user);
    uc_particle.birth = ((uint64_t)born.dwHighDateTime << 32) | born.dwLowDateTime;
    LARGE_INTEGER q = {0};
    QueryPerformanceFrequency(&q); uc_particle.frequency = (uint64_t)q.QuadPart;
    q.QuadPart = 0;
    QueryPerformanceCounter(&q); uc_particle.initialized_qpc = (uint64_t)q.QuadPart;
#endif
}
int uc_particle_restoration_enabled(void) { return uc_particle.enabled; }
static int uc_particle_ram(uint32_t address, uint64_t bytes) {
    return address >= 0x10000u && !(address & 3u) &&
           (uint64_t)address + bytes <= (uint64_t)g_xbox_total_ram;
}
static int uc_particle_overlap(uint32_t a, uint64_t an, uint32_t b, uint64_t bn) {
    return (uint64_t)a < (uint64_t)b + bn && (uint64_t)b < (uint64_t)a + an;
}
/* Match recomp_normalize_xbox_va exactly for the one explicit WC alias.
 * Reject normalized wrap, aperture crossing and every other mirror guess. */
static int uc_particle_output(uint32_t address, uint64_t bytes, uint32_t *physical) {
    uint32_t normalized = address;
    if ((uint64_t)address + bytes > 0x100000000ull) return 0;
    if (address >= 0xF0000000u && address < 0xFD000000u) {
        if ((uint64_t)address + bytes > 0xFD000000ull) return 0;
        normalized = address & 0x07FFFFFFu;
        if ((uint64_t)normalized + bytes > 0x08000000ull) return 0;
    }
    if (!uc_particle_ram(normalized, bytes)) return 0;
    *physical = normalized;
    return 1;
}

/* Runs once after native filler and Begin, before the native item loop. The
 * complete frame contains its scratch colors, positions and all nested call
 * stack writes. Rejection does not alter any guest byte or exported CPU state. */
int uc_particle_restoration_admit(uint32_t wrapper, uint32_t frame, uint32_t frame_end,
                                  uint32_t count, uint32_t textured) {
#if defined(UCRR_PARTICLE_RESTORATION)
    if (!uc_particle.enabled || !uc_particle_owner()) return 0;
    ++uc_particle.counts.calls;
    unsigned reason = 0, kind;
    if (!uc_particle_ram(wrapper, 8u) || MEM32(wrapper) != 0x00455FF0u) goto rejected;
    uint32_t object = MEM32(wrapper + 4u);
    if (!uc_particle_ram(object, 4u)) goto rejected;
    uint32_t table = MEM32(object);
    if (table == 0x00482FC0u) kind = 0;
    else if (table == 0x00482FF0u) kind = 1;
    else if (table == 0x00483050u) kind = 2;
    else goto rejected;
    ++uc_particle.counts.class_calls[kind];
    reason = 1;
    if (textured != 1u) goto rejected;
    reason = 2;
    if (!count || count > 128u) goto rejected;
    reason = 3;
    if (frame < 0x10040u || (frame & 15u) || frame_end < frame ||
        (uint64_t)frame + 0x2040u > frame_end ||
        (uint64_t)frame_end - frame > 0x2060u ||
        !uc_particle_ram(frame - 64u, (uint64_t)frame_end - frame + 64u)) goto rejected;
    uint32_t stack_low = frame - 64u;
    uint64_t stack_bytes = (uint64_t)frame_end - stack_low;
    reason = 4;
    uint32_t device = MEM32(0x003F4238u);
    if (!uc_particle_ram(device, 12u)) goto rejected;
    uint32_t cursor = MEM32(device), limit = MEM32(device + 4u);
    uint32_t physical;
    uint64_t bytes = (uint64_t)count * 42u * 4u + 8u; /* include native End */
    reason = 5;
    if (cursor >= limit || (limit & 3u) || (uint64_t)cursor + bytes > limit ||
        !uc_particle_output(cursor, (uint64_t)limit - cursor, &physical)) goto rejected;
    reason = 6;
    const uint32_t protected_address[] = {stack_low, device, wrapper, object, 0x3F4238u, 0x4F879Cu, 0x4505CCu};
    const uint64_t protected_bytes[] = {stack_bytes, 12u, 8u, 4u, 4u, 32u, 4u};
    for (unsigned i = 0; i < 7; ++i)
        if (uc_particle_overlap(physical, bytes, protected_address[i], protected_bytes[i])) goto rejected;
    reason = 7;
    if (uc_particle_overlap(device, 12u, stack_low, stack_bytes) ||
        uc_particle_overlap(device, 12u, 0x3F4238u, 4u) ||
        uc_particle_overlap(device, 12u, 0x4F879Cu, 32u) ||
        uc_particle_overlap(device, 12u, 0x4505CCu, 4u)) goto rejected;
    ++uc_particle.counts.eligible;
    ++uc_particle.counts.class_eligible[kind];
    uc_particle.counts.eligible_items += count;
    if (uc_particle.mask) {
        ++uc_particle.counts.optimized;
        ++uc_particle.counts.class_optimized[kind];
        uc_particle.counts.optimized_items += count;
        uc_particle.counts.optimized_packet_words += (uint64_t)count * 42u;
        return 1;
    }
    ++uc_particle.counts.forwarded;
    return 0;
rejected:
    ++uc_particle.counts.reasons[reason];
    ++uc_particle.counts.rejected;
    ++uc_particle.counts.forwarded;
#else
    (void)wrapper; (void)frame; (void)frame_end; (void)count; (void)textured;
#endif
    return 0;
}

static void uc_particle_write_counts(FILE *f, const UCParticleRestorationCounts *c) {
    fprintf(f, "{\"calls\":%llu,\"eligible\":%llu,\"optimized\":%llu,\"forwarded\":%llu,\"rejected\":%llu,\"eligible_items\":%llu,\"optimized_items\":%llu,\"optimized_packet_words\":%llu",
        (unsigned long long)c->calls,(unsigned long long)c->eligible,(unsigned long long)c->optimized,
        (unsigned long long)c->forwarded,(unsigned long long)c->rejected,(unsigned long long)c->eligible_items,
        (unsigned long long)c->optimized_items,(unsigned long long)c->optimized_packet_words);
    fputs(",\"classes\":[", f);
    for (unsigned i=0;i<3;++i) fprintf(f,"%s{\"calls\":%llu,\"eligible\":%llu,\"optimized\":%llu}",i?",":"",
        (unsigned long long)c->class_calls[i],(unsigned long long)c->class_eligible[i],(unsigned long long)c->class_optimized[i]);
    fputs("],\"reasons\":[",f);
    for(unsigned i=0;i<8;++i) fprintf(f,"%s%llu",i?",":"",(unsigned long long)c->reasons[i]);
    fputs("]}",f);
}
static void uc_particle_snapshot(void) {
    wchar_t path[MAX_PATH];
    DWORD length=GetModuleFileNameW(NULL,path,MAX_PATH);
    wchar_t *slash=wcsrchr(path,L'\\');
    if(!length||length>=MAX_PATH||!slash||slash-path>MAX_PATH-70)return;
    swprintf(slash+1,MAX_PATH-(slash+1-path),L"particle-restoration-%lu.json",GetCurrentProcessId());
    LARGE_INTEGER now={0};QueryPerformanceCounter(&now);
    FILE *f=_wfopen(path,L"wb");if(!f)return;
    fprintf(f,"{\"schema\":1,\"experiment\":\"particle-immediate-quad-restoration\",\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"mask\":%u,\"immutable_mode\":true,\"owner_tid\":%lu,\"owner_error\":%ld,\"qpc_frequency\":%llu,\"initialized_qpc\":%llu,\"snapshot_qpc\":%llu,\"counters\":",
        GetCurrentProcessId(),(unsigned long long)uc_particle.birth,uc_particle.mask,uc_particle.owner_tid,uc_particle.owner_error,
        (unsigned long long)uc_particle.frequency,(unsigned long long)uc_particle.initialized_qpc,(unsigned long long)now.QuadPart);
    uc_particle_write_counts(f,&uc_particle.counts);
    fprintf(f,",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",
        (unsigned long long)uc_particle.capture_start,(unsigned long long)uc_particle.capture_end);
    uc_particle_write_counts(f,&uc_particle.start_counts);fputs(",\"end_counts\":",f);
    uc_particle_write_counts(f,&uc_particle.end_counts);fputs("}}\n",f);fclose(f);
}
void uc_particle_restoration_capture_boundary(int start,uint64_t qpc) {
    if(!uc_particle.enabled||!uc_particle_owner())return;
    if(start){uc_particle.capture_start=qpc;uc_particle.start_counts=uc_particle.counts;}
    else {uc_particle.capture_end=qpc;uc_particle.end_counts=uc_particle.counts;uc_particle_snapshot();}
}
