/* Constraint removal operates on the authoritative guest graph. The two lists
 * have different policies: reciprocal pointers swap the tail; owner records
 * shift stably and repair packed-runtime pointers. No host shadow ownership. */
#include "migration_physics_constraints.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002E0DA0(void);
extern void sub_002ED490(void);

static int enabled(void) {
    static int flag = -1;
    if (flag < 0) { const char *v = getenv("UCRR_NATIVE_PHYSICS_CONSTRAINTS"); flag = v && !strcmp(v, "1"); }
    return flag;
}
static int span(uint32_t p, uint32_t n) {
    return p >= 0x10000u && p < 0x80000000u && (uint64_t)p + n <= g_xbox_total_ram;
}
static int apart(uint32_t p, uint32_t n, uint32_t q, uint32_t m) {
    return (uint64_t)p + n <= q || (uint64_t)q + m <= p;
}
static int safe(uint32_t p, uint32_t n) { return span(p,n) && apart(p,n,g_esp-1024,1040); }
static int body_valid(uint32_t b) {
    if (!safe(b,0x8c)) return 0;
    const unsigned offsets[] = {0x68,0x74,0x80}, strides[] = {28,4,1};
    for (unsigned i=0;i<3;i++) {
        uint32_t a=b+offsets[i], count=MEM32(a+4), cap=MEM32(a+8)&0x3fffffffu;
        /* ponytail: bounded graph scan; extend the proof before lifting this admission ceiling. */
        if (count>cap || cap>65536 || (cap && !safe(MEM32(a),cap*strides[i]))) return 0;
    }
    return 1;
}
static int constraint_valid(uint32_t c) {
    if (!safe(c,40)) return 0;
    uint32_t island=MEM32(c+8), r=MEM32(c+36);
    if (!safe(island,0x28) || !safe(MEM32(island),20) || !safe(r,28) || MEM32(r)!=c || MEM8(r+17)>1) return 0;
    uint32_t owner=MEM32(r+4+4*MEM8(r+17)), other=MEM32(r+8-4*MEM8(r+17));
    if (!body_valid(owner) || !body_valid(other)) return 0;
    uint32_t start=MEM32(owner+0x68), count=MEM32(owner+0x6c), index=MEM16(r+18);
    if (r<start || (r-start)%28 || (r-start)/28>=count || index>=MEM32(other+0x78) || MEM32(MEM32(other+0x74)+index*4)!=c) return 0;
    if (MEM16(c+4) && !safe(MEM32(c),4)) return 0;
    uint32_t runtime=MEM32(owner+0x80), bytes=0;
    for (uint32_t i=0;i<count;i++) {
        uint32_t row=start+i*28, q=MEM32(row), data=MEM32(row+12), payload=MEM32(row+24);
        if (!safe(q,40) || MEM32(q+36)!=row || MEM8(row+17)>1 || MEM32(row+4+4*MEM8(row+17))!=owner || !safe(data,4) || !safe(MEM32(data),36)) return 0;
        uint32_t reciprocal=MEM32(row+8-4*MEM8(row+17));
        if (!body_valid(reciprocal) || MEM16(row+18)>=MEM32(reciprocal+0x78) || MEM32(MEM32(reciprocal+0x74)+MEM16(row+18)*4)!=q) return 0;
        if (payload) { uint32_t width=MEM16(row+20); if (payload!=runtime+bytes || (uint64_t)bytes+width>MEM32(owner+0x84)) return 0; bytes+=width; }
    }
    if (bytes!=MEM32(owner+0x84)) return 0;
    for (uint32_t i=0;i<MEM32(other+0x78);i++) {
        uint32_t q=MEM32(MEM32(other+0x74)+4*i);
        if (!safe(q,40) || !safe(MEM32(q+36),28)) return 0;
        uint32_t row=MEM32(q+36);
        if (MEM32(row)!=q || MEM8(row+17)>1 || MEM32(row+8-4*MEM8(row+17))!=other || MEM16(row+18)!=i) return 0;
    }
    return 1;
}
static int listeners_valid(uint32_t world) {
    if (!safe(world,0xe8)) return 0;
    uint32_t count=MEM32(world+0xe4), list=MEM32(world+0xe0);
    if (count>65536 || (count && !safe(list,count*4))) return 0;
    for (uint32_t i=0;i<count;i++) {
        uint32_t p=MEM32(list+i*4);
        if (p && (!safe(p,4) || !safe(MEM32(p),12))) return 0;
    }
    return 1;
}

