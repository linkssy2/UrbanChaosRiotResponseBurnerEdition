#include "feedback_sample.h"
#include "euler_rotation.h"
#include <fenv.h>
#include <xmmintrin.h>
static int arithmetic(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>1e15f)return 0;return 1;}
static double time_weight(float now,float start,float duration){
    float t=1.f-(now-start)/duration;if(!(t>=0&&t<=1))return 0;
    t=t*t;t=t*t;return (double)t*t;
}
double uc_feedback_spatial_weight(const UCSpatialFeedbackEffect *e,const float position[3]){
    double x=(double)position[0]-e->position[0],y=(double)position[1]-e->position[1],z=(double)position[2]-e->position[2];
    double distance=sqrt((z*z+x*x)+y*y);if(e->radius<=distance)return 0;
    float t=1.f-(float)distance/e->radius;t=t*t;return ((double)t*t)*e->amplitude;
}
int uc_feedback_sample(const UCShotFeedback*p,const UCFeedbackObservers*v,uint32_t player,
    const float position[3],uint32_t kind,float clock,uint32_t mode,float*out){
    if(!p||!v||!out||player>=4||!p->sentinel.next||!p->spatial_sentinel.next||!arithmetic())return 1;
    if(kind==2&&!p->enabled[player]){*out=0;return 0;}
    if(!position||!finite_values(position,3)||!finite_values(&clock,1)||!finite_values(&p->time_offset,1)||
       v->position_count>65536||v->player_count>65536||
       (v->position_count&&!v->position)||(v->player_count&&!v->player))return 1;
    for(uint32_t i=0;i<v->position_count;i++)if(!v->position[i]||!v->position[i]->kind||!v->position[i]->sample)return 1;
    for(uint32_t i=0;i<v->player_count;i++)if(!v->player[i]||!v->player[i]->kind||!v->player[i]->player||!v->player[i]->sample)return 1;
    float now=mode==2?p->time_offset+clock:clock,result=0;
    for(const UCFeedbackNode*n=p->sentinel.next;n!=&p->sentinel;n=n->next)
        if((n->effect.kind&kind)&&n->effect.player==player)
            result=(float)(time_weight(now,n->effect.start,n->effect.duration)*n->effect.amplitude+result);
    for(const UCSpatialFeedbackNode*n=p->spatial_sentinel.next;n!=&p->spatial_sentinel;n=n->next)
        if(n->effect.kind&kind){double t=time_weight(now,n->effect.start,n->effect.duration);
            result=(float)((t?uc_feedback_spatial_weight(&n->effect,position)*t:0)+result);}
    for(uint32_t i=0;i<v->position_count;i++){
        const UCFeedbackPositionObserver*o=v->position[i];uint32_t mask;double value;
        if(o->kind(o->owner,&mask))return 2;
        if(mask&kind){if(o->sample(o->owner,position,&value)||!isfinite(value))return 2;result=(float)(value+result);}
    }
    for(uint32_t i=0;i<v->player_count;i++){
        const UCFeedbackPlayerObserver*o=v->player[i];uint32_t mask,owner;double value;
        if(o->kind(o->owner,&mask))return 2;
        if(mask&kind){if(o->player(o->owner,&owner))return 2;
            if(owner==player){if(o->sample(o->owner,&value)||!isfinite(value))return 2;result=(float)(value+result);}}
    }
    if(!isfinite(result))return 2;*out=result;return 0;
}
int uc_feedback_perturb(UCVectorRandom*r,float amplitude,const float world[16],float out[16]){
    if(!world||!out||!isfinite(amplitude)||fabsf(amplitude)>65536||!finite_values(world,16)||!arithmetic()||
       ((uintptr_t)out>(uintptr_t)world?(uintptr_t)out-(uintptr_t)world<64:(uintptr_t)world-(uintptr_t)out<64))return 1;
    if(amplitude<=0){memmove(out,world,64);return 0;}
    float angles[4],rotation[16],result[16];
    if(uc_vector_random_signed(r,amplitude,angles))return 1;
    for(unsigned i=0;i<4;i++)angles[i]=0.f+angles[i]; /* Actual54160 center addition. */
    uc_euler_rotation(angles[0],angles[1],angles[2],rotation);
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++)
        result[4*i+j]=(rotation[j]*world[4*i]+rotation[4+j]*world[4*i+1])+
            (rotation[8+j]*world[4*i+2]+rotation[12+j]*world[4*i+3]);
    memcpy(result+12,world+12,16);memcpy(out,result,64);return 0;
}
