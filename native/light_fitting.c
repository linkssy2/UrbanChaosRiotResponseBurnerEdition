#include "light_fitting.h"
#include <math.h>
int uc_light_fitting_area_contains(const float*b,const float*p,unsigned*out){
    if(!b||!p||!out)return 1;
    for(unsigned k=0;k<6;k++)if(!isfinite(b[k]))return 1;
    for(unsigned k=0;k<3;k++)if(!isfinite(p[k]))return 1;
    *out=p[0]>=b[0]&&p[0]<=b[3]&&p[1]>=b[1]&&p[1]<=b[4]&&p[2]>=b[2]&&p[2]<=b[5];return 0;
}
int uc_light_fitting_parameters(const UCLevelEntity*d,const uint8_t**cursor,UCLightFittingParameters*out){
    static const uint8_t types[]={1,2,1,1,2,2};
    if(!d||!cursor||!out)return 1;
    const uint8_t*p=*cursor;UCLightFittingParameters v;
    for(unsigned i=0;i<6;i++){int found;if(uc_entity_stream_get(d,&p,types[i],(uint8_t*)&v+i*4,&found)||!found)return 1;}
    *cursor=p;*out=v;return 0;
}
int uc_light_fitting_values(uint32_t groups,float a,float b,float c,float base,UCLightFittingValues*out){
    if(!out||groups>3||!isfinite(a)||!isfinite(b)||!isfinite(c)||!isfinite(base)||
       fabsf(a)>10000||fabsf(b)>10000||fabsf(c)>10000||fabsf(base)>10000)return 1;
    UCLightFittingValues v={0};
    if(b==0){if(groups==2||groups==3)v.group=1;}
    else if(c<.005f){if(groups==3)v.group=2;else if(groups==2)v.group=1;}
    v.factor=v.group==0&&c<.1f&&c!=0?(b*a)*.1f:(c*b)*a;
    v.intensity=(0.f+v.factor)*base;*out=v;return 0;
}
int uc_light_fitting_delay(UCGameRandom*r,float*out){
    if(!out)return 1;int32_t word;if(uc_game_random_next(r,&word))return 1;
    *out=(float)(((double)word*2.)*0x1p-31+2.);return 0;
}
