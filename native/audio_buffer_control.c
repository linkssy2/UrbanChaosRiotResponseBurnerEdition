#include "audio_buffer_control.h"
static int ring(const UCAudioLink*p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
int uc_audio_buffer_attenuation(UCAudioDeviceBuffer*d,int16_t value,const UCAudioBufferControl*h){
 if(!d||d->stream||d->stream_helper||!h||!h->preflight||!h->bins||!h->volume||h->preflight(h->context,d,0))return 1;
 d->gain=value<-10000?-10000:value>-600?-600:value;
 if(d->buffer){UCAudioMixBin pairs[8];uint32_t count=0;
  if(h->bins(h->context,d->buffer,pairs,&count)<0)return 0;
  if(count>8)return 2;
  for(uint32_t i=0;i<count;i++)if(pairs[i].index==10){h->volume(h->context,d->buffer,10,value);break;}
 }return 0;
}
int uc_audio_buffer_clear(UCAudioBuffer*v,const UCAudioBufferControl*h){
 if(!v)return 1;UCAudioDeviceBuffer*d=(UCAudioDeviceBuffer*)v->device;if(!d)return 0;
 if(d->stream||d->stream_helper||!d->provider||!ring(&d->membership)||!ring(&d->provider->devices)||!h||!h->preflight||!h->output||h->preflight(h->context,d,1))return 1;
 uc_audio_link_remove(&d->membership);uc_audio_link_append(&d->provider->devices,&d->membership);
 if(d->buffer)h->output(h->context,d->buffer);return 0;
}
