#include "audio_live_mix.h"
#include <math.h>
#include <stddef.h>
#include <xmmintrin.h>
static uint16_t word(const uint8_t*p){return(uint16_t)(p[0]|p[1]<<8);}
static UCAudioInstanceTrack *track(UCAudioLink*n){return(UCAudioInstanceTrack*)((char*)n - offsetof(UCAudioInstanceTrack,membership));}
static int ring(const UCAudioLink*h){unsigned count=0;if(!h||!h->next||!h->previous||h->next->previous!=h||h->previous->next!=h)return 0;for(const UCAudioLink*n=h->next;n!=h;n=n->next)if(++count>255||!n->next||!n->previous||n->next->previous!=n||n->previous->next!=n)return 0;return 1;}
static int admit(UCAudioInstance*p,UCAudioInstanceTrack*t,const UCAudioLiveMixServices*h,unsigned op){return p&&h&&h->preflight&&h->group&&h->filter_fields&&h->gain&&h->pitch&&h->filter&&!(_mm_getcsr()&0xe040)&&!h->preflight(h->context,p,t,op);}
static int group_mix(UCAudioInstance*p,const UCAudioLiveMixServices*h,UCAudioGroupMix*out){if(!p->group){*out=(UCAudioGroupMix){0};return 0;}const UCAudioGroup*g=h->group(h->context,p->group);return !g||uc_audio_group_mix(g,out);}
static void scalar(UCAudioDeviceBuffer*d,int32_t value,const UCAudioLiveMixServices*h,int pitch){void(*send)(void*,void*,uint8_t,int32_t)=pitch?h->pitch:h->gain;if(d->buffer)send(h->context,d->buffer,0,value);if(d->stream)send(h->context,d->stream,1,value);}
int uc_audio_live_gain(UCAudioInstance*p,UCAudioInstanceTrack*t,const UCAudioLiveMixServices*h){
 if(!t)return 0;if(!admit(p,t,h,UC_AUDIO_LIVE_GAIN))return 1;UCAudioBuffer*v=t->voice;if(!v||!v->device)return 0;
 UCAudioMix m={0};m.sound_gain=p->sound?word(p->sound+4):0;m.instance_gain=p->volume;m.fade_gain=p->parameter60;m.track_gain=(int16_t)t->parameters[2];m.mute=(uint8_t)(p->buffer&&(((UCAudioBuffer*)p->buffer)->flags&0x1000000));
 if(!m.mute){UCAudioGroupMix g;if(group_mix(p,h,&g))return 1;m.group_gain=g.gain;}scalar(v->device,uc_audio_event_gain(&m,&t->state),h,0);return 0;
}
int uc_audio_live_pitch(UCAudioInstance*p,UCAudioInstanceTrack*t,const UCAudioLiveMixServices*h){
 if(!t)return 0;if(!admit(p,t,h,UC_AUDIO_LIVE_PITCH)||!ring(&p->links[4]))return 1;UCAudioBuffer*v=t->voice;if(!v||!v->device)return 0;
 UCAudioMix m={0};m.sound_pitch=p->sound?(int16_t)word(p->sound+10):0;m.instance_pitch=p->pitch;m.track_pitch=(int16_t)t->parameters[1];m.sample_pitch=(int16_t)t->parameters[0];
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];n=n->next)if(track(n)->state.flags&0x200){m.parent_timing_track=1;break;}
 if(!m.parent_timing_track){UCAudioGroupMix g;if(group_mix(p,h,&g))return 1;m.group_pitch=g.pitch;}scalar(v->device,uc_audio_event_pitch(&m,&t->state),h,1);return 0;
}
static int filter_sum(const UCAudioGroup*g,const UCAudioLiveMixServices*h,double*out){const UCAudioGroup*nodes[64];const float*fields[64][2];unsigned count=0;
 while(g){if(count==64)return 1;for(unsigned n=0;n<count;n++)if(nodes[n]==g)return 1;nodes[count]=g;if(h->filter_fields(h->context,g,&fields[count][0],&fields[count][1])||!fields[count][0]||!fields[count][1]||!isfinite(*fields[count][0])||!isfinite(*fields[count][1]))return 1;count++;g=g->parent;}
 out[0]=out[1]=0;while(count){--count;out[0]+=(double)*fields[count][0];out[1]+=(double)*fields[count][1];}return 0;
}
int uc_audio_live_filter(UCAudioInstance*p,UCAudioInstanceTrack*t,const UCAudioLiveMixServices*h){
 if(!t)return 0;if(!admit(p,t,h,UC_AUDIO_LIVE_FILTER))return 1;UCAudioBuffer*v=t->voice;if(!v||!v->device)return 0;uint16_t cutoff=t->parameters[6];
 if(p->group){const UCAudioGroup*g=h->group(h->context,p->group);double sums[2];if(!g||filter_sum(g,h,sums))return 1;double value=sums[0]*(double)cutoff;if(value<INT32_MIN||value>INT32_MAX)return 1;cutoff=(uint16_t)(int32_t)value;}
 uint32_t coefficient;if(cutoff<=30)coefficient=32768;else if(cutoff>=8000)coefficient=0;else{double value=(double)cutoff/48000.;value*=3.1415927410125732421875;value=sin(value);value+=value;value=0.69314718055994530942*log2(value);value*=5909.68115234375;coefficient=(uint16_t)(int32_t)value;}
 uint32_t packet[6]={1,0,coefficient,32768,coefficient,32768};UCAudioDeviceBuffer*d=v->device;if(d->buffer)h->filter(h->context,d->buffer,0,packet);if(d->stream)h->filter(h->context,d->stream,1,packet);return 0;
}
int uc_audio_live_all_gains(UCAudioInstance*p,const UCAudioLiveMixServices*h){if(!admit(p,NULL,h,UC_AUDIO_LIVE_ALL_GAINS)||!ring(&p->links[4]))return 1;for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];n=n->next)if(uc_audio_live_gain(p,track(n),h))return 2;return 0;}
