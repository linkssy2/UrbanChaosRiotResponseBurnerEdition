/* Default-OFF decal admission experiment. The original visibility getter runs
 * first; its original caller owns queue rejection. Simulation, ring contents,
 * material ownership and render cleanup remain original. */
#include "decal_removal.h"
#include "effect_diagnostics.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
extern void sub_0008A090(void);
extern void sub_0009C4A0(void);

#if UCRR_DECAL_REMOVAL
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "graphics_features.h"

typedef struct {
    uint64_t calls, eligible, suppressed, forwarded, rejected, ring_invalid;
    uint64_t positive_admissions, admission_items, suppressed_positive, suppressed_items;
    uint64_t render_calls, render_positive, render_items, render_rejected, render_other_kind;
} UCDecalRemovalCounts;
typedef struct { uint32_t object, manager, records, head, tail, capacity, occupied; } UCDecalRing;
static struct {
    int enabled;
    unsigned mask;
    DWORD owner_tid;
#if !UCRR_PLAY_ONLY
    LONG owner_error;
    uint64_t birth, frequency, initialized_qpc, capture_start, capture_end;
    UCDecalRemovalCounts counts, start_counts, end_counts;
    UCDecalRing last_admission, last_render;
#endif
} uc_decal;

static int uc_decal_thread(void) {
    if (GetCurrentThreadId() == uc_decal.owner_tid) return 1;
    UC_EFFECT_DIAGNOSTIC(InterlockedIncrement(&uc_decal.owner_error)); return 0;
}
static int uc_decal_span(uint32_t address, uint64_t bytes) {
    return address >= 0x10000u && !(address & 3u) &&
        (uint64_t)address + bytes <= (uint64_t)g_xbox_total_ram &&
        (uint64_t)address + bytes <= 0x100000000ull;
}
/* 1 = exact owner plus valid ring; 0 = other/invalid identity; -1 = malformed
 * selected ring. Range checks precede every guest read, including global owner. */
static int uc_decal_ring(uint32_t object, UCDecalRing *ring) {
    if (object < 0x10010u || !uc_decal_span(object - 0x10u, 0x90u) ||
        !uc_decal_span(0x00519F60u, 4u)) return 0;
    const uint32_t manager = object - 0x10u;
    if (MEM32(object) != 0x00456A78u || MEM32(manager) != 0x00456AE0u) return 0;
    const uint32_t graphics = MEM32(0x00519F60u);
    if (!uc_decal_span(graphics, 0x54u) || MEM32(graphics) != 0x004561F8u ||
        MEM32(graphics + 0x50u) != manager) return 0;
    const uint32_t records = MEM32(manager + 0x80u);
    const uint32_t head = MEM32(manager + 0x84u), tail = MEM32(manager + 0x88u);
    const uint32_t capacity = MEM32(manager + 0x8Cu);
    if (!capacity || capacity > 0x7FFFFFFFu || head >= capacity || tail >= capacity ||
        !uc_decal_span(records, (uint64_t)capacity * 0x4Cu)) return -1;
    UC_EFFECT_DIAGNOSTIC(
        ring->object=object; ring->manager=manager; ring->records=records;
        ring->head=head; ring->tail=tail; ring->capacity=capacity;
        ring->occupied=tail >= head ? tail-head : capacity-head+tail;
    );
    return 1;
}
void uc_decal_removal_init(void) {
    const char *enabled=getenv("UCRR_DECAL_REMOVAL"), *mask=getenv("UCRR_DECAL_REMOVAL_MASK");
    if (!enabled || strcmp(enabled,"1") || !mask ||
        (mask[0]!='0' && mask[0]!='1') || mask[1] || uc_decal.enabled) return;
    uc_decal.enabled=1; uc_decal.mask=(unsigned)(mask[0]-'0');
    if(uc_graphics_features_enabled())uc_decal.mask=!(uc_graphics_features_load()&8);
    uc_decal.owner_tid=GetCurrentThreadId();
#if !UCRR_PLAY_ONLY
    FILETIME born,dead,kernel,user; memset(&born,0,sizeof(born));
    GetProcessTimes(GetCurrentProcess(),&born,&dead,&kernel,&user);
    uc_decal.birth=((uint64_t)born.dwHighDateTime<<32)|born.dwLowDateTime;
    LARGE_INTEGER value; value.QuadPart=0; QueryPerformanceFrequency(&value);
    uc_decal.frequency=(uint64_t)value.QuadPart;
    value.QuadPart=0; QueryPerformanceCounter(&value); uc_decal.initialized_qpc=(uint64_t)value.QuadPart;
#endif
}
int uc_decal_removal_enabled(void) { return uc_decal.enabled; }
int uc_decal_visibility_ready(void){return uc_decal.enabled&&uc_decal_thread();}
void uc_decal_visibility_apply(unsigned value){uc_decal.mask=!(value&8u);}
uint8_t uc_decal_removal_filter(uint8_t original, uint32_t object, uint32_t caller) {
    if (!uc_decal.enabled || !uc_decal_thread()) return original;
#if UCRR_PLAY_ONLY
    if (!uc_decal.mask || !original) return original;
#endif
    UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.calls);
    UCDecalRing ring;
    const int status=caller==0x000968C2u ? uc_decal_ring(object,&ring) : 0;
    if (status!=1) {
        UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.rejected); UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.forwarded);
        if (status<0) UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.ring_invalid);
        return original;
    }
    UC_EFFECT_DIAGNOSTIC(uc_decal.last_admission=ring);
    if (original) {
        UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.eligible);
        UC_EFFECT_DIAGNOSTIC(if (ring.occupied) ++uc_decal.counts.positive_admissions);
        UC_EFFECT_DIAGNOSTIC(uc_decal.counts.admission_items+=ring.occupied);
    }
    if (!uc_decal.mask || !original) { UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.forwarded); return original; }
    UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.suppressed);
    UC_EFFECT_DIAGNOSTIC(if (ring.occupied) ++uc_decal.counts.suppressed_positive);
    UC_EFFECT_DIAGNOSTIC(uc_decal.counts.suppressed_items+=ring.occupied);
    return 0;
}
/* Observe entry to the original decal material dispatcher, never skip it here.
 * Positive kind-2 entries establish active render preparation, not quad/pixel
 * counts. Occupied records may still be rejected downstream by material flags. */
