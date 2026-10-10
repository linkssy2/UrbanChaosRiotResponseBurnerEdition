#include "mission_run.h"
int uc_mission_run(UCMissionEvent *e,uint8_t *condition,const UCLevel *level,
    uint32_t entity,void *owner,int (*decode)(void *,int32_t *),
    int (*yield)(void *),int32_t *out){
    if(!uc_mission_event_valid(e)||!condition||!level||!level->entities||
       entity>=level->entity_count||!decode||!yield||!out||
       (level->entities[entity].script_count&&!level->entities[entity].scripts))return 1;
    return uc_mission_run_descriptor(e,condition,level->entities+entity,owner,decode,yield,out);
}
int uc_mission_run_descriptor(UCMissionEvent *e,uint8_t *condition,const UCLevelEntity *definition,
    void *owner,int (*decode)(void *,int32_t *),int (*yield)(void *),int32_t *out){
    if(!uc_mission_event_valid(e)||!condition||!definition||!decode||!yield||!out||
       (definition->script_count&&!definition->scripts))return 1;
    for(;;){
        int32_t result;
        if(decode(owner,&result))return 2;
        while(result<-1){if(yield(owner)||decode(owner,&result))return 2;}
        if(*e->secondary_flags&0x1000){*e->secondary_flags&=~0x1000u;*out=-2;return 0;}
        if(result==-1&&!(*e->secondary_flags&0x8000)){
            *e->secondary_flags=(*e->secondary_flags&~8u)|0x100;
            const UCLevelEntity *v=definition;
            *e->cursor=*e->script_index<v->script_count?v->scripts[*e->script_index].code.data:0;
            continue;
        }
        if(result==-1){
            *e->priority=*e->condition_index=-1;*e->flags&=~0x40000000u;
            *e->secondary_flags=(*e->secondary_flags&~8u)|0x840;
            *e->new_script=*condition=0;result=-2;
        }else{
            *e->new_script=*condition=0;*e->flags&=~0x40000000u;
            *e->priority=*e->condition_index=-1;
            *e->secondary_flags=(*e->secondary_flags&~8u)|0x840;
        }
        *out=result;return 0;
    }
}
