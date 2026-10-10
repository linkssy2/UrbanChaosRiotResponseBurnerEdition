#include "audio_zero_policy.h"
#include <string.h>
static uint16_t word(const uint8_t*p){return(uint16_t)(p[0]|p[1]<<8);}
static const uint8_t*name(const UCAudioPolicyBank*b,const uint8_t*s){unsigned at=word(s+14)&16383u;if(!b||!b->strings||!b->string_bytes||b->string_bytes>16777216u||at>=b->string_bytes)return NULL;return memchr(b->strings+at,0,b->string_bytes-at)?b->strings+at:NULL;}
static unsigned fold(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static int equal(const uint8_t*a,const uint8_t*b){while(fold(*a)==fold(*b)){if(!*a)return 1;++a;++b;}return 0;}
int uc_audio_zero_policy(const UCAudioPolicyBank*b,const uint8_t*event,const uint8_t*sound,
 const UCAudioPolicyInstance*v,uint32_t count,unsigned source,const uint8_t*matches,
 const uint32_t*const*flags,uint32_t*request,
 int(*fade)(void*,uint32_t),int(*stop)(void*,uint32_t),void*owner,uint8_t*accepted){
 if(!b||!event||!sound||!request||!accepted||!fade||!stop||count>4096||(count&&(!v||!flags)))return 1;
 if(b->type!=104||(event[0]!=0&&event[0]!=0x80&&event[0]!=0x84&&event[0]!=0x86)||word(sound+14)>=16384)return 1;
 unsigned scoped=source&&(sound[21]&4);if(scoped&&count&&!matches)return 1;
 const uint8_t*requested=name(b,sound);if(!requested)return 1;uint32_t first=UINT32_MAX;
 for(uint32_t i=0;i<count;i++){
  const UCAudioPolicyInstance*p=v+i;if(p->resource_type!=104||!p->bank||p->bank->type!=104||!p->sound||word(p->sound+14)==65535)continue;
  const uint8_t*n=name(p->bank,p->sound);if(!n)return 1;
  if(!equal(n,requested)||(scoped&&!matches[i]))continue;
  if((event[0]&0x80)&&p->event==event){*accepted=0;return 0;}
  if(first==UINT32_MAX)first=i;
 }
 if(first!=UINT32_MAX){const UCAudioPolicyInstance*p=v+first;
  if(!p->event||!flags[first])return 1;
  if(sound[8]<p->sound[8]){*accepted=0;return 0;}
  if(sound[8]>p->sound[8])return 1; /*23FDF0 policy-wide purge needs a completing owner.*/
  if(event[0]&2)*request|=0x2000;
  if(p->event[0]&4){if(!(*flags[first]&0x4000)&&fade(owner,first))return 2;if(!(event[0]&2))*request|=0x8000;}
  else if(!(*flags[first]&0x4000)&&!(*request&0x8000)&&stop(owner,first))return 2;
 }
 *accepted=1;return 0;
}
