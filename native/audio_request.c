#include "audio_request.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
UCAudioStatus uc_audio_request_prepare(const uint8_t *event,
    const UCAudioRequestServices *h,UCAudioRequestSelection *out,uint8_t *found){
    UCAudioRequestSelection next;uint16_t index;
    if(!h||!h->sound||!h->group||!out||!found)return UC_AUDIO_ARGUMENT;
    if(!event){*found=0;return UC_AUDIO_OK;}
    if(word(event+8)!=65535||word(event+10)!=65535)return UC_AUDIO_UNSUPPORTED;
    index=word(event+2);
    if(index==65535){*found=0;return UC_AUDIO_OK;}
    next=*out;next.sound=h->sound(h->context,index);
    if(!next.sound)return UC_AUDIO_ARGUMENT;
    if(word(next.sound+14)!=65535)return UC_AUDIO_UNSUPPORTED;
    if(!next.group){
        if(!(next.sound[21]&2))return UC_AUDIO_UNSUPPORTED;
        next.group=h->group(h->context,word(next.sound+16));
    }
    *out=next;*found=1;return UC_AUDIO_OK;
}
