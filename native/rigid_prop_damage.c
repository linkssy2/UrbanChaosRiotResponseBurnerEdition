#include "rigid_prop_damage.h"
#include <string.h>
#include <math.h>
#define CALL(f,...) do{if(!h->f||h->f(h->owner,__VA_ARGS__))return 2;}while(0)
#define CALL0(f) do{if(!h->f||h->f(h->owner))return 2;}while(0)
static int valid(UCRigidPropDamage*s,const UCRigidPropParameters*p,const UCRigidPropDamageServices*h){return s&&p&&h&&h->position&&isfinite(h->delta)&&h->delta>=0;}
static int32_t owner(const UCRigidPropDamage*s,const UCRigidPropDamageServices*h){return s->owner==-1?h->id:s->owner;}
void uc_rigid_prop_damage_begin(UCRigidPropDamage*s,const UCRigidPropParameters*p){if(!s||!p)return;memset(s,0,sizeof *s);s->owner=-1;s->delay=p->explode_delay;s->point_particle=p->point_particle;s->shot_particle=p->shot_particle;}
int uc_rigid_prop_explode(UCRigidPropDamage*s,const UCRigidPropParameters*p,const UCRigidPropDamageServices*h,int32_t who){
    if(!valid(s,p,h))return 1;CALL(flags,0x20);s->exploded=1;s->explosions++;
    UCRigidPropExplosion x={0};for(unsigned k=0;k<3;k++)x.position[k]=h->position[k]+p->explosion_offset[k];
    x.sound=p->sound;x.scale=.5f;x.radius=500;x.power=7500;x.owner=who;x.particle_type=2;x.ignore_collision=1;x.parent=h->id;
    CALL(explosion,&x);int32_t groups=0;CALL(groups,&groups);if(groups>1)CALL0(group_one);return 0;
}
int uc_rigid_prop_damage_event(UCRigidPropDamage*s,const UCRigidPropParameters*p,const UCRigidPropDamageServices*h,int32_t sender){
    if(!valid(s,p,h))return 1;uint8_t body=0;CALL(body,&body);if(body)CALL0(wake);
    if(!s->exploded){if(p->explodable){s->hits=(int32_t)((uint32_t)s->hits+1);s->owner=sender;
        if(s->hits>=(int32_t)p->max_bullet_hits&&uc_rigid_prop_explode(s,p,h,sender))return 2;}
        if(!s->exploded&&s->shot_particle&&*s->shot_particle)CALL(particle,s->shot_particle,"HK_GAS_01",0);}
    return 0;
}
int uc_rigid_prop_damage_bullet(UCRigidPropDamage*s,const UCRigidPropParameters*p,const UCRigidPropDamageServices*h,int32_t projectile){
    if(!valid(s,p,h))return 1;
    if(!s->exploded){if(p->explodable){uint8_t found=0;int32_t parent=-1;CALL(resolve_parent,projectile,&parent,&found);
        if(found){s->hits=(int32_t)((uint32_t)s->hits+1);s->owner=parent;}
        if(s->hits>=(int32_t)p->max_bullet_hits&&uc_rigid_prop_explode(s,p,h,s->owner))return 2;}
        if(!s->exploded&&s->shot_particle&&*s->shot_particle)CALL(particle,s->shot_particle,"HK_GAS_01",1);}
    return 0;
}
int uc_rigid_prop_damage_step(UCRigidPropDamage*s,const UCRigidPropParameters*p,const UCRigidPropDamageServices*h){
    if(!valid(s,p,h))return 1;CALL0(base_update);uint8_t body=0;CALL(body,&body);if(!body)return 0;
    if(s->point_particle&&*s->point_particle){CALL(particle,s->point_particle,"FXPOINT",1);s->point_particle=0;}
    int32_t type;float velocity[3];CALL(motion,&type,velocity);if(type!=6)for(unsigned k=0;k<3;k++)if(velocity[k]>1000)CALL0(fixed);
    if(!s->exploded&&p->explodable){if(s->hits>0&&s->delay>0){double delay=(double)s->delay-h->delta;s->delay=(float)delay;
        if(delay<=0&&uc_rigid_prop_explode(s,p,h,owner(s,h)))return 2;}
        uint32_t word;float blast;CALL(value,0x16,&word);memcpy(&blast,&word,4);
        if(s->blast!=blast){if(uc_rigid_prop_explode(s,p,h,owner(s,h)))return 2;s->blast=blast;}}
    if(p->max_life!=0){uint32_t count,word;float blast;CALL(value,0x15,&count);CALL(value,0x16,&word);memcpy(&blast,&word,4);
        if(s->contacts!=(int32_t)count){int32_t diff=(int32_t)(count-(uint32_t)s->contacts);s->life=(float)diff*10.f+s->life;s->contacts=(int32_t)count;}
        if(s->blast!=blast){s->life=(blast-s->blast)+s->life;s->blast=blast;}
        if(s->life>p->max_life){CALL(sound,"Destroyed");CALL0(destroy);}}
    return 0;
}
