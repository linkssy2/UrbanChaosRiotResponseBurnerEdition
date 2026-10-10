#include "mainplayer_actions.h"
#include <math.h>
#include <fenv.h>
#include <xmmintrin.h>

static int item_ok(uint32_t item) { return item==2 || item==17; }
UCAnimationStatus uc_mainplayer_fire_gate(const UCMainplayerFireGate *in,
    UCMainplayerFireDecision *out) {
    UCMainplayerFireDecision r={0};
    if(!in || !out || !item_ok(in->item) || !isfinite(in->fire_pressed))
        return UC_ANIMATION_ARGUMENT;
    r.alternate=in->alternate;
    if(!(in->manager_flags&0x8200) && (in->input_flags&1)
       && (in->fire_mode>=5 || !(in->input_flags&2))) {
        if(in->can_current) r.allowed=1;
        else if(in->can_other) { r.allowed=1; r.alternate=(uint8_t)!r.alternate; }
        else r.misfire=(uint8_t)(in->fire_pressed!=0.0f);
    }
    *out=r;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_gate(const UCMainplayerReloadGate *in,
    UCMainplayerReloadDecision *out) {
    UCMainplayerReloadDecision r={0};
    if(!in || !out || !item_ok(in->item) || !isfinite(in->reload_pressed))
        return UC_ANIMATION_ARGUMENT;
    if(!in->lowered && in->reload_amount>0) {
        if(in->pending) r.requested=1;
        else if(!in->can_primary) { r.requested=1; r.forced_stat=1; }
        else if(in->reload_pressed!=0.0f && !in->consumed && !in->blocked
                && in->permission) {
            r.requested=1;r.manual_stat=1;r.tutorial_record=1;
        }
    }
    *out=r;return UC_ANIMATION_OK;
}
static int fire_start(uint32_t item,const UCMainplayerActionState *s) {
    return item_ok(item) && s && s->state==4 && s->stage==0 && s->animation<18;
}
UCAnimationStatus uc_mainplayer_fire_enter(uint32_t item,
    UCMainplayerActionState *s,int32_t *request) {
    if(!fire_start(item,s)||!request)return UC_ANIMATION_ARGUMENT;
    *request=s->animation?0:-1;
    s->input_flags|=2;s->ready_latch=0;s->ready_pulse=0;s->animation=0;
    return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_fire_animation(uint32_t item,
    UCMainplayerActionState *s,int32_t *request) {
    if(!fire_start(item,s)||!request)return UC_ANIMATION_ARGUMENT;
    s->animation=3;*request=3;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_fire_commit(uint32_t item,UCMainplayerActionState *s) {
    if(!fire_start(item,s)||s->animation!=3)return UC_ANIMATION_ARGUMENT;
    s->stage=2;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_fire_wait(uint32_t item,UCMainplayerActionState *s,
    uint8_t done,UCMainplayerFireNext *next) {
    if(!item_ok(item)||!s||!next||s->state!=4||s->stage!=2||s->animation!=3)
        return UC_ANIMATION_ARGUMENT;
    if(s->ready_pulse)s->ready_latch=1;
    *next=done||s->ready_latch?UC_MAINPLAYER_FIRE_FINISH:UC_MAINPLAYER_FIRE_INPUT;
    return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_fire_finish(uint32_t item,UCMainplayerActionState *s) {
    if(!item_ok(item)||!s||s->state!=4||s->stage!=2||s->animation!=3)
        return UC_ANIMATION_ARGUMENT;
    s->state=0;s->stage=0;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_enter(uint32_t item,
    UCMainplayerActionState *s,int32_t *request) {
    if(!item_ok(item)||!s||!request||s->state!=7||s->stage!=0||s->animation>=18)
        return UC_ANIMATION_ARGUMENT;
    s->animation=4;s->stage=1;s->ready_latch=0;s->reload_latch=0;*request=4;
    return UC_ANIMATION_OK;
}
static int reload_active(uint32_t item,const UCMainplayerActionState *s) {
    return item_ok(item)&&s&&s->state==7&&s->stage==1&&s->animation<18;
}
static int reload_owner(const UCMainplayerReloadOwner *r) {
    return r&&isfinite(r->progress)&&isfinite(r->lowering);
}
static int reload_rounding(void) {
    return (_mm_getcsr()&0xE040u)==0&&fegetround()==FE_TONEAREST;
}
UCAnimationStatus uc_mainplayer_reload_start(uint32_t item,
    const UCMainplayerActionState *s,UCMainplayerReloadOwner *r,UCMainplayerReloadStart *next) {
    if(!reload_active(item,s)||s->animation!=4||!reload_owner(r)||!next)
        return UC_ANIMATION_ARGUMENT;
    if(r->pending)*next=UC_MAINPLAYER_RELOAD_RESUME;
    else {r->pending=1;r->progress=0;*next=UC_MAINPLAYER_RELOAD_LOAD_SOUND;}
    return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_resumed(uint32_t item,
    const UCMainplayerActionState *s,UCMainplayerReloadOwner *r) {
    if(!reload_active(item,s)||s->animation!=4||!reload_owner(r)||!r->pending)
        return UC_ANIMATION_ARGUMENT;
    r->lowering=1;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_seek(UCAnimationController *const *list,
    uint32_t count,float phase) {
    if((count&&!list)||count>32||!isfinite(phase)||phase<0||phase>=1)
        return UC_ANIMATION_ARGUMENT;
    for(uint32_t i=0;i<count;i++) {
        if(!list[i])return UC_ANIMATION_ARGUMENT;
        for(uint32_t j=0;j<i;j++)if(list[i]==list[j])return UC_ANIMATION_ARGUMENT;
    }
    for(uint32_t i=0;i<count;i++)list[i]->phase=phase;
    return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_wait(uint32_t item,int32_t reload_amount,
    UCMainplayerActionState *s,UCMainplayerReloadOwner *r,uint8_t *reevaluate) {
    if(!reload_active(item,s)||!reload_owner(r)||!reevaluate)return UC_ANIMATION_ARGUMENT;
    if(reload_amount)s->ready_latch=0;
    else if(s->ready_pulse){s->ready_latch=1;r->lowering_mode=0;}
    *reevaluate=(uint8_t)(s->ready_latch!=0);return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_poll(uint32_t item,
    const UCAnimationController *controller,UCMainplayerActionState *s,
    UCMainplayerReloadOwner *r,int32_t *request) {
    float duration,progress;
    if(!item_ok(item)||!s||s->state>11||s->animation>=18||!reload_owner(r)||!request)
        return UC_ANIMATION_ARGUMENT;
    if((!controller||!(controller->flags&0x40000000u))&&s->state==7) {
        r->pending=0;s->animation=0;*request=0;return UC_ANIMATION_OK;
    }
    if(!controller||!isfinite(controller->phase)||controller->phase<0||controller->phase>=1
       ||!isfinite(controller->phase_scale)||controller->phase_scale<=0)
        return UC_ANIMATION_ARGUMENT;
    if(!reload_rounding())return UC_ANIMATION_UNSUPPORTED;
    duration=(float)(1.0/(double)controller->phase_scale);
    progress=(float)(((double)controller->phase/(double)controller->phase_scale)/(double)duration);
    if(!isfinite(duration)||duration<=0||!isfinite(progress))return UC_ANIMATION_UNSUPPORTED;
    r->progress=progress;*request=-1;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_finish(uint32_t item,UCMainplayerActionState *s) {
    if(!item_ok(item)||!s||s->state!=7||s->animation)return UC_ANIMATION_ARGUMENT;
    s->state=0;s->stage=0;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_lowering(float delta,float *lowering) {
    float result;
    if(!lowering||!isfinite(*lowering)||!isfinite(delta)||delta<0)return UC_ANIMATION_ARGUMENT;
    if(!reload_rounding())return UC_ANIMATION_UNSUPPORTED;
    result=*lowering<=.01f?0:(float)((double)*lowering-(double)delta*(double)*lowering*6.0);
    if(!isfinite(result))return UC_ANIMATION_UNSUPPORTED;
    *lowering=result;return UC_ANIMATION_OK;
}
