#include "audio_fade.h"
#include "audio_group.h"
#include <math.h>
#include <string.h>
static uint16_t word(const uint8_t*p){return(uint16_t)(p[0]|p[1]<<8);}
int uc_audio_fade_begin(UCAudioFadeState*s,uint32_t flags,const uint8_t*xfde,uint32_t tracks,uint64_t now,int(*queue)(void*,uint32_t,uint64_t,const uint8_t[44]),void*owner){
 if(!s||tracks>4096)return 1;
 if(flags&0x20000){s->flags&=~0x1000u;return 0;}
 if(tracks&&xfde&&(flags&0x6000)&&!queue)return 1;
 s->flags|=flags|0x1000;if(!s->phase||(s->phase>6&&s->phase<=8))return 0;
 s->phase=3;if(!xfde){s->flags&=~0x1000u;return 0;}
 uint8_t command[44]={0};command[5]=10;command[12]=100;
 for(uint32_t i=0;i<tracks;i++){
  if(!(flags&0x6000))continue;unsigned n=word(xfde+((flags&0x4000)?8:2))/100;
  if(!n){s->flags&=~0x1000u;break;}
  int16_t step=(int16_t)((flags&0x4000)?-32768/(int)n:32767/(int)n);
  command[16]=(uint8_t)step;command[17]=(uint8_t)((uint16_t)step>>8);
  if(!(flags&0x4000))s->gain=-10000;
  command[6]|=1;/* actual24E3A0 copies after marking the shared command */
  if(queue(owner,i,now,command))return 2;
 }
 return 0;
}
int uc_audio_fade_tick(UCAudioFadeState*s,const uint8_t c[44],uint64_t now,int(*schedule)(void*,uint64_t),int(*mix)(void*),void*owner,uint8_t*repeat){
 if(!s||!c||!schedule||!mix||!repeat||c[4]||c[5]!=10||(word(c+6)&~1u)||c[12]!=100||c[13]||c[14]||c[15])return 1;
 float amplitude=s->gain>=0?1.f:s->gain<=-10000?0.f:(float)pow(10.,(double)s->gain*(double)0.0005f);
 int16_t step=(int16_t)word(c+16);float increment=(float)step*0.000030517578125f;
 float volume=amplitude+increment;UCAudioGroup g={0};if(uc_audio_group_volume(&g,volume))return 1;s->gain=g.gain;
 if(s->gain>-10000&&s->gain<0){if(schedule(owner,now+100)||mix(owner))return 2;*repeat=1;}
 else{s->flags&=~0x1000u;*repeat=0;}
 return 0;
}

int uc_audio_fade_transition(UCAudioFadeState*s,uint16_t track_flags,int(*stop)(void*),void*owner){
 if(!s||!stop||(track_flags&0x2200))return 1;
 if(!(track_flags&4)||(s->phase!=3&&s->phase!=4)||(s->flags&0x1000))return 0;
 if(s->flags&0x2000){s->flags&=~0x2000u;s->phase=2;}
 else if(s->flags&0x4000){s->flags&=~0x4000u;if(stop(owner))return 2;}
 return 0;
}
