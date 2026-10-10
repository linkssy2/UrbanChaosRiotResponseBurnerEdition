#include "particle_motion.h"
#include <float.h>
#include <fenv.h>
#include <math.h>
#include <xmmintrin.h>
static int bounded(float v){return isfinite(v)&&fabsf(v)<=1048576.f;}
int uc_particle_motion(UCParticle *p,uint32_t capacity,uint32_t *count,
    const UCParticleMotion *in,UCParticleBounds *bounds){
    if(!count||!in||!bounds||capacity>1048576||*count>capacity||(!p&&capacity)||
       !isfinite(in->delta)||in->delta<0||in->delta>1||!bounded(in->drag)||
       !bounded(in->size_start)||!bounded(in->size_delta)||fegetround()!=FE_TONEAREST||
       (_mm_getcsr()&0xE040u))return 1;
    for(unsigned j=0;j<3;j++)if(!bounded(in->gravity_step[j])||!bounded(in->wind_step[j]))return 1;
    for(uint32_t i=0;i<*count;i++){
        if(!bounded(p[i].radius))return 1;
        for(unsigned j=0;j<3;j++)if(!bounded(p[i].position[j])||!bounded(p[i].velocity[j]))return 1;
    }
    if(!*count)return 0;
    float low[3]={FLT_MAX,FLT_MAX,FLT_MAX},high[3]={-FLT_MAX,-FLT_MAX,-FLT_MAX};
    const float size_scale=in->size_delta*(1.f/65535.f);uint32_t live=0;
    for(uint32_t i=0;i<*count;i++){
        const uint16_t age=(uint16_t)(p[i].age+in->age_step);
        if(age<p[i].age)continue;
        UCParticle next=p[i];next.age=age;
        for(unsigned j=0;j<3;j++){
            const float resistance=(-next.velocity[j])*in->drag;
            next.velocity[j]=(next.velocity[j]+in->gravity_step[j])+resistance;
            next.position[j]=(next.position[j]+next.velocity[j]*in->delta)+in->wind_step[j];
            low[j]=low[j]<next.position[j]?low[j]:next.position[j];
            high[j]=high[j]>next.position[j]?high[j]:next.position[j];
        }
        next.radius=(float)age*size_scale+in->size_start;p[live++]=next;
    }
    for(unsigned j=0;j<3;j++){
        const float lo=high[j]<low[j]?high[j]:low[j],hi=high[j]<low[j]?low[j]:high[j];
        bounds->minimum[j]=lo;bounds->maximum[j]=hi;
        bounds->center[j]=(hi+lo)*.5f;bounds->half_extent[j]=(hi-lo)*.5f;
    }
    *count=live;return 0;
}
