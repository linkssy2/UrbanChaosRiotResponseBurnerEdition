#include "collision_body.h"
#include <fenv.h>
#include <math.h>
#include <xmmintrin.h>
int uc_collision_body_filter(uint32_t ray,const UCCollisionBodyFilter *b) {
    if(!b||b->collidable_type>255||b->phantom_owner_present>1)return -1;
    if(b->collidable_type==2&&b->phantom_owner_present&&
        (b->property29!=1||b->property2a!=1))return 0;
    if(!((ray^b->filter_info)&0xfffffu)||!((ray>>6)&b->filter_info&0x3f00000u))return 0;
    return b->shape_type==24||b->shape_type==12||((ray>>20)&b->shape_user_data&63u)!=0;
}
int uc_collision_body_local(const float from[4],const UCCollisionRay *ray,
    const float m[4][4],UCCollisionRay *out) {
    if(!from||!ray||!m||!out||(_mm_getcsr()&0xe040u)||fegetround()!=FE_TONEAREST)return -1;
    for(unsigned i=0;i<4;i++){
        if(!isfinite(from[i])||!isfinite(ray->to[i]))return -1;
        for(unsigned j=0;j<4;j++)if(!isfinite(m[i][j]))return -1;
    }
    __m128 a=_mm_loadu_ps(m[0]),b=_mm_loadu_ps(m[1]),c=_mm_loadu_ps(m[2]);
    __m128 low=_mm_shuffle_ps(a,b,0x44),high=_mm_shuffle_ps(a,b,0xee);
    __m128 cl=_mm_shuffle_ps(c,c,0x44),ch=_mm_shuffle_ps(c,c,0xee);
    __m128 x=_mm_shuffle_ps(low,cl,0x88),y=_mm_shuffle_ps(low,cl,0xdd),z=_mm_shuffle_ps(high,ch,0x88);
    UCCollisionRay next;
    for(unsigned i=0;i<2;i++){
        __m128 p=_mm_sub_ps(_mm_loadu_ps(i?ray->to:from),_mm_loadu_ps(m[3]));
        __m128 result=_mm_add_ps(_mm_add_ps(_mm_mul_ps(x,_mm_shuffle_ps(p,p,0)),
            _mm_mul_ps(y,_mm_shuffle_ps(p,p,0x55))),_mm_mul_ps(z,_mm_shuffle_ps(p,p,0xaa)));
        float *dst=i?next.to:next.from;_mm_storeu_ps(dst,result);
        for(unsigned j=0;j<4;j++)if(!isfinite(dst[j]))return -1;
    }
    next.filter=ray->filter;*out=next;return 1;
}
