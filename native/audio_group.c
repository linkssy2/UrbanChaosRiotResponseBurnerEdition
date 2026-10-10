#include "audio_group.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static int16_t signed_word(uint16_t x){return (int16_t)(x<32768?x:(int32_t)x-65536);}
static UCAudioStatus chain(const UCAudioGroup *g,const UCAudioGroup **nodes,unsigned *count){
 unsigned n=0;while(g){
  unsigned i;if(n==64)return UC_AUDIO_LIMIT;
  for(i=0;i<n;i++)if(nodes[i]==g)return UC_AUDIO_ARGUMENT;
  nodes[n++]=g;g=g->parent;
 }*count=n;return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_group_seed(const void *record,size_t bytes,const void *names,size_t size,UCAudioGroupSeed *out){
 const uint8_t *r=(const uint8_t*)record;const char *s=(const char*)names;size_t offset,n;
 if(!r||!s||!out)return UC_AUDIO_ARGUMENT;
 if(bytes!=20)return UC_AUDIO_FORMAT;
 offset=word(r);if(offset>=size)return UC_AUDIO_FORMAT;
 for(n=0;n<=255&&offset+n<size;n++)if(!s[offset+n])break;
 if(!n||n>255||offset+n==size)return UC_AUDIO_FORMAT;
 {UCAudioGroupSeed seed={s+offset,signed_word(word(r+2)),signed_word(word(r+4)),signed_word(word(r+6))};*out=seed;}
 return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_group_initialize(UCAudioGroup *g,const UCAudioGroupSeed *seed,const UCAudioGroup *parent){
 const UCAudioGroup *nodes[64];unsigned n,i;UCAudioStatus s;
 if(!g||!seed||!seed->name)return UC_AUDIO_ARGUMENT;
 s=chain(parent,nodes,&n);if(s)return s;if(n==64)return UC_AUDIO_LIMIT;
 for(i=0;i<n;i++)if(nodes[i]==g)return UC_AUDIO_ARGUMENT;
 {UCAudioGroup next={0};next.parent=parent;next.gain=seed->gain;next.pitch=seed->pitch;next.secondary_gain=seed->secondary_gain;*g=next;}
 return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_group_volume(UCAudioGroup *g,float volume){
 int16_t gain;
 if(!g||!isfinite(volume)||fegetround()!=FE_TONEAREST)return UC_AUDIO_ARGUMENT;
 if(volume<=0)gain=-10000;
 else if(volume>=1)gain=0;
 else{
  const double value=(0.30102999566398119521*log2(1.0/(double)volume))*-2000.0;
  gain=signed_word((uint16_t)(int32_t)value);
  if(gain< -10000)gain=-10000;else if(gain>0)gain=0;
 }
 g->gain=gain;g->dirty|=4;return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_group_mix(const UCAudioGroup *g,UCAudioGroupMix *out){
 const UCAudioGroup *nodes[64];unsigned n;UCAudioStatus s;UCAudioGroupMix result={0};
 if(!g||!out)return UC_AUDIO_ARGUMENT;
 s=chain(g,nodes,&n);if(s)return s;
 while(n){
  int32_t gain,pitch,secondary;g=nodes[--n];
  gain=(int32_t)g->envelope_gain+g->gain+g->auxiliary_gain+result.gain;
  pitch=(int32_t)g->pitch+g->auxiliary_pitch+result.pitch;
  secondary=(int32_t)g->secondary_gain+result.secondary_gain;
  result.gain=gain< -10000?-10000:signed_word((uint16_t)gain);
  result.pitch=(int16_t)(pitch< -32767?-32767:pitch>8191?8191:pitch);
  result.secondary_gain=secondary< -10000?-10000:signed_word((uint16_t)secondary);
 }*out=result;return UC_AUDIO_OK;
}
