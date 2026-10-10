#include "npc_melee_controller.h"
#include <math.h>
#include <string.h>
static const int32_t types[]={27,9,28,29,34,8,5,35,0,36};
static const float ranges[]={320,225,320,320,10,320,320,10,320,320};
static const float cones[]={.94f,.94f,.94f,.94f,.1f,.94f,.85f,.1f,.9f,.9f};
static const float damage[]={200,100,0,100,0,150,100,0,100,100};
double uc_npc_melee_range(const UCNpcMeleeController*s){return s&&s->profile>=0&&s->profile<10?ranges[s->profile]:-1.f;} /*2A2C0*/
double uc_npc_melee_facing(const UCNpcMeleeController*s){return s&&s->profile>=0&&s->profile<10?cones[s->profile]:1.1f;} /*2A2E0*/
int uc_npc_melee_contact(int32_t profile,const float*a,const float*t,const float*f,uint8_t*out){
    if(profile<0||profile>9||!a||!t||!f||!out)return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(a[i])||!isfinite(t[i])||!isfinite(f[i]))return 1;
    float x=t[0]-a[0],y=t[1]-a[1],z=t[2]-a[2],square=z*z+x*x;
    if(sqrt((double)y*y+square)>ranges[profile]){*out=0;return 0;}
    float inverse=(float)(1./sqrt((double)f[2]*f[2]+(double)f[0]*f[0]));
    float fx=f[0]*inverse,fz=f[2]*inverse,fy=0.f*inverse;
    inverse=(float)(1./sqrt((double)square));
    float dot=(fz*(z*inverse)+(x*inverse)*fx)+(inverse*0.f)*fy;
    /*Original COMISS/JBE accepts unordered (coincident positions included).*/
    *out=(uint8_t)(!(cones[profile]>dot));return 0;
}
int uc_npc_melee_hit(const UCNpcMeleeController*s,const float*a,const float*t,float*amount,UCPlayerFireDamage*out){
    if(!s||s->profile<0||s->profile>9||!a||!t||!amount||!out)return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(a[i])||!isfinite(t[i]))return 1;
    memset(out,0,sizeof*out);out->source=s->type;out->kind=s->option?9:8;out->zero=6;
    out->direction[0]=t[0]-a[0];out->direction[2]=t[2]-a[2];out->owner=s->actor;
    out->other=out->session=out->unique=-1;*amount=damage[s->profile];return 0;
}
void uc_npc_melee_construct(UCNpcMeleeController*s,int32_t actor,void*body,void*events,void*bank){
    s->weapon=s->type=s->selected=s->last_target=-1;s->detached=s->hidden=0;
    s->actor=actor;s->body=body;s->events=events;s->bank=bank;
    s->watching=s->active=s->enabled=0;s->profile=s->target=-1;
}
int uc_npc_melee_close(UCNpcMeleeController*s,const UCNpcMeleeServices*h){int found;
    if(!s||!h)return 1;
    if(s->watching){if(!h->unwatch||h->unwatch(h->owner,&s->start)||h->unwatch(h->owner,&s->end))return 2;s->watching=0;}
    if(s->weapon!=-1){if(!h->resolve||(found=h->resolve(h->owner,s->weapon))<0)return 2;
        if(found&&(!h->retire||h->retire(h->owner,s->weapon)))return 2;
        s->weapon=s->type=s->selected=-1;s->detached=0;}
    return 0;
}
int uc_npc_melee_bind(UCNpcMeleeController*s,int32_t weapon,const UCNpcMeleeServices*h){int32_t type=0;unsigned i;int found;
    if(!s||!h)return 1;if(uc_npc_melee_close(s,h))return 2;
    if(weapon==-1)type=-1;else if(!h->weapon_type||h->weapon_type(h->owner,weapon,&type))return 2;
    s->profile=-1;for(i=0;i<sizeof types/sizeof types[0];i++)if(type==types[i]){s->profile=(int32_t)i;break;}
    if(s->profile==-1)return 0;
    if(!h->resolve||(found=h->resolve(h->owner,s->actor))<0)return 2;if(!found)return 0;
    s->type=type;s->selected=2;s->weapon=weapon;
    if(!s->watching){if(!h->watch||h->watch(h->owner,&s->start,"MeleeAttackStartHit",s->actor)||h->watch(h->owner,&s->end,"MeleeAttackEndHit",s->actor))return 2;s->watching=1;}
    return 0;
}
void uc_npc_melee_target(UCNpcMeleeController*s,int32_t target,int32_t count,uint8_t option){s->target=target;s->count=count;s->option=option;}
void uc_npc_melee_cancel(UCNpcMeleeController*s){s->enabled=s->active=0;s->count=0;s->target=-1;}
int uc_npc_melee_update(UCNpcMeleeController*s,const UCNpcMeleeServices*h){int found;uint8_t hit;
    if(!s||!h)return 1;if(s->weapon==-1)return 0;
    if(s->detached){if(!h->resolve||(found=h->resolve(h->owner,s->weapon))<0)return 2;if(!found)s->weapon=-1;return 0;}
    if(s->start){if(s->count>0){--s->count;s->enabled=1;}if(s->target!=-1)s->last_target=s->target;s->start=0;s->active=1;}
    if(s->end){s->enabled=s->end=s->active=0;}
    if(s->enabled&&s->active){if(!h->eligible||h->eligible(h->owner,s->target,&hit))return 2;
        if(hit){if(!h->strike||h->strike(h->owner))return 2;
            if(s->profile<0||s->profile>9)return 2;
            /*Every table44D050 row's continuous byte is read by the source.*/
            s->enabled=0;}}
    return 0;
}
