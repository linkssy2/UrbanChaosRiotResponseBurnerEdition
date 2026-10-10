#ifndef UCRR_CPU_BUNDLE_GUEST_H
#define UCRR_CPU_BUNDLE_GUEST_H
/* Opt-in closed guest replacements, not persistent guest-state caches. */
#include <stdlib.h>
#include <string.h>
#include <emmintrin.h>
static int cpu_bundle_copy_option=-1,cpu_bundle_matrix_option=-1,cpu_bundle_emitter_option=-1;
static int cpu_bundle_vertex_tail_option=-1;
static __forceinline int cpu_bundle_enabled(int*value,const char*name){
    if(*value<0){const char*s=getenv(name);*value=s&&s[0]=='1'&&!s[1];}return *value;
}
static __forceinline int cpu_bundle_span(uint32_t va,uint32_t n,uint32_t*p){
    *p=recomp_normalize_xbox_va(va);
    return n&&n<=0x04000000u&&(uint64_t)va+n<=0x100000000ull&&
        *p<=0x04000000u-n&&recomp_normalize_xbox_va(va+n-1)==*p+n-1;
}
static __forceinline int cpu_bundle_overlap(uint32_t a,uint32_t n,uint32_t b,uint32_t m){
    return a<b+m&&b<a+n;
}
static __forceinline int cpu_bundle_copy_admit(void){
    uint32_t st,src,dst;
    if(g_esp<16||!cpu_bundle_span(g_esp-16,32,&st))return 0;
    uint32_t s=MEM32(g_esp+8),d=MEM32(g_esp+4),count=MEM32(g_esp+12);
    uint32_t prefix=((0u-d)&31u)>>2;
    if((d&3u)||count<prefix+32u||count>0x01000000u)return 0;
    uint32_t n=count*4;
    return cpu_bundle_span(s,n,&src)&&cpu_bundle_span(d,n,&dst)&&
        !cpu_bundle_overlap(src,n,dst,n)&&!cpu_bundle_overlap(st,32,src,n)&&
        !cpu_bundle_overlap(st,32,dst,n);
}
/* Synchronous guest writes, not a CRT/OS output buffer. Explicit volatile SSE
 * stores retain the existing protected-RAM fault path, including aliased views.
 * No resource native epoch bump: the decoder cannot reenter inside this loop. */
static __forceinline void cpu_bundle_copy_blocks(uint32_t source,uint32_t dest,uint32_t bytes){
    const unsigned char*s=(const unsigned char*)XBOX_PTR(source);
    unsigned char*d=(unsigned char*)XBOX_PTR(dest);
    __m128i a,b,c,e;
    for(uint32_t at=0;at<bytes;at+=64){
        a=_mm_loadu_si128((const __m128i*)(s+at));
        b=_mm_loadu_si128((const __m128i*)(s+at+16));
        c=_mm_loadu_si128((const __m128i*)(s+at+32));
        e=_mm_loadu_si128((const __m128i*)(s+at+48));
        /* The original prefix makes the bulk destination 32-byte aligned. */
        *(volatile __m128i*)(d+at)=a;*(volatile __m128i*)(d+at+16)=b;
        *(volatile __m128i*)(d+at+32)=c;*(volatile __m128i*)(d+at+48)=e;
    }
    /* Input and output never overlap on this route; preserve all eight guest
     * MMX return values even though EMMS does not consume them in our ABI. */
    _mm_storeu_si128((__m128i*)&g_mmx[0],a);_mm_storeu_si128((__m128i*)&g_mmx[2],b);
    _mm_storeu_si128((__m128i*)&g_mmx[4],c);_mm_storeu_si128((__m128i*)&g_mmx[6],e);
}
static __forceinline int cpu_bundle_matrix_admit(void){
    uint32_t frame,input;
    if(g_esp<0xd0)return 0;
    uint32_t low=((g_esp-4)&~15u)-0xc0;
    if(!cpu_bundle_span(low,g_esp+4-low,&frame)||!cpu_bundle_span(g_ecx,64,&input)||
       cpu_bundle_overlap(frame,g_esp+4-low,input,64)||
       cpu_bundle_overlap(frame,g_esp+4-low,0x44c174,4))return 0;
    /* NaN operand payload selection is not allowed to change under register
     * localization. Large/nonfinite inputs and unusual global constants fall
     * back. Singular finite matrices remain in the differential fixture. */
    if((MEM32(0x44c174)&0x7fffffffu)>0x49800000u)return 0;
    __m128i bad=_mm_setzero_si128(),mask=_mm_set1_epi32(0x7fffffff),limit=_mm_set1_epi32(0x49800000);
    for(unsigned k=0;k<64;k+=16){
        __m128i value=_mm_loadu_si128((const __m128i*)(g_xbox_mem_offset+input+k));
        bad=_mm_or_si128(bad,_mm_cmpgt_epi32(_mm_and_si128(value,mask),limit));
    }
    return !_mm_movemask_epi8(bad);
}
typedef struct CpuBundleRegs {uint32_t a,c,d,s,b,i,j;} CpuBundleRegs;
static __forceinline CpuBundleRegs cpu_bundle_regs_get(void){
    CpuBundleRegs r={g_eax,g_ecx,g_edx,g_esp,g_ebx,g_esi,g_edi};return r;
}
static __forceinline void cpu_bundle_regs_put(CpuBundleRegs r){
    g_eax=r.a;g_ecx=r.c;g_edx=r.d;g_esp=r.s;g_ebx=r.b;g_esi=r.i;g_edi=r.j;
}
static __forceinline int cpu_bundle_emitter_admit(void){
    uint32_t frame,out;
    if(g_esp<64||!cpu_bundle_span(g_esp-64,72,&frame))return 0;
    uint32_t cursor=MEM32(0x3f4240),end=MEM32(0x3f4244);
    if((uint64_t)cursor+40>=end||!cpu_bundle_span(cursor,40,&out)||
       cpu_bundle_overlap(frame,72,out,40))return 0;
    /* Exclude aliases of all control/table/constant ranges read or written by
     * this closure. Without callbacks these cannot mutate between five pairs. */
    const uint32_t regions[][2]={{0x3f3f00,0x360},{0x44b70c,20},{0x44c510,4},{0x44d660,4}};
    for(unsigned k=0;k<sizeof(regions)/sizeof(regions[0]);k++)
        if(cpu_bundle_overlap(out,40,regions[k][0],regions[k][1])||
           cpu_bundle_overlap(frame,72,regions[k][0],regions[k][1]))return 0;
    return 1;
}
#endif
