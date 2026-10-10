#include "player_cutscene.h"
#include "firstperson_look.h"
#include <string.h>
static int valid(const UCPlayerCutscene*p,const UCPlayerCutsceneServices*s){
    return p&&p->equipment&&p->equipment->inventory&&p->equipment->action&&p->input&&p->disabled&&
        s&&s->preflight&&s->action&&s->transition&&s->input&&s->inhibit;
}
#define CALL(x) do{if(x)return 2;}while(0)
int uc_player_cutscene_take(UCPlayerCutscene*p,const UCPlayerCutsceneServices*s,int32_t sender){
    if(!valid(p,s)||!s->stop_idle||!s->sender_class||!s->stop_zoom||s->preflight(s->owner,0))return 1;
    UCPlayerEquipmentTransition*e=p->equipment;void*o=s->owner;
    if(!e->lowered&&e->inventory->active_item!=-1){
        if(e->action->state)CALL(s->action(o,0,0,0));
        CALL(s->stop_idle(o));
    }
    CALL(s->transition(o,13,0));
    if(*p->input)CALL(s->input(o,*p->input,0));
    const char*name=NULL;CALL(s->sender_class(o,sender,&name));
    CALL(s->inhibit(o,1,(uint8_t)(name&&!strcmp(name,"EnemyDeathCamera"))));
    e->lowering_mode=0;CALL(s->stop_zoom(o));return 0;
}
int uc_player_cutscene_return(UCPlayerCutscene*p,const UCPlayerCutsceneServices*s){
    if(!valid(p,s)||!s->commit||!s->flags_ready||s->preflight(s->owner,1))return 1;
    UCPlayerEquipmentTransition*e=p->equipment;void*o=s->owner;
    if(e->action->state==13){
        if(*p->input)CALL(s->input(o,*p->input,1));
        CALL(s->inhibit(o,0,0));
    }
    e->dirty1=e->dirty0=1;CALL(s->transition(o,0,0));CALL(s->commit(o));
    if(!*p->disabled){uint8_t ready=0;CALL(s->flags_ready(o,&ready));
        if(!ready){CALL(s->action(o,2,0,0));return 0;}}
    if(!e->lowered&&e->inventory->active_item!=-1)CALL(s->action(o,0,-1,1));
    return 0;
}
#undef CALL
static int take_event(void*owner,const void*payload,uint32_t*sender){
    UCPlayerCutsceneEvents*e=owner;(void)payload;
    if(!sender)return 1;
    return uc_player_cutscene_take(e->player,e->services,(int32_t)*sender);
}
static int return_event(void*owner,const void*payload,uint32_t*sender){
    UCPlayerCutsceneEvents*e=owner;(void)payload;(void)sender;
    return uc_player_cutscene_return(e->player,e->services);
}
static int reset_event(void*owner,const void*payload,uint32_t*sender){
    UCPlayerCutsceneEvents*e=owner;(void)payload;(void)sender;
    return uc_firstperson_look_reset(e->angles,*e->pitch_limit);
}
int uc_player_cutscene_events_init(UCPlayerCutsceneEvents*e,UCPlayerCutscene*p,
    const UCPlayerCutsceneServices*s,float angles[3],const float*limit){
    if(!e||!angles||!limit||!valid(p,s)||!s->stop_idle||!s->sender_class||!s->stop_zoom||!s->commit||!s->flags_ready)return 1;
    e->player=p;e->services=s;
    e->angles=angles;e->pitch_limit=limit;
    e->rows[0]=(UCEntityEventSubscription){113,take_event};
    e->rows[1]=(UCEntityEventSubscription){166,return_event};
    e->rows[2]=(UCEntityEventSubscription){34,reset_event};
    e->listener=(UCEntityEventListener){e->rows,3,e};return 0;
}
int uc_player_presentation_update(UCPlayerPresentationGate*p,const UCPlayerPresentationGateServices*s){
    uint8_t active;
    if(!p||!s||!s->query||!s->set)return 1;
    if(s->query(s->owner,&active))return 2;
    uint8_t desired=(uint8_t)(!p->inhibited&&(p->override_on||(p->enabled&&p->compatible)));
    if(!!active!=desired&&s->set(s->owner,desired))return 2;
    return 0;
}
int uc_player_presentation_inhibit(UCPlayerPresentationGate*p,const UCPlayerPresentationGateServices*s,uint8_t enabled,uint8_t death){
    if(!p||!s||!s->query||!s->set||(!death&&(!s->suspend||!s->resume)))return 1;
    p->inhibited=enabled;
    if(uc_player_presentation_update(p,s))return 2;
    if(!death&&p->restore&&(enabled?s->suspend(s->owner):s->resume(s->owner)))return 2;
    return 0;
}
