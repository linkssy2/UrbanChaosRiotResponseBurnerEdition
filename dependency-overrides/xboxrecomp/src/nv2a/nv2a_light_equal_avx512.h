#ifndef UCRR_LIGHT_EQUAL_AVX512_H
#define UCRR_LIGHT_EQUAL_AVX512_H
#include <immintrin.h>
#include <intrin.h>
/* Opt-in candidate. AVX512F only; OS must save XMM/YMM/opmask/ZMM state.
 * Exact equality, no ordering guarantee, no load outside either input range. */
static int light_avx512_flags(unsigned leaf1_ecx,unsigned leaf7_ebx,unsigned long long xcr0){
 return (leaf1_ecx&0x18000000)==0x18000000&&(xcr0&0xE6)==0xE6&&(leaf7_ebx&(1u<<16));
}
static int light_avx512_supported(void){
 int cpu[4];__cpuid(cpu,0);if(cpu[0]<7)return 0;
 __cpuid(cpu,1);unsigned ecx=(unsigned)cpu[2];if((ecx&0x18000000)!=0x18000000)return 0;
 unsigned long long xcr0=_xgetbv(0);
 __cpuidex(cpu,7,0);return light_avx512_flags(ecx,(unsigned)cpu[1],xcr0);
}
__declspec(noinline) static int avx512_equal(const unsigned char*a,const unsigned char*b,size_t n){
 const __m512i zero=_mm512_setzero_si512();
 while(n>=256){
  __m512i x0=_mm512_xor_si512(_mm512_loadu_si512(a),_mm512_loadu_si512(b));
  __m512i x1=_mm512_xor_si512(_mm512_loadu_si512(a+64),_mm512_loadu_si512(b+64));
  __m512i x2=_mm512_xor_si512(_mm512_loadu_si512(a+128),_mm512_loadu_si512(b+128));
  __m512i x3=_mm512_xor_si512(_mm512_loadu_si512(a+192),_mm512_loadu_si512(b+192));
  __m512i x=_mm512_or_si512(_mm512_or_si512(x0,x1),_mm512_or_si512(x2,x3));
  if(_mm512_cmpeq_epi64_mask(x,zero)!=255){_mm256_zeroupper();return 0;}
  a+=256;b+=256;n-=256;
 }
 while(n>=64){
  __m512i x=_mm512_xor_si512(_mm512_loadu_si512(a),_mm512_loadu_si512(b));
  if(_mm512_cmpeq_epi64_mask(x,zero)!=255){_mm256_zeroupper();return 0;}
  a+=64;b+=64;n-=64;
 }
 _mm256_zeroupper();while(n--){if(*a++!=*b++)return 0;}return 1;
}
#endif
