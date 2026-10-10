#include "entity_fresh_cleanup.h"
#include <fenv.h>
#include <math.h>
#include <xmmintrin.h>
static int valid(const UCEntityFreshView*v){
    if(!v||!v->target.identity||!v->target.flags||!v->id||!v->model74||!v->helper78||
       !v->aux84||!v->actor||(*v->target.flags&0x1000)||*v->model74||*v->helper78||
       v->actor->audioC0||v->actor->handleBC||v->actor->scriptC8||
       v->actor->flagCC||v->actor->flagCD||v->actor->flagCE)return 0;
    for(unsigned i=0;i<3;i++)if(v->target.identity->listeners[i])return 0;
    return uc_entity_fresh_aux_empty(*v->aux84);
}
int uc_entity_fresh_update(const UCEntityFreshView*v,void(*advance)(void*),void*owner){
    if(!valid(v)||!advance)return 1;
    advance(owner);
    return uc_entity_fresh_aux_clear(*v->aux84)?2:0;
}
int uc_entity_fresh_late(const UCEntityFreshView*v){
    if(!valid(v)||!v->transform)return 1;
    if(v->actor->word90&1)return 0;
    if((_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    for(unsigned i=12;i<15;i++)if(!isfinite(v->transform[i]))return 1;
    for(unsigned i=12;i<15;i++)v->transform[i]=v->transform[i]+0.0f;
    v->transform[15]=1.0f;return 0;
}
int uc_entity_fresh_deactivate(const UCEntityFreshView*v,const UCEntityRetirement*r,uint8_t*queued){
    if(!valid(v)||!r||!r->registry||!r->live||!r->pending||r->live==r->pending||
       !r->remove_update_record||!r->remove_render_record||!queued)return 1;
    v->actor->audioC0=NULL;
    if(uc_entity_fresh_aux_clear(*v->aux84))return 2;
    *v->target.flags|=0x8100;
    *v->helper78=NULL;
    uint8_t result;
    if(uc_entity_retire(r,*v->id,&result))return 2;
    *v->target.flags|=0x80000;*queued=result;return 0;
}
static int destroy_actor(const UCEntityFreshView*v,
    void(*release)(void*,UCEntityEventTarget*),void*owner,uint32_t deleting_flags){
    if(uc_entity_fresh_aux_clear(*v->aux84)||uc_entity_fresh_aux_destroy(*v->aux84))return 2;
    *v->helper78=NULL;
    if(deleting_flags&1)release(owner,v->target.identity);
    return 0;
}
int uc_entity_fresh_destroy(const UCEntityFreshView*v,void(*finish)(void*),
    void(*release)(void*,UCEntityEventTarget*),void*owner,uint32_t deleting_flags){
    if(!valid(v)||!finish||((deleting_flags&1)&&!release))return 1;
    finish(owner);
    return destroy_actor(v,release,owner,deleting_flags);
}
int uc_entity_fresh_actor_destroy(const UCEntityFreshView*v,
    void(*release)(void*,UCEntityEventTarget*),void*owner,uint32_t deleting_flags){
    if(!valid(v)||((deleting_flags&1)&&!release))return 1;
    return destroy_actor(v,release,owner,deleting_flags);
}
