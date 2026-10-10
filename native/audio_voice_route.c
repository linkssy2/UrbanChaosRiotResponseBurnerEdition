#include "audio_voice_route.h"
static int ring(const UCAudioLink*p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
static int device(const UCAudioDeviceBuffer*d){return !d||(!d->stream&&!d->stream_helper&&d->provider&&ring(&d->provider->devices)&&ring(&d->membership)&&ring(&d->children));}
static void spatial(UCAudioDeviceBuffer*d,void*p,const UCAudioVoiceRouting*h){
    static const uint32_t pairs[4][2]={{0,0},{1,0},{10,0},{10,0}};
    d->spatial=p;
    if(p){d->flags|=0x80u;if(d->flags&0x4000000u){d->channel_mask=0x403u;if(d->buffer)h->mixbins(h->context,d->buffer,pairs);}}
    else d->flags&=~0x80u;
}
int uc_audio_voice_route(UCAudioInstance*i,UCAudioBuffer*v,const UCAudioDeviceFormat*f,const UCAudioVoiceRouting*h){
    UCAudioBuffer*p;UCAudioDeviceBuffer*d,*parent;
    if(!i||!v||!f)return 1;
    if(i->sound&&i->sound[22]==255&&i->sound[23]==255)return 0;
    p=i->buffer;
    if(!h||!h->preflight||!h->parameter||!h->output||!h->mixbins||
       (f->channels==1&&!p)||!device(v->device)||(p&&!device(p->device))||
       (p&&v->device&&v->device==p->device)||h->preflight(h->context,i,v,f))return 1;
    if(f->channels==1){
        parent=p?p->device:NULL;d=v->device;
        if(d){
            (void)uc_audio_link_remove(&d->membership);
            (void)uc_audio_link_append(parent?&parent->children:&d->provider->devices,&d->membership);
            if(parent&&(parent->flags&2u)&&d->buffer)h->parameter(h->context,d->buffer,1200);
            if(d->buffer)h->output(h->context,d->buffer,parent?parent->buffer:NULL);
        }
        p=i->buffer;
        if((p->flags&2u)&&p->device)spatial(p->device,p->spatial,h);
    }else if(p&&(p->flags&2u)){
        v->flags|=0x10000000u;p->flags|=0x10000000u;
        if(v->device)spatial(v->device,p->spatial,h);
    }
    return 0;
}
