#include "environment_tone.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

/* Retain the original rounded integer / fractional exp2 split, including the
 * subtract-one/add-one stores in the double-backed compatibility math. */
static int split_exp2(double value,double *out) {
    double integer, fraction;
    if(!isfinite(value)||value < -1022.0||value > 1023.0) return 0;
    integer=nearbyint(value);
    fraction=exp2(-(integer-value))-1.0;
    *out=ldexp(fraction+1.0,(int)integer);
    return isfinite(*out);
}
static int curve_exp(double value,float *out) {
    double result;
    if(!split_exp2(value*1.44269504088896340736,&result)) return 0;
    *out=(float)(1.0-result);
    return isfinite(*out);
}
UCToneStatus uc_environment_tone_build(const UCEnvironmentTone *s,uint8_t out[768]) {
    /* ponytail: rebuild 256 base entries per call; cache only in a future owned
     * dirty-update path if measured cost warrants it. No shared mutable state. */
    float table[256],position=0.0f,step,fraction=0.0f;
    uint8_t bytes[768];
    unsigned index=0;
    double exponent;
    if(!s||!out) return UC_TONE_ARGUMENT;
    if(!isfinite(s->exposure)||!isfinite(s->input_scale)||
       !isfinite(s->gamma_parameter)||!isfinite(s->inversion)||
       s->input_scale<0.0f||s->gamma_parameter<=-.25f) return UC_TONE_ARGUMENT;
    for(unsigned c=0;c<3;++c)
        if(!isfinite(s->offset[c])||!isfinite(s->gain[c])) return UC_TONE_ARGUMENT;
    if(fegetround()!=FE_TONEAREST||(_mm_getcsr()&0xE040u)!=0) return UC_TONE_UNSUPPORTED;
    step=(s->exposure*s->input_scale)*0.25098040699958801270f;
    if(!isfinite(step)||step>2147483000.0f) return UC_TONE_UNSUPPORTED;
    exponent=1.0/((double)s->gamma_parameter+.25);
    for(unsigned i=0;i<256;++i) {
        if(!curve_exp((double)position,&table[i])) return UC_TONE_UNSUPPORTED;
        position=position-0.01568627543747425079f;
    }
    for(unsigned i=0;i<256;++i) {
        float q,other,rgb[3];
        double powered;
        if(s->exposure>=0.0f&&index<=255) {
            q=table[index];
            if(index!=255) q=table[index+1]*fraction+(1.0f-fraction)*q;
            fraction=fraction+step;
            if(fraction>=1.0f) {
                unsigned advance=(unsigned)(int)fraction;
                fraction=fraction-(float)advance;
                index+=advance;
            }
        } else {
            q=((float)i*0.00392156885936856270f)*s->input_scale;
            if(!isfinite(q)) return UC_TONE_UNSUPPORTED;
            if(s->exposure>=0.0f&&!curve_exp(-((double)q*s->exposure),&q))
                return UC_TONE_UNSUPPORTED;
        }
        if(!isfinite(q)||q<0.0f) return UC_TONE_UNSUPPORTED;
        if(q==0.0f) powered=0.0;
        else if(!split_exp2(exponent*log2((double)q),&powered)) return UC_TONE_UNSUPPORTED;
        q=(float)powered;
        if(!isfinite(q)) return UC_TONE_UNSUPPORTED;
        other=(1.0f-q)*s->inversion;
        q=(1.0f-s->inversion)*q+other;
        rgb[0]=s->gain[0]*q+s->offset[0];
        rgb[1]=s->offset[1]+s->gain[1]*q;
        rgb[2]=s->offset[2]+s->gain[2]*q;
        if(!isfinite(q)) return UC_TONE_UNSUPPORTED;
        for(unsigned c=0;c<3;++c) {
            if(!isfinite(rgb[c])) return UC_TONE_UNSUPPORTED;
            if(rgb[c]<0.0f) rgb[c]=0.0f;
            if(rgb[c]>1.0f) rgb[c]=1.0f;
            bytes[c*256+i]=(uint8_t)((double)rgb[c]*255.0);
        }
    }
    memcpy(out,bytes,sizeof(bytes));return UC_TONE_OK;
}
