#include "audio_buffer_properties.h"
#include <float.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
int uc_audio_buffer_set(UCAudioBuffer*b,uint32_t p,const void*v,const UCAudioBufferProperties*h,int*out){
    if(!b||!out)return 1;
    if(!((p>=3&&p<=7)||p==9||p==11)){*out=0;return 0;}
    if(!v)return 1;
    int effect=b->device&&((p>=3&&p<=6)||p==11);
    if(effect&&(!h||!h->preflight||!h->device_parameter||h->preflight(h->context,b,p,v)))return 1;
    UCAudioSpatial*s=b->spatial;
    if(s){
        if(p==3)memcpy(&s->mode,v,4);
        if(p==4)memcpy(s->range,v,8);
        if(p==5)memcpy(s->position,v,12);
        if(p==6)memcpy(s->velocity,v,12);
        if(p==7)s->flags=(s->flags&~2u)|((*(const uint8_t*)v&1u)<<1);
    }
    if(p==9)memcpy(&b->priority,v,2);
    if(effect&&h->device_parameter(h->context,b->device,p,v))return 2;
    *out=1;return 0;
}
int uc_audio_buffer_get(const UCAudioBuffer*b,uint32_t p,void*v,int*out){
    if(!b||!out)return 1;
    const UCAudioSpatial*s=b->spatial;
    if(p==1){*out=1;return 0;}
    if((p==3||p==4)&&s){if(!v)return 1;memcpy(v,p==3?(const void*)&s->mode:(const void*)s->range,p==3?4:8);}
    else if(p==9){if(!v)return 1;memcpy(v,&b->priority,2);}
    else{*out=0;return 0;}
    *out=1;return 0;
}
int uc_audio_device_spatial(UCAudioDeviceBuffer*d,uint32_t p,const void*v,const UCAudioSpatialEffects*h){
    if(!d||p<3||p>6)return 1;
    if(!(d->flags&4u))return 0;
    if(!v||!h||!h->preflight||!h->send||(_mm_getcsr()&0xe040)||h->preflight(h->context,d,p,v))return 1;
    if(p==3){
        uint32_t mode;memcpy(&mode,v,4);if(mode>2)mode=0;
        if(d->buffer)h->send(h->context,d->buffer,0,UC_AUDIO_SPATIAL_MODE,&mode);
        if(d->stream)h->send(h->context,d->stream,1,UC_AUDIO_SPATIAL_MODE,&mode);
    }else if(p==4){
        float r[2];memcpy(r,v,8);if(isnan(r[0])||isnan(r[1]))return 0;
        if(r[0]<.01f)r[0]=.01f;if(r[0]>=r[1])r[1]=r[0]+.01f;
        const float maximum=FLT_MAX,curve[2]={1,0};
        if(d->buffer){
            h->send(h->context,d->buffer,0,UC_AUDIO_SPATIAL_MAXIMUM,&maximum);
            h->send(h->context,d->buffer,0,UC_AUDIO_SPATIAL_MINIMUM,r);
            h->send(h->context,d->buffer,0,UC_AUDIO_SPATIAL_MAXIMUM,r+1);
            h->send(h->context,d->buffer,0,UC_AUDIO_SPATIAL_CURVE,curve);
        }
        if(d->stream){
            h->send(h->context,d->stream,1,UC_AUDIO_SPATIAL_MAXIMUM,&maximum);
            h->send(h->context,d->stream,1,UC_AUDIO_SPATIAL_MINIMUM,r);
            h->send(h->context,d->buffer,0,UC_AUDIO_SPATIAL_MAXIMUM,r+1);
            h->send(h->context,d->stream,1,UC_AUDIO_SPATIAL_CURVE,curve);
        }
    }else{
        float a[3];memcpy(a,v,12);if(isnan(a[0])||isnan(a[1])||isnan(a[2]))return 0;
        unsigned op=p==5?UC_AUDIO_SPATIAL_POSITION:UC_AUDIO_SPATIAL_VELOCITY;
        if(d->buffer)h->send(h->context,d->buffer,0,op,a);
        if(d->stream){memcpy(a,v,12);h->send(h->context,d->stream,1,op,a);}
    }
    return 0;
}
