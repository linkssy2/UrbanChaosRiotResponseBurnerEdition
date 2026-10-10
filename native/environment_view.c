#include "environment_view.h"
#include <math.h>
int uc_environment_view_reset(UCEnvironmentView *v,UCEnvironmentExposure *s,
    const UCExposureProfile *p,float initial){
    if(!v||!s||!isfinite(initial))return 1;
    UCEnvironmentExposure next;
    if(uc_environment_exposure_reset(p,&next))return 1;
    *s=next;*v=(UCEnvironmentView){initial,1,0};return 0;
}
int uc_environment_view_begin(UCEnvironmentView *v,UCEnvironmentExposure *s,
    UCExposureToneProvider *p,const UCExposureProfile *profile,const UCExposureInput *input,
    uint32_t flags,UCExposurePublication *out){
    if(!v||!s||!p||!profile||!input||!out||v->pending||v->refresh_profile>1||!isfinite(v->brightness))return 1;
    UCEnvironmentExposure next=*s;UCExposureToneProvider tone=*p;UCExposurePublication publication;
    if(v->refresh_profile){if(uc_environment_exposure_refresh(profile,&next))return 1;}
    else if(!(flags&0x800))next.measured_brightness=v->brightness;
    if(uc_environment_exposure_step(&next,&tone,input,&publication)||uc_environment_exposure_prepare(&tone))return 1;
    *s=next;*p=tone;*out=publication;v->refresh_profile=0;v->pending=1;return 0;
}
int uc_environment_view_end(UCEnvironmentView *v,int32_t *retained,
    const UCExposureToneProvider *p,const UCExposureImage *image,const UCExposureViewport *viewport,
    unsigned sample,unsigned glow,unsigned guard,int32_t count){
    if(!v||!retained||!p||v->pending!=1||sample>1||glow>1||guard>1)return 1;
    int32_t value=*retained;float published;
    if(sample&&uc_environment_brightness_sample(image,viewport,value,&value))return 1;
    UCBrightnessPublication in={value,count,p->settings.input_scale,glow,guard};
    if(uc_environment_brightness_publish(&in,&published))return 1;
    *retained=value;v->brightness=published;v->pending=0;return 0;
}
