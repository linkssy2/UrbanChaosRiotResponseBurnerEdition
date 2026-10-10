#include "cutscene_camera_cleanup.h"
static int valid(const UCEntityFreshView*v){
    return v&&v->target.identity&&v->target.flags&&v->id&&v->model74&&v->helper78&&v->aux84&&v->actor&&
        !(*v->target.flags&0x1000)&&!*v->model74&&!*v->helper78&&!v->actor->audioC0&&!v->actor->handleBC&&
        !v->actor->scriptC8&&!v->actor->flagCC&&!v->actor->flagCD&&!v->actor->flagCE&&uc_entity_fresh_aux_empty(*v->aux84);
}
static int retirement(const UCEntityRetirement*r){return r&&r->registry&&r->live&&r->pending&&r->live!=r->pending&&r->remove_update_record&&r->remove_render_record;}
int uc_cutscene_camera_remove(const UCEntityFreshView*v,UCCutsceneCamera*c,const UCCutsceneCameraCleanup*s){
    if(!valid(v)||!c||!s||!retirement(s->retirement)||!s->preflight||!s->remove_camera||!s->current_provider||
       !s->player||!s->return_control||!s->mode||s->preflight(s->owner))return 1;
    if(s->remove_camera(s->owner))return 2;
    if(s->current_provider(s->owner)){
        if(c->control_player){
            int32_t index=c->player_index,id;void*p=s->current_provider(s->owner);
            if(!p||s->player(s->owner,p,index,&id)||s->return_control(s->owner,id,(uint32_t)*v->id))return 2;
        }
        void*p=s->current_provider(s->owner);if(!p||s->mode(s->owner,p,-1,0))return 2;
    }
    v->actor->audioC0=NULL;
    if(uc_entity_fresh_aux_clear(*v->aux84))return 2;
    *v->target.flags|=0x8100;*v->helper78=NULL;
    uint8_t queued;if(uc_entity_retire(s->retirement,*v->id,&queued))return 2;
    *v->target.flags|=0x80000;
    return s->remove_camera(s->owner)?2:0;
}
int uc_cutscene_camera_destroy(const UCEntityFreshView*v,UCEntitySubscriptions*l,const UCCutsceneCameraCleanup*s,uint32_t flags){
    if(!valid(v)||!l||!s||!retirement(s->retirement)||uc_entity_event_lookup(s->retirement->registry,*v->id)||
       !s->preflight||!s->remove_camera||((flags&1)&&!s->release)||s->preflight(s->owner))return 1;
    uc_entity_subscriptions_clear(l);
    if(s->remove_camera(s->owner)||uc_entity_fresh_aux_clear(*v->aux84)||uc_entity_fresh_aux_destroy(*v->aux84))return 2;
    *v->helper78=NULL;
    if(flags&1)s->release(s->owner,v->target.identity);
    return 0;
}
