#ifndef UCRR_NATIVE_BOUNDS_LEAF_H
#define UCRR_NATIVE_BOUNDS_LEAF_H
#include <stddef.h>
#include "../third_party/xboxrecomp/src/d3d/d3d8_leaf_coverage.h"
typedef struct UcBoundsLeaf {
    const void *a,*c,*d;void *stack;const void *threshold;
    uint32_t input_eax,output_eax;
    RecompXmm x[8];
} UcBoundsLeaf;
_Static_assert(offsetof(UcBoundsLeaf,x)==48,"native leaf ABI");
void ucrr_native_bounds_execute(UcBoundsLeaf*);
/* Ordinary mapped retail RAM only. No MMIO, 32-bit wrap or alias-window edge.
 * A closed leaf cannot reenter guest producers; overlapping RAM remains legal
 * because the native body preserves the original load/store ordering. */
static int uc_bounds_span(uint32_t va,unsigned bytes,uint32_t *out){
    uint32_t p=recomp_normalize_xbox_va(va);
    if((uint64_t)va+bytes>0x100000000ull||p>0x04000000u-bytes||
       recomp_normalize_xbox_va(va+bytes-1)!=p+bytes-1)return 0;
    *out=p;return 1;
}
static int uc_bounds_finite(const void *p,unsigned words){
    const volatile uint32_t *v=p;
    for(unsigned i=0;i<words;i++){uint32_t bits=v[i];if((bits&0x7f800000u)==0x7f800000u){uc_leaf_count((bits&0x007fffffu)?2:3);return 0;}}
    return 1;
}
static int ucrr_native_bounds_try(void){
    uint32_t a,c,d,s;
    if(g_esp<28||!uc_bounds_span(g_eax,12,&a)||!uc_bounds_span(g_ecx,12,&c)||
       !uc_bounds_span(g_edx,24,&d)||!uc_bounds_span(g_esp-28,28,&s)){uc_leaf_count(1);return 0;}
    UcBoundsLeaf v;
    v.a=(void*)(g_xbox_mem_offset+a);v.c=(void*)(g_xbox_mem_offset+c);
    v.d=(void*)(g_xbox_mem_offset+d);v.stack=(void*)(g_xbox_mem_offset+s);
    v.threshold=(void*)(g_xbox_mem_offset+0x44d660);
    /* The historical C reference does not preserve authored operand order for
     * every exceptional pair. Keep its exact behavior on exceptional inputs
     * until full-title acceptance of the native SSE NaN contract is available. */
    if(!uc_bounds_finite(v.a,3)||!uc_bounds_finite(v.c,3)||!uc_bounds_finite(v.d,6)||
       !uc_bounds_finite(v.threshold,1))return 0;
    v.input_eax=g_eax;ucrr_native_bounds_execute(&v);
    g_eax=v.output_eax;g_recomp_zf=(g_esp==0);g_esp+=4;
    g_xmm0=v.x[0];g_xmm1=v.x[1];g_xmm2=v.x[2];g_xmm3=v.x[3];
    g_xmm4=v.x[4];g_xmm5=v.x[5];g_xmm6=v.x[6];g_xmm7=v.x[7];
    uc_leaf_count(0);
    return 1;
}
#endif
