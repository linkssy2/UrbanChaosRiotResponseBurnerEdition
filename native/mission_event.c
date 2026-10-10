#include "mission_event.h"
#include "entity_fresh.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
int uc_mission_event_valid(const UCMissionEvent *e){
    return e&&e->flags&&e->secondary_flags&&e->flags!=e->secondary_flags&&
        e->priority&&e->condition_index&&e->script_index&&e->cursor&&e->command&&e->new_script;
}
int uc_mission_event_bind(const UCEntityFreshView *v,UCMissionEvent *out){
    if(!v||!out||!v->actor||!v->target.flags||!v->flags60||v->target.flags==v->flags60)return 1;
    UCEntityFreshActor *a=v->actor;
    *out=(UCMissionEvent){v->target.flags,v->flags60,&a->idB0,&a->idB4,
        &a->script_indexB8,&a->wordC4,&a->audioC0,&a->flagCC};return 0;
}
int uc_mission_event_start(UCMissionEvent*e,const UCLevel*l,uint32_t entity,
    uint32_t request,uint8_t grouped,uint32_t*seed,
    void(*destroy)(void*,void*),void*owner,uint8_t*accepted){
    if(!uc_mission_event_valid(e)||!accepted)return 1;
    if(request==UINT32_MAX){*accepted=0;return 0;}
    if(!l||(l->entity_count&&!l->entities))return 1;
    return uc_mission_event_start_descriptor(e,entity<l->entity_count?l->entities+entity:NULL,
        request,grouped,seed,destroy,owner,accepted);
}
int uc_mission_event_start_descriptor(UCMissionEvent*e,const UCLevelEntity*definition,
    uint32_t request,uint8_t grouped,uint32_t*seed,
    void(*destroy)(void*,void*),void*owner,uint8_t*accepted){
    if(!uc_mission_event_valid(e)||!accepted)return 1;
    if(request==UINT32_MAX){*accepted=0;return 0;}
    UCMissionGroup group={0};
    if(grouped){
        if(!definition||uc_mission_group(definition,request,&group)!=UC_MISSION_READY||group.count>INT32_MAX)return 1;
        if(group.priority<*e->priority){*accepted=0;return 0;}
        if(group.count>1&&!seed)return 1;
    }
    if(*e->command&&!destroy)return 1;
    if(grouped){
        *e->priority=group.priority;uint32_t selected=0;
        if(group.count>1){
            *seed=*seed*214013u+2531011u;int32_t draw=(int32_t)((*seed>>16)&32767u)%100;
            for(uint32_t i=0;i<group.count;i++){
                int32_t low,high;memcpy(&low,group.rows+i*12+4,4);memcpy(&high,group.rows+i*12+8,4);
                if(low<draw&&high>draw){selected=i;break;}
            }
        }
        request=word(group.rows+selected*12);
    }
    *e->cursor=definition&&request<definition->script_count&&definition->scripts?definition->scripts[request].code.data:NULL;
    if(!*e->cursor){*accepted=0;return 0;}
    *e->script_index=request;*e->new_script=0;*e->flags|=0x40000000;
    *e->secondary_flags&=0xffff66bf;
    if(*e->command)destroy(owner,*e->command);
    *e->command=NULL;*e->new_script=1;
    if(grouped)*e->secondary_flags|=8;
    *accepted=1;return 0;
}
int uc_mission_event_cancel(UCMissionEvent*e,void(*destroy)(void*,void*),void*owner){
    if(!uc_mission_event_valid(e)||(*e->command&&!destroy))return 1;
    *e->secondary_flags&=0xffffffb7;*e->flags&=0xbfffffff;
    if(*e->command)destroy(owner,*e->command);
    *e->command=NULL;*e->priority=-1;*e->new_script=0;*e->condition_index=-1;return 0;
}
