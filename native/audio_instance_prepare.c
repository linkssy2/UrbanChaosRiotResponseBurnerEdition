#include "audio_instance_prepare.h"
#include <string.h>
static uint16_t word(const uint8_t*p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t dword(const uint8_t*p){return (uint32_t)word(p)|(uint32_t)word(p+2)<<16;}
static int ring(const UCAudioLink*p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
static const uint8_t *definition(const UCAudioInstanceBank*b,uint32_t offset,unsigned index){
    size_t at=(size_t)offset+12;if(at>b->track_bytes)return NULL;
    for(unsigned i=0;i<=index;i++){
        if(b->track_bytes-at<8||memcmp(b->track_chunks+at,"ampt",4))return NULL;
        uint32_t bytes=dword(b->track_chunks+at+4);if(bytes<5||bytes>b->track_bytes-at-8)return NULL;
        if(i==index)return b->track_chunks+at+8;at+=bytes+8;
    }return NULL;
}
int uc_audio_instance_prepare(UCAudioInstance*p,const UCAudioInstanceBank*b,
    uint32_t index,uint32_t*flags,const UCAudioInstancePrepareServices*h,uint8_t*initialized){
    const uint8_t *event,*sound;uint16_t si,spatial;unsigned count;void *old;
    if(!p||!initialized)return 1;if(p->state||!b){*initialized=0;return 0;}
    if(!b->identity||!b->metadata||b->metadata->policy.type!=104||!b->metadata->events||!b->metadata->sounds||
       index>=b->metadata->event_count||b->metadata->event_count>65535||b->metadata->sound_count>65535||b->spatial_count>65535||
       !b->track_chunks||b->track_bytes<12||memcmp(b->track_chunks,"LIST",4)||memcmp(b->track_chunks+8,"trkl",4)||
       !h||!h->preflight||!h->retain_bank||!h->release_bank||!h->allocate_track||!h->buffer_auxiliary||
       !h->group_gain||!h->set_attenuation||!h->set_spatial||!ring(h->engine_instances)||
       !ring(&p->links[2])||!ring(&p->links[4])||h->engine_instances==&p->links[2])return 1;
    event=b->metadata->events+(size_t)index*16;si=word(event+2);
    if(si>=b->metadata->sound_count)return 1;sound=b->metadata->sounds+(size_t)si*28;count=sound[6];
    for(unsigned i=0;i<count;i++)if(!definition(b,dword(sound),i))return 1;
    spatial=word(sound+22);if(spatial!=65535&&(!b->spatial||spatial>=b->spatial_count))return 1;
    if(!h->preflight(h->context,p,b,count))return 1;
    old=p->bank;p->bank=b->identity;h->retain_bank(h->context,b->identity);p->sound=sound;
    if(old)h->release_bank(h->context,old);p->event=index;
    if(p->links[2].next==&p->links[2]){uc_audio_instance_retain(p);uc_audio_link_append(h->engine_instances,&p->links[2]);}
    for(unsigned i=0;i<count;i++){
        UCAudioInstanceTrack*t=h->allocate_track(h->context);if(!t)return 2;
        uc_audio_instance_track_begin(t);const uint8_t *record=definition(b,dword(sound),i);
        uc_audio_instance_track_bind(t,p,b->identity,record,5);uc_audio_link_append(&p->links[4],&t->membership);
    }
    if(p->buffer){
        uint32_t buffer_flags=0;void *aux=h->buffer_auxiliary(h->context,p->buffer,&buffer_flags);
        if(aux){
            if(!(buffer_flags&2))h->set_attenuation(h->context,aux,(uint16_t)(int16_t)-10000);
            else{
                int16_t gain=p->group?h->group_gain(h->context,p->group):(int16_t)-600;
                h->set_attenuation(h->context,aux,(uint16_t)(word(sound+12)+(uint16_t)gain));
                if(spatial!=65535)h->set_spatial(h->context,aux,b->spatial+(size_t)spatial*40);
            }
        }
    }
    if(flags)*flags|=(uint32_t)(word(event)&0x800)<<2;
    p->state=1;*initialized=1;return 0;
}
