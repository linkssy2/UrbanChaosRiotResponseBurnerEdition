/* Exact-packet decal command batching. Geometry/material/lifecycle stay original.
 * One preflight replaces eight SDK leaf reservations per qualifying quad.
 * Default OFF; every declined guard preserves original execution. */
#include "decal_restoration.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
extern void sub_0009C2C0(void);
extern void sub_003EF700(void);
extern void sub_003EF600(void);
extern void sub_003EF5C0(void);
extern void sub_003EF740(void);

#if UCRR_DECAL_RESTORATION
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
typedef struct {
    uint64_t calls, eligible, optimized, forwarded, rejected;
    uint64_t rendered_quads, batch_eligible_quads, optimized_quads, fallback_quads;
    uint64_t sdk_calls_avoided, packet_words;
    uint64_t declined_device, declined_room, declined_overlap, declined_owner;
} UCDecalRestorationCounts;
typedef struct { uint32_t device, put; } UCDecalBatch;
static struct {
    int enabled, busy, trace;
    unsigned mask;
    DWORD owner_tid;
    LONG owner_error;
    uint64_t birth, frequency, initialized_qpc, capture_start, capture_end;
    UCDecalRestorationCounts counts, start_counts, end_counts;
} uc_decal_restore;

static int uc_decal_restore_thread(void) {
    if (GetCurrentThreadId() == uc_decal_restore.owner_tid) return 1;
    InterlockedIncrement(&uc_decal_restore.owner_error); return 0;
}
static int uc_decal_restore_span(uint32_t p, uint64_t n) {
    return p >= 0x10000u && !(p & 3u) && (uint64_t)p+n <= g_xbox_total_ram &&
        (uint64_t)p+n <= 0x100000000ull;
}
/* Only ordinary RAM and the exact runtime-normalized write-combined alias.
 * No mirror guesses, device apertures, cross-alias extents or wrapped spans. */
static int uc_decal_restore_output(uint32_t p, uint32_t n, uint32_t *physical) {
    if ((uint64_t)p+n > 0x100000000ull) return 0;
    uint32_t q=p;
    if (p >= 0xF0000000u && p < 0xFD000000u) {
        if ((uint64_t)p+n > 0xFD000000ull) return 0;
        q=p & 0x07FFFFFFu;
        if ((uint64_t)q+n > 0x08000000ull) return 0;
    }
    if (!uc_decal_restore_span(q,n)) return 0;
    *physical=q; return 1;
}
static int uc_decal_overlap(uint32_t a,uint32_t an,uint32_t b,uint32_t bn) {
    return (uint64_t)a+an>b && (uint64_t)b+bn>a;
}
static int uc_decal_restore_owner(uint32_t manager,uint32_t *records,uint32_t *capacity,uint32_t *occupied) {
    if (!uc_decal_restore_span(manager,0x94) || !uc_decal_restore_span(0x519F60u,4)) return 0;
    if (MEM32(manager)!=0x456AE0u || MEM32(manager+0x10)!=0x456A78u) return 0;
    uint32_t graphics=MEM32(0x519F60u);
    if (!uc_decal_restore_span(graphics,0x54) || MEM32(graphics)!=0x4561F8u || MEM32(graphics+0x50)!=manager) return 0;
    uint32_t r=MEM32(manager+0x80),h=MEM32(manager+0x84),t=MEM32(manager+0x88),c=MEM32(manager+0x8C);
    if (!c || c>65536u || h>=c || t>=c || !uc_decal_restore_span(r,(uint64_t)c*0x4C)) return 0;
    *records=r;*capacity=c;*occupied=t>=h?t-h:c-h+t;return 1;
}
static int uc_decal_restore_callback(uint32_t callback) {
    if (!uc_decal_restore_span(callback,0x20) || !uc_decal_restore_span(g_esp,4) || MEM32(callback)!=0x456AF4u) return 0;
    uint32_t manager=MEM32(callback+8),records,capacity,occupied;
    if (!uc_decal_restore_owner(manager,&records,&capacity,&occupied) ||
        MEM32(callback+4)>=occupied || MEM32(manager+0x90)!=callback+0xC) return 0;
    uint32_t bits=MEM32(callback+0x14),words=(occupied+31u)/32u;
    return uc_decal_restore_span(bits,(uint64_t)words*4u);
}
static UCDecalBatch uc_decal_batch_begin(uint32_t manager,uint32_t frame) {
    UCDecalBatch result={0,0};
    ++uc_decal_restore.counts.rendered_quads;
    uc_decal_restore.counts.packet_words+=32;
    uint32_t records,capacity,occupied;
    if (!uc_decal_restore_owner(manager,&records,&capacity,&occupied) || frame<0x1001Cu ||
        !uc_decal_restore_span(frame-0x1Cu,0x70u)) {
        ++uc_decal_restore.counts.declined_owner;goto decline;
    }
    if (!uc_decal_restore_span(0x3F4238u,4)) { ++uc_decal_restore.counts.declined_device;goto decline; }
    uint32_t device=MEM32(0x3F4238u);
    if (!uc_decal_restore_span(device,12)) { ++uc_decal_restore.counts.declined_device;goto decline; }
    uint32_t put=MEM32(device),limit=MEM32(device+4),physical;
    if ((uint64_t)put+128u>limit || !uc_decal_restore_output(put,128u,&physical)) {
        ++uc_decal_restore.counts.declined_room;goto decline;
    }
    uint32_t bitset=MEM32(manager+0x90);
    if (!uc_decal_restore_span(bitset,0x14u)) { ++uc_decal_restore.counts.declined_owner;goto decline; }
    uint32_t bits=MEM32(bitset+8),bitbytes=((occupied+31u)/32u)*4u;
    if (!uc_decal_restore_span(bits,bitbytes)) { ++uc_decal_restore.counts.declined_owner;goto decline; }
    /* The CPU put pointer is not a GPU submission. No guest call occurs inside
     * the four-vertex fast region; retain every ordered command word and final
     * CPU put. Reject output aliasing anything the region reads or writes. */
    if (uc_decal_overlap(physical,128,frame-0x1C,0x70) ||
        uc_decal_overlap(device,12,frame-0x1C,0x70) ||
        uc_decal_overlap(device,12,0x3F4238,4) ||
        uc_decal_overlap(frame-0x1C,0x70,0x3F4238,4) ||
        uc_decal_overlap(physical,128,device,12) ||
        uc_decal_overlap(physical,128,0x3F4238,4) ||
        uc_decal_overlap(physical,128,manager,0x94) ||
        uc_decal_overlap(physical,128,records,capacity*0x4C) ||
        uc_decal_overlap(physical,128,bitset,0x14) ||
        uc_decal_overlap(physical,128,bits,bitbytes) ||
        uc_decal_overlap(physical,128,0x51A3BC,0x24) ||
        uc_decal_overlap(physical,128,0x44C174,4)) {
        ++uc_decal_restore.counts.declined_overlap;goto decline;
    }
    ++uc_decal_restore.counts.batch_eligible_quads;
    if (!uc_decal_restore.mask || uc_decal_restore.trace) goto decline;
    ++uc_decal_restore.counts.optimized_quads;
    uc_decal_restore.counts.sdk_calls_avoided+=8;
    result.device=device;result.put=put;return result;
decline:
    ++uc_decal_restore.counts.fallback_quads;return result;
}
/* Reproduce both SDK leaves' ordered packets and their final guest state,
 * including the stale argument/return/save words below the live stack. The
 * first call's stale words are all overwritten by the second call. No float
 * arithmetic is introduced or omitted here. */
