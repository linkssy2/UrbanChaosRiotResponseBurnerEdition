#ifndef UCRR_LIGHT_EQUAL_AVX2_H
#define UCRR_LIGHT_EQUAL_AVX2_H
/* Opt-in candidate. Caller must check AVX2 plus OS XMM/YMM context support.
 * Exact equality, not lexicographic ordering; no loads past either range. */
#include <immintrin.h>
#include <intrin.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nv2a_light_equal_avx512.h"
#include "nv2a_light_ab_phase.h"
static NvLightAbPhase light_ab_phase;
static uint32_t light_ab_frame;
static void nv_light_ab_reset(void){memset(&light_ab_phase,0,sizeof(light_ab_phase));light_ab_frame=0;}
static int nv_light_avx2_supported(void){
 int cpu[4];__cpuid(cpu,0);if(cpu[0]<7)return 0;__cpuid(cpu,1);
 if((cpu[2]&0x18000000)!=0x18000000||(_xgetbv(0)&6)!=6)return 0;
 __cpuidex(cpu,7,0);return !!(cpu[1]&32);
}
__declspec(noinline) static int avx_equal(const unsigned char*a,const unsigned char*b,size_t n){
 while(n>=128){
  __m256i x0=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)a),_mm256_loadu_si256((const __m256i*)b));
  __m256i x1=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+32)),_mm256_loadu_si256((const __m256i*)(b+32)));
  __m256i x2=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+64)),_mm256_loadu_si256((const __m256i*)(b+64)));
  __m256i x3=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+96)),_mm256_loadu_si256((const __m256i*)(b+96)));
  __m256i x=_mm256_or_si256(_mm256_or_si256(x0,x1),_mm256_or_si256(x2,x3));
  if(!_mm256_testz_si256(x,x)){_mm256_zeroupper();return 0;}
  a+=128;b+=128;n-=128;
 }
 while(n>=32){
  __m256i x=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)a),_mm256_loadu_si256((const __m256i*)b));
  if(!_mm256_testz_si256(x,x)){_mm256_zeroupper();return 0;}a+=32;b+=32;n-=32;
 }
 _mm256_zeroupper();while(n--){if(*a++!=*b++)return 0;}return 1;
}
/* Renderer-owned initialization. Only equality is consumed by the lighting
 * cache; this helper deliberately does NOT promise memcmp's ordering result. */
static int nv_light_compare_bytes(const void*a,const void*b,size_t n){
 static int enabled=-1,wide,ab;
 if(enabled<0){
  const char*flag=getenv("UCRR_LIGHT_AVX2");enabled=flag&&!strcmp(flag,"1")&&nv_light_avx2_supported();
  flag=getenv("UCRR_LIGHT_AVX512");wide=flag&&!strcmp(flag,"1")&&light_avx512_supported();
  flag=getenv("UCRR_LIGHT_AB");ab=flag&&!strcmp(flag,"1")&&enabled&&wide;
 }
 /* Measured large cube only; the small volume loses with wider vectors. */
 if(wide&&n==393216){
  if(ab){
   if(nv_light_ab_step(&light_ab_phase,light_ab_frame))fprintf(stderr,"[LIGHT-AB] frame=%u phase=%u mode=%s exhausted=%u\n",light_ab_frame,light_ab_phase.phase,nv_light_ab_wide(&light_ab_phase)?"avx512":"avx2",light_ab_phase.phase>=12);
   if(!nv_light_ab_wide(&light_ab_phase))return !avx_equal((const unsigned char*)a,(const unsigned char*)b,n);
  }
  static int reported;
  if(!reported){reported=1;fprintf(stderr,"[LIGHT-EQUALITY] active=avx512f bytes=%zu os_state=verified\n",n);}
  return !avx512_equal((const unsigned char*)a,(const unsigned char*)b,n);
 }
 return enabled?!avx_equal((const unsigned char*)a,(const unsigned char*)b,n):memcmp(a,b,n);
}
#endif
