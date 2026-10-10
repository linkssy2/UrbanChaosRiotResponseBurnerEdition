#include "glass_entity_update.h"
#include <math.h>
#include <xmmintrin.h>
int uc_glass_entity_update(const UCGlassEntityUpdate*v,const UCGlassEntityAudio*a){
    if(!v||!v->entity||!v->properties||!v->world||!v->did_break||!v->forced_break||
       v->did_break==v->forced_break||*v->forced_break||v->properties->npc_smash||
       v->properties->on_broken[1]!=UINT32_MAX||(_mm_getcsr()&0xE040u))return 1;
    const UCGlassEntityProperties*p=v->properties;
    for(unsigned n=0;n<16;n++)if(!isfinite(v->world[n])||fabsf(v->world[n])>1e6f)return 1;
    const float f[]={p->left,p->right,p->up,p->down,p->collision_height};
    for(unsigned n=0;n<5;n++)if(!isfinite(f[n])||fabsf(f[n])>1e6f)return 1;
    float delay=(p->collision_height-v->world[13])*.0001f;
    if(delay>0||fabsf(v->world[9])>.5f)return 1;
    UCGlassEntity*e=v->entity;
    if(*v->did_break||!e->container11C||!uc_glass_container_state(e->container11C))return 0;
    float volume=((p->up+p->down)*(p->left+p->right))*.000002f;
    if(e->bank114){
        if(!a||!a->preflight||!a->play||a->preflight(a->context,e->bank114,"GlassBreak",0,volume))return 1;
        if(a->play(a->context,e->bank114,"GlassBreak",0,volume))return 2;
    }
    *v->did_break=1;
    if(!e->state118)e->state118=1;
    return 0;
}
