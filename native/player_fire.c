#include "player_fire.h"
#include <limits.h>
#include <math.h>
static void aim(UCPlayerFire *p,const UCPlayerFireServices *h,void *c){if(p->action->animation!=8)h->aim(c);}
static void wait(UCPlayerFire *p,const UCPlayerFireServices *h,void *c){
    UCMainplayerActionState *s=p->action;
    if(s->ready_pulse)s->ready_latch=1;
    if(h->controller_done(c)||s->animation==15||s->ready_latch){
        if(s->animation==15)h->action(c,0);
        h->stop_fire(c);s->state=s->stage=0;h->reevaluate(c);return;
    }
    h->utility(c);h->switch_weapon(c);
    if(s->state==4){h->fire_input(c);if(s->state==4)h->reload_input(c);}
    aim(p,h,c);
}
UCPlayerFireStatus uc_player_fire_update(UCPlayerFire *p,const UCPlayerFireServices *h,void *c,uint8_t *applied){
    if(!applied)return UC_PLAYER_FIRE_ARGUMENT;*applied=0;
    if(!p||!p->action||!p->weapon||!p->reserve||!h||!h->preflight||!h->action||!h->reset_fire_input||
       !h->shot||!h->controller_done||!h->utility||!h->switch_weapon||!h->fire_input||!h->reload_input||
       !h->aim||!h->stop_fire||!h->reevaluate)return UC_PLAYER_FIRE_ARGUMENT;
    UCMainplayerActionState *s=p->action;
    if((p->item!=2&&p->item!=17)||p->weapon->alternate||*p->reserve>INT32_MAX||
       s->state!=4||(s->stage!=0&&s->stage!=2)||s->animation>=18||(s->stage==2&&s->animation!=3)||
       !isfinite(p->clock)||p->clock<0||!isfinite(p->delta)||p->delta<0||!h->preflight(c,p))return UC_PLAYER_FIRE_UNSUPPORTED;
    *applied=1;
    if(s->ready_pulse)s->ready_latch=1;
    if(!s->stage){
        s->input_flags|=2;s->ready_latch=0;s->ready_pulse=0;
        if(s->animation)h->action(c,0);
        uc_weapon_reset_shots(p->weapon);h->action(c,3);
        UCWeaponState next=*p->weapon;UCWeaponCadenceResult result;
        if(uc_weapon_cadence(p->item,*p->reserve,p->clock,p->delta,&next,&result)!=UC_MAINPLAYER_OK)
            return UC_PLAYER_FIRE_UNSUPPORTED;
        if(result.reset_fire_input)h->reset_fire_input(c);
        *p->weapon=next;
        if(result.ready&&h->shot(c))return UC_PLAYER_FIRE_EFFECT_FAILED;
        s->stage=2;wait(p,h,c);aim(p,h,c);
    }else wait(p,h,c);
    return UC_PLAYER_FIRE_OK;
}
