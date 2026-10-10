#include "point_light_pixel.h"
#include <math.h>
#include <xmmintrin.h>
static float clamp(float x,float low,float high){return x<low?low:x>high?high:x;}
static int rounding(void){return !(_mm_getcsr()&0xE040u);}
int uc_point_light_constant(const float rgba[4],uint32_t *argb){
    if(!rgba||!argb||!rounding())return 0;
    for(unsigned i=0;i<4;++i)if(!isfinite(rgba[i]))return 0;
    uint32_t c[4];
    for(unsigned i=0;i<4;++i){
        float value=clamp(rgba[i],0,1)*255.0f;
        c[i]=(uint32_t)_mm_cvtss_si32(_mm_set_ss(value));
    }
    *argb=(c[3]<<24)|(c[0]<<16)|(c[1]<<8)|c[2];return 1;
}
int uc_point_light_pixel(const float direction[3],const float normal[3],
    float attenuation,uint32_t color_argb,uint8_t scalar,float rgba[4]){
    if(!direction||!normal||!rgba||!rounding()||!isfinite(attenuation)||
       attenuation<0||attenuation>1)return 0;
    for(unsigned i=0;i<3;++i)
        if(!isfinite(direction[i])||!isfinite(normal[i])||
           direction[i]<-1||direction[i]>1||normal[i]<-1||normal[i]>1)return 0;
    float dot=direction[0]*normal[0]+direction[1]*normal[1];
    dot=clamp(dot+direction[2]*normal[2],-1,1);
    float threshold=(float)scalar/255.0f;
    float gate=clamp(dot+-threshold,-1,1);
    gate=gate>0?gate:0;gate=clamp((gate+gate)*2.0f,-1,1);
    float result[4];
    for(unsigned i=0;i<3;++i){
        float color=(float)((color_argb>>(16-i*8))&255u)/255.0f;
        float lit=clamp(attenuation*color,-1,1);
        lit=clamp(dot*lit,-1,1);
        result[i]=clamp(clamp(lit*gate,-1,1),0,1);
    }
    result[3]=0;
    for(unsigned i=0;i<4;++i)rgba[i]=result[i];
    return 1;
}
