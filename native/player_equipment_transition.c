#include "player_equipment_transition.h"
#include <math.h>

UCMainplayerStatus uc_player_equipment_clear(UCPlayerEquipmentReset *r){
    if(!r)return UC_MAINPLAYER_ARGUMENT;
    r->at2e0[6]=0;r->at2e0[5]=0;r->at2e0[4]=0;r->at2e0[1]=0;
    r->at2e0[0]=0;r->at2e0[3]=0;r->at2e0[2]=0;r->at2fc=0;
    r->at224[2]=1;r->at224[1]=0;r->at224[0]=0;
    return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_player_equipment_idle_transition(UCMainplayerActionState *a,uint32_t state,uint32_t stage){
    if(!a)return UC_MAINPLAYER_ARGUMENT;
    if(a->state!=0&&a->state!=7&&a->state!=8&&a->state!=13)return UC_MAINPLAYER_UNSUPPORTED;
    a->state=state;a->stage=stage;return UC_MAINPLAYER_OK;
}

static int item(int32_t id){return id==0||id==1||id==2||id==11||id==17;}
static int valid(const UCPlayerEquipmentTransition *p){
    return p&&p->inventory&&p->action&&p->control&&
        item(p->inventory->active_item)&&item(p->inventory->requested_item)&&
        p->action->state<=11&&!p->lowered&&!p->raising&&!p->restore_lowered&&
        p->special_action!=5&&p->special_action!=6;
}
UCMainplayerStatus uc_player_equipment_request(UCPlayerEquipmentTransition *p,
    const UCPlayerEquipmentSwitchInput *in,const UCPlayerEquipmentServices *h,
    void *ctx,uint8_t *requested){
    uint32_t state;
    if(!p||!in||!h||!requested||!h->preflight||!h->select_automatic||
        !h->select_category0||!h->transition)return UC_MAINPLAYER_ARGUMENT;
    if(!valid(p)||p->control->select_empty||p->control->select_previous||p->inventory->active_item==0||p->inventory->requested_item<=1||!isfinite(in->category_pressed[0])||
        in->category_pressed[1]!=0||in->category_pressed[2]!=0)return UC_MAINPLAYER_UNSUPPORTED;
    if(!h->preflight(ctx,p,0))return UC_MAINPLAYER_UNSUPPORTED;
    *requested=0;
    if(p->switch_blocked||p->blocked)return UC_MAINPLAYER_OK;
    state=p->action->state;
    if(state!=8&&state!=4&&state!=9&&!in->current_available){
        h->select_automatic(ctx);h->transition(ctx,8,0);*requested=1;return UC_MAINPLAYER_OK;
    }
    if(in->category_pressed[0]!=0){p->special_action=-1;h->select_category0(ctx);}
    if(p->inventory->requested_item==p->inventory->active_item)return UC_MAINPLAYER_OK;
    state=p->action->state;
    if(state==11||state==3||(state==8&&p->action->stage!=2))return UC_MAINPLAYER_OK;
    h->transition(ctx,8,0);*requested=1;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_player_equipment_update(UCPlayerEquipmentTransition *p,
    const UCPlayerEquipmentServices *h,void *ctx){
    if(!p||!h||!h->preflight||!h->transition||!h->action||!h->switch_sound||
        !h->controller_done||!h->commit_equipment||!h->current_pending_reload||
        !h->reload||!h->clear_action||!h->reload_gate||!h->reevaluate||!h->tail)
        return UC_MAINPLAYER_ARGUMENT;
    if(!valid(p)||(p->control->select_previous&&!p->control->select_empty&&!item(p->inventory->previous_item))||p->action->state!=8||p->action->stage>2||!h->preflight(ctx,p,1))
        return UC_MAINPLAYER_UNSUPPORTED;
    if(p->action->stage==0){
        h->action(ctx,1);
        if(!p->control->select_empty&&!p->control->select_previous)p->lowering_mode=0;
        h->switch_sound(ctx);p->action->stage=1;
    }
    if(p->action->stage==1){
        const uint8_t immediate=(uint8_t)(p->control->select_empty||p->control->select_previous);
        if(!h->controller_done(ctx)&&!immediate)return UC_MAINPLAYER_OK;
        p->previous_equipped=p->inventory->active_item;p->dirty1=p->dirty0=1;
        h->commit_equipment(ctx);
        if(h->current_pending_reload(ctx)){h->transition(ctx,7,0);h->reload(ctx);return UC_MAINPLAYER_OK;}
        p->action->stage=2;if((p->inventory->active_item==0||p->control->select_empty)&&!p->control->select_previous)p->special_action=-1;
        h->action(ctx,2);return UC_MAINPLAYER_OK;
    }
    if(h->controller_done(ctx)){
        h->clear_action(ctx);h->action(ctx,0);h->transition(ctx,0,0);
    }
    if(h->reload_gate(ctx)){h->clear_action(ctx);return UC_MAINPLAYER_OK;}
    h->reevaluate(ctx);
    if(p->action->state==4)h->clear_action(ctx);
    h->tail(ctx);return UC_MAINPLAYER_OK;
}
