#include "audio_device_bus.h"
#include <string.h>
static int ring(const UCAudioLink*p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
static int flags_ok(uint32_t f){f&=~0x300u;return f==0x12u||f==0x14u;}
static void setup(UCAudioDeviceBuffer*d,uint32_t flags,const UCAudioDeviceBusEffects*h){
    static const UCAudioMixBin ordinary[6]={{0,0},{4,0},{1,0},{5,0},{2,0},{3,-1200}};
    static const UCAudioMixBins six={6,ordinary};
    UCAudioMixBin spatial[7]={{6,0},{8,0},{7,0},{9,0},{2,0},{10,-600},{3,-1200}};
    UCAudioMixBins seven={7,spatial};
    UCAudioDeviceBusDescriptor desc={0,0x2000u,0,NULL,&six,0};
    d->flags=flags;
    if(flags&4u){desc.flags|=0x10u;spatial[5].volume=d->gain;desc.mix_bins=&seven;}
    else {d->channel_mask=0x3Fu;d->flags|=2u;}
    d->buffer=h->create(h->context,d->provider->backend,&desc);
}
int uc_audio_device_bus_setup(UCAudioDeviceBuffer*d,uint32_t flags,const UCAudioDeviceBusEffects*h,int*created){
    if(!d||!d->provider||d->stream||d->stream_helper||!created||!flags_ok(flags)||
       !h||!h->preflight||!h->create||h->preflight(h->context,d->provider,d,flags))return 1;
    setup(d,flags,h);*created=d->buffer!=NULL;return 0;
}
int uc_audio_device_bus_create(UCAudioDeviceProvider*p,uint32_t flags,const UCAudioDeviceBusEffects*h,UCAudioDeviceBuffer**out){
    UCAudioDeviceBuffer*d;
    if(!p||!ring(&p->devices)||!out||!flags_ok(flags)||!h||!h->preflight||!h->allocate||!h->create||
       h->preflight(h->context,p,NULL,flags))return 1;
    d=h->allocate(h->context);if(!d){*out=NULL;return 2;}
    d->membership.next=d->membership.previous=&d->membership;
    d->children.next=d->children.previous=&d->children;
    d->references=1;d->flags=0;d->buffer=d->stream=d->stream_helper=NULL;
    d->provider=p;d->spatial=NULL;d->channel_mask=0;d->gain=-600;memset(d->values,0,sizeof d->values);
    setup(d,flags,h);(void)uc_audio_link_append(&p->devices,&d->membership);*out=d;return 0;
}
