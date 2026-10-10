#include "audio_command_submit.h"
#include <string.h>
static int valid(const UCAudioCommandSubmit*h){return h&&h->current_time&&*h->current_time<=INT64_MAX&&h->random&&h->heap&&h->heap->items&&h->heap->capacity&&h->heap->capacity<=65535&&h->heap->count<=h->heap->capacity&&h->preflight&&h->allocate&&h->release&&h->events;}
static void discard(UCAudioQueuedCommand*p,const UCAudioCommandSubmit*h){--p->pending.references;uc_audio_link_remove(&p->pending.membership);h->release(h->context,p);}
static int execute(UCAudioQueuedCommand*p,const UCAudioCommandSubmit*h){
    const UCAudioEventServices*effects=h->events(h->context,p->pending.track);
    if(uc_audio_timed_execute(&p->pending.track->state,&p->decoded,h->random,effects))return 2;
    discard(p,h);return 0;
}
static int drain(uint64_t limit,const UCAudioCommandSubmit*h){
    while(h->heap->count&&h->heap->items[0]->time<=limit){
        UCAudioQueuedCommand*p=(UCAudioQueuedCommand*)h->heap->items[0];uint8_t removed;
        if(uc_audio_commands_remove(h->heap,&p->pending,0,&removed)||!removed)return 2;
        if(execute(p,h))return 2;
    }return 0;
}
int uc_audio_command_dispatch_due(uint64_t limit,const UCAudioCommandSubmit*h){
    if(!valid(h)||limit>INT64_MAX||!h->preflight(h->context,NULL,NULL))return 1;
    return drain(limit,h);
}
int uc_audio_command_schedule(UCAudioInstanceTrack*t,const UCAudioTrackCommands*v,uint64_t now,uint32_t horizon,const UCAudioCommandSubmit*h){
    if(!t||!v||!valid(h)||now>INT64_MAX-UINT32_MAX-65535||(!v->data&&v->bytes)||v->count>255||v->bytes>65535||(t->state.flags&0x2200)||t->state.resume_index>v->count)return 1;
    size_t at=0,resume=0;UCAudioTimedCommand c;
    for(unsigned i=0;i<v->count;i++){if(i==t->state.resume_index)resume=at;if(uc_audio_timed_command_read(v->data?v->data+at:NULL,v->bytes-at,&c))return 1;at+=c.command.bytes;}
    if(t->state.resume_index==v->count)resume=at;
    if(at!=v->bytes||resume!=t->state.resume_offset||!h->preflight(h->context,t,v))return 1;
    UCAudioTrackState*s=&t->state;s->time=now;s->index=s->resume_index;s->offset=s->resume_offset;s->flags&=0xfffb;
    while(s->index<v->count){
        uc_audio_timed_command_read(v->data+s->offset,v->bytes-s->offset,&c);
        if(c.command.time>=s->origin&&c.command.time>=horizon)break;
        if(c.command.time<s->origin){++s->index;s->offset=(uint16_t)(s->offset+c.command.bytes);continue;}
        UCAudioQueuedCommand*p=h->allocate(h->context);if(!p)return 2;
        memset(p,0,sizeof *p);p->pending.references=1;p->pending.membership.next=p->pending.membership.previous=&p->pending.membership;p->pending.track=t;p->source=v->data+s->offset;p->decoded=c;
        uint64_t when=now+(uint32_t)(c.command.time-s->origin);
        if(c.command.opcode==2&&c.random_delay)when+=uc_audio_random(h->random)%c.random_delay;
        if(c.command.opcode==3)s->repeat=c.command.repeat;
        ++s->index;s->offset=(uint16_t)(s->offset+c.command.bytes);
        uc_audio_link_append(&t->commands,&p->pending.membership);p->pending.time=when;
        uint64_t current=*h->current_time;
        if(when<=current){if(drain(current,h)||execute(p,h))return 2;}
        else{uint8_t added;if(uc_audio_commands_push(h->heap,&p->pending,0,&added))return 2;if(!added)discard(p,h);}
    }return 0;
}
