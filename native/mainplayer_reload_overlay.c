#include "mainplayer_reload_overlay.h"
#include <math.h>

void uc_mainplayer_reload_overlay_clear(UCMainplayerReloadOverlay *overlay) {
    if(overlay){overlay->clip=NULL;overlay->annotations=NULL;}
}
UCAnimationStatus uc_mainplayer_reload_overlay_start(uint32_t item,
    UCMainplayerReloadOverlay *overlay,const UCAnimationClip *clip,
    const UCAnimationAnnotations *annotations,UCMainplayerReloadOwner *owner,uint8_t *load_sound) {
    if((item!=2&&item!=17)||!overlay||!clip||!annotations||!owner||!load_sound||
        (annotations->event_count&&!annotations->events)||
        (owner->pending&&(!isfinite(owner->progress)||owner->progress<0||owner->progress>=1)))
        return UC_ANIMATION_ARGUMENT;
    UCMainplayerReloadOverlay next={0};
    UCAnimationStatus status=uc_animation_controller_init(clip,0,&next.controller);
    if(status)return status;
    next.clip=clip;next.annotations=annotations;
    if(owner->pending)next.controller.phase=owner->progress;
    uint8_t sound=!owner->pending;
    if(sound){owner->pending=1;owner->progress=0;}
    *overlay=next;*load_sound=sound;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_overlay_activate(UCMainplayerReloadOverlay *overlay) {
    if(!overlay||!overlay->clip||!overlay->annotations)return UC_ANIMATION_ARGUMENT;
    overlay->controller.flags|=0xc0000000u;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_overlay_advance(UCMainplayerReloadOverlay *overlay,
    uint32_t category,const UCAnimationClip *reload,float delta,
    UCAnimationEventQueue *queue,uint8_t *dispatch) {
    if(!overlay||!queue||!dispatch||queue->count>16)return UC_ANIMATION_ARGUMENT;
    if(!overlay->clip){*dispatch=0;return UC_ANIMATION_OK;}
    if(category||!reload||reload!=overlay->clip){
        uc_mainplayer_reload_overlay_clear(overlay);*dispatch=0;return UC_ANIMATION_OK;
    }
    UCAnimationController next=overlay->controller;UCAnimationAdvance advance;
    UCAnimationEventQueue events=*queue;
    UCAnimationStatus status=uc_animation_controller_advance(&next,delta,&advance);
    if(status)return status;
    status=uc_animation_collect_events(overlay->annotations,&advance,&events);
    if(status)return status;
    overlay->controller=next;*queue=events;*dispatch=1;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_reload_overlay_finish(UCMainplayerReloadOverlay *overlay,
    UCMainplayerReloadOwner *owner) {
    if(!overlay||!overlay->clip||!owner)return UC_ANIMATION_ARGUMENT;
    if(overlay->controller.flags&0x40000000u){
        if(!isfinite(overlay->controller.phase))return UC_ANIMATION_ARGUMENT;
        owner->progress=overlay->controller.phase;
    }else{uc_mainplayer_reload_overlay_clear(overlay);owner->pending=0;}
    return UC_ANIMATION_OK;
}
