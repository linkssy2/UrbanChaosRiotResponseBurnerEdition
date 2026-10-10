#include "audio_selector_weighted.h"
static uint16_t word(const uint8_t *p){return(uint16_t)(p[0]|(uint16_t)p[1]<<8);}
UCAudioStatus uc_audio_selector_weighted(const void *data,size_t size,UCAudioRandom *random,UCAudioSelection *out){
    if(!data||!random||!out||size<4)return UC_AUDIO_ARGUMENT;
    const uint8_t *p=(const uint8_t*)data;const uint32_t count=word(p)&32767u;
    if(!count||size!=4+(size_t)count*12)return UC_AUDIO_FORMAT;
    uint32_t selected=0;
    if(count>1){
        const uint16_t value=uc_audio_random(random)&32767u;
        for(uint32_t i=0;i<count;i++)if(value>=word(p+12+i*12)&&value<=word(p+14+i*12)){selected=i;break;}
    }
    UCAudioSelection next={selected,word(p+4+selected*12),word(p+6+selected*12)};*out=next;return UC_AUDIO_OK;
}
