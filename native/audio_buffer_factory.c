#include "audio_buffer_factory.h"
#include <math.h>
#include <xmmintrin.h>
static int spatial_begin(UCAudioSpatial *p,const UCAudioBufferFactory *h){
    p->owner0=p->owner4=NULL;p->mode=0;p->flags&=~3u;
    p->range[0]=1.f;p->range[1]=100.f;
    for(unsigned i=0;i<3;i++)p->position[i]=p->velocity[i]=0.f;
    p->membership.next=p->membership.previous=&p->membership;
    void *engine=h->current(h->context);if(!engine)return 2;
    void *listener=h->listener(h->context,engine);
    if(listener){double scale=h->scale(h->context,listener);
        if(!isfinite(scale)||scale<1e-20||scale>1e20)return 2;
        double inverse=1./scale;p->range[0]=(float)(inverse*p->range[0]);
        p->range[1]=(float)(inverse*p->range[1]);}
    p->value40=0.f;p->word38=p->word3c=p->word44=0;
    p->value48=.25f;p->value4c=1.5f;return 0;
}
int uc_audio_buffer_factory_create(const UCAudioBufferGroup *g,
    const UCAudioBufferDescriptor *d,const UCAudioBufferFactory *h,UCAudioBuffer **out){
    if(!g||!g->identity||!g->references||!g->priority||!d||!out||!h||
       !h->preflight||!h->allocate_buffer||!h->allocate_spatial||!h->current||
       !h->listener||!h->scale||!h->device||!h->steal||!h->initialize||
       (_mm_getcsr()&0xe040)||h->preflight(h->context,g,d))return 1;
    if(!h->current(h->context))return 2;
    UCAudioBuffer *b=h->allocate_buffer(h->context);
    if(!b){*out=NULL;return 0;}uc_audio_buffer_begin(b);b->flags=d->flags;
    if(b->flags&2){UCAudioSpatial *p=h->allocate_spatial(h->context);
        if(p&&spatial_begin(p,h))return 2;b->spatial=p;}
    b->priority=d->priority;uint16_t channels=d->channels;
    void *engine=h->current(h->context);if(!engine)return 2;
    uint32_t flags=b->flags,device_flags;
    if(flags&0x40000000u)device_flags=8;
    else{device_flags=flags&2?4u:2u;if(!(flags&8))device_flags|=0x10;}
    if(flags&4)device_flags|=0x100;if(flags&0x10)device_flags|=0x200;
    b->channels=channels;b->device=h->device(h->context,engine,device_flags);
    if(!b->device&&b->priority!=65535)b->device=h->steal(h->context,engine,device_flags&8,b->priority);
    if(b->device&&h->initialize(h->context,b))return 2;
    if(b->owner34)return 2; /* fresh factory domain, never fake old-group release */
    b->owner34=g->identity;++*g->references;b->priority=*g->priority;
    *out=b;return 0;
}
