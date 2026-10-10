#ifndef NV2A_INDEX_BOUNDS_H
#define NV2A_INDEX_BOUNDS_H
#include <stdint.h>
#include <string.h>
#if defined(_M_X64) || defined(__SSE2__)
#include <emmintrin.h>
#endif
/* Fresh packet-owned bytes, unsigned comparisons, no speculative tail reads.
 * SSE2 is mandatory on x64; no additional CPU feature requirement. */
static void nv_index_bounds(const void *source,unsigned width,unsigned count,
    uint32_t *low,uint32_t *high){
    const uint8_t *p=(const uint8_t*)source;
    uint32_t lo=UINT32_MAX,hi=0;unsigned i=0;
#if defined(_M_X64) || defined(__SSE2__)
    if(width==2 && count>=16){
        __m128i mn=_mm_set1_epi32(-1),mx=_mm_setzero_si128();
        if(width==2){
            /* Bias makes signed SSE2 word min/max implement unsigned order. */
            __m128i bias=_mm_set1_epi16((short)0x8000);
            mn=_mm_set1_epi16(32767);mx=_mm_set1_epi16((short)0x8000);
            for(;i+8<=count;i+=8){
                __m128i v=_mm_xor_si128(_mm_loadu_si128((const __m128i*)(p+2*i)),bias);
                mn=_mm_min_epi16(mn,v);mx=_mm_max_epi16(mx,v);
            }
            uint16_t a[8],b[8];
            _mm_storeu_si128((__m128i*)a,_mm_xor_si128(mn,bias));
            _mm_storeu_si128((__m128i*)b,_mm_xor_si128(mx,bias));
            for(unsigned j=0;j<8;j++){if(a[j]<lo)lo=a[j];if(b[j]>hi)hi=b[j];}
        }
    }
#endif
    for(;i<count;i++){
        uint32_t v;if(width==2){uint16_t s;memcpy(&s,p+2*i,2);v=s;}else memcpy(&v,p+4*i,4);
        if(v<lo)lo=v;if(v>hi)hi=v;
    }
    *low=lo;*high=hi;
}
#endif
