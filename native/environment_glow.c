#include "environment_glow.h"
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
int uc_environment_glow_reset(UCEnvironmentGlowKernel *k){
 if(!k)return 1;
 memset(k,0,sizeof *k);k->sigma=-1;return 0;
}
int uc_environment_glow_prepare(UCEnvironmentGlowKernel *k,float sigma,UCEnvironmentGlowTaps *out){
 if(!k||!out||!isfinite(sigma)||sigma<.5f||sigma>64||fegetround()!=FE_TONEAREST)return 1;
 const uintptr_t a=(uintptr_t)k,b=(uintptr_t)out;
 if(a<=b?b-a<sizeof *k:a-b<sizeof *out)return 1;
 if(k->sigma!=-1&&(!isfinite(k->sigma)||k->sigma<.5f||k->sigma>64))return 1;
 UCEnvironmentGlowKernel next=*k;UCEnvironmentGlowTaps taps;
 if(sigma!=k->sigma){
  const float scale=(float)(1.0/sqrt((double)sigma*(double)6.2831854820251465f));
  float denominator=sigma*sigma;denominator=denominator*2.0f;
  const float inverse=1.0f/denominator;
  float sum=0;
  for(unsigned i=0;i<5;i++){
   double exponent=-(double)(i*i)*(double)inverse;
   exponent=exponent*1.44269504088896340736;
   const double integral=nearbyint(exponent);
   double weight=exp2(exponent-integral)-1.0;weight=weight+1.0;
   weight=ldexp(weight,(int)integral);weight=weight*(double)scale;
   next.weights[i]=(float)weight;
   const float term=i?next.weights[i]*2.0f:next.weights[i];sum=term+sum;
  }
  const float reciprocal=1.0f/sum;
  /* Source loop normalizes only indices0..3 for count9. Preserve the raw
   * outermost index4; normalizing all five would change the original filter. */
  for(unsigned i=0;i<4;i++)next.weights[i]=next.weights[i]*reciprocal;
  next.sigma=sigma;
 }
 for(unsigned i=0;i<5;i++)if(!isfinite(next.weights[i])||next.weights[i]<0)return 1;
 taps.center=next.weights[0];
 for(unsigned i=0;i<2;i++){
  const float lo=next.weights[2*i+1],hi=next.weights[2*i+2];
  taps.pair_weight[i]=lo+hi;if(!(taps.pair_weight[i]>0))return 1;
  float offset=hi/taps.pair_weight[i];offset=offset+(float)(2*i);taps.pair_offset[i]=offset+1.0f;
 }
 *k=next;*out=taps;return 0;
}
