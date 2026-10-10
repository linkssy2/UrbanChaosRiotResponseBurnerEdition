#include "audio_source_request.h"
#include <string.h>
#include <xmmintrin.h>
static uint16_t word(const uint8_t*p){return(uint16_t)(p[0]|p[1]<<8);}
static uint32_t dword(const uint8_t*p){return word(p)|(uint32_t)word(p+2)<<16;}
int uc_audio_source_request(UCAudioSourceEngine*s,const UCAudioInstanceBank*b,uint32_t index,const UCAudioSourceRequestServices*h,UCAudioInstance**out){
 if(!s)return 1;if(!b){if(out)*out=NULL;return 0;}
 if(!b->identity||!b->metadata||b->metadata->policy.type!=104||!h||!h->preflight||!h->select||!h->source||!h->listener||!h->instance||!h->bind||!h->prepare||!h->publish||!h->failed||(_mm_getcsr()&0xe040)||h->preflight(h->context,s,b,index))return 1;
 if(out)*out=NULL;if(index==UINT32_MAX)return 0;
 uint32_t flags=0;uint8_t found=0;UCAudioRequestSelection choice={0};
 if(h->select(h->context,b,index,&flags,&choice,&found))return 2;if(!found)return 0;
 void*group=(void*)choice.group;if(!group)group=s->group;if(!group)return 0;
 const uint8_t*sound=choice.sound;if(!sound)return 2;
 UCAudioSourceConfig c={0};c.group=group;c.priority=sound[8];uint16_t spatial=word(sound+22);
 if((sound[20]&128)&&spatial!=65535){if(!b->spatial||spatial>=b->spatial_count)return 2;const uint8_t*p=b->spatial+(size_t)spatial*40;c.flags=2;c.mode=word(p+14);memcpy(c.range,p+16,8);c.flag=(uint8_t)((dword(p+36)>>1)&1);}
 else c.flags=1;
 UCAudioBuffer*buffer=NULL;if(uc_audio_source_engine_find(s,&c,h->source,&buffer))return 2;
 if(!buffer&&uc_audio_source_engine_create(s,&c,h->source,&buffer))return 2;if(!buffer)return 0;
 if(s->enabled)flags|=0x40;
 if(buffer->flags&2){
  if(c.flags!=2||(sound[20]&32))return 2;
  const float*p=h->listener(h->context);if(!p)return 2;
  float dz=s->position[2]-p[2],dx=s->position[0]-p[0],dy=s->position[1]-p[1];
  float distance=dz*dz+dx*dx;distance=distance+dy*dy;float radius=c.range[1]*c.range[1];
  if(!(radius>distance)&&!sound[9])return 0;
 }
 flags|=1;UCAudioInstance*i=h->instance(h->context,group);if(!i)return 0;
 if(h->bind(h->context,i,buffer))return 2;
 uint8_t ready=0;if(h->prepare(h->context,i,b,index,&flags,&ready))return 2;
 if(ready&&h->publish(h->context,b,i,flags,&ready))return 2;
 if(!ready)return h->failed(h->context,i)?2:0;
 if(out){uc_audio_instance_retain(i);*out=i;}return 0;
}
