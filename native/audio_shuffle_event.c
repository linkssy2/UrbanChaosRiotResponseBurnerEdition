#include "audio_shuffle_event.h"
#include <string.h>
UCAudioStatus uc_audio_shuffle_command_read(const void *data,size_t size,UCAudioCommand *out){
    if(!data||!out)return UC_AUDIO_ARGUMENT;
    if(size<24)return UC_AUDIO_FORMAT;
    const uint8_t *p=(const uint8_t*)data;
    if(p[4]!=1||p[5]!=12||p[6]!=0xa4||p[7]||p[14]||p[15])return UC_AUDIO_UNSUPPORTED;
    uint8_t direct[24];memcpy(direct,p,sizeof direct);direct[6]=0x20;direct[14]=direct[15]=255;
    UCAudioCommand command;UCAudioStatus status=uc_audio_command_read(direct,sizeof direct,&command);
    if(status)return status;command.flags=0xa4;*out=command;return UC_AUDIO_OK;
}
typedef struct Bridge {const UCAudioEventServices *h;const UCAudioCommand *command;} Bridge;
static int admit(void *p,const UCAudioTrackState *s,const UCAudioCommand *c){
    Bridge *b=(Bridge*)p;(void)c;return b->h->preflight(b->h->user,s,b->command);
}
static int voice(void *p){Bridge *b=(Bridge*)p;return b->h->has_voice(b->h->user);}
static void pitch(void *p,UCAudioTrackState *s){Bridge *b=(Bridge*)p;b->h->pitch_changed(b->h->user,s);}
static void volume(void *p,UCAudioTrackState *s){Bridge *b=(Bridge*)p;b->h->volume_changed(b->h->user,s);}
static void create(void *p,UCAudioTrackState *s,const UCAudioCommand *c){Bridge *b=(Bridge*)p;(void)c;b->h->create_voice(b->h->user,s,b->command);}
static void start(void *p){Bridge *b=(Bridge*)p;b->h->start_voice(b->h->user);}
UCAudioStatus uc_audio_shuffle_event_execute(UCAudioTrackState *s,const UCAudioCommand *c,
    UCAudioRandom *random,const UCAudioEventServices *h){
    if(!s||!c||!random||!h||!h->preflight||!h->has_voice||!h->pitch_changed||!h->volume_changed||!h->create_voice||!h->start_voice)
        return UC_AUDIO_ARGUMENT;
    if(c->opcode!=1||c->bytes!=24||c->flags!=0xa4||c->repeat!=1||c->reference==65535)return UC_AUDIO_UNSUPPORTED;
    UCAudioCommand direct=*c;direct.flags=0x20;Bridge b={h,c};
    UCAudioEventServices projected={&b,admit,voice,pitch,volume,create,start};
    return uc_audio_event_execute(s,&direct,random,&projected);
}
