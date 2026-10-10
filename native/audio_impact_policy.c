#include "audio_impact_policy.h"
#include <string.h>
static uint16_t word(const uint8_t*p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static const uint8_t *name(const UCAudioPolicyBank*b,const uint8_t*s){
    uint32_t index=word(s+14)&16383u;
    if(!b||!b->strings||!b->string_bytes||b->string_bytes>16777216u||index>=b->string_bytes)return NULL;
    const uint8_t*p=b->strings+index;return memchr(p,0,b->string_bytes-index)?p:NULL;
}
static unsigned fold(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static int equal(const uint8_t*a,const uint8_t*b){
    while(fold(*a)==fold(*b)){if(!*a)return 1;a++;b++;}return 0;
}
UCAudioStatus uc_audio_impact_policy(const UCAudioPolicyBank*b,const uint8_t*event,
    const uint8_t*sound,const UCAudioPolicyInstance*instances,uint32_t count,uint8_t*accepted){
    if(!b||!event||!sound||!accepted||(count&&!instances))return UC_AUDIO_ARGUMENT;
    if(count>4096)return UC_AUDIO_LIMIT;
    uint16_t policy=word(sound+14);unsigned remaining=policy>>14;
    if(b->type!=104||!(event[0]&128)||!remaining||policy==65535||(sound[21]&4))return UC_AUDIO_UNSUPPORTED;
    const uint8_t*requested=name(b,sound);if(!requested)return UC_AUDIO_FORMAT;
    for(uint32_t i=0;i<count;i++){
        const UCAudioPolicyInstance*v=instances+i;
        if(v->resource_type==104&&v->bank&&v->bank->type==104&&v->sound&&word(v->sound+14)!=65535&&!name(v->bank,v->sound))return UC_AUDIO_FORMAT;
    }
    for(uint32_t i=0;i<count;i++){
        const UCAudioPolicyInstance*v=instances+i;
        if(v->resource_type!=104||!v->bank||v->bank->type!=104||!v->sound||word(v->sound+14)==65535)continue;
        if(equal(name(v->bank,v->sound),requested)&&v->event==event){
            if(!remaining){*accepted=0;return UC_AUDIO_OK;}remaining--;
        }
    }
    *accepted=1;return UC_AUDIO_OK;
}
