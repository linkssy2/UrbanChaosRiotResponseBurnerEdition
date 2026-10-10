#include "audio_impact_request.h"
static uint16_t word(const uint8_t*p){return(uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t dword(const uint8_t*p){return word(p)|(uint32_t)word(p+2)<<16;}
int uc_audio_impact_request(const UCAudioImpactBank*b,uint32_t index,
    uint32_t*flags,UCAudioRandom*random,const UCAudioImpactVoices*v,
    UCAudioRequestSelection*out,uint8_t*found){
    if(!b||!flags||!v||!out||!found||v->count>4096||(v->count&&!v->instances)||b->policy.type!=104||
       b->event_count>65535||b->sound_count>65535||b->selector_count>65535||b->group_count>65536||
       (b->event_count&&!b->events)||(b->sound_count&&!b->sounds)||
       (b->selector_count&&!b->selectors)||(b->group_count&&!b->groups))return 1;
    if(index==UINT32_MAX){*found=0;return 0;}
    if(index>=b->event_count)return 1;
    uint8_t*event=b->events+16*index;const uint32_t selector=dword(event+8);
    UCAudioSelectorPayload payload={0};
    if(selector!=UINT32_MAX){if(selector>=b->selector_count)return 1;payload=b->selectors[selector];}
    if(uc_audio_event_select(event,*flags,selector,payload.data,payload.bytes,random))return 1;
    const uint16_t sound_index=word(event+2);
    if(sound_index==65535){*found=0;return 0;}
    if(sound_index>=b->sound_count)return 2;
    out->sound=b->sounds+28*sound_index;
    if(word(out->sound+14)!=65535){
        uint8_t accepted;int status;
        if(event[0]&128)status=uc_audio_impact_policy(&b->policy,event,out->sound,v->instances,v->count,&accepted);
        else status=uc_audio_default_policy(&b->policy,event,out->sound,v->instances,v->count,
            v->flags,*flags,v->stop,v->context,&accepted);
        if(status)return 2;
        if(!accepted){*found=0;return 0;}
    }
    if(!out->group){
        const uintptr_t p=(uintptr_t)out->sound,base=(uintptr_t)b->sounds;
        if(p<base||p-base>=(size_t)b->sound_count*28||(p-base)%28||!(out->sound[21]&2))return 2;
        const uint16_t group=word(out->sound+16);if(group>=b->group_count)return 2;
        out->group=b->groups[group];
    }
    *found=1;return 0;
}
