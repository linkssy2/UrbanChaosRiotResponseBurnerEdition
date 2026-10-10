#include "environment_exposure.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

static int finite_values(const float *v,unsigned n) {
    for(unsigned i=0;i<n;++i)if(!isfinite(v[i]))return 0;
    return 1;
}
static int rounding_ok(void) {return fegetround()==FE_TONEAREST&&(_mm_getcsr()&0xE040u)==0;}
static int state_finite(const UCEnvironmentExposure *s) {
    const float v[]={s->input_scale,s->target_brightness,s->dark_point,s->multiplier,
        s->glow_point,s->glow_multiplier,s->inverse_gamma,s->maximum,s->minimum,
        s->effect_exposure,s->effect_amount,s->adaptation_rate,s->effect_gain[0],
        s->effect_gain[1],s->effect_gain[2],s->adapted_exposure,s->effect_mix,
        s->effect_auxiliary,s->measured_brightness};
    return finite_values(v,sizeof(v)/sizeof(v[0]))&&s->effect_latched<=1;
}
static int provider_finite(const UCExposureToneProvider *p) {
    const UCEnvironmentTone *s=&p->settings;
    const float v[]={s->exposure,s->input_scale,s->gamma_parameter,s->inversion,
        s->offset[0],s->offset[1],s->offset[2],s->gain[0],s->gain[1],s->gain[2],p->post_scale};
    return finite_values(v,sizeof(v)/sizeof(v[0]))&&p->cpu_dirty<=1&&p->upload_dirty<=1&&p->alternate<=1;
}
static void scalar(float *dst,float value,UCExposureToneProvider *p) {
    if(*dst!=value){*dst=value;p->cpu_dirty=1;}
}
static void rgb(float dst[3],const float value[3],UCExposureToneProvider *p) {
    if(dst[0]!=value[0]||dst[1]!=value[1]||dst[2]!=value[2]) {
        memcpy(dst,value,3*sizeof(float));p->cpu_dirty=1;
    }
}
UCExposureStatus uc_environment_exposure_refresh(const UCExposureProfile *p,UCEnvironmentExposure *s) {
    if(!p||!s)return UC_EXPOSURE_ARGUMENT;
    const float v[]={p->dark_point,p->bright_point,p->glow_point,p->glow_multiplier,
        p->gamma,p->multiplier,p->target_brightness,p->minimum,p->maximum};
    if(!finite_values(v,9))return UC_EXPOSURE_ARGUMENT;
    if(!rounding_ok())return UC_EXPOSURE_UNSUPPORTED;
    UCEnvironmentExposure next=*s;
    next.dark_point=p->dark_point>=1?1:p->dark_point>0?p->dark_point:0;
    next.input_scale=1.0f/(p->bright_point<.01f?.01f:p->bright_point);
    next.glow_point=p->glow_point;next.glow_multiplier=p->glow_multiplier;
    next.inverse_gamma=1.0f/(p->gamma<.01f?.01f:p->gamma);
    next.multiplier=p->multiplier;next.target_brightness=p->target_brightness;
    next.minimum=p->minimum;next.maximum=p->maximum;*s=next;return UC_EXPOSURE_OK;
}
UCExposureStatus uc_environment_exposure_reset(const UCExposureProfile *p,UCEnvironmentExposure *s) {
    if(!p||!s)return UC_EXPOSURE_ARGUMENT;
    UCEnvironmentExposure next={0};
    UCExposureStatus status=uc_environment_exposure_refresh(p,&next);
    if(status)return status;
    next.effect_exposure=4;next.adaptation_rate=.45f;
    next.effect_gain[0]=.75f;next.effect_gain[1]=2;next.effect_gain[2]=.75f;
    next.adapted_exposure=2;next.measured_brightness=.5f;*s=next;return UC_EXPOSURE_OK;
}
UCExposureStatus uc_environment_exposure_tone_reset(UCExposureToneProvider *p) {
    if(!p)return UC_EXPOSURE_ARGUMENT;
    p->settings=(UCEnvironmentTone){-1,1,.8f,0,{0,0,0},{1,1,1}};
    p->post_scale=1;p->cpu_dirty=1;p->upload_dirty=p->alternate=0;return UC_EXPOSURE_OK;
}
UCExposureStatus uc_environment_exposure_prepare(UCExposureToneProvider *p) {
    if(!p||!provider_finite(p))return UC_EXPOSURE_ARGUMENT;
    if(!rounding_ok())return UC_EXPOSURE_UNSUPPORTED;
    if(p->cpu_dirty) {
        if(uc_environment_tone_build(&p->settings,p->lut)!=UC_TONE_OK)return UC_EXPOSURE_UNSUPPORTED;
        p->cpu_dirty=0;p->upload_dirty=1;
    }
    return UC_EXPOSURE_OK;
}
static int inverse(const uint8_t lut[768],float brightness,float target,float *value) {
    unsigned i;
    for(i=0;i<256;++i) {
        float sample=(float)(lut[i]+lut[256+i]+lut[512+i])*0.0013071895809844136f;
        if(sample>brightness)break;
    }
    float q=(float)i*0.00392156885936856270f;
    /* B493C overwrites argument1 with q; after both POPs the logarithm
     * consumes argument2 (target), and division consumes the overwritten q. */
    *value=q==0?1000.0f:(float)(-((0.69314718055994530942*log2(1.0-(double)target))/(double)q));
    return !isnan(*value); /* target1 yields+infinity, then49D00 clamps it. */
}
static int retention(float rate,float delta,double *result) {
    double base=1.0-(double)rate;
    if(delta==0){*result=1;return 1;}
    if(base==0){*result=0;return 1;}
    double value=(double)delta*log2(base);
    if(!isfinite(value)||value < -1022||value > 1023)return 0;
    double integer=nearbyint(value),fraction=exp2(-(integer-value))-1.0;
    *result=ldexp(fraction+1.0,(int)integer);return isfinite(*result);
}
UCExposureStatus uc_environment_exposure_step(UCEnvironmentExposure *state,
    UCExposureToneProvider *provider,const UCExposureInput *input,UCExposurePublication *out) {
    if(!state||!provider||!input||!out||!state_finite(state)||!provider_finite(provider)||
       !isfinite(input->delta)||input->delta<0||input->bypass>1||input->glow_present>1||
       input->force_inversion>1||input->disable_post_scale>1||input->disable_glow>1)return UC_EXPOSURE_ARGUMENT;
    if(!rounding_ok())return UC_EXPOSURE_UNSUPPORTED;
    UCEnvironmentExposure s=*state;UCExposureToneProvider p=*provider;UCExposurePublication result={0};
    if(input->bypass) {
        const float zero[3]={0,0,0};scalar(&p.post_scale,0,&p);rgb(p.settings.offset,zero,&p);
        scalar(&p.settings.gamma_parameter,1,&p);scalar(&p.settings.exposure,1,&p);scalar(&p.settings.input_scale,1,&p);
    } else {
        if(s.target_brightness<=0||s.inverse_gamma<=0||s.adaptation_rate<0||s.adaptation_rate>1)return UC_EXPOSURE_UNSUPPORTED;
        if(p.settings.inversion>0)scalar(&p.settings.inversion,p.settings.inversion-.05f,&p);
        p.alternate=0;
        if(uc_environment_exposure_prepare(&p)!=UC_EXPOSURE_OK)return UC_EXPOSURE_UNSUPPORTED;
        float wanted;
        if(!inverse(p.lut,s.measured_brightness,s.target_brightness,&wanted))return UC_EXPOSURE_UNSUPPORTED;
        float mix=s.effect_mix;
        if(mix==0)s.effect_latched=0;
        if(s.effect_mix>0&&(s.effect_latched||s.measured_brightness>.85f)) {
            if(s.effect_amount<1)s.effect_amount=s.effect_amount+.02f;
        } else if(s.effect_amount>0)s.effect_amount=s.effect_amount-.02f;
        float remaining=1.0f-mix,offset=0.0f-s.dark_point*remaining;
        offset=offset+s.effect_mix*s.effect_amount;
        const float offsets[3]={offset,offset,offset};rgb(p.settings.offset,offsets,&p);
        if(wanted>s.maximum)wanted=s.maximum;else if(s.minimum>wanted)wanted=s.minimum;
        double keep;if(!retention(s.adaptation_rate,input->delta,&keep))return UC_EXPOSURE_UNSUPPORTED;
        s.adapted_exposure=(float)((1.0-keep)*(double)wanted+keep*(double)s.adapted_exposure);
        float gamma=1.0f/s.inverse_gamma;gamma=gamma+.15f;gamma=1.0f/gamma;
        scalar(&p.settings.gamma_parameter,gamma,&p);
        float exposure=s.effect_exposure*mix;exposure=exposure+remaining*s.adapted_exposure;
        scalar(&p.settings.exposure,exposure,&p);scalar(&p.settings.input_scale,s.input_scale,&p);
        float gain[3],multiplier=s.multiplier+.4f;
        for(unsigned i=0;i<3;++i)gain[i]=(remaining+s.effect_gain[i]*mix)*multiplier;
        rgb(p.settings.gain,gain,&p);scalar(&p.post_scale,remaining,&p);
        if(input->force_inversion)scalar(&p.settings.inversion,1,&p);
        if(input->disable_post_scale)scalar(&p.post_scale,0,&p);
        result.post_parameter=.29f;result.write_post_parameter=1;
    }
    if(input->glow_present) {
        result.glow_threshold=s.glow_point-.2f;result.glow_multiplier=s.glow_multiplier;result.write_glow=1;
        if(!input->bypass&&input->disable_glow)result.glow_threshold=0;
    }
    if(!state_finite(&s)||!provider_finite(&p)||!isfinite(result.glow_threshold)||!isfinite(result.glow_multiplier))return UC_EXPOSURE_UNSUPPORTED;
    *state=s;*provider=p;*out=result;return UC_EXPOSURE_OK;
}
