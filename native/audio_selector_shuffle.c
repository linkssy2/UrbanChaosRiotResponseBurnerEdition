#include "audio_selector_shuffle.h"
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t dword(const uint8_t *p){return word(p)|(uint32_t)word(p+2)<<16;}
UCAudioStatus uc_audio_selector_shuffle(void *payload,size_t size,UCAudioRandom *random,UCAudioSelection *out){
    if(!payload||!random||!out||size<4)return UC_AUDIO_ARGUMENT;
    uint8_t *p=(uint8_t*)payload;const uint32_t count=dword(p)&32767u;
    if(!count||size!=4+(size_t)count*12)return UC_AUDIO_FORMAT;
    uint32_t selected=0,available=0;
    if(count>1){
        for(uint32_t i=0;i<count;i++)available+=dword(p+8+i*12)==UINT32_MAX;
        if(!available)return UC_AUDIO_UNSUPPORTED;
        do{selected=uc_audio_random(random)%count;}while(dword(p+8+selected*12)!=UINT32_MAX);
        uint8_t *marker=p+8+selected*12;marker[0]=1;marker[1]=marker[2]=marker[3]=0;
        if(available==1)for(uint32_t i=0;i<count;i++)if(i!=selected){
            marker=p+8+i*12;marker[0]=marker[1]=marker[2]=marker[3]=255;
        }
    }
    UCAudioSelection result={selected,word(p+4+selected*12),word(p+6+selected*12)};
    *out=result;return UC_AUDIO_OK;
}
