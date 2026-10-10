#include "audio_bus_effect.h"
#include <fenv.h>
#include <math.h>
int uc_audio_bus_envelope_begin(UCAudioBusEnvelope*s,int inserted,float clock,
 float cutoff,float attack,float hold,float release){
 if(!s||!isfinite(clock)||!isfinite(cutoff)||!isfinite(attack)||!isfinite(hold)||!isfinite(release)||
    attack<0||hold<0||release<0||cutoff<0||cutoff>=8000.f)return 1;
 s->target=cutoff*.0001250000059371814f;s->attack=attack;s->hold=hold;s->release=release;
 s->inverse_attack=attack==0?1.f:1.f/attack;s->inverse_release=release==0?1.f:1.f/release;
 if(inserted){s->start=clock;s->current=1.f;}
 else {float x=1.f-s->current,y=1.f-s->target;x/=y;x*=attack;s->start-=x;}
 return 0;
}
int uc_audio_bus_envelope_step(UCAudioBusEnvelope*s,float clock,int*remove){
 float elapsed,end;if(!s||!remove||!isfinite(clock))return 1;
 elapsed=clock-s->start;
 if(elapsed<s->attack){float x=s->inverse_attack*elapsed,y=1.f-s->target;x=1.f-x;x*=y;s->current=x+s->target;}
 else {float held=s->attack+s->hold;if(elapsed>=held){end=s->release+s->attack;end+=s->hold;
   if(elapsed<end){float x;elapsed-=held;x=s->inverse_release*elapsed;x*=1.f-s->target;s->current=x+s->target;}}}
 end=s->release+s->hold;end+=s->attack;*remove=elapsed>=end;return 0;
}
int uc_audio_bus_pitch(UCAudioGroup*g,float ratio){
 int32_t value;uint16_t bits;if(!g||!isfinite(ratio)||fegetround()!=FE_TONEAREST)return 1;
 if(ratio<0)ratio=0;else if(ratio>4)ratio=4;
 value=ratio==0?INT32_MIN:(int32_t)nearbyint(log2((double)ratio)*4096.0);
 bits=(uint16_t)value;g->pitch=(int16_t)(bits<32768?bits:(int32_t)bits-65536);g->dirty|=4;return 0;
}

