#include "audio_instance_cleanup.h"
#include <stddef.h>
static int ring(const UCAudioLink*p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
static int empty(const UCAudioLink*p){return ring(p)&&p->next==p;}
static UCAudioInstanceTrack *track(UCAudioLink*n){return(UCAudioInstanceTrack*)((char*)n - offsetof(UCAudioInstanceTrack,membership));}
static uint16_t word(const uint8_t*p){return(uint16_t)(p[0]|p[1]<<8);}
static int clean_track(UCAudioInstanceTrack*t,const UCAudioInstanceCleanup*h){
 uint32_t remaining;
 if(uc_audio_commands_cancel(h->heap,h->ready,t,h->release_command,h->context))return 2;
 if(t->voice){if(uc_audio_voice_release(t->voice,h->voices,&remaining))return 2;t->voice=NULL;}
 return 0;
}
int uc_audio_track_cleanup(UCAudioInstanceTrack*t,const UCAudioInstanceCleanup*h){
 if(!t||!t->parent||t->aux40||(t->state.flags&0x200)||!h||!h->preflight||
    !h->heap||!h->ready||!h->voices||!h->release_command||!empty(h->engine_timing)||
    h->preflight(h->context,t->parent))return 1;
 return clean_track(t,h);
}
/* Called only after admitted24A500 has released every outside membership. */
static int destroy(UCAudioInstance*p,const UCAudioInstanceCleanup*h){
 (void)uc_audio_link_remove(&p->links[1]);
 if(p->links[2].next!=&p->links[2])(void)uc_audio_link_remove(&p->links[2]);
 if(p->links[0].next!=&p->links[0])(void)uc_audio_link_remove(&p->links[0]);
 if(p->links[5].next!=&p->links[5])(void)uc_audio_link_remove(&p->links[5]);
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];){
  UCAudioInstanceTrack*t=track(n);n=n->next;
  if(!--t->references){if(clean_track(t,h))return 2;(void)uc_audio_link_remove(&t->membership);h->release_track(h->context,t);}
 }
 if(p->bank){h->release_bank(h->context,p->bank);p->bank=NULL;}
 (void)uc_audio_link_remove(&p->links[1]);h->release_instance(h->context,p);return 0;
}
static int release(UCAudioInstance*p,const UCAudioInstanceCleanup*h,uint32_t*out){
 uint32_t n=--p->references;if(n==1&&(p->flags&1))p->references=n=0;
 if(!n&&destroy(p,h))return 2;*out=n;return 0;
}
int uc_audio_instance_delete(UCAudioInstance*p,const UCAudioInstanceCleanup*h){
 unsigned count=0;
 if(!p||p->references||p->state!=8||p->group||p->buffer||p->aux40||p->aux4c||
    !h||!h->preflight||!h->release_command||!h->release_bank||!h->release_track||!h->release_instance||
    !h->heap||!h->ready||!h->voices||!empty(h->engine_auxiliary)||!empty(h->engine_timing)||!empty(h->bank_timing)||
    (p->bank&&p->bank!=h->bank))return 1;
 for(unsigned i=0;i<6;i++)if(!ring(&p->links[i])||(i!=1&&i!=4&&!empty(&p->links[i])))return 1;
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];n=n->next){
  if(++count>255||!ring(n))return 1;UCAudioInstanceTrack*t=track(n);
  if(t->parent!=p||t->references!=1||t->aux40||t->voice||(t->state.flags&0x200)||!empty(&t->commands))return 1;
 }
 if(h->preflight(h->context,p))return 1;return destroy(p,h);
}
int uc_audio_instance_cleanup(UCAudioInstance*p,const UCAudioInstanceCleanup*h,uint32_t*out){
 unsigned count=0,members=0;uint32_t remaining;UCAudioBuffer*b;
 if(!p||!h||!out||!h->preflight||!h->release_command||!h->release_bank||!h->release_track||!h->release_instance||
    !h->heap||!h->ready||!h->voices||!empty(h->engine_auxiliary)||!empty(h->engine_timing)||!empty(h->bank_timing)||
    (p->bank&&p->bank!=h->bank)||p->aux40||p->aux4c)return 1;
 for(unsigned i=0;i<6;i++)if(!ring(&p->links[i]))return 1;
 if(p->links[0].next==&p->links[0]&&p->sound&&word(p->sound+14)!=UINT16_MAX)return 1;
 if(p->links[3].next!=&p->links[3]&&!p->bank)return 1;
 if(p->group&&(!h->group||h->group->identity!=p->group||!h->group->references||*h->group->references<2))return 1;
 b=p->buffer;if(b&&(b->references<2||(b->flags&8)||b->owner38||!ring(&b->links[0])))return 1;
 if(p->links[5].next!=&p->links[5]&&!b)return 1;
 for(unsigned i=0;i<6;i++)if((i==0||i==2||i==3||i==5)&&p->links[i].next!=&p->links[i])++members;
 if(p->references<=members||p->references==UINT32_MAX)return 1;
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];n=n->next){
  if(++count>255||!ring(n))return 1;UCAudioInstanceTrack*t=track(n);
  if(t->parent!=p||t->references!=1||t->aux40||(t->state.flags&0x200))return 1;
 }
 if(h->preflight(h->context,p))return 1;
 uc_audio_instance_retain(p);
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];){UCAudioInstanceTrack*t=track(n);n=n->next;if(clean_track(t,h))return 2;}
 if(p->links[5].next!=&p->links[5]&&p->buffer){(void)uc_audio_link_remove(&p->links[5]);if(release(p,h,&remaining))return 2;}
 if(p->links[2].next!=&p->links[2]){(void)uc_audio_link_remove(&p->links[2]);if(release(p,h,&remaining))return 2;}
 if(p->links[0].next!=&p->links[0]){(void)uc_audio_link_remove(&p->links[0]);if(release(p,h,&remaining))return 2;}
 if(p->links[3].next!=&p->links[3]&&p->bank){(void)uc_audio_link_remove(&p->links[3]);if(release(p,h,&remaining))return 2;}
 p->state=8;
 if(p->group){--*h->group->references;p->group=NULL;}
 b=p->buffer;if(b){if(--b->references==1&&b->links[0].next!=&b->links[0])(void)uc_audio_link_remove(&b->links[0]);p->buffer=NULL;}
 return release(p,h,out);
}
