#include "audio_source_retirement.h"
#include <stddef.h>
static int link(const UCAudioLink*p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
static int empty(const UCAudioLink*p){return link(p)&&p->next==p;}
static int ring(const UCAudioLink*h){unsigned n=0;if(!link(h))return 0;for(const UCAudioLink*p=h->next;p!=h;p=p->next)if(++n>4096||!link(p))return 0;return 1;}
static UCAudioBuffer *buffer(UCAudioLink*p){return(UCAudioBuffer*)((char*)p-(offsetof(UCAudioBuffer,links)+3*sizeof(UCAudioLink)));}
static UCAudioInstance *instance(UCAudioLink*p){return(UCAudioInstance*)((char*)p-(offsetof(UCAudioInstance,links)+2*sizeof(UCAudioLink)));}
static int matching(const UCAudioLink*h,const UCAudioBuffer*b){for(UCAudioLink*n=h->next;n!=h;){UCAudioInstance*p=instance(n);n=n->next;if(p->buffer==b)return 1;}return 0;}
int uc_audio_source_release(UCAudioSourceEngine*s,const UCAudioSourceRetirement*h,uint32_t*out){
 unsigned owned=0;const UCAudioVoiceRetirement*v;
 if(!s||!out||!h||!h->preflight||!h->release_source||!h->buffers||!s->references||!link(&s->membership)||!empty(&s->pending)||!ring(&s->buffers)||!ring(h->instances))return 1;
 v=h->buffers;if(!v->available[0]||!v->available[1]||!v->available[2]||!v->preflight||!v->status||!v->stop_extended||!v->release_backend||!v->release_device||!v->release_voice)return 1;
 for(UCAudioLink*n=s->buffers.next;n!=&s->buffers;n=n->next){UCAudioBuffer*b=buffer(n);UCAudioDeviceBuffer*d=b->device;
  if(!b->references||(b->flags&8)||b->owner38||b->spatial||!link(&b->links[0])||!empty(&b->links[1])||!empty(&b->links[2])||!empty(&b->links[4])||matching(h->instances,b))return 1;
  if(b->owner34){if(!h->group||!h->group->references||b->owner34!=h->group->identity)return 1;++owned;}
  if(d&&(d->stream||d->stream_helper||!d->provider||!link(&d->membership)||!empty(&d->children)||!link(&d->provider->devices)))return 1;
  if(v->preflight(v->context,b))return 1;
 }
 if((owned&&*h->group->references<=owned)||h->preflight(h->context,s))return 1;
 if(--s->references){*out=s->references;return 0;}
 (void)uc_audio_link_remove(&s->membership);
 while(s->buffers.next!=&s->buffers){UCAudioBuffer*b=buffer(s->buffers.next);uint32_t left;(void)uc_audio_link_remove(&b->links[3]);
  /* Actual240550 captures next before querying. Only the checked nonmatching
   * branch is admitted; no missing pause/stop service is reported complete. */
  if(matching(h->instances,b)||uc_audio_source_buffer_release(b,h->group,v,&left))return 2;
 }
 (void)uc_audio_link_remove(&s->membership);h->release_source(h->context,s);*out=0;return 0;
}
