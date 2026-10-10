#include "player_shield.h"
#include <math.h>
static int item(int32_t n){return n==0||n==1||n==2||n==17;}
UCMainplayerStatus uc_player_shield_input(UCPlayerEquipmentTransition *p,uint8_t held,
    const UCPlayerShieldServices *h,void *ctx,uint8_t *requested){
    if(!p||!p->inventory||!p->action||!p->control||!h||!h->sound||
       !h->transition||!h->update_switch||!requested)return UC_MAINPLAYER_ARGUMENT;
    if(!item(p->inventory->active_item)||(!item(p->inventory->requested_item)&&p->inventory->requested_item!=11)||
       (!item(p->inventory->previous_item)&&p->inventory->previous_item!=11)||p->lowered||p->raising||p->restore_lowered||
       (p->action->state!=0&&p->action->state!=8))return UC_MAINPLAYER_UNSUPPORTED;
    *requested=0;
    if(p->blocked||!p->inventory->owned[0])return UC_MAINPLAYER_OK;
    const int shielded=(p->inventory->active_item==0||p->control->select_empty)&&!p->control->select_previous;
    if(!shielded&&p->inventory->active_item!=0){
        if(!held||p->control->select_empty)return UC_MAINPLAYER_OK;
        p->lowered=p->restore_lowered=p->raising=0;
        h->sound(ctx,1);p->control->select_empty=1;
    }else{
        if(held||p->control->select_previous)return UC_MAINPLAYER_OK;
        h->sound(ctx,0);p->control->select_previous=1;
    }
    h->transition(ctx,8,0);h->update_switch(ctx);p->special_action=-1;
    *requested=1;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_player_shield_blocks(int32_t id,const UCMainplayerEquipControl *control,float impact,
    const float b[16],const float d[3],uint8_t *blocked){
    if(!b||!d||!blocked||!control)return UC_MAINPLAYER_ARGUMENT;
    if(!isfinite(impact))return UC_MAINPLAYER_UNSUPPORTED;
    for(unsigned j=0;j<3;j++)if(!isfinite(b[j])||!isfinite(b[8+j])||!isfinite(d[j]))return UC_MAINPLAYER_UNSUPPORTED;
    double fs=((double)b[10]*b[10]+(double)b[9]*b[9])+(double)b[8]*b[8];
    double rs=((double)b[2]*b[2]+(double)b[1]*b[1])+(double)b[0]*b[0];
    if(fs<1e-30||rs<1e-30||fs>1e30||rs>1e30)return UC_MAINPLAYER_UNSUPPORTED;
    float fi=(float)(1.0/sqrt(fs)),ri=(float)(1.0/sqrt(rs));
    float f[3],r[3];for(unsigned j=0;j<3;j++){f[j]=b[8+j]*fi;r[j]=b[j]*ri;}
    float front=(d[1]*f[1]+d[2]*f[2])+d[0]*f[0];
    float right=(d[1]*r[1]+d[2]*r[2])+d[0]*r[0];
    *blocked=(uint8_t)((id==0||control->select_empty)&&!control->select_previous&&impact<100.f&&front<=0.f&&fabs((double)front)>fabs((double)right));
    return UC_MAINPLAYER_OK;
}
