#include "fade.h"
#include <math.h>
static int valid(const UCFade *f,const UCFadeServices *s){
    return f&&s&&s->registry&&s->push_white&&s->pop_white&&f->mode<=2&&f->white<=1&&f->held<=1&&
        isfinite(f->alpha)&&f->alpha>=0&&f->alpha<=1.1f&&isfinite(f->rate)&&f->rate>=0&&
        (!f->completion||uc_entity_event_lookup(s->registry,f->completion_owner));
}
int uc_fade_request(UCFade *f,const UCFadeServices *s,uint32_t mode,float seconds,
    uint8_t *completion,int32_t owner,uint32_t white){
    if(!valid(f,s)||mode>2||white>1||!isfinite(seconds)||seconds<0||
       (seconds!=0&&!isfinite(1.f/seconds))||
       (completion&&!uc_entity_event_lookup(s->registry,owner)))return 1;
    if(f->completion){*f->completion=1;f->completion=0;f->completion_owner=-1;}
    f->mode=mode;f->white=white;
    const float target=mode==2?0.f:1.f;
    if((f->alpha>1.f?1.f:f->alpha)==target){if(completion)*completion=1;return 0;}
    if(seconds==0){f->alpha=target;if(completion)*completion=1;return 0;}
    f->completion=completion;f->completion_owner=owner;
    if(completion)*completion=0;
    f->rate=1.f/seconds;
    if(mode==2&&f->alpha>=1.f)f->alpha=1.1f;
    if(white==1&&!f->held){if(s->push_white(s->owner))return 2;f->held=1;}
    return 0;
}
int uc_fade_step(UCFade *f,const UCFadeServices *s,float seconds){
    if(!valid(f,s)||!isfinite(seconds)||seconds<0||!isfinite(f->rate*seconds)||
       !isfinite(f->alpha+f->rate*seconds))return 1;
    const float delta=f->rate*seconds,limit=f->mode==2?0.f:f->mode==1?.7f:1.f;
    f->alpha=f->mode==2?f->alpha-delta:delta+f->alpha;
    if(f->mode==2?f->alpha<limit:f->alpha>limit){
        f->alpha=limit;
        if(f->held){if(s->pop_white(s->owner))return 2;f->held=0;}
        if(f->completion){*f->completion=1;f->completion=0;}
    }
    return 0;
}
int uc_fade_close(UCFade *f,const UCFadeServices *s){
    if(!valid(f,s))return 1;
    if(f->completion)*f->completion=1;
    if(f->held){if(s->pop_white(s->owner))return 2;f->held=0;}
    return 0;
}