/* Reverse notification takes its initial cursor once, but reloads the list
 * after every callback. The separate stable compaction uses the live count. */
static void notify_removed(void) {
    PUSH32(g_esp,g_ebx); PUSH32(g_esp,g_esi); g_esi=MEM32(g_esp+12);
    PUSH32(g_esp,g_edi); g_edi=MEM32(g_esi+0xe4)-1;
    if ((int32_t)g_edi>=0) {
        g_ebx=MEM32(g_esp+20);
        do {
            g_eax=MEM32(g_esi+0xe0); g_ecx=MEM32(g_eax+g_edi*4);
            if (g_ecx) {
                g_edx=MEM32(g_ecx); uint32_t saved=g_esp;
                PUSH32(g_esp,g_ebx); uint32_t target=MEM32(g_edx+8);
                PUSH32(g_esp,0x2fae77); RECOMP_ICALL_SAFE(target,saved);
            }
            --g_edi;
        } while ((int32_t)g_edi>=0);
    }
    g_edx=MEM32(g_esi+0xe4)-1;
    while ((int32_t)g_edx>=0) {
        g_eax=MEM32(g_esi+0xe0);
        if (!MEM32(g_eax+g_edx*4)) {
            g_ebx=MEM32(g_esi+0xe4)-1; g_ecx=g_ebx; MEM32(g_esi+0xe4)=g_ebx; g_eax=g_edx;
            while ((int32_t)g_eax<(int32_t)g_ecx) {
                g_ecx=MEM32(g_esi+0xe0); g_edi=MEM32(g_ecx+g_eax*4+4); g_ecx+=g_eax*4;
                MEM32(g_ecx)=g_edi; g_ecx=MEM32(g_esi+0xe4); ++g_eax;
            }
        }
        --g_edx;
    }
    POP32(g_esp,g_edi); POP32(g_esp,g_esi); POP32(g_esp,g_ebx);
    g_recomp_zf=0; g_esp+=4;
}

static void unlink_constraint(void) {
    uint32_t frame=g_seh_ebp;
    g_esp-=16; uint32_t saved=g_esp;
    PUSH32(g_esp,g_ebx); g_ebx=MEM32(g_esp+24); g_eax=MEM32(g_ebx+8);
    PUSH32(g_esp,frame); PUSH32(g_esp,g_esi); g_esi=MEM32(g_ebx+36);
    MEM8(g_eax+38)=1; g_ecx=MEM32(g_esi+12); g_edx=MEM32(g_ecx);
    PUSH32(g_esp,g_edi); g_eax=g_esp+16; PUSH32(g_esp,g_eax);
    uint32_t target=MEM32(g_edx+32); PUSH32(g_esp,0x2fef12); RECOMP_ICALL_SAFE(target,saved);
    /* Info callbacks may change the constraint's island. Resolve it again. */
    g_ecx=MEM32(g_ebx+8); g_edx=MEM32(g_ecx); g_eax=g_esp+16; saved=g_esp;
    PUSH32(g_esp,g_eax); PUSH32(g_esp,g_ebx); target=MEM32(g_edx+16);
    PUSH32(g_esp,0x2fef20); RECOMP_ICALL_SAFE(target,saved);
    MEM32(g_ebx+8)=0;

    /* Reciprocal pointer removal swaps the last pointer and repairs its index. */
    g_edx=MEM8(g_esi+17); SET_LO16(g_ecx,MEM16(g_esi+18));
    g_eax=MEM32(g_esi+(2-g_edx)*4); g_edx=MEM32(g_eax+0x74);
    g_edi=MEM32(g_eax+0x78); g_edi=MEM32(g_edx+g_edi*4-4); g_eax+=0x74;
    frame=(uint16_t)g_ecx; MEM32(g_edx+frame*4)=g_edi; --MEM32(g_eax+4);
    g_edx=MEM32(g_edi+36); MEM16(g_edx+18)=(uint16_t)g_ecx;
    g_eax=MEM8(g_esi+17); frame=MEM32(g_esi+g_eax*4+4); g_eax=MEM32(g_esi+24);
    g_edi=0; MEM32(g_esp+36)=0;
    if (g_eax) {
        g_edx=MEM16(g_esi+20); g_ecx=0-g_edx; MEM32(g_esp+36)=g_ecx;
        g_ecx=g_edx+g_eax; g_edx=MEM32(frame+0x84)-g_ecx+MEM32(frame+0x80);
        PUSH32(g_esp,g_edx); PUSH32(g_esp,g_ecx); PUSH32(g_esp,g_eax);
        PUSH32(g_esp,0x2fef90); sub_002E0DA0();
        g_eax=MEM16(g_esi+20); g_ecx=MEM32(frame+0x84); g_esp+=12;
        g_ecx-=g_eax; MEM32(frame+0x84)=g_ecx; MEM32(g_esi+24)=g_edi;
    }
    /* Stable record compaction preserves solver order and repairs both kinds
     * of pointers. Each move is one nonoverlapping 28-byte record. */
    MEM32(g_ebx+36)=g_edi; g_ecx=MEM32(frame+0x6c)*28; g_edx=MEM32(frame+0x68)+g_ecx-28; g_eax=g_esi;
    if (g_eax<g_edx) {
        do {
            g_esi=g_eax+28; g_ecx=7; g_edi=g_eax;
            memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),28); g_esi+=28; g_edi+=28; g_ecx=0;
            g_ecx=MEM32(g_eax); MEM32(g_ecx+36)=g_eax; g_ecx=MEM32(g_eax+24);
            if (g_ecx) { g_esi=MEM32(g_esp+36); g_ecx+=g_esi; }
            MEM32(g_eax+24)=g_ecx; g_eax+=28;
        } while (g_eax<g_edx);
        g_edi=0;
    }
    --MEM32(frame+0x6c);
    if (MEM16(g_ebx+4)!=(uint16_t)g_edi) {
        --MEM16(g_ebx+6);
        if (MEM16(g_ebx+6)==(uint16_t)g_edi) {
            g_edx=MEM32(g_ebx); saved=g_esp; PUSH32(g_esp,1); g_ecx=g_ebx;
            target=MEM32(g_edx); PUSH32(g_esp,0x2ff009); RECOMP_ICALL_SAFE(target,saved);
        }
    }
    POP32(g_esp,g_edi); POP32(g_esp,g_esi); POP32(g_esp,frame); POP32(g_esp,g_ebx);
    g_esp+=16; g_recomp_zf=!g_esp; g_esp+=4;
}

