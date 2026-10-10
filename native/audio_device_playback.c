#include "audio_device_playback.h"
static int valid(const UCAudioDeviceBuffer*d,const UCAudioPlaybackServices*h){return d&&!d->stream&&!d->stream_helper&&h&&h->preflight;}
int uc_audio_voice_start(UCAudioBuffer*v,const UCAudioPlaybackServices*h){
 UCAudioDeviceBuffer*d;uint32_t repeat;
 if(!v)return 1;d=v->device;if(!d)return 0;repeat=v->word48&1u;
 if(!valid(d,h)||!h->play||h->preflight(h->context,d,0,repeat))return 1;
 d->flags|=repeat;
 if(d->buffer&&(d->flags&0x3000000u)==0x3000000u)h->play(h->context,d->buffer,repeat);
 return 0;
}
int uc_audio_device_stop(UCAudioDeviceBuffer*d,const UCAudioPlaybackServices*h){
 if(!valid(d,h)||!h->stop||h->preflight(h->context,d,1,0))return 1;
 if(d->buffer)h->stop(h->context,d->buffer);return 0;
}
int uc_audio_voice_stop(UCAudioBuffer*v,uint32_t flags,const UCAudioPlaybackServices*h){
 (void)flags;if(!v)return 1;if(!v->device)return 0;
 return uc_audio_device_stop(v->device,h); /*252B60 flags unused without helper*/
}
int uc_audio_device_status(UCAudioDeviceBuffer*d,uint32_t*out,const UCAudioPlaybackServices*h){
 if(!d)return 1;if(!out)return 0;
 if(!valid(d,h)||!h->status||h->preflight(h->context,d,2,0))return 1;
 *out=0;if(d->buffer)h->status(h->context,d->buffer,out);return 0;
}
