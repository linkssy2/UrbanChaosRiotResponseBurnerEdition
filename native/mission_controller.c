#include "mission_controller.h"
static int update(const UCMissionController *c,const UCLevelEntity *definition,int direct){
    if(!c||!c->event||!c->group||!c->model)return 1;
    UCMissionEvent *e=c->event;
    if(!uc_mission_event_valid(e)||
       (*c->model&&(!c->destroy_model||!c->release_model)))return 1;
    *e->flags|=0x100;
    void *model=*c->model;
    if(model){c->destroy_model(c->owner,model);c->release_model(c->owner,model);}
    uint32_t group=*c->group;
    *c->model=0;
    if(group==UINT32_MAX)return 0;
    if(!(*e->flags&0x40000000)){
        if(*e->secondary_flags&0x44)return 0;
    }else{
        if(!(*e->secondary_flags&0x100)){*c->group=UINT32_MAX;return 0;}
        if(*e->secondary_flags&0x1000)*c->group=*e->script_index;
        group=*c->group;
    }
    uint8_t accepted;
    return (direct?uc_mission_event_start_descriptor(e,definition,group,1,c->crt_seed,
        c->destroy_command,c->owner,&accepted):uc_mission_event_start(e,c->level,c->entity,group,1,c->crt_seed,
        c->destroy_command,c->owner,&accepted))?2:0;
}

int uc_mission_controller_update(const UCMissionController *c){return update(c,0,0);}
int uc_mission_controller_update_descriptor(const UCMissionController *c,const UCLevelEntity *d){return update(c,d,1);}
