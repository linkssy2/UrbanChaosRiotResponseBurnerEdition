#include "mainplayer_events.h"
#include <string.h>

const UCMainplayerEventRegistration uc_mainplayer_event_registrations[UC_MAINPLAYER_EVENT_COUNT]={
    {"GunReloadTrigger",0x194},{"GunHalfReload",0x195},{"ThrownWeaponThrow",0x196},
    {"MeleeAttackStartHit",0x199},{"ReadyToFire",0x198},{"FootstepLeft",0x19a},
    {"FootstepRight",0x19b},{"HandstepLeft",0x19c},{"HandstepRight",0x19d},
    {"LadderSlide",0x19e},{"SmallRumble",0x19f},{"MediumRumble",0x1a0},
    {"BigRumble",0x1a1},{"Heal",0x1a2},{"Twirl",0x1a3}
};
UCAnimationStatus uc_mainplayer_events_init(const uint32_t codes[UC_MAINPLAYER_EVENT_COUNT],UCMainplayerEvents *out){
    if(!codes||!out)return UC_ANIMATION_ARGUMENT;
    UCMainplayerEvents state={0};memcpy(state.codes,codes,sizeof(state.codes));*out=state;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_events_init_original(UCMainplayerEvents *out){
    static const uint32_t codes[UC_MAINPLAYER_EVENT_COUNT]={4,49,44,37,0,18,19,0,0,0,50,51,52,0,0};
    return uc_mainplayer_events_init(codes,out);
}
static int valid(const UCMainplayerEvents *s){return s&&!(s->pending&0x8000u);}
static uint8_t collect(UCAnimationEventRef ref,UCAnimationDispatch *dispatch){
    UCMainplayerEvents *s=(UCMainplayerEvents*)dispatch->user;uint8_t handled=0;
    uint32_t code=ref.owner->events[ref.index].code;
    for(uint32_t i=0;i<UC_MAINPLAYER_EVENT_COUNT;i++)if(s->codes[i]==code){
        s->pending|=(uint16_t)(1u<<i);handled=1;
    }
    return handled;
}
UCAnimationStatus uc_mainplayer_events_collect(UCMainplayerEvents *state,UCAnimationEventQueue *queue){
    if(!valid(state)||!queue)return UC_ANIMATION_ARGUMENT;
    UCMainplayerEvents copy=*state;UCAnimationHandlers local={1,{collect}},global={0};
    UCAnimationDispatch dispatch={*queue,&local,&global,&copy};
    UCAnimationStatus status=uc_animation_dispatch_events(&dispatch);if(status)return status;
    *state=copy;*queue=dispatch.queue;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_events_publish(UCMainplayerEvents *state){
    if(!valid(state))return UC_ANIMATION_ARGUMENT;
    for(uint32_t i=0;i<UC_MAINPLAYER_EVENT_COUNT;i++)state->published[i]=(uint8_t)((state->pending>>i)&1);
    state->pending=0;return UC_ANIMATION_OK;
}
