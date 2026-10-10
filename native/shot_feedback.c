#include "shot_feedback.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int ready(const UCShotFeedback *p) {
    return p && p->sentinel.next && p->sentinel.previous
        && p->spatial_sentinel.next && p->spatial_sentinel.previous
        && p->memory.allocate && p->memory.release && p->count<=0x0CCCCCCCu
        && p->spatial_count<=0x07FFFFFFu;
}
static int arithmetic(void) {
    return (_mm_getcsr()&0xE040u)==0 && fegetround()==FE_TONEAREST;
}
UCFeedbackStatus uc_shot_feedback_init(UCShotFeedback *p, UCFeedbackMemory m) {
    if(!p||!m.allocate||!m.release)return UC_FEEDBACK_ARGUMENT;
    memset(p,0,sizeof(*p));p->sentinel.next=p->sentinel.previous=&p->sentinel;
    p->spatial_sentinel.next=p->spatial_sentinel.previous=&p->spatial_sentinel;
    memset(p->enabled,1,4);p->memory=m;return UC_FEEDBACK_OK;
}
UCFeedbackStatus uc_feedback_enqueue(UCShotFeedback *p,uintptr_t player,
    float amplitude,float duration,uint32_t kind,float clock,uint32_t mode) {
    UCFeedbackNode *n;float start=clock;
    if(!ready(p)||!isfinite(amplitude)||!isfinite(duration)||!isfinite(clock)
       ||!isfinite(p->time_offset))return UC_FEEDBACK_ARGUMENT;
    if(!arithmetic())return UC_FEEDBACK_UNSUPPORTED;
    if(mode==2)start+=p->time_offset;
    if(!isfinite(start))return UC_FEEDBACK_UNSUPPORTED;
    if(amplitude==0||duration==0)return UC_FEEDBACK_OK;
    if(p->count==0x0CCCCCCCu)return UC_FEEDBACK_UNSUPPORTED;
    n=p->memory.allocate(p->memory.context,sizeof(*n));
    if(!n)return UC_FEEDBACK_MEMORY;
    n->next=&p->sentinel;n->previous=p->sentinel.previous;
    n->effect=(UCFeedbackEffect){player,amplitude,duration>0?duration:.0001f,kind,start};
    ++p->count;p->sentinel.previous=n;n->previous->next=n;
    return UC_FEEDBACK_OK;
}
UCFeedbackStatus uc_shot_feedback_enqueue(UCShotFeedback *p,uint32_t item,
    uint8_t alternate,uintptr_t player,float clock,uint32_t mode) {
    if((item!=2&&item!=17)||alternate)return UC_FEEDBACK_UNSUPPORTED;
    return uc_feedback_enqueue(p,player,item==2?.5f:1.f,item==2?.1f:.5f,2,clock,mode);
}
UCFeedbackStatus uc_spatial_feedback_enqueue(UCShotFeedback *p,const float position[3],
    float radius,float amplitude,float duration,uint32_t kind,float clock,uint32_t mode) {
    UCSpatialFeedbackNode *n;float start=clock;
    if(!ready(p)||!position||!isfinite(radius)||!isfinite(amplitude)||!isfinite(duration)
       ||!isfinite(clock)||!isfinite(p->time_offset))return UC_FEEDBACK_ARGUMENT;
    for(unsigned i=0;i<3;++i)if(!isfinite(position[i]))return UC_FEEDBACK_ARGUMENT;
    if(!arithmetic())return UC_FEEDBACK_UNSUPPORTED;
    if(mode==2)start+=p->time_offset;
    if(!isfinite(start))return UC_FEEDBACK_UNSUPPORTED;
    if(radius==0||amplitude==0||duration==0)return UC_FEEDBACK_OK;
    if(p->spatial_count==0x07FFFFFFu)return UC_FEEDBACK_UNSUPPORTED;
    n=p->memory.allocate(p->memory.context,sizeof(*n));
    if(!n)return UC_FEEDBACK_MEMORY;
    n->next=&p->spatial_sentinel;n->previous=p->spatial_sentinel.previous;
    memcpy(n->effect.position,position,12);n->effect.radius=radius>0?radius:.0001f;
    n->effect.amplitude=amplitude;n->effect.duration=duration>0?duration:.0001f;
    n->effect.kind=kind;n->effect.start=start;
    ++p->spatial_count;p->spatial_sentinel.previous=n;n->previous->next=n;
    return UC_FEEDBACK_OK;
}
UCFeedbackStatus uc_shot_feedback_advance(UCShotFeedback *p,float clock,
    float delta,uint32_t mode,int (*admit)(void *),void *context) {
    UCFeedbackNode *n,*next;UCSpatialFeedbackNode *s,*after;float offset=0,now=clock;
    if(!ready(p)||!admit||!isfinite(clock)||!isfinite(delta)
       ||!isfinite(p->time_offset))return UC_FEEDBACK_ARGUMENT;
    if(!arithmetic())return UC_FEEDBACK_UNSUPPORTED;
    if(mode==2){offset=delta+p->time_offset;now=offset+clock;}
    if(!isfinite(offset)||!isfinite(now))return UC_FEEDBACK_UNSUPPORTED;
    /* Admission also certifies a live list built exclusively by these APIs. */
    if(!admit(context))return UC_FEEDBACK_UNSUPPORTED;
    for(unsigned i=0;i<4;++i)p->output[i]=0;
    p->time_offset=offset;
    for(s=p->spatial_sentinel.next;s!=&p->spatial_sentinel;s=after) {
        after=s->next;
        if(now>s->effect.start+s->effect.duration) {
            s->previous->next=after;after->previous=s->previous;
            p->memory.release(p->memory.context,s);--p->spatial_count;
        }
    }
    for(n=p->sentinel.next;n!=&p->sentinel;n=next) {
        next=n->next;
        if(now>n->effect.start+n->effect.duration) {
            n->previous->next=next;next->previous=n->previous;
            p->memory.release(p->memory.context,n);--p->count;
        }
    }
    p->suppress_once=0;return UC_FEEDBACK_OK;
}
UCFeedbackStatus uc_shot_feedback_clear(UCShotFeedback *p) {
    UCFeedbackNode *n,*next;UCSpatialFeedbackNode *s,*after;
    if(!ready(p))return UC_FEEDBACK_ARGUMENT;
    s=p->spatial_sentinel.next;
    p->spatial_sentinel.next=p->spatial_sentinel.previous=&p->spatial_sentinel;p->spatial_count=0;
    while(s!=&p->spatial_sentinel){after=s->next;p->memory.release(p->memory.context,s);s=after;}
    n=p->sentinel.next;p->sentinel.next=p->sentinel.previous=&p->sentinel;p->count=0;
    while(n!=&p->sentinel){next=n->next;p->memory.release(p->memory.context,n);n=next;}
    return UC_FEEDBACK_OK;
}
