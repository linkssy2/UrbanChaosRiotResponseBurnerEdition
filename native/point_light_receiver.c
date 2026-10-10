#include "point_light_receiver.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
#define SHUFFLE(v,n) _mm_shuffle_ps((v),(v),(n))
static int inverse(const float m[16],float out[16],int control_reciprocal){
    if(!m||!out||(_mm_getcsr()&0xE040u)||!finite_values(m,16))return 0;
    /* The original cofactor kernel loads transposed columns, with the odd
     * columns rotated by two lanes. Named temporaries retain its add order. */
    __m128 a=_mm_loadu_ps(m),b=_mm_loadu_ps(m+4),c=_mm_loadu_ps(m+8),d=_mm_loadu_ps(m+12);
    _MM_TRANSPOSE4_PS(a,b,c,d);
    b=SHUFFLE(b,0x4E);d=SHUFFLE(d,0x4E);
    __m128 pair=SHUFFLE(_mm_mul_ps(c,d),0xB1);
    __m128 base0=_mm_mul_ps(b,pair),base1=_mm_mul_ps(a,pair);
    pair=SHUFFLE(pair,0x4E);
    __m128 cofactor0=_mm_sub_ps(_mm_mul_ps(b,pair),base0);
    __m128 cross0=_mm_sub_ps(_mm_mul_ps(a,pair),base1);
    pair=SHUFFLE(_mm_mul_ps(b,c),0xB1);
    __m128 cross1=_mm_mul_ps(a,pair),contribution0=_mm_mul_ps(d,pair);
    pair=SHUFFLE(pair,0x4E);
    __m128 subtract0=_mm_mul_ps(d,pair);
    cross1=_mm_sub_ps(_mm_mul_ps(a,pair),cross1);
    pair=SHUFFLE(_mm_mul_ps(SHUFFLE(b,0x4E),d),0xB1);
    c=SHUFFLE(c,0x4E);
    __m128 contribution1=_mm_mul_ps(c,pair),cross2=_mm_mul_ps(a,pair);
    pair=SHUFFLE(pair,0x4E);
    cofactor0=_mm_sub_ps(_mm_add_ps(contribution1,_mm_sub_ps(_mm_add_ps(contribution0,cofactor0),subtract0)),_mm_mul_ps(c,pair));
    cross2=_mm_sub_ps(_mm_mul_ps(a,pair),cross2);
    pair=SHUFFLE(_mm_mul_ps(a,b),0xB1);
    __m128 cofactor2=_mm_mul_ps(d,pair),cofactor3=_mm_mul_ps(c,pair);
    pair=SHUFFLE(pair,0x4E);
    __m128 alternate2=_mm_mul_ps(d,pair),subtract3=_mm_mul_ps(c,pair);
    pair=SHUFFLE(_mm_mul_ps(a,d),0xB1);
    __m128 subtract1=_mm_mul_ps(c,pair),add2=_mm_mul_ps(b,pair);
    pair=SHUFFLE(pair,0x4E);
    __m128 add1=_mm_mul_ps(c,pair),subtract2=_mm_mul_ps(b,pair);
    pair=SHUFFLE(_mm_mul_ps(a,c),0xB1);
    __m128 cofactor1=_mm_mul_ps(d,pair),subtract4=_mm_mul_ps(b,pair);
    pair=SHUFFLE(pair,0x4E);
    __m128 subtract5=_mm_mul_ps(d,pair),add3=_mm_mul_ps(b,pair);
    __m128 determinant=_mm_mul_ps(a,cofactor0);
    determinant=_mm_add_ps(SHUFFLE(determinant,0x4E),determinant);
    determinant=_mm_add_ss(SHUFFLE(determinant,0xB1),determinant);
    float det=_mm_cvtss_f32(determinant);
    if(!isfinite(det)||det==0)return 0;
    __m128 reciprocal=control_reciprocal?_mm_div_ss(_mm_set_ss(1.f),determinant):_mm_rcp_ss(determinant);
    reciprocal=_mm_sub_ss(_mm_add_ss(reciprocal,reciprocal),_mm_mul_ss(determinant,_mm_mul_ss(reciprocal,reciprocal)));
    reciprocal=SHUFFLE(reciprocal,0);
    cofactor1=_mm_sub_ps(_mm_add_ps(cofactor1,_mm_add_ps(add1,_mm_sub_ps(SHUFFLE(cross0,0x4E),subtract1))),subtract5);
    cofactor2=_mm_sub_ps(_mm_add_ps(add2,_mm_sub_ps(alternate2,_mm_add_ps(cofactor2,SHUFFLE(cross2,0x4E)))),subtract2);
    cofactor3=_mm_add_ps(add3,_mm_sub_ps(_mm_sub_ps(_mm_sub_ps(cofactor3,SHUFFLE(cross1,0x4E)),subtract3),subtract4));
    float result[16];
    _mm_storeu_ps(result,_mm_mul_ps(reciprocal,cofactor0));
    _mm_storeu_ps(result+4,_mm_mul_ps(reciprocal,cofactor1));
    _mm_storeu_ps(result+8,_mm_mul_ps(reciprocal,cofactor2));
    _mm_storeu_ps(result+12,_mm_mul_ps(reciprocal,cofactor3));
    if(!finite_values(result,16))return 0;
    memcpy(out,result,sizeof(result));return 1;
}
int uc_point_light_inverse(const float m[16],float out[16]){return inverse(m,out,0);}
/* AF5F0's retained CONTROL geometry uses the lift's exact reciprocal, while
 * existing receiver qualification deliberately models the source RCPSS. */
