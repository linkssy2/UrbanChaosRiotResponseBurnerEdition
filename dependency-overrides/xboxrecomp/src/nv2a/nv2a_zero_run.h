#ifndef NV_ZERO_RUN_H
#define NV_ZERO_RUN_H
#include <emmintrin.h>
#include <stdint.h>
#include <string.h>
/* Caller bounds count by packet, published PUT, RAM and drain budget. Loads
 * never cross that range. A nonzero lane stays unconsumed for scalar dispatch. */
static unsigned nv_zero_prefix(const uint8_t*p,unsigned count){
 unsigned i=0;const __m128i zero=_mm_setzero_si128();
 for(;count-i>=4;i+=4){
  __m128i v=_mm_loadu_si128((const __m128i*)(p+4*i));
  unsigned mask=(unsigned)_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(v,zero)));
  if(mask!=15){for(unsigned j=0;j<4;j++)if(!(mask&(1u<<j)))return i+j;}
 }
 for(;i<count;i++){uint32_t v;memcpy(&v,p+4*i,4);if(v)break;}
 return i;
}
#endif
