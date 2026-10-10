#include "light_flash.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int rounding(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
static int bounded(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>1048576.f)return 0;return 1;}
static int valid(const UCLightFlashParameters *p){
    return p&&bounded(&p->lifespan,5)&&bounded(p->color_start,3)&&bounded(p->color_end,3)&&
        isfinite(p->curve_power)&&p->curve_power>=0&&p->curve_power<=16;
}
static void color(UCPointLightState *p,const float rgb[3]){
    for(unsigned i=0;i<3;i++){
        float value=rgb[i]*255.f;int32_t byte=(int32_t)value;
        if(byte<0)byte=0;else if(byte>255)byte=255;
        p->color[i]=(float)byte*(1.f/255.f);
    }
    p->color[3]=1;
}
UCLightFlashStatus uc_light_flash_initialize(UCLightFlash *p,const UCLightFlashParameters *in,
    const float position[3],const UCLightFlashServices *h,void *context){
    if(!p||p->light||p->phase||p->elapsed||p->resume_elapsed||!valid(in)||!position||!bounded(position,3)||
       !h||!h->create||!h->release||!h->remove_entity)return UC_LIGHT_FLASH_ARGUMENT;
    if(!rounding())return UC_LIGHT_FLASH_UNSUPPORTED;
    p->parameters=*in;p->elapsed=p->resume_elapsed=0;
    p->light=h->create(context,position);
    if(p->light){
        p->light->enabled=1;p->light->shadows=in->shadows!=0;
        color(p->light,in->color_start);p->light->multiplier=in->multiplier_start;
        p->light->attenuation=in->attenuation_start;p->light->dirty=1;
        p->light->specular=in->specular!=0;
    }
    return UC_LIGHT_FLASH_OK;
}
UCLightFlashStatus uc_light_flash_step(UCLightFlash *p,float delta,const UCLightFlashServices *h,void *context){
    if(!p||!valid(&p->parameters)||!isfinite(delta)||delta<=0||delta>1||p->phase>2||
       !isfinite(p->elapsed)||p->elapsed<0||!isfinite(p->resume_elapsed)||p->resume_elapsed<0||
       !h||!h->release||!h->remove_entity)return UC_LIGHT_FLASH_ARGUMENT;
    if(!rounding())return UC_LIGHT_FLASH_UNSUPPORTED;
    if(p->phase==2)return UC_LIGHT_FLASH_FINISHED;
    const UCLightFlashParameters *v=&p->parameters;
    /*176344 stores float local elapsed, but compares the still-live unrounded
     * x87 sum. A rounded-equal value can therefore execute one more update. */
    double resumed=p->phase?(double)p->resume_elapsed+delta:0;
    float resume=(float)resumed;
    if((double)v->lifespan<=resumed){
        p->resume_elapsed=resume;
        if(p->light){h->release(context,p->light);p->light=NULL;}
        h->remove_entity(context);p->phase=2;return UC_LIGHT_FLASH_FINISHED;
    }
    float elapsed=(float)((double)p->elapsed+delta),rgb[3],multiplier=0,attenuation=0;
    if(!isfinite(elapsed)||!isfinite(resume))return UC_LIGHT_FLASH_UNSUPPORTED;
    if(p->light&&elapsed>0){
        float factor=(float)pow((double)elapsed/v->lifespan,(double)v->curve_power);
        for(unsigned i=0;i<3;i++)rgb[i]=(v->color_end[i]-v->color_start[i])*factor+v->color_start[i];
        attenuation=(float)(((double)v->attenuation_end-v->attenuation_start)*factor+v->attenuation_start);
        multiplier=(float)(((double)v->multiplier_end-v->multiplier_start)*factor+v->multiplier_start);
        if(!bounded(rgb,3)||!isfinite(attenuation)||!isfinite(multiplier))return UC_LIGHT_FLASH_UNSUPPORTED;
    }
    p->resume_elapsed=resume;p->elapsed=elapsed;p->phase=1;
    if(p->light&&elapsed>0){color(p->light,rgb);p->light->multiplier=multiplier;p->light->attenuation=attenuation;p->light->dirty=1;}
    return UC_LIGHT_FLASH_OK;
}