int uc_point_light_inverse_control(const float m[16],float out[16]){return inverse(m,out,1);}
#undef SHUFFLE
/* ACD00 is private in the existing whole-model bounds module. */
static void transform_bounds(const UCModelBounds *s,const float m[16],UCModelBounds *out){
    float c[3],e[3];for(unsigned i=0;i<3;i++){c[i]=(s->minimum[i]+s->maximum[i])*.5f;e[i]=(s->maximum[i]-s->minimum[i])*.5f;}
    for(unsigned i=0;i<3;i++){
        float middle=(m[i]*c[0]+m[12+i])+(m[4+i]*c[1]+m[8+i]*c[2]);
        float radius=fabsf(m[i]*e[0])+(fabsf(m[4+i]*e[1])+fabsf(m[8+i]*e[2]));
        float low=middle-radius,high=middle+radius;
        out->minimum[i]=high<low?high:low;out->maximum[i]=high>low?high:low;
        out->center[i]=(out->maximum[i]+out->minimum[i])*.5f;out->extent[i]=(out->maximum[i]-out->minimum[i])*.5f;
    }
}
int uc_point_light_receiver(const UCPointLightGeometry *light,const float world[16],
    const UCModelBounds *bounds,const void *receiver,const void *pass,const UCPointLightReceiverTarget *target){
    if(!light||!world||!bounds||!receiver||!target||!target->provider_world||!target->provider_dirty||
       !target->light_local_center||!target->upload||!target->draw||
       !finite_values(light->world_to_volume,16)||!finite_values(light->world_center,4)||
       !finite_values(bounds->minimum,3)||!finite_values(bounds->maximum,3))return 0;
    float inverse[16],volume[16],center[4],rows[16];UCModelBounds transformed;
    if(!uc_point_light_inverse(world,inverse))return 0;
    for(unsigned row=0;row<4;row++)for(unsigned col=0;col<4;col++)
        volume[row*4+col]=(light->world_to_volume[col]*world[row*4]+light->world_to_volume[4+col]*world[row*4+1])+
            (light->world_to_volume[8+col]*world[row*4+2]+light->world_to_volume[12+col]*world[row*4+3]);
    transform_bounds(bounds,volume,&transformed);
    if(!finite_values(volume,16)||!finite_values((const float *)&transformed,12))return 0;
    float distance=0;
    for(unsigned i=0;i<3;i++){
        float delta=0;if(transformed.minimum[i]>0)delta=0-transformed.minimum[i];
        else if(transformed.maximum[i]<0)delta=0-transformed.maximum[i];
        if(delta!=0)distance=delta*delta+distance;
    }
    if(!isfinite(distance))return 0;
    int visible=distance<.25f;
    if(visible){
        for(unsigned i=0;i<3;i++)volume[12+i]+=.5f;
        for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)rows[i*4+j]=volume[j*4+i];
        for(unsigned i=0;i<4;i++)center[i]=(inverse[i]*light->world_center[0]+inverse[4+i]*light->world_center[1])+
            (inverse[8+i]*light->world_center[2]+inverse[12+i]*light->world_center[3]);
        if(!finite_values(rows,16)||!finite_values(center,4))return 0;
    }
    memcpy(target->provider_world,world,64);*target->provider_dirty=1;
    if(!visible)return 1;
    target->upload(target->context,113,rows,16);
    memcpy(target->light_local_center,center,16);
    target->upload(target->context,112,target->light_local_center,4);
    target->draw(target->context,receiver,pass);return 2;
}
