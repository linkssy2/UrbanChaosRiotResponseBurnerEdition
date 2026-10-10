#include "audio_device_buffer.h"
#include <string.h>
static int ring(const UCAudioLink *p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
static int ordinary(const UCAudioDeviceBuffer*d){return d&&!d->stream&&!d->stream_helper;}
int uc_audio_device_create(UCAudioDeviceProvider*p,uint32_t flags,const UCAudioDeviceEffects*h,UCAudioDeviceBuffer**out){
    UCAudioDeviceBuffer*d;const UCAudioDeviceFormat f={1,2,48000,192000,4,16,0};
    if(!p||!ring(&p->devices)||!out||!h||!h->allocate||!h->create||
       (flags&~0x300u)!=2)return 1;
    d=h->allocate(h->context);if(!d){*out=NULL;return 2;}
    d->membership.next=d->membership.previous=&d->membership;
    d->children.next=d->children.previous=&d->children;
    d->references=1;d->flags=0;d->buffer=d->stream=d->stream_helper=NULL;
    d->provider=p;d->spatial=NULL;d->channel_mask=0;d->gain=-600;memset(d->values,0,sizeof d->values);
    d->flags=flags;d->channel_mask=3;
    d->buffer=h->create(h->context,p->backend,&f);
    (void)uc_audio_link_append(&p->devices,&d->membership);*out=d;return 0;
}
int uc_audio_device_initialize(UCAudioBuffer*v,const UCAudioDeviceEffects*h){
    UCAudioDeviceBuffer*d;static const uint32_t filter[6]={1,0,0,32768,0,32768};
    if(!v||!h||!h->preflight||!h->output||!h->stop||!h->filter||
       (v->device&&(!ordinary(v->device)||!((UCAudioDeviceBuffer*)v->device)->provider||
        !ring(&((UCAudioDeviceBuffer*)v->device)->membership)||
        !ring(&((UCAudioDeviceBuffer*)v->device)->provider->devices)))||
       h->preflight(h->context,v))return 1;
    d=v->device;
    if(d){(void)uc_audio_link_remove(&d->membership);
        (void)uc_audio_link_append(&d->provider->devices,&d->membership);
        if(d->buffer)h->output(h->context,d->buffer);}
    /* Original actual246FC0 secondary245470 rejects property1. */
    d=v->device;
    if(d&&(v->flags&8u)){
        if(d->buffer)h->stop(h->context,d->buffer);
        d=v->device;if(d->buffer)h->filter(h->context,d->buffer,filter);
    }
    return 0;
}
int uc_audio_device_format(UCAudioDeviceBuffer*d,const UCAudioDeviceFormat*f,const UCAudioDeviceEffects*h){
    if(!ordinary(d)||!h||!h->format)return 1;
    if(f){d->flags|=0x1000000u;if(f->channels!=1)d->flags|=0x4000000u;}
    if(d->buffer)h->format(h->context,d->buffer,f);return 0;
}
int uc_audio_device_data(UCAudioDeviceBuffer*d,const void*p,uint32_t n,const UCAudioDeviceEffects*h){
    if(!ordinary(d)||!h||!h->data)return 1;
    if(p&&n)d->flags|=0x2000000u;
    if(d->buffer)h->data(h->context,d->buffer,p,n);return 0;
}
int uc_audio_device_play_region(UCAudioDeviceBuffer*d,uint32_t start,uint32_t n,const UCAudioDeviceEffects*h){
    if(!ordinary(d)||!h||!h->play_region)return 1;
    if(d->buffer)h->play_region(h->context,d->buffer,start,n);return 0;
}
