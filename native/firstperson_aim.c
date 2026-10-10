#include "firstperson_aim.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

static int finite_values(const float *values, unsigned count) {
    for(unsigned i=0;i<count;++i) if(!isfinite(values[i])) return 0;
    return 1;
}
/* Same ordered four-term composition used by the existing look/placement math. */
static void transform(const float vector[4],const float matrix[16],float out[3]) {
    for(unsigned i=0;i<3;++i)
        out[i]=(vector[0]*matrix[i]+vector[1]*matrix[4+i])
            +(vector[2]*matrix[8+i]+vector[3]*matrix[12+i]);
}
static int normalize(float direction[3]) {
    double sum=(double)direction[2]*direction[2]+(double)direction[1]*direction[1];
    float inverse;
    sum=sum+(double)direction[0]*direction[0];
    if(!isfinite(sum)||sum<=0) return 0;
    inverse=(float)(1.0/sqrt(sum));
    if(!isfinite(inverse)) return 0;
    for(unsigned i=0;i<3;++i) direction[i]=direction[i]*inverse;
    return finite_values(direction,3);
}
UCMotionStatus uc_firstperson_aim_ray_prepare(const float look[16],float length,
    UCFirstpersonAimRay *out) {
    static const float forward[4]={0,0,1,0};
    UCFirstpersonAimRay ray;
    if(!look||!out||!finite_values(look,16)||!isfinite(length)||length<0)
        return UC_MOTION_ARGUMENT;
    if((_mm_getcsr()&0xE040u)!=0) return UC_MOTION_UNSUPPORTED;
    memcpy(ray.origin,look+12,sizeof(ray.origin));
    transform(forward,look,ray.direction);
    if(!normalize(ray.direction)) return UC_MOTION_UNSUPPORTED;
    for(unsigned i=0;i<3;++i) ray.end[i]=length*ray.direction[i]+ray.origin[i];
    ray.miss_distance=length+.01f;
    if(!finite_values(ray.end,3)||!isfinite(ray.miss_distance)) return UC_MOTION_UNSUPPORTED;
    *out=ray;return UC_MOTION_OK;
}
UCMotionStatus uc_firstperson_aim_hit_begin(const UCFirstpersonAimCollision *collision,
    UCFirstpersonAimHit *out) {
    UCFirstpersonAimHit hit;
    if(!collision||!out||!isfinite(collision->distance)||!finite_values(collision->point,3))
        return UC_MOTION_ARGUMENT;
    hit.entity=collision->entity;hit.distance=collision->distance;
    memcpy(hit.point,collision->point,sizeof(hit.point));hit.detail=collision->detail;
    hit.classification=collision->entity==-1?0:5;
    *out=hit;return UC_MOTION_OK;
}
UCMotionStatus uc_firstperson_aim_history_begin(UCFirstpersonAimHistory *state,
    const float look[16],const float *held,uint32_t classification,unsigned *search_needed) {
    UCFirstpersonAimHistory next;
    float direction[3],origin[3];
    if(!state||!look||!search_needed||classification>5||!finite_values(state->point,3)
        ||!finite_values(look,16)||(held&&!finite_values(held,16))) return UC_MOTION_ARGUMENT;
    if((_mm_getcsr()&0xE040u)!=0) return UC_MOTION_UNSUPPORTED;
    if(held) {transform(held+8,look,direction);transform(held+12,look,origin);}
    else {memcpy(direction,look+8,sizeof(direction));memcpy(origin,look+12,sizeof(origin));}
    if(!finite_values(origin,3)||!normalize(direction)) return UC_MOTION_UNSUPPORTED;
    next.selected_entity=-1;memcpy(next.previous_point,state->point,sizeof(next.previous_point));
    for(unsigned i=0;i<3;++i) next.point[i]=direction[i]*15000.0f+origin[i];
    if(!finite_values(next.point,3)) return UC_MOTION_UNSUPPORTED;
    *state=next;*search_needed=classification!=1&&classification!=4;return UC_MOTION_OK;
}
