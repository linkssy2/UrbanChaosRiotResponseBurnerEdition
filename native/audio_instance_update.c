#include "audio_instance_update.h"
#include <stddef.h>
static int ring(const UCAudioLink*p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
static UCAudioInstanceTrack *track(UCAudioLink*n){return(UCAudioInstanceTrack*)((char*)n - offsetof(UCAudioInstanceTrack,membership));}
int uc_audio_instance_update(UCAudioInstance*p,const UCAudioInstanceUpdate*h,uint8_t*out){
 unsigned count=0,done=0;const UCAudioGroup*groups[64];unsigned depth=0;uint32_t remaining;
 if(!p||!out)return 1;
 if(p->state==0||p->state==1||p->state==6||p->state==8){*out=0;return 0;}
 if((p->state!=2&&p->state!=7)||!h||!h->engine_flags||(*h->engine_flags&2)||!h->group||!h->preflight||!h->playback||!h->cleanup||!ring(&p->links[4]))return 1;
 if(p->buffer){UCAudioBuffer*b=p->buffer;if(!ring(&b->links[2])||b->links[2].next!=&b->links[2])return 1;}
 if(p->group){const UCAudioGroup*g=h->group(h->context,p->group);if(!g)return 1;
  while(g){if(depth==64||g->dirty)return 1;for(unsigned i=0;i<depth;i++)if(groups[i]==g)return 1;groups[depth++]=g;g=g->parent;}
 }
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];n=n->next){
  if(++count>255||!ring(n))return 1;UCAudioInstanceTrack*t=track(n);UCAudioBuffer*v=t->voice;
  if(t->parent!=p||(t->state.flags&0x2200)||(v&&(v->flags&0x40000000u)))return 1;
 }
 if(h->preflight(h->context,p))return 1;
 for(UCAudioLink*n=p->links[4].next;n!=&p->links[4];){
  UCAudioInstanceTrack*t=track(n);n=n->next;if(p->state!=7&&!(t->state.flags&4))continue;
  UCAudioBuffer*v=t->voice;uint32_t status=0;
  if(v&&v->device&&uc_audio_device_status(v->device,&status,h->playback))return 2;
  if(!status){++done;if(uc_audio_track_cleanup(t,h->cleanup))return 2;}
 }
 if(done==count){if(uc_audio_instance_cleanup(p,h->cleanup,&remaining))return 2;*out=1;}else *out=0;
 return 0;
}
