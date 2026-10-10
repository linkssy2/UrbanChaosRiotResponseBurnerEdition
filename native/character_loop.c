#include "character_loop.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int valid(const UCCharacterLoop *s,const UCCharacterLoopServices *h){
    return s&&h&&s->world&&isfinite(s->duration)&&isfinite(s->elapsed)&&
        s->interrupt_count<=64&&(s->interrupt_count<=0||s->interrupts)&&
        h->sound&&h->attach_condition&&h->collision&&h->animation_owner&&h->animation_begin&&
        h->animation_cycle&&h->condition&&h->interrupt&&h->transition&&
        h->base_exit&&h->delta&&h->actor_world&&(_mm_getcsr()&0xE040)==0;
}
static void exit_loop(UCCharacterLoop*s,const UCCharacterLoopServices*h){
    h->sound(h->owner,s->actor,s->bank,0);
    if(s->disable_collision)h->collision(h->owner,s->actor,1);
    h->base_exit(h->owner,s);
}
int uc_character_loop_enter(UCCharacterLoop*s,UCCharacterLoopController*c,const UCCharacterLoopServices*h){
    if(!valid(s,h)||!c||!c->actor)return 1;
    s->controller=c;s->actor=c->actor;s->active=1;
    h->sound(h->owner,s->actor,s->bank,1);
    if(s->exit_condition!=-1)h->attach_condition(h->owner,s,s->exit_condition);
    s->elapsed=0;
    if(s->disable_collision)h->collision(h->owner,s->actor,0);
    void *animation_owner=h->animation_owner(h->owner,s->actor);
    h->animation_begin(h->owner,animation_owner,s->clip,&s->animation);
    return 0;
}
int uc_character_loop_update(UCCharacterLoop*s,const UCCharacterLoopServices*h){
    if(!valid(s,h)||!s->controller)return 1;
    if(s->animation&&((unsigned)h->animation_cycle(h->owner,s->animation)&255u))
        h->sound(h->owner,s->actor,s->bank,1);
    if(!s->condition||h->condition(h->owner,s->condition)!=0){
        if((s->duration!=-1.f&&s->elapsed>s->duration)||
           (s->interrupt_count>0&&((unsigned)h->interrupt(h->owner,s->controller,s->interrupt_count,s->interrupts)&255u))){
            if(h->transition_ready&&!h->transition_ready(h->owner,s,s->next))return 2;
            if(!((unsigned)h->transition(h->owner,s,s->next)&255u))exit_loop(s,h);
            return 0;
        }
        /*1F9010 returns x87 float; addition/store rounds once to source float.*/
        float delta=h->delta(h->owner);s->elapsed=(float)((double)delta+(double)s->elapsed);
    }
    if(s->actor)memcpy(s->world,h->actor_world(h->owner,s->actor),16*sizeof(float));
    return 0;
}
int uc_character_loop_flags(const uint8_t*flags,size_t size,int32_t count,const int32_t*indices){
    if(count<=0)return 1;
    if(!flags||!indices||count>64)return -1;
    for(int32_t i=0;i<count;i++)if(indices[i]<0||(size_t)indices[i]>=size)return -1;
    for(int32_t i=0;i<count;i++)if(!flags[indices[i]])return 0;
    return 1;
}
int uc_character_loop_exit(UCCharacterLoop*s,const UCCharacterLoopServices*h){
    if(!valid(s,h))return 1;
    exit_loop(s,h);return 0;
}
