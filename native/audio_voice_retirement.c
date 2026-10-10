#include "audio_voice_retirement.h"
static int ring(const UCAudioLink*p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
static int empty(const UCAudioLink*p){return ring(p)&&p->next==p;}
static int destroy_device(UCAudioDeviceBuffer*d,const UCAudioVoiceRetirement*h){
 uint32_t status=0;unsigned polls=0;
 (void)uc_audio_link_remove(&d->children);
 d->stream_helper=NULL;
 if(d->buffer&&!(d->flags&0x10u)){
  h->status(h->context,d->buffer,&status);
  if(status&1u){h->stop_extended(h->context,d->buffer);do{if(++polls>4096)return 2;h->status(h->context,d->buffer,&status);}while(status&1u);}
 }
 if(d->buffer){h->release_backend(h->context,d->buffer);d->buffer=NULL;}
 (void)uc_audio_link_remove(&d->membership);h->release_device(h->context,d);return 0;
}
static int destroy_voice(UCAudioBuffer*v,const UCAudioVoiceRetirement*h,const UCAudioBufferGroup*g){
 UCAudioDeviceBuffer*d=v->device;
 if(d){if(d->flags&8u)++*h->available[2];else if(d->flags&4u)++*h->available[1];else if(d->flags&2u)++*h->available[0];
  if(!--d->references&&destroy_device(d,h))return 2;v->device=NULL;}
 if(v->owner34){--*g->references;v->owner34=NULL;}
 (void)uc_audio_link_remove(&v->links[0]);h->release_voice(h->context,v);return 0;
}
int uc_audio_voice_release(UCAudioBuffer*v,const UCAudioVoiceRetirement*h,uint32_t*out){
 UCAudioDeviceBuffer*d;uint32_t status=0;
 if(!v||!out||!h||!h->pending||!h->available[0]||!h->available[1]||!h->available[2]||!h->preflight||!h->stop||!h->status||!h->output_null||!h->stop_extended||!h->release_backend||!h->release_device||!h->release_voice)return 1;
 d=v->device;
 if(v->owner34||v->owner38||v->spatial||!ring(&v->links[0])||!empty(&v->links[1])||!empty(&v->links[4])||!ring(h->pending)||h->pending==&v->links[0]||
    (d&&(d->stream||d->stream_helper||!d->provider||!ring(&d->membership)||!empty(&d->children)||!ring(&d->provider->devices)))||h->preflight(h->context,v))return 1;
 --v->references;
 if(!v->references){if(destroy_voice(v,h,NULL))return 2;*out=0;return 0;}
 if(v->references==1){
  if(v->links[0].next!=&v->links[0])(void)uc_audio_link_remove(&v->links[0]);
  if(v->flags&8u){d=v->device;if(d&&d->buffer)h->stop(h->context,d->buffer);
   if(d&&d->buffer)h->status(h->context,d->buffer,&status);
   if(status){if(d){(void)uc_audio_link_remove(&d->membership);(void)uc_audio_link_append(&d->provider->devices,&d->membership);if(d->buffer)h->output_null(h->context,d->buffer);}
    (void)uc_audio_link_append(h->pending,&v->links[0]);
   }else{if(destroy_voice(v,h,NULL))return 2;*out=0;return 0;}
  }
 }
 *out=v->references;return 0;
}
int uc_audio_source_buffer_release(UCAudioBuffer*v,const UCAudioBufferGroup*g,const UCAudioVoiceRetirement*h,uint32_t*out){
 UCAudioDeviceBuffer*d;
 if(!v||!out||!h||!h->available[0]||!h->available[1]||!h->available[2]||!h->preflight||!h->status||!h->stop_extended||!h->release_backend||!h->release_device||!h->release_voice)return 1;
 d=v->device;
 if(!v->references||(v->flags&8)||v->owner38||v->spatial||!ring(&v->links[0])||!empty(&v->links[1])||!empty(&v->links[2])||!empty(&v->links[3])||!empty(&v->links[4])||
    (v->owner34&&(!g||v->owner34!=g->identity||!g->references||*g->references<2))||
    (d&&(d->stream||d->stream_helper||!d->provider||!ring(&d->membership)||!empty(&d->children)||!ring(&d->provider->devices)))||h->preflight(h->context,v))return 1;
 if(!--v->references){if(destroy_voice(v,h,g))return 2;*out=0;return 0;}
 if(v->references==1&&v->links[0].next!=&v->links[0])(void)uc_audio_link_remove(&v->links[0]);
 *out=v->references;return 0;
}
