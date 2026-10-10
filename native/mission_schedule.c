#include "mission_schedule.h"
#include <string.h>
static uint32_t word(const uint8_t *p){uint32_t x;memcpy(&x,p,4);return x;}
static int valid(const UCMissionSchedule *s){
    return s&&uc_mission_event_valid(s->event)&&s->condition_started&&
        s->conditions_enabled&&s->definition&&s->decode&&s->yield&&s->destroy&&
        (!s->condition_count||s->conditions)&&s->condition_count<=INT32_MAX&&
        (!s->definition->script_count||s->definition->scripts);
}
int uc_mission_condition_run(const UCMissionSchedule *s){
    if(!valid(s))return 1;
    UCMissionEvent *e=s->event;
    if(!(*e->secondary_flags&4))return 0;
    int32_t index=*e->condition_index;
    if(index<0||(uint32_t)index>=s->condition_count)return 1;
    UCMissionGroup g=s->conditions[index].scripts;
    if(!g.rows||!g.count||g.count>INT32_MAX||(g.count>1&&!s->crt_seed))return 1;
    *e->secondary_flags&=~4u;*e->flags|=0x40000000;
    uint32_t selected=0;
    if(g.count>1){
        *s->crt_seed=*s->crt_seed*214013u+2531011u;
        int32_t draw=(int32_t)((*s->crt_seed>>16)&32767u)%100;
        for(uint32_t i=0;i<g.count;i++){
            int32_t low,high;memcpy(&low,g.rows+i*12+4,4);memcpy(&high,g.rows+i*12+8,4);
            if(low<draw&&high>draw){selected=i;break;}
        }
    }
    *e->script_index=word(g.rows+selected*12);
    for(;;){
        uint32_t script=*e->script_index;
        if(script==UINT32_MAX)break;
        const UCLevelEntity *entity=s->definition;
        *e->cursor=script<entity->script_count?entity->scripts[script].code.data:NULL;
        if(!*e->cursor)break;
        *e->script_index=script;*e->new_script=0;*e->flags|=0x40000000;
        *e->secondary_flags&=0xffff66bfu;
        if(*e->command)s->destroy(s->owner,*e->command);
        *e->command=NULL;*e->new_script=1;
        while(!(*e->secondary_flags&0x800)&&!(*e->secondary_flags&12)){
            int32_t result;
            if(uc_mission_run_descriptor(e,s->condition_started,s->definition,
                s->owner,s->decode,s->yield,&result)||s->yield(s->owner))return 2;
        }
        if(!(*e->secondary_flags&0x100))break;
    }
    if(uc_mission_event_cancel(e,s->destroy,s->owner))return 2;
    *e->priority=*e->condition_index=-1;
    return 0;
}
int uc_mission_schedule_run(const UCMissionSchedule *s){
    if(!valid(s))return 1;
    for(;;){
        if(*s->conditions_enabled&&uc_mission_conditions_update(s->event,
            s->condition_started,s->conditions,s->condition_count,s->flags,
            s->flag_count,s->destroy,s->owner))return 2;
        if(*s->condition_started&&uc_mission_condition_run(s))return 2;
        if(*s->event->new_script){int32_t result;
            if(uc_mission_run_descriptor(s->event,s->condition_started,s->definition,
                s->owner,s->decode,s->yield,&result))return 2;
        }
        if(s->yield(s->owner))return 2;
    }
}
