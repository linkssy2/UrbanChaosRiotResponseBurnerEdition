#include "audio_instance_start.h"
#include <stddef.h>
static int ring(const UCAudioLink*h,unsigned maximum){unsigned count=0;if(!h||!h->next||!h->previous||h->next->previous!=h||h->previous->next!=h)return 0;for(const UCAudioLink*n=h->next;n!=h;n=n->next)if(++count>maximum||!n->next||!n->previous||n->next->previous!=n||n->previous->next!=n)return 0;return 1;}
static UCAudioInstanceTrack *track(UCAudioLink*n){return(UCAudioInstanceTrack*)((char*)n-(offsetof(UCAudioInstanceTrack,membership)));}
static int span(const UCAudioInstanceTrack*t,const UCAudioTrackCommands*v,int preload){
 if(!t->definition||!v||!v->data||!v->count||v->count>255||v->bytes>65535||t->state.resume_index>v->count||(preload&&t->state.resume_index==v->count)||(t->state.flags&0x2200))return 0;
 size_t at=0,resume=0;UCAudioTimedCommand c;for(unsigned n=0;n<v->count;n++){if(n==t->state.resume_index)resume=at;if(uc_audio_timed_command_read(v->data+at,v->bytes-at,&c))return 0;at+=c.command.bytes;}if(t->state.resume_index==v->count)resume=at;return at==v->bytes&&resume==t->state.resume_offset;
}
int uc_audio_instance_publish(UCAudioInstance*p,uint32_t flags,const UCAudioInstanceStart*h,uint8_t*out){
 if(!out)return 1;if(!p){*out=0;return 0;}
 if((flags&0x1a000)||!h||!h->preflight||!h->commands||!h->preload||!h->clock||!h->mix||!h->submit||!h->bank||p->bank!=h->bank||!ring(h->bank_instances,4096)||!ring(h->bank_timing,0)||!ring(&p->links[3],0)||!ring(&p->links[4],255)||h->bank_instances==&p->links[3]||h->preflight(h->context,p,flags))return 1;
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];n=n->next){UCAudioInstanceTrack*t=track(n);if(t->parent!=p||!span(t,h->commands(h->context,t),!(flags&4)))return 1;}
 uc_audio_instance_retain(p);if(uc_audio_link_append(h->bank_instances,&p->links[3]))return 2;
 uint8_t accepted;if(uc_audio_instance_state(p,2,NULL,&accepted))return 2;if(!accepted){*out=0;return 0;}p->flags=flags;
 if(!(p->flags&4)){
  for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];n=n->next){UCAudioInstanceTrack*t=track(n);t->state.flags|=1;const UCAudioTrackCommands*v=h->commands(h->context,t);UCAudioTimedCommand c;
   if(uc_audio_timed_command_read(v->data+t->state.resume_offset,v->bytes-t->state.resume_offset,&c))return 2;
   if(!c.command.time&&h->preload(h->context,p,t,&c))return 2;
  }p->flags|=4;
 }
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];n=n->next)track(n)->state.flags&=0xfffe;
 p->flags|=0x40000;p->flags&=~4u;
 int32_t now=h->clock(h->context);if(now<0)return 2;
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];){UCAudioInstanceTrack*t=track(n);n=n->next;if(uc_audio_command_schedule(t,h->commands(h->context,t),(uint64_t)now,600000,h->submit))return 2;}
 if(h->mix(h->context,p))return 2;*out=1;return 0;
}
