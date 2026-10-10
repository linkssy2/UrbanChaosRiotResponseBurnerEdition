#include "collision_result.h"
#include <fenv.h>
#include <math.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n) {
    for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;
    return 1;
}
static uint16_t expand(uint32_t bits) {
    return (uint16_t)(((bits&1)?0x400:0)|((bits&2)?0x200:0)|
        ((bits&4)?0x4000:0)|((bits&8)?0x2000:0)|
        ((bits&16)?0x80:0)|((bits&32)?0x100:0));
}
static void render(const float *v,float *out) {
    out[0]=v[0]*100.0f;out[1]=v[2]*100.0f;out[2]=v[1]*100.0f;
}
int uc_collision_result_convert(const UCCollisionRay *ray,const UCCollisionClosest *closest,
    const UCCollisionResultOwner *owner,float impulse,UCCollisionResult *out) {
    if(!ray||!closest||!out)return -1;
    if(!closest->root_body)return 0;
    if(!owner||!owner->identity||owner->kind>255||owner->impulse_blocked>255||
       !finite_values(ray->from,4)||!finite_values(ray->to,4)||
       !finite_values(closest->hit.normal,4)||!isfinite(closest->hit.fraction)||
       closest->hit.fraction<0||closest->hit.fraction>1||!isfinite(impulse)||
       (_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return -1;
    if(owner->kind==1&&impulse!=0&&(owner->property20||!owner->impulse_blocked))return -1;
    UCCollisionResult next={0};float point[4],start[3],end[3],delta[3],products[3];
    __m128 f=_mm_set1_ps(closest->hit.fraction);
    _mm_storeu_ps(point,_mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1),f),_mm_loadu_ps(ray->from)),
        _mm_mul_ps(f,_mm_loadu_ps(ray->to))));
    render(point,next.point);render(ray->from,start);render(ray->to,end);
    render(closest->hit.normal,next.normal);
    if(!finite_values(point,4)||!finite_values(next.point,3)||!finite_values(start,3)||
       !finite_values(end,3)||!finite_values(next.normal,3))return -1;
    double x=(double)next.point[0]-start[0],y=(double)next.point[1]-start[1],z=(double)next.point[2]-start[2];
    next.distance=(float)sqrt((z*z+y*y)+x*x);
    for(unsigned i=0;i<3;i++){delta[i]=end[i]-start[i];products[i]=next.normal[i]*delta[i];}
    float dot=(products[2]+products[1])+products[0];
    if(!finite_values(delta,3)||!finite_values(products,3)||!isfinite(dot)||!isfinite(next.distance))return -1;
    if(dot>0)for(unsigned i=0;i<3;i++)next.normal[i]*=-1.0f;
    x=next.normal[0];y=next.normal[1];z=next.normal[2];
    double length=sqrt((x*x+y*y)+z*z);
    if(!(length>0)||!isfinite(length))return -1;
    double inverse=1.0/length;
    for(unsigned i=0;i<3;i++)next.normal[i]=(float)(inverse*next.normal[i]);
    if(!finite_values(next.normal,3))return -1;
    next.material=(uint16_t)(closest->material>>12);
    next.primary_mask=expand(closest->material);next.secondary_mask=expand(closest->material>>6);
    next.property45=owner->property45;
    if(owner->kind==1)next.rigid_body=owner->identity;
    if(owner->kind==2)next.phantom=owner->identity;
    *out=next;return 1;
}