static __forceinline void uc_decal_batch_vertex(UCDecalBatch *batch,uint32_t frame,uint32_t vertex,uint32_t y) {
    uint32_t uv_v=MEM32(vertex*8u+0x51A3C0u),uv_u=MEM32(vertex*8u+0x51A3BCu);
    uint32_t z=MEM32(frame+0x28),x=MEM32(frame+0x20),put=batch->put;
    MEM32(put)=0x00081898u;MEM32(put+4)=uv_u;MEM32(put+8)=uv_v;
    MEM32(put+12)=0x00101518u;MEM32(put+16)=x;MEM32(put+20)=y;
    MEM32(put+24)=z;MEM32(put+28)=0x3F800000u;
    MEM32(frame-4)=0x3F800000u;MEM32(frame-8)=z;MEM32(frame-12)=y;
    MEM32(frame-16)=x;MEM32(frame-20)=0xFFFFFFFFu;MEM32(frame-24)=0x0009C24Du;
    MEM32(frame-28)=vertex;
    batch->put=put+32u;MEM32(batch->device)=batch->put;
    g_eax=batch->put;g_ecx=z;g_edx=0x3F800000u;g_recomp_zf=0;
}
#include "decal_restoration_body.inc"

void uc_decal_restoration_init(void) {
    const char *on=getenv("UCRR_DECAL_RESTORATION"),*mask=getenv("UCRR_DECAL_RESTORATION_MASK");
    if (uc_decal_restore.enabled || !on || strcmp(on,"1") || !mask || (mask[0]!='0'&&mask[0]!='1') || mask[1]) return;
    uc_decal_restore.enabled=1;uc_decal_restore.mask=(unsigned)(mask[0]-'0');
    uc_decal_restore.trace=getenv("UCRR_RETICLE_TRACE")!=NULL;
    uc_decal_restore.owner_tid=GetCurrentThreadId();
    FILETIME born,dead,kernel,user;memset(&born,0,sizeof(born));
    GetProcessTimes(GetCurrentProcess(),&born,&dead,&kernel,&user);
    uc_decal_restore.birth=((uint64_t)born.dwHighDateTime<<32)|born.dwLowDateTime;
    LARGE_INTEGER v;v.QuadPart=0;QueryPerformanceFrequency(&v);uc_decal_restore.frequency=(uint64_t)v.QuadPart;
    v.QuadPart=0;QueryPerformanceCounter(&v);uc_decal_restore.initialized_qpc=(uint64_t)v.QuadPart;
}
int uc_decal_restoration_enabled(void) { return uc_decal_restore.enabled; }
void uc_decal_restoration_callback_wrapper(void) {
    if (!uc_decal_restore.enabled || !uc_decal_restore_thread()) { sub_0009C2C0();return; }
    ++uc_decal_restore.counts.calls;
    if (uc_decal_restore.busy || !uc_decal_restore_callback(g_ecx)) {
        ++uc_decal_restore.counts.rejected;++uc_decal_restore.counts.forwarded;sub_0009C2C0();return;
    }
    ++uc_decal_restore.counts.eligible;uc_decal_restore.busy=1;
    uint64_t before=uc_decal_restore.counts.optimized_quads;
    /* Exact 9C2C0 callback, with only its direct 9BF50 target replaced. */
    g_eax=MEM32(g_ecx+4);g_ecx=MEM32(g_ecx+8);
    PUSH32(g_esp,g_eax);PUSH32(g_esp,0x0009C2CCu);uc_decal_restoration_body();g_esp+=4;
    if (uc_decal_restore.counts.optimized_quads!=before) ++uc_decal_restore.counts.optimized;
    else ++uc_decal_restore.counts.forwarded;
    uc_decal_restore.busy=0;
}
static void uc_decal_restore_counts(FILE *f,const UCDecalRestorationCounts *c) {
#define COUNT(name) fprintf(f,"\"" #name "\":%llu",(unsigned long long)c->name)
    fputc('{',f);COUNT(calls);fputc(',',f);COUNT(eligible);fputc(',',f);COUNT(optimized);fputc(',',f);COUNT(forwarded);fputc(',',f);COUNT(rejected);
    fputc(',',f);COUNT(rendered_quads);fputc(',',f);COUNT(batch_eligible_quads);fputc(',',f);COUNT(optimized_quads);fputc(',',f);COUNT(fallback_quads);
    fputc(',',f);COUNT(sdk_calls_avoided);fputc(',',f);COUNT(packet_words);fputc(',',f);COUNT(declined_device);fputc(',',f);COUNT(declined_room);
    fputc(',',f);COUNT(declined_overlap);fputc(',',f);COUNT(declined_owner);fputc('}',f);
#undef COUNT
}
void uc_decal_restoration_capture_boundary(int start,uint64_t qpc) {
    if (!uc_decal_restore.enabled || !uc_decal_restore_thread()) return;
    if (start) { uc_decal_restore.capture_start=qpc;uc_decal_restore.start_counts=uc_decal_restore.counts;return; }
    uc_decal_restore.capture_end=qpc;uc_decal_restore.end_counts=uc_decal_restore.counts;
    wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);
    if (!n || n>=MAX_PATH) return;wchar_t *slash=wcsrchr(path,L'\\');if (!slash || slash-path>MAX_PATH-70) return;
    swprintf(slash+1,MAX_PATH-(slash+1-path),L"decal-restoration-%lu.json",GetCurrentProcessId());
    LARGE_INTEGER now;QueryPerformanceCounter(&now);FILE *f=_wfopen(path,L"wb");if (!f) return;
    fprintf(f,"{\"schema\":1,\"experiment\":\"decal-command-batch\",\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"mask\":%u,\"immutable_mode\":true,\"owner_tid\":%lu,\"owner_error\":%ld,\"qpc_frequency\":%llu,\"initialized_qpc\":%llu,\"snapshot_qpc\":%llu,\"counters\":",GetCurrentProcessId(),(unsigned long long)uc_decal_restore.birth,uc_decal_restore.mask,uc_decal_restore.owner_tid,uc_decal_restore.owner_error,(unsigned long long)uc_decal_restore.frequency,(unsigned long long)uc_decal_restore.initialized_qpc,(unsigned long long)now.QuadPart);
    uc_decal_restore_counts(f,&uc_decal_restore.counts);
    fprintf(f,",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",(unsigned long long)uc_decal_restore.capture_start,(unsigned long long)uc_decal_restore.capture_end);
    uc_decal_restore_counts(f,&uc_decal_restore.start_counts);fputs(",\"end_counts\":",f);uc_decal_restore_counts(f,&uc_decal_restore.end_counts);
    fputs("}}\n",f);fclose(f);
}
#else
void uc_decal_restoration_init(void) {}
int uc_decal_restoration_enabled(void) { return 0; }
void uc_decal_restoration_callback_wrapper(void) { sub_0009C2C0(); }
void uc_decal_restoration_capture_boundary(int start,uint64_t qpc) { (void)start;(void)qpc; }
#endif
