#include "mission_trigger_actor.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static int finite_values(const float *p,unsigned n,float bound){
    if(!p)return 0;for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>bound)return 0;return 1;
}
int uc_mission_trigger_overlap(const UCLevelTrigger *t,const UCMissionTriggerBody *b,uint8_t *out){
    if(!t||!b||!out||t->shape!=3||t->boxes.size%24||
       (t->boxes.size&&!t->boxes.data)||t->boxes.size/24>100000||
       !finite_values(b->position,3,1e6f)||!finite_values(b->distance88,1,1e12f)||
       *b->distance88<0||(_mm_getcsr()&0xE040u))return 1;
    for(uint32_t at=0;at<t->boxes.size;at+=24){float p[6];memcpy(p,t->boxes.data+at,24);if(!finite_values(p,6,1e6f))return 1;}
    uint8_t result=0;
    for(uint32_t at=0;at<t->boxes.size;at+=24){
        float p[6],distance=0;memcpy(p,t->boxes.data+at,24);
        for(unsigned j=0;j<3;j++){
            float low=p[j+3]<p[j]?p[j+3]:p[j],high=p[j+3]<p[j]?p[j]:p[j+3];
            float delta;
            if(low>b->position[j])delta=b->position[j]-low;
            else if(b->position[j]>high)delta=b->position[j]-high;
            else continue;
            distance=delta*delta+distance;
        }
        if(*b->distance88>distance){result=1;break;}
    }
    *out=result;return 0;
}
int uc_mission_trigger_query(const UCLevelTrigger *t,const UCMissionTriggerWorld *w,uint8_t *out){
    if(!t||!w||!out||!w->registry||!w->body)return 1;
    if(t->kind!=17&&t->kind!=37){*out=0;return 0;}
    const int32_t *named=NULL;uint32_t count=t->subjects.count;
    if(t->kind==37){if(!w->names||uc_entity_names_view(w->names,"PLAYER",&named,&count))return 1;}
    else if((count&&!t->subjects.data)||!w->slots||!w->slots->count||
        *w->slots->count<0||(size_t)*w->slots->count>w->slots->capacity||
        (*w->slots->count&&!w->slots->slots))return 1;
    if(count>100000)return 1;
    uint8_t all=(uint8_t)(t->flags&1),result=count?all:0;
    for(uint32_t i=0;i<count;i++){
        const UCLevelEntitySlot *slot=NULL;int32_t id;
        if(named)id=named[i];
        else{
            uint32_t index=t->subjects.data[i];
            if(index>=(uint32_t)*w->slots->count||!(slot=w->slots->slots[index])||!slot->id||!slot->flags)return 1;
            id=*slot->id;
        }
        UCEntityEventTarget *entity=uc_entity_event_lookup(w->registry,id);
        if(!entity){if(named)return 1;result=0;continue;}
        if(slot&&!(*slot->flags&8)){if(all){result=0;break;}continue;}
        UCMissionTriggerBody body;uint8_t inside;
        if(w->body(w->owner,entity,&body)||uc_mission_trigger_overlap(t,&body,&inside))return 1;
        if(all&&!inside){result=0;break;}
        if(!all&&inside){result=1;break;}
    }
    *out=result;return 0;
}
int uc_mission_trigger_construct(UCMissionTriggerActor *a,const UCEntityFreshView *v,
    UCLevelTrigger *t,int32_t source_index,uint32_t groups,const uint8_t **cursor,
    UCMissionCondition *conditions,uint32_t condition_count,const UCEntityFreshMemory *m,
    void *owner,int (*base)(void *)){
    if(!a||!v||!v->source_index||!v->definition70||!v->transform||!v->actor||!base||
       !m||!m->allocate||!m->release||(condition_count&&!conditions)||condition_count>100000)return 1;
    if(t&&groups){
        if(!cursor||!t->script.properties.data||!(*cursor)||
           (uintptr_t)*cursor<(uintptr_t)t->script.properties.data||
           (uintptr_t)*cursor>(uintptr_t)t->script.properties.data+t->script.properties.size)return 1;
        size_t left=(uintptr_t)t->script.properties.data+t->script.properties.size-(uintptr_t)*cursor;
        if(left<4||word(*cursor)>100000||word(*cursor)>(left-4)/8)return 1;
    }
    if(base(owner))return 2;
    a->started=0;a->count=0;a->rows=NULL;a->state=0;
    if(!t)return 0;
    *v->source_index=source_index;*v->definition70="InfoNodeSentient";
    if(groups){
        a->count=(int32_t)word(*cursor);*cursor+=4;
        if(a->count){a->rows=m->allocate(m->owner,(size_t)a->count*sizeof(*a->rows));if(!a->rows)return 2;}
        for(int32_t i=0;i<a->count;i++){a->rows[i].type=word(*cursor);*cursor+=4;a->rows[i].group=word(*cursor);*cursor+=4;}
    }
    if(condition_count){v->actor->flagCE=1;for(uint32_t i=0;i<condition_count;i++)conditions[i].used=0;}
    memcpy(v->transform+12,t->position,12);v->transform[15]=1;return 0;
}
int uc_mission_trigger_update(UCMissionTriggerActor *a,UCLevelTrigger *t,const UCMissionTriggerWorld *w,
    UCMissionEvent *event,uint32_t *seed,void (*destroy)(void *,void *),void *owner,int (*suffix)(void *)){
    if(!a||!t||!w||!uc_mission_event_valid(event)||!suffix||a->count<0||a->count>100000||
       (a->count&&!a->rows)||(*event->command&&!destroy))return 1;
    const uint8_t flags=t->flags;const int32_t state=a->state;
    if((flags&16)&&!((flags&2)&&(flags&8))&&state>=0&&state<=2){
        uint8_t found=0;
        for(int32_t i=0;i<a->count;i++){
            uint32_t type=a->rows[i].type;uint8_t inside=0,accepted=0;
            if(state==0){
                if(type==0){if(a->started)continue;}
                else if(type==1){if(uc_mission_trigger_query(t,w,&inside))return 2;if(!inside)continue;}
                else continue;
            }else{
                if(type!=(state==1?2u:1u))continue;found=1;
                if(uc_mission_trigger_query(t,w,&inside))return 2;
                if(inside!=(state==2))continue;
            }
            uint32_t group=a->rows[i].group;
            if(state&&group==UINT32_MAX)accepted=1;
            else if(uc_mission_event_start_descriptor(event,&t->script,group,1,seed,destroy,owner,&accepted))return 2;
            if(accepted){
                if(state==0&&type==0)a->started=1;
                else{a->state=state==1?2:1;if(state==1)t->flags|=2;}
            }
        }
        if(state&&!found){uint8_t inside;if(uc_mission_trigger_query(t,w,&inside))return 2;
            if(inside==(state==2)){a->state=state==1?2:1;if(state==1)t->flags|=2;}}
    }
    return suffix(owner)?2:0;
}
int uc_mission_trigger_deactivate(UCMissionTriggerActor *a,const UCEntityFreshMemory *m,void *owner,int (*deactivate)(void *)){
    if(!a||!m||!m->release||!deactivate)return 1;
    m->release(m->owner,a->rows);return deactivate(owner)?2:0;
}
