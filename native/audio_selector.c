#include "audio_selector.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t dword(const uint8_t *p){return word(p)|(uint32_t)word(p+2)<<16;}
UCAudioStatus uc_audio_selector_cycle(void *payload,size_t size,UCAudioSelection *out){
    if(!payload||!out||size<4)return UC_AUDIO_ARGUMENT;
    uint8_t *p=(uint8_t*)payload;uint32_t header=dword(p),count=header&32767u;
    if(!count||size!=4+(size_t)count*12)return UC_AUDIO_FORMAT;
    uint32_t index=count==1?0:header>>17;
    if(index>=count)return UC_AUDIO_FORMAT;
    UCAudioSelection selected={index,word(p+4+index*12),word(p+6+index*12)};
    if(count>1){
        header=(header&0x1FFFFu)|((index+1)%count)<<17;
        for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(header>>(8*i));
    }
    *out=selected;return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_selector_random(void *payload,size_t size,UCAudioRandom *rng,UCAudioSelection*out){
    if(!payload||!rng||!out||size<4)return UC_AUDIO_ARGUMENT;
    uint8_t*p=(uint8_t*)payload;uint32_t header=dword(p),count=header&32767u,index=0;
    if(!count||size!=4+(size_t)count*12)return UC_AUDIO_FORMAT;
    if(count>1){
        do{index=uc_audio_random(rng)%count;}while(index==header>>17);
        header=(header&0x1FFFFu)|(index<<17);
        for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(header>>(8*i));
    }
    UCAudioSelection selected={index,word(p+4+index*12),word(p+6+index*12)};*out=selected;return UC_AUDIO_OK;
}
