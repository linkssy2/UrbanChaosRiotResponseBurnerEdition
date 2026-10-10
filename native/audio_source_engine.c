#include "audio_source_engine.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <xmmintrin.h>
static int link_valid(const UCAudioLink*p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
static int ring(const UCAudioLink*h){unsigned n=0;if(!link_valid(h))return 0;for(const UCAudioLink*p=h->next;p!=h;p=p->next)if(++n>4096||!link_valid(p))return 0;return(int)n+1;}
static UCAudioBuffer *buffer(UCAudioLink*p){return(UCAudioBuffer*)((char*)p-(offsetof(UCAudioBuffer,links)+3*sizeof(UCAudioLink)));}
static int admit(UCAudioSourceEngine*s,const UCAudioSourceEngineServices*h,unsigned op){return s&&h&&h->preflight&&!h->preflight(h->context,s,op);}
int uc_audio_source_engine_begin(UCAudioSourceEngine*s,const UCAudioSourceEngineServices*h){
 if(!s||!h||!h->current||!h->listener||!h->scale||!h->registry||(_mm_getcsr()&0xe040)||!admit(s,h,0))return 1;
 memset(s,0,sizeof*s);s->membership.next=s->membership.previous=&s->membership;s->buffers.next=s->buffers.previous=&s->buffers;s->pending.next=s->pending.previous=&s->pending;
 s->references=s->counter=1;s->kind=2;s->minimum=1.f;s->maximum=100.f;
 void*engine=h->current(h->context);if(!engine)return 2;void*listener=h->listener(h->context,engine);
 if(listener){double scale=h->scale(h->context,listener);if(!isfinite(scale)||scale<1e-20||scale>1e20)return 2;double inverse=1./scale;s->minimum=(float)(inverse*(double)s->minimum);s->maximum=(float)(inverse*(double)s->maximum);}
 engine=h->current(h->context);if(engine&&uc_audio_link_append(h->registry(h->context,engine),&s->membership))return 2;return 0;
}
int uc_audio_source_engine_position(UCAudioSourceEngine*s,const float*p,const UCAudioSourceEngineServices*h){
 if(!s||!p||!h||!h->mode||!h->parameter||!ring(&s->buffers)||!link_valid(&s->pending)||s->pending.next!=&s->pending||!admit(s,h,1))return 1;
 memcpy(s->position,p,12);
 for(UCAudioLink*n=s->buffers.next;n!=&s->buffers;n=n->next){UCAudioBuffer*b=buffer(n);if(b->flags&2){uint32_t mode=h->mode(h->context,b);if((mode==0||mode==3||mode==4)&&h->parameter(h->context,b,5,p))return 2;}}
 return 0;
}
int uc_audio_source_engine_find(UCAudioSourceEngine*s,const UCAudioSourceConfig*c,const UCAudioSourceEngineServices*h,UCAudioBuffer**out){
 if(!s||!c||!out||!h||!h->ranges||!h->mode||(_mm_getcsr()&0xe040)||!ring(&s->buffers)||!admit(s,h,2))return 1;
 for(UCAudioLink*n=s->buffers.next;n!=&s->buffers;n=n->next){UCAudioBuffer*b=buffer(n);if(b->owner34!=c->group)continue;
  if((c->flags&2)&&(b->flags&2)){float r[2];if(h->ranges(h->context,b,r))return 2;if(r[0]!=c->range[0]||r[1]!=c->range[1])continue;if(h->mode(h->context,b)!=c->mode)continue;}
  else if(!(c->flags&1)||(b->flags&2))continue;*out=b;return 0;
 }*out=NULL;return 0;
}
int uc_audio_source_engine_create(UCAudioSourceEngine*s,const UCAudioSourceConfig*c,const UCAudioSourceEngineServices*h,UCAudioBuffer**out){
 int count=s?ring(&s->buffers):0;
 if(!s||!c||!out||!c->group||!h||!h->create||!h->parameter||!count||count>4096||!admit(s,h,3))return 1;
 UCAudioBuffer*b=h->create(h->context,c->group,c->flags,1,(uint16_t)c->priority);
 if(!b){*out=NULL;return 0;}if(b->flags&2){if(h->parameter(h->context,b,7,&c->flag)||h->parameter(h->context,b,3,&c->mode)||h->parameter(h->context,b,4,c->range)||h->parameter(h->context,b,5,s->position))return 2;}
 if(uc_audio_link_append(&s->buffers,&b->links[3]))return 2;*out=b;return 0;
}
int uc_audio_source_engine_enable(UCAudioSourceEngine*s){if(!s)return 1;s->enabled=1;return 0;}
int uc_audio_source_engine_range(UCAudioSourceEngine*s,float a,float b){if(!s)return 1;memcpy(&s->minimum,&a,4);memcpy(&s->maximum,&b,4);return 0;}
int uc_audio_source_engine_end(UCAudioSourceEngine*s,const UCAudioSourceEngineServices*h){
 if(!s||!h||!h->current||!link_valid(&s->membership)||!link_valid(&s->buffers)||s->buffers.next!=&s->buffers||!link_valid(&s->pending)||s->pending.next!=&s->pending||!admit(s,h,4))return 1;
 if(h->current(h->context)&&uc_audio_link_remove(&s->membership))return 2;return uc_audio_link_remove(&s->membership)?2:0;
}
