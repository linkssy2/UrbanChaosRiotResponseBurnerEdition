#include "audio_resident_voice.h"
int uc_audio_resident_voice(UCAudioInstance*i,UCAudioInstanceTrack*t,UCAudioCommand*c,
                          const UCAudioResidentEffects*h,uint8_t*bound){
    const UCAudioResidentWave*w;const UCAudioDeviceFormat*f;
    UCAudioBuffer*v,*parent;UCAudioDeviceBuffer*d;
    if(!i||!t||!c||!h||!bound||!h->preflight)return 1;
    if((t->state.flags&0x1200u)||(t->definition&&(t->definition[4]&2u))){*bound=1;return 0;}
    if(!i->bank||!h->resource||!h->voice||!h->auxiliary_gain||!h->sound_parameter||
       !h->sample_pitch||!h->route||!h->pitch||!h->gain||!h->filter||!h->device||
       !h->device->format||!h->device->data||!h->device->play_region||
       h->preflight(h->context,i,t,c))return 1;
    w=h->resource(h->context,t,c);if(!w){*bound=0;return 0;}
    v=h->voice(h->context,i,t);if(!v||!v->device){*bound=0;return 0;}
    d=v->device;if(!w->data||!w->format){*bound=0;return 0;}
    f=w->format;
    (void)uc_audio_device_format(d,f,h->device);
    (void)uc_audio_device_data(d,w->data,w->bytes,h->device);
    (void)uc_audio_device_play_region(d,w->play_start,w->play_length,h->device);
    parent=i->buffer;if(parent&&(parent->flags&1u))h->auxiliary_gain(h->context,d,-10000);
    /* AMPC leaves command.repeat unchanged. Full2519A0 only writes an existing
     * stream helper; the admitted ordinary wrapper has none. FullBD660 is ret8. */
    if(i->sound&&(i->sound[20]&2u))h->sound_parameter(h->context,d,0);
    t->parameters[0]=(uint16_t)h->sample_pitch(h->context,i,f->rate);
    if(c->repeat==1)v->word48&=~1u;else v->word48|=1u;
    if(v->device)((UCAudioDeviceBuffer*)v->device)->flags|=v->word48;
    if(c->repeat!=1)i->flags|=0x20u;
    h->route(h->context,i,t,v,f);h->pitch(h->context,i,t);h->gain(h->context,i,t);
    h->filter(h->context,i,t,(uint8_t)((t->state.flags>>2)&1u));
    t->state.flags|=0x1000u;*bound=1;return 0;
}
