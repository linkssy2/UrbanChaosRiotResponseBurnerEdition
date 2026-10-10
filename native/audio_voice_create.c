#include "audio_voice_create.h"
int uc_audio_buffer_begin(UCAudioBuffer *b){
    if(!b)return 1;
    for(unsigned i=0;i<5;i++)b->links[i].next=b->links[i].previous=&b->links[i];
    b->references=1;b->owner34=b->owner38=b->device=b->spatial=NULL;
    b->flags=b->word48=b->word50=0;b->channels=0;b->priority=65535;return 0;
}
static int ring(const UCAudioLink *p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
int uc_audio_voice_create(UCAudioInstance *s,UCAudioInstanceTrack *t,const UCAudioVoiceCreate *h){
    if(!s||!t||!h||!h->remaining_devices||!h->preflight||!h->allocate||
       !h->create_device||!h->steal_device||!h->initialize_device)return 1;
    UCAudioBuffer *parent=(UCAudioBuffer*)s->buffer;
    if(t->voice||s->aux40||(parent&&!ring(&parent->links[1])))return 1;
    if(!h->preflight(h->context,s,t))return 1;
    uint32_t flags=9;uint16_t priority=parent?parent->priority:0;
    if(parent&&(parent->flags&2))flags|=4;
    if(s->flags&0x40)flags|=0x10;
    if(s->sound)priority=s->sound[8];
    UCAudioBuffer *voice=h->allocate(h->context);
    if(voice){
        uc_audio_buffer_begin(voice);voice->flags=flags;voice->priority=priority;voice->channels=1;
        uint32_t device_flags=2|(flags&4?0x100u:0)|(flags&0x10?0x200u:0);
        if(*h->remaining_devices){
            voice->device=h->create_device(h->context,device_flags);
            if(voice->device)--*h->remaining_devices;
        }
        if(!voice->device&&voice->priority!=65535)
            voice->device=h->steal_device(h->context,0,voice->priority);
        if(voice->device)h->initialize_device(h->context,voice);
    }
    t->voice=voice;
    if(voice&&parent){++voice->references;uc_audio_link_append(&parent->links[1],&voice->links[0]);}
    return 0;
}
