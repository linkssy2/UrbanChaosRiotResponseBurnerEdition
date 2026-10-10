#ifndef NV_FIXED_EQUAL_H
#define NV_FIXED_EQUAL_H
#include <emmintrin.h>
/* Exact byte equality, no FP comparisons, no alignment requirement or overread.
 * SSE2 is baseline on the AMD64 host. These helpers avoid CRT dispatch for
 * the two fixed-size keys observed in the CPU capture. */
static __forceinline int nv_equal32(const void*a,const void*b){
 const __m128i*x=(const __m128i*)a,*y=(const __m128i*)b;
 __m128i d=_mm_or_si128(_mm_xor_si128(_mm_loadu_si128(x),_mm_loadu_si128(y)),
                        _mm_xor_si128(_mm_loadu_si128(x+1),_mm_loadu_si128(y+1)));
 return _mm_movemask_epi8(_mm_cmpeq_epi8(d,_mm_setzero_si128()))==65535;
}
static __forceinline int nv_equal96(const void*a,const void*b){
 const __m128i*x=(const __m128i*)a,*y=(const __m128i*)b;
 __m128i d=_mm_setzero_si128();
 for(unsigned i=0;i<6;i++)d=_mm_or_si128(d,_mm_xor_si128(_mm_loadu_si128(x+i),_mm_loadu_si128(y+i)));
 return _mm_movemask_epi8(_mm_cmpeq_epi8(d,_mm_setzero_si128()))==65535;
}
#endif
