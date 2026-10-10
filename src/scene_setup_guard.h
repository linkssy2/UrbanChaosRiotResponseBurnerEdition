#ifndef UCRR_SCENE_SETUP_GUARD_H
#define UCRR_SCENE_SETUP_GUARD_H
#include <emmintrin.h>
/* Closed matrix spans only: finite bounded ordinary RAM, no stack/source
 * overlap. Large/exceptional inputs retain the original translated span.
 * 2^20 inputs keep these two fixed matrix products far below float overflow.
 * This is NOT a visibility, distance, or LOD threshold. */
static __forceinline int scene_setup_span(uint32_t va,unsigned n,uint32_t *p){
    *p=recomp_normalize_xbox_va(va);
    return n && (uint64_t)va+n<=0x100000000ull && *p<=0x04000000u-n &&
        recomp_normalize_xbox_va(va+n-1)==*p+n-1;
}
static __forceinline int scene_setup_values(uint32_t p,unsigned n){
    __m128i bad=_mm_setzero_si128(),mask=_mm_set1_epi32(0x7fffffff),limit=_mm_set1_epi32(0x49800000);
    for(unsigned i=0;i<n;i+=16){
        __m128i v=_mm_loadu_si128((const __m128i*)(g_xbox_mem_offset+p+i));
        bad=_mm_or_si128(bad,_mm_cmpgt_epi32(_mm_and_si128(v,mask),limit));
    }
    return !_mm_movemask_epi8(bad);
}
static __forceinline int scene_setup_matrix_admit(uint32_t stack,uint32_t object,int material){
    uint32_t s,o;
    if(!scene_setup_span(stack,0x100,&s)||!scene_setup_span(object,0x1b0,&o)||
       (s<o+0x1b0 && o<s+0x100))return 0;
    if(material)return scene_setup_values(o+0x50,0xc0);
    return scene_setup_values(s+0x10,0x40)&&scene_setup_values(o+0xe0,0x40);
}
#endif
