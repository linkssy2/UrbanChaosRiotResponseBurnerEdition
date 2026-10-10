#include "audio_timed_event.h"
#include <string.h>
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t dword(const uint8_t *p){return (uint32_t)word(p)|(uint32_t)word(p+2)<<16;}
static int valid(const UCAudioTimedCommand *c){
    const UCAudioCommand *v=&c->command;
    return v->opcode>=1&&v->opcode<=3&&v->bytes==(v->opcode==3?16:v->opcode==2?28:24)&&
        (v->opcode==3||v->repeat==1||(v->opcode==1&&!v->repeat))&&v->reference!=65535&&
        (v->flags==0||v->flags==0x20||v->flags==0xa0||v->flags==4||v->flags==0x24||v->flags==0x64||v->flags==0xa4||v->flags==0x84)&&
        (v->opcode==2||!c->random_delay);
}
UCAudioStatus uc_audio_timed_command_read(const void *data,size_t size,UCAudioTimedCommand *out){
    const uint8_t *p=(const uint8_t*)data;UCAudioTimedCommand c;uint8_t direct[24];
    if(!p||!out)return UC_AUDIO_ARGUMENT;
    if(size<12)return UC_AUDIO_FORMAT;
    if(p[4]<1||p[4]>3||p[5]!=(p[4]==3?4:p[4]==2?16:12))return UC_AUDIO_UNSUPPORTED;
    if(size<(size_t)p[5]+12)return UC_AUDIO_FORMAT;
    uint16_t flags=word(p+6);uint8_t opcode=p[4];
    if(word(p+14)!=((flags&4)?0:65535))return UC_AUDIO_UNSUPPORTED;
    memcpy(direct,p,opcode==3?16:24);direct[4]=opcode==2?1:opcode;direct[5]=opcode==3?4:12;
    direct[6]=0x20;direct[7]=0;direct[8]=1;direct[9]=0;direct[14]=direct[15]=255;
    memset(&c,0,sizeof c);UCAudioStatus status=uc_audio_command_read(direct,opcode==3?16:24,&c.command);
    if(status)return status;
    c.command.flags=flags;c.command.opcode=opcode;c.command.bytes=(uint8_t)(p[5]+12);c.command.repeat=word(p+8);
    if(opcode==2)c.random_delay=dword(p+24);
    if(!valid(&c))return UC_AUDIO_UNSUPPORTED;*out=c;return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_timed_schedule(UCAudioTrackState *s,const UCAudioTrackCommands *v,
    uint64_t now,uint32_t horizon,UCAudioRandom *random,UCAudioTimedEnqueue enqueue,void *context){
    UCAudioTimedCommand c;size_t at=0,resume=0;UCAudioStatus status;
    if(!s||!v||!random||!enqueue||(!v->data&&v->bytes))return UC_AUDIO_ARGUMENT;
    if(v->count>255||v->bytes>65535||(s->flags&0x0200))return UC_AUDIO_UNSUPPORTED;
    if(s->resume_index>v->count)return UC_AUDIO_FORMAT;
    for(uint16_t i=0;i<v->count;i++){
        if(i==s->resume_index)resume=at;
        status=uc_audio_timed_command_read(v->data?v->data+at:NULL,v->bytes-at,&c);
        if(status)return status;at+=c.command.bytes;
    }
    if(s->resume_index==v->count)resume=at;
    if(at!=v->bytes||resume!=s->resume_offset)return UC_AUDIO_FORMAT;
    s->time=now;s->index=s->resume_index;s->offset=s->resume_offset;s->flags&=0xfffb;
    while(s->index<v->count){
        uc_audio_timed_command_read(v->data+s->offset,v->bytes-s->offset,&c);
        if(c.command.time>=s->origin&&c.command.time>=horizon)break;
        if(c.command.time<s->origin){++s->index;s->offset=(uint16_t)(s->offset+c.command.bytes);continue;}
        uint64_t when=now+(uint32_t)(c.command.time-s->origin);
        if(c.command.opcode==2&&c.random_delay)when+=uc_audio_random(random)%c.random_delay;
        if(c.command.opcode==3)s->repeat=c.command.repeat;
        ++s->index;s->offset=(uint16_t)(s->offset+c.command.bytes);
        enqueue(context,s,&c,when);
    }
    return UC_AUDIO_OK;
}
typedef struct Bridge {const UCAudioEventServices *hooks;const UCAudioCommand *command;}Bridge;
static int admit(void *p,const UCAudioTrackState *s,const UCAudioCommand *c){Bridge *b=p;(void)c;return b->hooks->preflight(b->hooks->user,s,b->command);}
static int voice(void *p){Bridge *b=p;return b->hooks->has_voice(b->hooks->user);}
static void pitch(void *p,UCAudioTrackState *s){Bridge *b=p;b->hooks->pitch_changed(b->hooks->user,s);}
static void volume(void *p,UCAudioTrackState *s){Bridge *b=p;b->hooks->volume_changed(b->hooks->user,s);}
static void create(void *p,UCAudioTrackState *s,const UCAudioCommand *c){Bridge *b=p;(void)c;b->hooks->create_voice(b->hooks->user,s,b->command);}
static void start(void *p){Bridge *b=p;b->hooks->start_voice(b->hooks->user);}
UCAudioStatus uc_audio_timed_execute(UCAudioTrackState *s,const UCAudioTimedCommand *c,
    UCAudioRandom *random,const UCAudioEventServices *h){
    if(!s||!c||!random||!h||!h->preflight||!h->has_voice||!h->pitch_changed||!h->volume_changed||!h->create_voice||!h->start_voice)return UC_AUDIO_ARGUMENT;
    if(!valid(c))return UC_AUDIO_UNSUPPORTED;
    UCAudioCommand projected=c->command;projected.flags=0x20;projected.repeat=1;
    if(projected.opcode==2){projected.opcode=1;projected.bytes=24;}
    Bridge b={h,&c->command};UCAudioEventServices service={&b,admit,voice,pitch,volume,create,start};
    return uc_audio_event_execute(s,&projected,random,&service);
}
