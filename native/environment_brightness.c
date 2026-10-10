#include "environment_brightness.h"
#include <fenv.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

static int rounding_ok(void) {return fegetround()==FE_TONEAREST&&(_mm_getcsr()&0xE040u)==0;}
int uc_environment_brightness_sample(const UCExposureImage *image,const UCExposureViewport *v,
    int32_t prior,int32_t *out) {
    if(!image||!v||!out||!image->pixels||!image->width||!image->height||
       image->width>INT32_MAX/4||image->height>INT32_MAX||
       image->row_stride<(size_t)image->width*4||image->row_stride>INT32_MAX/30)
        return UC_BRIGHTNESS_ARGUMENT;
    uint64_t extent=(uint64_t)(image->height-1)*image->row_stride+(uint64_t)image->width*4;
    if(extent>image->size||extent>INT32_MAX)return UC_BRIGHTNESS_ARGUMENT;
    const float values[]={v->x,v->y,v->width,v->height};
    for(unsigned i=0;i<4;++i)if(!isfinite(values[i])||values[i]<=-1.0f||values[i]>=2147483648.0f)
        return UC_BRIGHTNESS_ARGUMENT;
    if(!rounding_ok())return UC_BRIGHTNESS_UNSUPPORTED;
    int32_t x=(int32_t)v->x,y=(int32_t)v->y,w=(int32_t)v->width,h=(int32_t)v->height;
    if(w<40||h<30||(uint64_t)x+w>image->width||(uint64_t)y+h>image->height)
        return UC_BRIGHTNESS_ARGUMENT;
    uint64_t divisor=(uint64_t)(w/40)*(h/30)*3;
    if(divisor>INT32_MAX)return UC_BRIGHTNESS_ARGUMENT;
    uint32_t sum=(uint32_t)prior;
    for(int32_t sx=x+20;sx<x+w;sx+=40)
        for(int32_t sy=y+15;sy<y+h;sy+=30) {
            const uint8_t *p=image->pixels+(size_t)sy*image->row_stride+(size_t)sx*4;
            sum+=p[0];sum+=p[1];sum+=p[2];
        }
    int32_t signed_sum;memcpy(&signed_sum,&sum,sizeof(sum));
    *out=signed_sum/(int32_t)divisor;return UC_BRIGHTNESS_OK;
}
int uc_environment_brightness_publish(const UCBrightnessPublication *in,float *out) {
    if(!in||!out||!isfinite(in->input_scale)||in->glow_present>1||in->guard_enabled>1)
        return UC_BRIGHTNESS_ARGUMENT;
    if(!rounding_ok())return UC_BRIGHTNESS_UNSUPPORTED;
    float value=.8f;
    if(in->glow_present&&(!in->guard_enabled||in->guard_count<=1)&&in->brightness>=0) {
        double scaled=(double)in->input_scale*(double)in->brightness;
        value=(float)(scaled*(double)0.00392156885936856270f);
        if(!isfinite(value))return UC_BRIGHTNESS_UNSUPPORTED;
    }
    *out=value;return UC_BRIGHTNESS_OK;
}
