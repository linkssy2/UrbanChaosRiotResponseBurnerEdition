#include "animation_events.h"
#include <string.h>
static int handlers_valid(const UCAnimationHandlers *h){
    if(!h||h->count>16)return 0;
    for(uint32_t i=0;i<h->count;++i)if(!h->items[i])return 0;
    return 1;
}
static int reference_valid(UCAnimationEventRef r){
    return r.owner&&r.owner->events&&r.index<r.owner->event_count;
}
static int state_valid(const UCAnimationDispatch *s,const UCAnimationHandlers *global){
    if(!s||s->global!=global||!handlers_valid(s->local)||!handlers_valid(global)||s->queue.count>16)return 0;
    /* Null slots are original handled-marker tombstones during nested dispatch. */
    for(uint32_t i=0;i<s->queue.count;++i)
        if(s->queue.events[i].owner&&!reference_valid(s->queue.events[i]))return 0;
    return 1;
}
UCAnimationStatus uc_animation_handler_add(UCAnimationHandlers *h,UCAnimationEventHandler fn,int *out){
    if(!out||!fn||!handlers_valid(h))return UC_ANIMATION_ARGUMENT;
    *out=0;if(h->count==16)return UC_ANIMATION_OK;
    for(uint32_t i=0;i<h->count;++i)if(h->items[i]==fn)return UC_ANIMATION_OK;
    h->items[h->count++]=fn;*out=1;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_handler_remove(UCAnimationHandlers *h,UCAnimationEventHandler fn,int *out){
    if(!out||!fn||!handlers_valid(h))return UC_ANIMATION_ARGUMENT;
    *out=0;for(uint32_t i=0;i<h->count;++i)if(h->items[i]==fn){
        --h->count;memmove(h->items+i,h->items+i+1,(h->count-i)*sizeof(*h->items));*out=1;break;
    }
    return UC_ANIMATION_OK;
}
static UCAnimationStatus invoke(UCAnimationHandlers *h,UCAnimationEventRef event,
    UCAnimationDispatch *s,const UCAnimationHandlers *global,int *handled){
    if(!reference_valid(event))return UC_ANIMATION_ARGUMENT;
    *handled=0;
    for(uint32_t i=0;i<h->count;++i){
        uint8_t result=h->items[i](event,s);
        if(!state_valid(s,global)||!handlers_valid(h))return UC_ANIMATION_ARGUMENT;
        if(result){*handled=1;break;}
    }
    return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_dispatch_events(UCAnimationDispatch *s){
    if(!s)return UC_ANIMATION_ARGUMENT;UCAnimationHandlers *global=s->global;
    if(!state_valid(s,global))return UC_ANIMATION_ARGUMENT;
    for(uint32_t i=0;i<s->queue.count;++i){
        int handled;UCAnimationStatus status=invoke(s->local,s->queue.events[i],s,global,&handled);
        if(status)return status;
        if(!handled){status=invoke(global,s->queue.events[i],s,global,&handled);if(status)return status;}
        if(handled)s->queue.events[i]=(UCAnimationEventRef){0};
    }
    uint32_t kept=0;
    for(uint32_t i=0;i<s->queue.count;++i)if(s->queue.events[i].owner)s->queue.events[kept++]=s->queue.events[i];
    s->queue.count=kept;return UC_ANIMATION_OK;
}
