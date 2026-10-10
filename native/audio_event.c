#include "audio_event.h"
#include <string.h>
static uint16_t u16(const uint8_t *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const uint8_t *p){return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
static int16_t s16(uint32_t n){return (int16_t)(n<32768?n:(int32_t)(n&65535)-65536);}
uint16_t uc_audio_random(UCAudioRandom *r){r->seed=r->seed*0x15a4e35u+1;return (uint16_t)(r->seed>>16);}
int16_t uc_audio_random_range(UCAudioRandom *r,int16_t a,int16_t b){
    return a==b?a:s16((uint32_t)((int32_t)uc_audio_random(r)%((int32_t)b-a)+a)&65535);
}
static int command_ok(const UCAudioCommand *c){
    return (c->opcode==1||c->opcode==3)&&c->bytes==(c->opcode==1?24:16)&&
        (!(c->flags&~0x20)||(c->opcode==3&&c->flags==0x0104))&&
        (c->repeat==1||(c->opcode==3&&!(c->flags&~0x20)&&c->repeat==0))&&c->reference!=65535;
}
UCAudioStatus uc_audio_command_read(const void *data,size_t n,UCAudioCommand *out){
    const uint8_t *p=(const uint8_t *)data;UCAudioCommand c;
    if(!p||!out)return UC_AUDIO_ARGUMENT;
    if(n<12)return UC_AUDIO_FORMAT;
    if((p[4]!=1&&p[4]!=3)||p[5]!=(p[4]==1?12:4))return UC_AUDIO_UNSUPPORTED;
    if(n<(size_t)12+p[5])return UC_AUDIO_FORMAT;
    memset(&c,0,sizeof(c));c.time=u32(p);c.opcode=p[4];c.bytes=(uint8_t)(12+p[5]);
    c.flags=u16(p+6);c.repeat=u16(p+8);c.reference=u16(p+12);
    if(!command_ok(&c)||u16(p+14)!=(c.flags==0x0104?0:65535))return UC_AUDIO_UNSUPPORTED;
    if(c.opcode==1){c.pitch[0]=s16(u16(p+16));c.pitch[1]=s16(u16(p+18));
        c.volume[0]=s16(u16(p+20));c.volume[1]=s16(u16(p+22));}
    *out=c;return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_track_schedule(UCAudioTrackState *s,const UCAudioTrackCommands *v,
    uint64_t now,uint32_t horizon,UCAudioEnqueue enqueue,void *user){
    size_t at=0,resume=0;uint16_t i;UCAudioCommand c;UCAudioStatus status;
    if(!s||!v||!enqueue||(!v->data&&v->bytes))return UC_AUDIO_ARGUMENT;
    if(v->count>255||v->bytes>65535||(s->flags&0x0200))return UC_AUDIO_UNSUPPORTED;
    if(s->resume_index>v->count)return UC_AUDIO_FORMAT;
    for(i=0;i<v->count;i++){
        if(i==s->resume_index)resume=at;
        status=uc_audio_command_read(v->data? v->data+at:NULL,v->bytes-at,&c);
        if(status)return status;
        at+=c.bytes;
    }
    if(s->resume_index==v->count)resume=at;
    if(at!=v->bytes||resume!=s->resume_offset)return UC_AUDIO_FORMAT;
    s->time=now;s->index=s->resume_index;s->offset=s->resume_offset;s->flags&=0xfffb;
    while(s->index<v->count){
        uc_audio_command_read(v->data+s->offset,v->bytes-s->offset,&c);
        if(c.time>=s->origin&&c.time>=horizon)break;
        ++s->index;s->offset=(uint16_t)(s->offset+c.bytes);
        if(c.time<s->origin)continue;
        if(c.opcode==3)s->repeat=c.repeat;
        enqueue(user,s,&c,now+(uint32_t)(c.time-s->origin));
    }
    return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_event_execute(UCAudioTrackState *s,const UCAudioCommand *c,
    UCAudioRandom *r,const UCAudioEventServices *h){
    int voice;
    if(!s||!c||!r||!h||!h->preflight||!h->has_voice||!h->pitch_changed||
       !h->volume_changed||!h->create_voice||!h->start_voice)return UC_AUDIO_ARGUMENT;
    if(!command_ok(c)||(s->flags&0x2200))return UC_AUDIO_UNSUPPORTED;
    if(!h->preflight(h->user,s,c))return UC_AUDIO_UNSUPPORTED;
    voice=h->has_voice(h->user);
    if(c->opcode==1){s->pitch=uc_audio_random_range(r,c->pitch[0],c->pitch[1]);h->pitch_changed(h->user,s);
        s->volume=uc_audio_random_range(r,c->volume[0],c->volume[1]);h->volume_changed(h->user,s);}
    if(!(s->flags&0x0200)){
        if(!voice){h->create_voice(h->user,s,c);voice=h->has_voice(h->user);}
        /* Completing services preserve the ordinary non-subbank domain. */
        if(voice)h->start_voice(h->user);
    }
    s->flags=(uint16_t)((s->flags&0xefff)|0x24);return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_event_resolve(UCAudioTrackState *s,const UCAudioCommand *c,
    uint32_t count,const void *(*resolve)(void *,uint16_t),void *user){
    if(!s||!c||!resolve)return UC_AUDIO_ARGUMENT;
    if(!command_ok(c)||(c->flags&4)||c->reference>=count)return UC_AUDIO_UNSUPPORTED;
    s->resource=NULL;s->subsound=-1;s->resource=resolve(user,c->reference);return UC_AUDIO_OK;
}
int16_t uc_audio_event_gain(const UCAudioMix *m,const UCAudioTrackState *s){
    int32_t v=m->mute?-10000:-(int32_t)(m->sound_gain&511)*16+m->instance_gain+
        m->fade_gain+m->group_gain+s16(((uint16_t)s->volume+(uint32_t)(uint16_t)m->track_gain)&65535);
    return (int16_t)(v>0?0:v< -10000?-10000:v);
}
int16_t uc_audio_event_pitch(const UCAudioMix *m,const UCAudioTrackState *s){
    int32_t v=m->sound_pitch+s16(((uint16_t)s->pitch+(uint32_t)(uint16_t)m->track_pitch+
        (uint32_t)(uint16_t)m->sample_pitch)&65535);
    if(!m->parent_timing_track)v+=m->instance_pitch+m->group_pitch;
    return (int16_t)(v>8191?8191:v< -32767?-32767:v);
}
