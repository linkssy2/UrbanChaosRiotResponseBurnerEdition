#include "audio_default_policy.h"
#include <string.h>
static uint16_t word(const uint8_t*p){return(uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static const uint8_t *name(const UCAudioPolicyBank*b,const uint8_t*s){
    uint32_t i=word(s+14)&16383u;
    if(!b||!b->strings||!b->string_bytes||b->string_bytes>16777216u||i>=b->string_bytes)return NULL;
    const uint8_t*p=b->strings+i;return memchr(p,0,b->string_bytes-i)?p:NULL;
}
static unsigned fold(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static int equal(const uint8_t*a,const uint8_t*b){
    while(fold(*a)==fold(*b)){if(!*a)return 1;a++;b++;}return 0;
}
int uc_audio_default_policy(const UCAudioPolicyBank*b,const uint8_t*event,
    const uint8_t*sound,const UCAudioPolicyInstance*v,uint32_t count,
    const uint32_t*const*flags,uint32_t request_flags,
    int(*stop)(void*,uint32_t),void*context,uint8_t*accepted){
    if(!b||!event||!sound||!stop||!accepted||(count&&(!v||!flags))||count>4096)return 1;
    const uint16_t policy=word(sound+14);const unsigned limit=policy>>14;
    if(b->type!=104||event[0]||sound[8]!=255||!limit||policy==65535||(sound[21]&4))return 1;
    const uint8_t*requested=name(b,sound);if(!requested)return 1;
    unsigned matches=0;uint32_t first=UINT32_MAX;
    for(uint32_t i=0;i<count;i++){
        const UCAudioPolicyInstance*p=v+i;
        if(p->resource_type!=104||!p->bank||p->bank->type!=104||!p->sound||word(p->sound+14)==65535)continue;
        const uint8_t*n=name(p->bank,p->sound);if(!n)return 1;
        if(equal(n,requested)){if(first==UINT32_MAX)first=i;matches+=p->event==event;}
    }
    if(matches>limit){
        const UCAudioPolicyInstance*p=v+first;
        if(!p->event||p->event[0]&4||p->sound[8]!=255||!flags[first])return 1;
        if(!(*flags[first]&0x4000)&&!(request_flags&0x8000)&&stop(context,first))return 2;
    }
    *accepted=1;return 0;
}