void uc_decal_removal_render(uint32_t object, uint32_t context) {
#if !UCRR_PLAY_ONLY

    if (!uc_decal.enabled || !uc_decal_thread()) return;
    UCDecalRing ring;
    if (uc_decal_ring(object,&ring)!=1 || !uc_decal_span(context,4u)) {
        UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.render_rejected); return;
    }
    if (MEM32(context)!=2u) { UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.render_other_kind); return; }
    uc_decal.last_render=ring;
    UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.render_calls);
    if (ring.occupied) UC_EFFECT_DIAGNOSTIC(++uc_decal.counts.render_positive);
    UC_EFFECT_DIAGNOSTIC(uc_decal.counts.render_items+=ring.occupied);

#else
    (void)object; (void)context;
#endif
}
#if !UCRR_PLAY_ONLY
static void uc_decal_write_counts(FILE *file,const UCDecalRemovalCounts *c) {
    fprintf(file,"{\"calls\":%llu,\"eligible\":%llu,\"suppressed\":%llu,\"forwarded\":%llu,\"rejected\":%llu,\"ring_invalid\":%llu,\"positive_admissions\":%llu,\"admission_items\":%llu,\"suppressed_positive\":%llu,\"suppressed_items\":%llu,\"render_calls\":%llu,\"render_positive\":%llu,\"render_items\":%llu,\"render_rejected\":%llu,\"render_other_kind\":%llu}",
        (unsigned long long)c->calls,(unsigned long long)c->eligible,(unsigned long long)c->suppressed,
        (unsigned long long)c->forwarded,(unsigned long long)c->rejected,(unsigned long long)c->ring_invalid,
        (unsigned long long)c->positive_admissions,(unsigned long long)c->admission_items,
        (unsigned long long)c->suppressed_positive,(unsigned long long)c->suppressed_items,
        (unsigned long long)c->render_calls,(unsigned long long)c->render_positive,(unsigned long long)c->render_items,
        (unsigned long long)c->render_rejected,(unsigned long long)c->render_other_kind);
}
static void uc_decal_write_ring(FILE *file,const UCDecalRing *r) {
    fprintf(file,"{\"object\":%u,\"manager\":%u,\"records\":%u,\"head\":%u,\"tail\":%u,\"capacity\":%u,\"occupied\":%u}",
        r->object,r->manager,r->records,r->head,r->tail,r->capacity,r->occupied);
}
static void uc_decal_snapshot(void) {
    wchar_t path[MAX_PATH]; DWORD length=GetModuleFileNameW(NULL,path,MAX_PATH);
    if (!length || length>=MAX_PATH) return;
    wchar_t *slash=wcsrchr(path,L'\\'); if (!slash || slash-path>MAX_PATH-65) return;
    swprintf(slash+1,MAX_PATH-(slash+1-path),L"decal-removal-%lu.json",GetCurrentProcessId());
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    FILE *file=_wfopen(path,L"wb"); if (!file) return;
    fprintf(file,"{\"schema\":1,\"experiment\":\"decal-manager-admission\",\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"mask\":%u,\"immutable_mode\":true,\"owner_tid\":%lu,\"owner_error\":%ld,\"qpc_frequency\":%llu,\"initialized_qpc\":%llu,\"snapshot_qpc\":%llu,\"counters\":",
        GetCurrentProcessId(),(unsigned long long)uc_decal.birth,uc_decal.mask,uc_decal.owner_tid,uc_decal.owner_error,
        (unsigned long long)uc_decal.frequency,(unsigned long long)uc_decal.initialized_qpc,(unsigned long long)now.QuadPart);
    uc_decal_write_counts(file,&uc_decal.counts);
    fprintf(file,",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",
        (unsigned long long)uc_decal.capture_start,(unsigned long long)uc_decal.capture_end);
    uc_decal_write_counts(file,&uc_decal.start_counts); fputs(",\"end_counts\":",file);
    uc_decal_write_counts(file,&uc_decal.end_counts); fputs("},\"last_admission_ring\":",file);
    uc_decal_write_ring(file,&uc_decal.last_admission); fputs(",\"last_render_ring\":",file);
    uc_decal_write_ring(file,&uc_decal.last_render); fputs("}\n",file); fclose(file);
}
void uc_decal_removal_capture_boundary(int start,uint64_t qpc) {
    if (!uc_decal.enabled || !uc_decal_thread()) return;
    if (start) { uc_decal.capture_start=qpc; uc_decal.start_counts=uc_decal.counts; }
    else { uc_decal.capture_end=qpc; uc_decal.end_counts=uc_decal.counts; uc_decal_snapshot(); }
}
#else
void uc_decal_removal_capture_boundary(int start, uint64_t qpc) {
    (void)start; (void)qpc;
}
#endif
#else
void uc_decal_removal_init(void) {}
int uc_decal_visibility_ready(void){return 0;}
void uc_decal_visibility_apply(unsigned value){(void)value;}
int uc_decal_removal_enabled(void) { return 0; }
uint8_t uc_decal_removal_filter(uint8_t original,uint32_t object,uint32_t caller) { (void)object;(void)caller;return original; }
void uc_decal_removal_render(uint32_t object,uint32_t context) { (void)object;(void)context; }
void uc_decal_removal_capture_boundary(int start,uint64_t qpc) { (void)start;(void)qpc; }
#endif

/* Production entry wrappers live with the module so the native fixture executes
 * the exact integrated wrappers. Guest stack reads are checked independently of
 * the original functions; the original leaf/render function always runs once. */
void uc_decal_removal_visibility_wrapper(void) {
#if UCRR_DECAL_REMOVAL
    const uint32_t object=g_ecx;
    const uint32_t caller=g_esp>=0x10000u && (uint64_t)g_esp+4u<=g_xbox_total_ram ? MEM32(g_esp):0;
    sub_0008A090();
    SET_LO8(g_eax,uc_decal_removal_filter((uint8_t)g_eax,object,caller));
#else
    sub_0008A090();
#endif
}
void uc_decal_removal_render_wrapper(void) {
#if UCRR_DECAL_REMOVAL && !UCRR_PLAY_ONLY
    const uint32_t context=g_esp>=0x10000u && (uint64_t)g_esp+8u<=g_xbox_total_ram ? MEM32(g_esp+4u):0;
    uc_decal_removal_render(g_ecx,context);
#endif
    sub_0009C4A0();
}
