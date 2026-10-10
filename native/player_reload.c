#include "player_reload.h"
#include <limits.h>
#include <math.h>
static int valid(const UCPlayerReload *p){
    return p&&(p->item==2||p->item==17)&&p->action&&p->weapon&&p->progress&&p->lowering&&p->lowering_mode&&p->overlay&&
        (!p->overlay->clip==!p->overlay->annotations)&&p->action->state==7&&p->action->stage<=1&&p->action->animation<18&&
        isfinite(*p->progress)&&*p->progress>=0&&*p->progress<1&&isfinite(*p->lowering);
}
static UCAnimationStatus controllers(const UCPlayerReloadServices *h,void *ctx,
    UCAnimationController *const **out,uint32_t *count){
    *out=h->controllers(ctx,count);
    if(*count>32||(*count&&!*out))return UC_ANIMATION_UNSUPPORTED;
    for(uint32_t i=0;i<*count;i++){
        if(!(*out)[i])return UC_ANIMATION_UNSUPPORTED;
        for(uint32_t j=0;j<i;j++)if((*out)[j]==(*out)[i])return UC_ANIMATION_UNSUPPORTED;
    }
    return UC_ANIMATION_OK;
}
UCAnimationStatus uc_player_reload_update(UCPlayerReload *p,const UCPlayerReloadServices *h,void *ctx,uint8_t *applied){
    UCAnimationController *const *list;uint32_t count;UCAnimationStatus status;
    if(!applied)return UC_ANIMATION_ARGUMENT;
    *applied=0;
    if(!h||!h->preflight||!h->action||!h->reload_amount||!h->reevaluate||!h->load_sound||!h->controllers)
        return UC_ANIMATION_ARGUMENT;
    if(!valid(p)||!h->preflight(ctx,p))return UC_ANIMATION_UNSUPPORTED;
    *applied=1;
    if(!p->action->stage){
        h->action(ctx,4);p->action->stage=1;p->action->ready_latch=p->action->reload_latch=0;
        if(p->weapon->pending_reload){
            uc_mainplayer_reload_overlay_clear(p->overlay);
            status=controllers(h,ctx,&list,&count);if(status)return status;
            status=uc_mainplayer_reload_seek(list,count,*p->progress);if(status)return status;
            *p->lowering=1;
        }else{p->weapon->pending_reload=1;*p->progress=0;h->load_sound(ctx);}
        return UC_ANIMATION_OK;
    }
    uint32_t amount=h->reload_amount(ctx);if(amount>INT32_MAX)return UC_ANIMATION_UNSUPPORTED;
    UCMainplayerReloadOwner fields={*p->lowering_mode,*p->progress,*p->lowering,p->weapon->pending_reload};
    uint8_t evaluate=0;
    status=uc_mainplayer_reload_wait(p->item,(int32_t)amount,p->action,&fields,&evaluate);if(status)return status;
    *p->lowering_mode=fields.lowering_mode;
    if(evaluate)h->reevaluate(ctx);
    status=controllers(h,ctx,&list,&count);if(status)return status;
    int32_t request=-1;
    UCMainplayerActionState decision=*p->action;
    status=uc_mainplayer_reload_poll(p->item,count?list[0]:NULL,&decision,&fields,&request);if(status)return status;
    if(request==0){
        p->weapon->pending_reload=0;h->action(ctx,0);
        return uc_mainplayer_reload_finish(p->item,p->action);
    }
    *p->progress=fields.progress;return UC_ANIMATION_OK;
}
