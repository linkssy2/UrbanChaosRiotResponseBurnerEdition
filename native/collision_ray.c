#include "collision_ray.h"
#include <fenv.h>
#include <math.h>
#include <xmmintrin.h>
int uc_collision_ray_prepare(const float start[3],const float end[3],
    uint32_t type,uint32_t group,uint32_t mask,UCCollisionRay *out) {
    if(!start||!end||!out||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return -1;
    for(unsigned i=0;i<3;i++)if(!isfinite(start[i])||!isfinite(end[i]))return -1;
    double x=(double)start[0]-end[0],y=(double)start[1]-end[1],z=(double)start[2]-end[2];
    if(sqrt((z*z+y*y)+x*x)<(double)0.00001f)return 0;
    UCCollisionRay next;const unsigned axis[3]={0,2,1};
    for(unsigned i=0;i<3;i++){next.from[i]=start[axis[i]]*.01f;next.to[i]=end[axis[i]]*.01f;}
    next.from[3]=next.to[3]=1;
    __m128 from=_mm_loadu_ps(next.from),d=_mm_sub_ps(_mm_loadu_ps(next.to),from);
    __m128 p=_mm_mul_ps(d,d);
    __m128 squared=_mm_add_ss(_mm_shuffle_ps(p,p,0xAA),_mm_add_ss(_mm_shuffle_ps(p,p,0x55),p));
    if(!isfinite(_mm_cvtss_f32(squared)))return -1;
    if(_mm_cvtss_f32(_mm_sqrt_ss(squared))>550) {
        __m128 r=_mm_rsqrt_ss(squared);
        r=_mm_mul_ss(_mm_mul_ss(_mm_set_ss(.5f),r),
            _mm_sub_ss(_mm_set_ss(3),_mm_mul_ss(_mm_mul_ss(squared,r),r)));
        _mm_storeu_ps(next.to,_mm_add_ps(from,_mm_mul_ps(_mm_set1_ps(550),
            _mm_mul_ps(_mm_shuffle_ps(r,r,0),d))));
        next.to[3]=1;
    }
    for(unsigned i=0;i<3;i++)if(!isfinite(next.to[i]))return -1;
    if(!(mask&0xffffu))mask|=0x73a0u;
    const uint32_t bits=((mask&0x400u)>>10)|((mask&0x200u)>>8)|((mask&0x4000u)>>12)
        |((mask&0x2000u)>>10)|((mask&0x80u)>>3)|((mask&0x100u)>>3);
    next.filter=((((uint32_t)(uint16_t)type<<6)|bits)<<20)|(uint16_t)group;
    *out=next;return 1;
}
