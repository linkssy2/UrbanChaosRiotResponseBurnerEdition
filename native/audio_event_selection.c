#include "audio_event_selection.h"
#include "audio_selector_weighted.h"
#include "audio_selector_shuffle.h"
#include <string.h>
static uint16_t word(const uint8_t *p){return(uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t dword(const uint8_t *p){return word(p)|(uint32_t)word(p+2)<<16;}
UCAudioStatus uc_audio_event_select(uint8_t event[16],uint32_t request_flags,
    uint32_t selector_index,void *payload,size_t size,UCAudioRandom *random){
    if(!event)return UC_AUDIO_ARGUMENT;
    if(dword(event+8)==UINT32_MAX)return UC_AUDIO_OK;
    if(!payload||size<4)return UC_AUDIO_ARGUMENT;
    if(selector_index!=dword(event+8))return UC_AUDIO_FORMAT;
    uint8_t *p=(uint8_t*)payload;uint16_t selected;
    if(request_flags&0x10)selected=(uint16_t)(dword(p)>>17);
    else{
        const uint16_t flags=word(event);UCAudioSelection choice;UCAudioStatus status;
        if(flags&0x100)status=uc_audio_selector_random(p,size,random,&choice);
        else if(flags&0x200)status=uc_audio_selector_shuffle(p,size,random,&choice);
        else if(flags&0x400)status=uc_audio_selector_cycle(p,size,&choice);
        else status=uc_audio_selector_weighted(p,size,random,&choice);
        if(status)return status;
        selected=choice.reference;
        if(!(flags&0x700)){
            const uint32_t header=(dword(p)&0x1FFFFu)|((uint32_t)selected<<17);
            memcpy(p,&header,4);
        }
    }
    event[2]=(uint8_t)selected;event[3]=(uint8_t)(selected>>8);return UC_AUDIO_OK;
}
