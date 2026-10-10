#include "point_light_textures.h"
#include <math.h>
#include <fenv.h>
#include <xmmintrin.h>
static int rounding_ok(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
static uint32_t packed(const float p[3]){
    uint32_t rgb[3];
    for(unsigned i=0;i<3;i++){
        int value=(int)(p[i]*127.f);if(value>=127)value=127;else if(value<=-128)value=-128;
        rgb[i]=(uint32_t)value&255;
    }
    float value=p[1]*6.666666507720947265625f;value*=255.f;
    int alpha=(int)value;if(alpha<0)alpha=0;else if(alpha>255)alpha=255;
    return ((uint32_t)alpha<<24)|(rgb[0]<<16)|(rgb[1]<<8)|rgb[2];
}
int uc_point_light_normalization(uint32_t *cube,size_t words){
    if(!cube||words<UC_LIGHT_CUBE_WORDS||!rounding_ok())return 0;
    for(unsigned face=0;face<6;face++)for(int y=0;y<128;y++)for(int x=0;x<128;x++){
        float u=(float)(x-64)*.015625f,v=(float)(y-64)*.015625f,p[3];
        switch(face){
        case 0:p[0]=1;p[1]=0.f-v;p[2]=0.f-u;break;
        case 1:p[0]=-1;p[1]=0.f-v;p[2]=u;break;
        case 2:p[0]=u;p[1]=1;p[2]=v;break;
        case 3:p[0]=u;p[1]=-1;p[2]=0.f-v;break;
        case 4:p[0]=u;p[1]=0.f-v;p[2]=1;break;
        default:p[0]=0.f-u;p[1]=0.f-v;p[2]=-1;break;
        }
        double squared=(double)p[2]*p[2]+(double)p[1]*p[1];squared+=(double)p[0]*p[0];
        double inverse=1.0/sqrt(squared);
        for(unsigned i=0;i<3;i++)p[i]=(float)((double)p[i]*inverse);
        cube[(face*128+(unsigned)y)*128+(unsigned)x]=packed(p);
    }
    return 1;
}
int uc_point_light_attenuation(uint8_t *volume,size_t bytes){
    if(!volume||bytes<UC_LIGHT_VOLUME_BYTES||!rounding_ok())return 0;
    for(unsigned z=0;z<32;z++)for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
        unsigned at=(z*32+y)*32+x;
        if(!x||x==31||!y||y==31||!z||z==31){volume[at]=0;continue;}
        float fy=(float)y*.03125f;fy*=2.f;fy-=1.f;
        float fz=(float)z*.03125f;fz*=2.f;fz-=1.f;
        double fx=(double)x*.03125f;fx+=fx;fx-=1.f;
        double radius=sqrt((fx*fx+(double)fy*fy)+(double)fz*fz);radius-=.5f;radius+=radius;
        float r=radius<0?0:(float)radius;
        float divisor=r*r;divisor*=8.f;divisor+=1.f;
        float value=1.f/divisor;value+=.25f;value*=1.f-r;value*=255.f;
        int byte=(int)value;if(byte>=255)byte=255;else if(byte<=0)byte=0;
        volume[at]=(uint8_t)byte;
    }
    return 1;
}
