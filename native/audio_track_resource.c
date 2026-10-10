#include "audio_track_resource.h"
#include "audio_selector_shuffle.h"
#include "audio_selector_weighted.h"
static uint16_t word(const uint8_t *p){return(uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t dword(const uint8_t *p){return word(p)|(uint32_t)word(p+2)<<16;}
UCAudioStatus uc_audio_track_resource(UCAudioTrackState *s,const UCAudioCommand *c,
    const UCAudioImpactBank *bank,UCAudioRandom *random,
    const void *(*resolve)(void *,uint16_t),void *context){
    uint8_t *payload=NULL;size_t size=0;uint32_t count=0;uint16_t ref;
    if(!s||!c||!bank||!random||!resolve)return UC_AUDIO_ARGUMENT;
    if(c->flags!=0&&c->flags!=0x20&&c->flags!=0xa0&&c->flags!=4&&c->flags!=0x24&&
       c->flags!=0x64&&c->flags!=0x84&&c->flags!=0xa4&&c->flags!=0x104)
        return UC_AUDIO_UNSUPPORTED;
    ref=c->reference;
    if(c->flags&4){
        if(bank->selector_count>65535||ref>=bank->selector_count||!bank->selectors)return UC_AUDIO_FORMAT;
        payload=(uint8_t*)bank->selectors[ref].data;size=bank->selectors[ref].bytes;
        if(!payload||size<4)return UC_AUDIO_FORMAT;
        count=dword(payload)&32767u;
        if(!count||size!=4+(size_t)count*12)return UC_AUDIO_FORMAT;
        uint32_t available=0;
        for(uint32_t i=0;i<count;i++){
            const uint8_t *row=payload+4+i*12;
            if(word(row)==65535&&word(row+2)!=65535)return UC_AUDIO_UNSUPPORTED;
            available+=dword(row+4)==UINT32_MAX;
        }
        if(count>1&&((c->flags&0x80)&&!available))return UC_AUDIO_UNSUPPORTED;
        if(count>1&&c->flags==0x104&&(dword(payload)>>17)>=count)return UC_AUDIO_FORMAT;
    }
    s->resource=NULL;s->subsound=-1;
    if(payload){
        UCAudioSelection selected;
        if(c->flags==0x64){
            uint32_t index=0;
            if(count>1){
                uint32_t header=dword(payload);
                do{index=uc_audio_random(random)%count;}while(index==header>>17);
                header=(header&0x1ffffu)|(index<<17);
                for(unsigned i=0;i<4;i++)payload[i]=(uint8_t)(header>>(i*8));
            }
            ref=word(payload+4+index*12);
        }else{
            if(c->flags&0x80)uc_audio_selector_shuffle(payload,size,random,&selected);
            else if(c->flags==0x104)uc_audio_selector_cycle(payload,size,&selected);
            else uc_audio_selector_weighted(payload,size,random,&selected);
            ref=selected.reference;
        }
    }
    if(ref!=65535)s->resource=resolve(context,ref);
    return UC_AUDIO_OK;
}
