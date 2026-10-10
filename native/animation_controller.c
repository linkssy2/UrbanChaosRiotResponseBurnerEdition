#include "animation_controller.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int valid(const UCAnimationController *s) {
    return s && isfinite(s->phase) && s->phase>=0 && s->phase<1 &&
        isfinite(s->previous) && !isnan(s->weight) && isfinite(s->rate) && s->rate>=0 &&
        isfinite(s->phase_scale) && s->phase_scale>=0 && isfinite(s->blend_in) && isfinite(s->blend_out);
}
UCAnimationStatus uc_animation_controller_init(const UCAnimationClip *clip,uint32_t flags,UCAnimationController *out) {
    if(!clip||!out)return UC_ANIMATION_ARGUMENT;
    UCAnimationController s={0};s.rate=1;s.phase_scale=clip->source_timing[0];
    s.blend_in=clip->source_timing[1];s.blend_out=clip->source_timing[2];
    s.flags=flags;s.source_control=clip->source_control;
    if(!valid(&s))return UC_ANIMATION_ARGUMENT;
    *out=s;return UC_ANIMATION_OK;
}
static void interval(UCAnimationAdvance *a,float begin,float end,uint32_t repeats) {
    if(repeats)a->intervals[a->count++]=(UCAnimationInterval){begin,end,repeats};
}
UCAnimationStatus uc_animation_controller_advance(UCAnimationController *state,float delta,UCAnimationAdvance *out) {
    if(!out||!valid(state)||!isfinite(delta)||delta<0)return UC_ANIMATION_ARGUMENT;
    UCAnimationController s=*state;UCAnimationAdvance a={0};
    /* Original x87 multiplies retain intermediates until these float stores. */
    float step=(float)((double)s.phase_scale*(double)s.rate*(double)delta);
    float in=(float)((double)s.phase_scale*s.blend_in),fade=(float)((double)s.phase_scale*s.blend_out);
    if(!isfinite(step)||!isfinite(in)||!isfinite(fade))return UC_ANIMATION_LIMIT;
    if(!(s.flags&0x80000000u)){a.original_result=!(s.flags&8);*out=a;return UC_ANIMATION_OK;}
    float near_end=s.phase+fade;near_end=near_end-(float)s.repeats;near_end=near_end-1.0f;
    float past_end=0;s.previous=s.phase;
    if(!(s.flags&0x20)||s.weight==1.0f)s.phase=s.phase+step;
    if(!isfinite(s.phase)||s.phase>=2147483648.0f||!isfinite(near_end))return UC_ANIMATION_LIMIT;
    if(s.phase>=1.0f){
        interval(&a,s.previous,0x1.00068ep0f,1); /* 3F800347, original inclusive-one sentinel. */
        int32_t cycles=(int32_t)s.phase;s.phase=s.phase-(float)cycles;
        if(s.repeats>=0){
            s.repeats-=cycles;past_end=s.phase+fade;past_end=past_end-(float)s.repeats;past_end=past_end-1.0f;
            if(s.repeats<0){
                s.phase=0x1.fff2e4p-1f; /* 3F7FF972, original exhausted phase. */
                cycles+=s.repeats;s.repeats=0;
                if(fade<0){s.flags&=0xbfffffffu;past_end=0;}else s.flags&=0x3fffffffu;
            }
            if(cycles>1)interval(&a,0,0x1.00068ep0f,(uint32_t)(cycles-1));
            if(s.repeats>0)interval(&a,0,s.phase,1);
        }else{
            if(cycles>1)interval(&a,0,0x1.00068ep0f,(uint32_t)(cycles-1));
            interval(&a,0,s.phase,1);if(s.repeats < -1)++s.repeats;
        }
    }else{
        interval(&a,s.previous,s.phase,1);
        if(s.repeats>=0)past_end=near_end+step;
    }
    if(!isfinite(past_end))return UC_ANIMATION_LIMIT;
    if(in>=0){
        if(past_end>0){
            float ratio=past_end/fade;s.weight=1.0f-ratio;
        }else if(in==0)s.weight=1;
        else {float gain=step/in;s.weight=s.weight+gain;if(s.weight>1)s.weight=1;}
        if(isnan(s.weight))return UC_ANIMATION_LIMIT;
        if(s.weight<=0)a.original_result=!(s.flags&8);
        else if(s.weight>=1 && !(s.flags&2))a.original_result=2;
    }
    *state=s;*out=a;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_collect_events(const UCAnimationAnnotations *owner,
    const UCAnimationAdvance *advance,UCAnimationEventQueue *out) {
    if(!owner||!advance||!out||(owner->event_count&&!owner->events)||advance->count>3||out->count>16)return UC_ANIMATION_ARGUMENT;
    uint32_t count=owner->event_count;
    if(count>65536)return UC_ANIMATION_LIMIT;
    for(uint32_t i=0;i<count;++i)if(!isfinite(owner->events[i].phase))return UC_ANIMATION_ARGUMENT;
    for(uint32_t i=0;i<advance->count;++i)
        if(!isfinite(advance->intervals[i].begin)||!isfinite(advance->intervals[i].end))return UC_ANIMATION_ARGUMENT;
    UCAnimationEventQueue q=*out;
    for(uint32_t i=0;i<advance->count&&q.count<16;++i){
        UCAnimationInterval span=advance->intervals[i];
        for(uint32_t cycle=0;cycle<span.repetitions&&q.count<16;++cycle){
            uint32_t before=q.count;
            for(uint32_t j=0;j<count&&q.count<16;++j)
                if(owner->events[j].phase>=span.begin&&owner->events[j].phase<span.end)
                    q.events[q.count++]=(UCAnimationEventRef){owner,j};
            if(q.count==before)break;
        }
    }
    *out=q;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_controller_list_insert_index(const UCAnimationControllerEntry *entries,
    uint32_t count,const UCAnimationController *next,uint32_t *out) {
    if(!next||!out||(count&&!entries))return UC_ANIMATION_ARGUMENT;
    if(count>=32)return UC_ANIMATION_LIMIT;
    uint32_t index=count;
    for(uint32_t i=0;i<count;i++) {
        const UCAnimationController *s=entries[i].controller;
        if(!s||s==next)return UC_ANIMATION_ARGUMENT;
        for(uint32_t j=0;j<i;j++)if(entries[j].controller==s)return UC_ANIMATION_ARGUMENT;
        if(i&&(int32_t)entries[i-1].controller->source_control<(int32_t)s->source_control)return UC_ANIMATION_ARGUMENT;
        if(index==count&&(int32_t)s->source_control<=(int32_t)next->source_control)index=i;
    }
    *out=index;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_controller_list_advance(const UCAnimationControllerEntry *entries,
    uint32_t count,const UCAnimationTemplate *templ,float delta,UCAnimationControllerListUpdate *out) {
    if(!out||!templ||!templ->bone_count||!templ->bone_flags||(count&&!entries)||
        !isfinite(delta)||delta<0)return UC_ANIMATION_ARGUMENT;
    if(count>32||templ->bone_count>4096)return UC_ANIMATION_LIMIT;
    UCAnimationController states[32];uint32_t masks[32][128]={{0}},enabled[128]={0};
    uint8_t removed[32]={0};UCAnimationControllerListUpdate result={0};
    uint32_t words=(templ->bone_count+31)/32;
    for(uint32_t i=0;i<templ->bone_count;i++)if(templ->bone_flags[i])enabled[i/32]|=1u<<(i%32);
    for(uint32_t i=0;i<count;i++) {
        const UCAnimationControllerEntry *e=entries+i;const UCAnimationClip *c=e->clip;
        if(!valid(e->controller)||!c||!c->source_tracks||!c->track_count||!e->annotations)return UC_ANIMATION_ARGUMENT;
        if(c->track_count>4096)return UC_ANIMATION_LIMIT;
        for(uint32_t j=0;j<i;j++)if(entries[j].controller==e->controller)return UC_ANIMATION_ARGUMENT;
        if(i&&(int32_t)entries[i-1].controller->source_control<(int32_t)e->controller->source_control)return UC_ANIMATION_ARGUMENT;
        uint32_t last=(uint32_t)c->source_tracks[c->track_count-1];if((int32_t)last<0)last=~last;
        if(last>=templ->bone_count)return UC_ANIMATION_FORMAT;
        for(uint32_t j=0;j<c->track_count;j++) {
            uint32_t bone=(uint32_t)c->source_tracks[j];if((int32_t)bone<0)bone=~bone;
            if(bone>last)return UC_ANIMATION_UNSUPPORTED;
            masks[i][bone/32]|=1u<<(bone%32);
        }
        states[i]=*e->controller;
    }
    for(uint32_t i=0;i<count;i++) {
        if(removed[i])continue;
        UCAnimationAdvance *advance=result.advances+i;
        UCAnimationStatus status=uc_animation_controller_advance(states+i,delta,advance);
        if(status)return status;
        result.advanced[result.advanced_count++]=i;
        status=uc_animation_collect_events(entries[i].annotations,advance,&result.events);
        if(status)return status;
        if(advance->original_result==1) {
            removed[i]=1;result.removed[result.removed_count++]=i;states[i].flags&=0x3fffffffu;
        } else if(advance->original_result==2) {
            for(uint32_t j=i+1;j<count;j++)if(!removed[j]&&!(states[j].flags&1)) {
                uint32_t word=0;
                while(word<words&&!(masks[j][word]&enabled[word]&~masks[i][word]))++word;
                if(word==words){removed[j]=1;result.removed[result.removed_count++]=j;states[j].flags&=0x3fffffffu;}
            }
        }
    }
    for(uint32_t i=0;i<count;i++) {
        *entries[i].controller=states[i];
        if(!removed[i])result.survivors[result.survivor_count++]=i;
    }
    *out=result;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_tag_state(const UCAnimationControllerTag *tags,uint32_t count,
    uint16_t id,float phase,int *out) {
    if(!out||(count&&!tags)||!isfinite(phase))return UC_ANIMATION_ARGUMENT;
    if(count>65536)return UC_ANIMATION_LIMIT;
    for(uint32_t i=0;i<count;++i)if(!isfinite(tags[i].phase))return UC_ANIMATION_ARGUMENT;
    int state=0;
    for(uint32_t i=0;i<count&&phase>=tags[i].phase;++i)if(tags[i].id==id)state=tags[i].state!=0;
    *out=state;return UC_ANIMATION_OK;
}
typedef struct AnnotationReader { const uint8_t *data;size_t size,at; } AnnotationReader;
static const uint8_t *take(AnnotationReader *r,size_t n){
    if(n>r->size-r->at)return NULL;const uint8_t *p=r->data+r->at;r->at+=n;return p;
}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static const char *name(AnnotationReader *r){
    const uint8_t *p=r->data+r->at,*end=(const uint8_t*)memchr(p,0,r->size-r->at);
    if(!end)return NULL;r->at=(size_t)(end-r->data)+1;return (const char*)p;
}
void uc_animation_annotations_free(UCAnimationAnnotations *a){
    if(a){free(a->storage);free(a->events);free(a->parameters);free(a->tags);memset(a,0,sizeof(*a));}
}
UCAnimationStatus uc_animation_annotations_load(const UCAnimationClip *clip,UCAnimationAnnotations *out){
    if(!clip||!out||!clip->annotations)return UC_ANIMATION_ARGUMENT;
    if(clip->annotation_bytes>64u*1024u*1024u)return UC_ANIMATION_LIMIT;
    UCAnimationAnnotations a={0};UCAnimationStatus error=UC_ANIMATION_FORMAT;const uint8_t *p;
    a.storage=(uint8_t*)malloc(clip->annotation_bytes?clip->annotation_bytes:1);
    if(!a.storage)return UC_ANIMATION_MEMORY;
    memcpy(a.storage,clip->annotations,clip->annotation_bytes);
    AnnotationReader r={a.storage,clip->annotation_bytes,0};uint32_t used=0;
    p=take(&r,4);if(!p)goto fail;a.event_count=u32(p);
    if(a.event_count>65536){error=UC_ANIMATION_LIMIT;goto fail;}
    if(a.event_count){
        p=take(&r,4);if(!p)goto fail;a.parameter_count=u32(p);
        if(a.parameter_count>65536){error=UC_ANIMATION_LIMIT;goto fail;}
        a.events=(UCAnimationEvent*)calloc(a.event_count,sizeof(*a.events));
        a.parameters=(UCAnimationEventParameter*)calloc(a.parameter_count?a.parameter_count:1,sizeof(*a.parameters));
        if(!a.events||!a.parameters){error=UC_ANIMATION_MEMORY;goto fail;}
        for(uint32_t i=0;i<a.event_count;++i){
            UCAnimationEvent *e=a.events+i;p=take(&r,12);if(!p)goto fail;
            memcpy(&e->phase,p,4);e->code=u32(p+4);e->parameter_count=u32(p+8);
            if(!isfinite(e->phase)||e->parameter_count>a.parameter_count-used)goto fail;
            e->parameters=a.parameters+used;
            for(uint32_t j=0;j<e->parameter_count;++j){
                p=take(&r,8);if(!p)goto fail;a.parameters[used].words[0]=u32(p);a.parameters[used++].words[1]=u32(p+4);
            }
        }
        if(used!=a.parameter_count)goto fail;p=take(&r,4);if(!p)goto fail;
        uint32_t bytes=u32(p);p=take(&r,bytes);if(!p)goto fail;AnnotationReader strings={p,bytes,0};used=0;
        for(uint32_t i=0;i<a.event_count;++i){
            UCAnimationEvent *e=a.events+i;e->name=name(&strings);e->type=name(&strings);
            if(!e->name||!e->type)goto fail;
            for(uint32_t j=0;j<e->parameter_count;++j){
                UCAnimationEventParameter *v=a.parameters+used++;v->name=name(&strings);v->value=name(&strings);
                if(!v->name||!v->value)goto fail;
            }
        }
        if(strings.at!=strings.size)goto fail;
    }
    p=take(&r,4);if(!p)goto fail;a.tag_count=u32(p);
    if(a.tag_count>65536){error=UC_ANIMATION_LIMIT;goto fail;}
    if(a.tag_count){
        a.tags=(UCAnimationControllerTag*)calloc(a.tag_count,sizeof(*a.tags));
        if(!a.tags){error=UC_ANIMATION_MEMORY;goto fail;}
        for(uint32_t i=0;i<a.tag_count;++i){
            p=take(&r,8);if(!p)goto fail;a.tags[i].id=(uint16_t)(p[0]|(uint16_t)p[1]<<8);
            a.tags[i].state=(uint16_t)(p[2]|(uint16_t)p[3]<<8);memcpy(&a.tags[i].phase,p+4,4);
            if(!isfinite(a.tags[i].phase))goto fail;
        }
    }
    if(r.at!=r.size||a.event_count!=clip->event_count||a.parameter_count!=clip->parameter_count||a.tag_count!=clip->tag_count)goto fail;
    *out=a;return UC_ANIMATION_OK;
fail:uc_animation_annotations_free(&a);return error;
}
