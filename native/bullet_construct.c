#include "bullet_construct.h"
#include <string.h>
static int construct(const UCEntityFreshContext *c,const UCEntityFreshView *v,UCBulletState *s,const float*matrix){
    static const float defaults[9]={1200,900,500,350,.6f,.4f,1,1.5f,20};
    float q[4];UCBulletMotion motion;int result;
    if(!s)return 1;
    result=matrix?uc_entity_fresh_construct_matrix(c,v,matrix):uc_entity_fresh_construct_unplaced(c,v);if(result)return result;
    memcpy(s->parameters,defaults,sizeof defaults);
    memset(s->vectors,0,sizeof s->vectors);s->vectors[1][0]=1;s->value150=0;
    s->actor=UINT32_MAX;
    if(uc_bullet_frame_quaternion(v->transform,q)||
       uc_bullet_motion_reset(v->transform+12,q,&motion))return 2;
    s->motion=motion;*v->target.flags|=0xA00;
    s->item=s->definition_kind=s->team=0;s->impact=.1f;
    UCEntityEventTarget *t=uc_entity_event_lookup(c->registration->registry,*v->target.parent_id);
    if(t){const UCEntityFreshView *p=c->resolve(c->owner,t);
        if(!p||p->target.identity!=t||!p->target.flags)return 2;
        if(*p->target.flags&0x400000u)*v->target.flags|=0x400000u;}
    memset(s->vectorE4,0,sizeof s->vectorE4);s->valueDC=0;s->enabled1C8=1;
    return 0;
}
int uc_bullet_construct(const UCEntityFreshContext*c,const UCEntityFreshView*v,UCBulletState*s){return construct(c,v,s,NULL);}
int uc_bullet_construct_matrix(const UCEntityFreshContext*c,const UCEntityFreshView*v,UCBulletState*s,const float*m){return m?construct(c,v,s,m):1;}
