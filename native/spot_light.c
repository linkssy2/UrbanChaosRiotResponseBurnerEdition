#include "spot_light.h"
#include "point_light_receiver.h"
#include <math.h>
#include <string.h>
#include <fenv.h>
#include <xmmintrin.h>
static int rounding_ok(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
static int normalize(float v[3]){
    float square=(v[0]*v[0]+v[2]*v[2])+v[1]*v[1];
    if(!isfinite(square)||square<=0)return 0;
    float inverse=1.f/sqrtf(square),scale=(.5f*inverse)*(3.f-(inverse*square)*inverse);
    for(unsigned i=0;i<3;i++)v[i]*=scale;
    return 1;
}
static void cross(const float a[3],const float b[3],float out[3]){
    out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];
}
int uc_spot_light_build(const float position[3],const float direction[3],float radius,float cone,
    const float color[4],float intensity,UCNativeControlSpot *out){
    if(!position||!direction||!color||!out||!rounding_ok()||!isfinite(radius)||radius<=0||
       !isfinite(cone)||cone<=0||cone>=1.5707963267948966f||!isfinite(intensity)||intensity<0)return 0;
    for(unsigned i=0;i<3;i++)if(!isfinite(position[i])||!isfinite(direction[i]))return 0;
    for(unsigned i=0;i<4;i++)if(!isfinite(color[i]))return 0;
    float basis[16]={0},inverse[16],up[3]={0,1,0};memcpy(basis+8,direction,12);
    if(!normalize(basis+8))return 0;
    if(fabs((double)direction[1])>=.99){up[0]=1;up[1]=0;}
    cross(up,basis+8,basis);if(!normalize(basis))return 0;cross(basis+8,basis,basis+4);
    memcpy(basis+12,position,12);basis[15]=1;
    if(!uc_point_light_inverse_control(basis,inverse))return 0;
    float projection[16]={0};projection[0]=projection[5]=(float)(1./tan((double)cone));
    projection[10]=2;projection[11]=1;projection[14]=0.f-radius;
    UCNativeControlSpot next={0};
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++){
        next.projection[i*4+j]=(projection[j]*inverse[i*4]+projection[4+j]*inverse[i*4+1])+
            (projection[8+j]*inverse[i*4+2]+projection[12+j]*inverse[i*4+3]);
        if(!isfinite(next.projection[i*4+j]))return 0;
    }
    next.texture_bias[0]=next.texture_bias[5]=next.texture_bias[10]=.5f;
    next.texture_bias[12]=next.texture_bias[13]=.5f;next.texture_bias[14]=.25f;next.texture_bias[15]=1;
    memcpy(next.center,position,12);next.center[3]=1;memcpy(next.color,color,16);
    next.intensity=intensity;next.radius=radius;*out=next;return 1;
}
int uc_spot_light_attenuation(uint8_t *volume,size_t bytes){
    if(!volume||bytes<UC_SPOT_VOLUME_BYTES||!rounding_ok())return 0;
    const float scale=0.016129031777381897f;
    for(unsigned y=0;y<64;y++){
        float fy=((float)(y*2)-63.f)*scale;fy*=fy;
        for(unsigned x=0;x<64;x++){
            double fx=((double)(x*2)-63.f)*scale;
            double value=floor(((1.0-sqrt(fx*fx+fy))*4.0)*255.0+.5);
            volume[y*64+x]=(uint8_t)(value>=255?255:value>0?(int)value:0);
        }
    }
    memset(volume+4096,0,4096);return 1;
}
int uc_spot_light_rows(const UCNativeControlSpot *spot,float rows[4][4]){
    if(!spot||!rows||!rounding_ok()||!isfinite(spot->intensity)||!isfinite(spot->radius)||spot->radius<=0)return 0;
    for(unsigned i=0;i<16;i++)if(!isfinite(spot->projection[i])||!isfinite(spot->texture_bias[i]))return 0;
    for(unsigned i=0;i<4;i++)if(!isfinite(spot->center[i])||!isfinite(spot->color[i]))return 0;
    float next[4][4];
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++){
        next[j][i]=(spot->texture_bias[j]*spot->projection[i*4]+spot->texture_bias[4+j]*spot->projection[i*4+1])+
            (spot->texture_bias[8+j]*spot->projection[i*4+2]+spot->texture_bias[12+j]*spot->projection[i*4+3]);
        if(!isfinite(next[j][i]))return 0;
    }
    memcpy(rows,next,sizeof(next));return 1;
}
