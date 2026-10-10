#include "bullet_initialize.h"
#include <math.h>
#include <fenv.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
/* Full finite2D5640, after3BD10 has extracted this same three-by-three basis. */
static int quaternion(const float *m,float *q){
    float trace=(m[5]+m[0])+m[10];
    if(trace>=0){
        float s=(float)-sqrt((double)trace+1.0),r=.5f/s;
        q[0]=(m[6]-m[9])*r;q[1]=(m[8]-m[2])*r;
        q[2]=(m[1]-m[4])*r;q[3]=(float)((double)s*.5);
    }else{
        unsigned i=m[5]>m[0]?1u:0u;if(m[10]>m[i*5])i=2;
        unsigned j=(i+1)%3,k=(j+1)%3;float t[4];
        double s=sqrt(((double)m[i*5]-((double)m[k*5]+m[j*5]))+1.0);
        float rounded=(float)s,r=rounded!=0?.5f/rounded:rounded;
        t[i]=(float)(s*.5);t[3]=(m[j*4+k]-m[k*4+j])*r;
        t[j]=(m[j*4+i]+m[i*4+j])*r;t[k]=(m[k*4+i]+m[i*4+k])*r;
        for(unsigned n=0;n<4;n++)q[n]=0.f-t[n];
    }
    return finite_values(q,4);
}
int uc_bullet_frame_quaternion(const float *m,float *out){
    float q[4];
    if(!m||!out||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST||
       !finite_values(m,16)||!quaternion(m,q))return 1;
    memcpy(out,q,sizeof q);return 0;
}
static const UCEntityFreshView *parent(const UCBulletInitialize *h,uint32_t id){
    UCEntityEventTarget *t=uc_entity_event_lookup(h->registry,(int32_t)id);
    return t?h->resolve(h->context,t):NULL;
}
int uc_bullet_initialize(const UCBulletInitialize *h,const UCEntityFreshView *v,
    UCBulletState *s,const UCWeaponBulletRequest *r,uint32_t owner){
    float q[4];UCBulletPrepared p;UCBulletMotion motion;
    if(!h||!v||!s||!r||!h->registry||!h->group_counter||!h->frame||!h->bullet_counter||
       !h->progression181||!h->resolve||!h->preflight||!h->initial_update||
       !v->target.identity||!v->target.flags||!v->target.parent_id||!v->transform||
       !v->model74||*v->model74||!v->actor||!v->aux84||!uc_entity_fresh_aux_empty(*v->aux84)||
       v->actor->flagCC||v->actor->flagCD||v->actor->flagCE||r->sentinel!=-1||
       (_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST||
       uc_bullet_frame_quaternion(v->transform,q))return 1;
    UCEntityEventTarget *target=uc_entity_event_lookup(h->registry,(int32_t)owner);
    const UCEntityFreshView *admitted=target?h->resolve(h->context,target):NULL;
    if(target&&(!admitted||admitted->target.identity!=target||!admitted->target.flags||!admitted->actor))return 1;
    UCBulletPrepareInput in={owner,*h->frame,0,*h->progression181};
    if(uc_bullet_prepare(r,&in,&p)||uc_bullet_motion_reset(v->transform+12,q,&motion)||
       !h->preflight(h->context,v,s,r))return 1;
    *v->target.parent_id=(int32_t)owner;s->actor=UINT32_MAX;s->motion=motion;
    *v->target.flags|=0xA00;s->item=s->definition_kind=s->team=0;s->impact=.1f;
    const UCEntityFreshView *other=parent(h,owner);
    if(other&&(*other->target.flags&0x400000u))*v->target.flags|=0x400000u;
    memset(s->vectorE4,0,sizeof s->vectorE4);s->valueDC=0;s->enabled1C8=1;
    other=parent(h,(uint32_t)*v->target.parent_id);
    if(other&&(*other->target.flags&0x8000000u))v->actor->group94=other->actor->group94;
    else {uint32_t old=*h->group_counter,next=old+1u;*h->group_counter=(int32_t)next>=0x100000?1:next;v->actor->group94=(uint16_t)old;}
    s->item=r->item;s->definition_kind=r->definition_kind;s->team=r->team;s->actor=r->actor;
    memcpy(s->direction,r->direction,12);s->sentinel=r->sentinel;v->actor->valueA4=r->value20;
    s->scale1E4=r->scale24;s->value1E0=r->value28;s->tracer1E8=r->flag2c;
    memcpy(v->transform+12,r->origin,12);v->transform[15]=1;memcpy(s->previous218,r->origin,12);
    memcpy(s->parameters,p.parameters,sizeof s->parameters);s->value150=0;
    memset(s->vectors,0,sizeof s->vectors);s->vectors[1][0]=1;
    s->frame224=*h->frame;memset(s->vector20C,0,sizeof s->vector20C);
    s->started1F4=0;s->counter1EC=0;s->impact=p.impact_impulse;s->motion.mass=p.mass;
    v->actor->valueA4=p.value20;v->actor->word98=0x4000;
    for(unsigned i=0;i<12;i++)v->transform[i]=i%5==0?1.f:0.f;
    memcpy(s->direction,p.direction,12);memcpy(s->motion.velocity,p.velocity,12);
    for(unsigned i=0;i<3;i++)s->motion.drag[i]=1;s->motion.flags|=0xE0000;
    ++*h->bullet_counter;s->dead1F5=0;
    if(uc_bullet_motion_begin(&s->motion)||uc_entity_fresh_aux_clear(*v->aux84))return 2;
    return h->initial_update(h->context,v,s)?2:0;
}
