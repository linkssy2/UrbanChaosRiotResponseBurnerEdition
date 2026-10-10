#include "animating_prop_state.h"
#include <math.h>
static const char *const names[]={"anim-00","anim-01","anim-02","anim-03","anim-04","anim-05","anim-06","anim-07","idle"};
const char *uc_animating_prop_action(int32_t i){return i>=0&&i<9?names[i]:0;}
static int audio(UCAnimatingPropState*s,const UCAnimatingPropStateServices*h){
    if(s->stop_previous&&h->stop(h->owner,s->bank,s->sound_name))return 2;
    s->sound_name=names[s->current];
    if(h->play(h->owner,s->bank,s->sound_name))return 2;
    return h->parameter(h->owner,s->bank,names[s->current],s->volume)?2:0;
}
int uc_animating_prop_select(UCAnimatingPropState*s,const uint8_t*flags,size_t size){
    if(!s)return 1;
    for(unsigned i=0;i<9;i++){const UCAnimatingPropCondition*c=s->conditions+i;if(c->count>64||(c->count>0&&(!c->flags||!flags)))return 1;
        for(int32_t j=0;j<c->count;j++)if(c->flags[j]<0||(size_t)c->flags[j]>=size)return 1;}
    unsigned found=0;
    for(int32_t i=8;i>=0;i--){const UCAnimatingPropCondition*c=s->conditions+i;if(c->count<=0)continue;int32_t j=0;
        for(;j<c->count&&flags[c->flags[j]];j++){}if(j==c->count){s->desired=i;found=1;}}
    if(s->return_to_idle&&!found)s->desired=8;
    return 0;
}
int uc_animating_prop_state_update(UCAnimatingPropState*s,const uint8_t*flags,size_t size,const UCAnimatingPropStateServices*h){
    if(!s||!h||!s->entity_flags||!s->disabled||!s->model||s->current<0||s->current>8||s->desired<0||s->desired>8||!isfinite(s->volume)||
       !h->done||!h->has_animation||!h->model_groups||!h->apply||!h->event||!h->stop||!h->play||!h->parameter)return 1;
    if(uc_animating_prop_select(s,flags,size))return 1;
    if(!(h->done(h->owner)&255u)&&!s->interrupt[s->current])return 0;
    if(*s->model&&s->current!=8&&*s->disabled)return 0;
    if(*s->disabled||(*s->entity_flags&0x40000000u))return 0;
    if(!s->finished&&(h->done(h->owner)&255u)){
        if(h->event(h->owner,s->finish_event[s->current],1))return 2;s->finished=1;
    }
    if(*s->entity_flags&0x40000000u)return 0;
    if(s->current!=s->desired){
        s->current=s->desired;if(!(h->has_animation(h->owner,s->current)&255u))return 0;
        int32_t group=s->current==8?0:s->current+1;
        if(h->model_groups(h->owner,*s->model)>group){if(!h->group||h->group(h->owner,group))return 2;}
        if(h->apply(h->owner,s->current)||audio(s,h))return 2;
    }else{
        if(!s->loop[s->current]||!(h->done(h->owner)&255u)||!(h->has_animation(h->owner,s->current)&255u))return 0;
        if(s->restart_audio&&audio(s,h))return 2;
        if(h->apply(h->owner,s->current))return 2;
    }
    if(h->event(h->owner,s->start_event[s->current],1))return 2;s->finished=0;return 0;
}