int uc_migration_physics_constraints(unsigned entry) {
    if (!enabled() || (g_esp&3) || g_esp<0x10400 || !span(g_esp-1024,1040)) return 0;
    if (entry==0x2feef0) {
        if (!constraint_valid(MEM32(g_esp+4))) return 0;
        unlink_constraint(); return 1;
    }
    if (entry==0x2fae50) {
        if (!listeners_valid(MEM32(g_esp+4))) return 0;
        notify_removed(); return 1;
    }
    if (entry!=0x2f6600 || !listeners_valid(MEM32(g_esp+4)) || !constraint_valid(MEM32(g_esp+8))) return 0;
    /* Admission is complete before busy/listener/graph effects. Later callback
     * mutation and memory lifetime obey the same contract as original code. */
    uint32_t frame=g_seh_ebp;
    g_eax=MEM32(g_esp+12); PUSH32(g_esp,g_esi); g_esi=MEM32(g_esp+8);
    g_ecx=MEM32(g_esi+0x88)+1; PUSH32(g_esp,g_edi); g_edi=MEM32(g_esp+16); MEM32(g_esi+0x88)=g_ecx;
    if (g_eax) {
        g_eax=MEM32(g_esi+0xe4);
        if (g_eax) { PUSH32(g_esp,g_edi); PUSH32(g_esp,g_esi); PUSH32(g_esp,0x2f6630); notify_removed(); g_esp+=8; }
    }
    PUSH32(g_esp,g_edi); PUSH32(g_esp,0x2f6639); unlink_constraint();
    g_eax=MEM32(g_esi+0x88); g_esp+=4; --g_eax; MEM32(g_esi+0x88)=g_eax;
    if (!g_eax) {
        g_eax=MEM32(g_esi+0x84);
        if (g_eax) {
            SET_LO8(g_eax,MEM8(g_esi+0x90));
            if (!(uint8_t)g_eax) {
                POP32(g_esp,g_edi); g_ecx=g_esi; POP32(g_esp,g_esi);
                g_seh_ebp=frame; sub_002ED490(); return 1;
            }
        }
    }
    POP32(g_esp,g_edi); POP32(g_esp,g_esi); g_esp+=4; return 1;
}
