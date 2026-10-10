#include "audio_source_controller.h"
#include <string.h>
int uc_audio_source_begin(UCAudioSourceController*s,const float point[3]){
    if(!s||!point||s->active)return 1;
    s->position.attached_position=NULL;memcpy(s->position.position,point,12);
    s->engine=NULL;s->flag24=s->flag25=s->busy=0;s->active=1;return 0;
}
int uc_audio_source_ensure(UCAudioSourceController*s,const UCAudioSourceServices*h){
    if(!s||!s->active||!s->busy||!h||!h->current_audio||!h->factory||!h->create||!h->position)return 1;
    if(!s->engine){
        void*audio=h->current_audio(h->context);
        if(audio){
            void*factory=h->factory(h->context,audio);
            if(factory){
                s->engine=h->create(h->context,factory);
                if(!s->engine||h->position(h->context,s->engine,s->position.attached_position?
                   s->position.attached_position:s->position.position))return 2;
            }
        }
    }
    return 0;
}
int uc_audio_source_play(UCAudioSourceController*s,const UCAudioSourceBank*b,
    const char*name,const UCAudioSourceServices*h,uint8_t*accepted){
    if(!s||!s->active||s->busy||!h||!h->preflight||!h->current_audio||!h->factory||
       !h->create||!h->position||!h->enable||!h->event||!h->request||!accepted||
       (b&&(!b->identity||!name))||h->preflight(h->context,s,b))return 1;
    if(!b){*accepted=0;return 0;}
    s->busy=1;
    if(!s->engine){
        if(uc_audio_source_ensure(s,h))goto failed;
        if(s->engine&&s->flag24&&h->enable(h->context,s->engine))goto failed;
    }
    s->flag25=0;
    if(!s->engine){s->busy=0;*accepted=0;return 0;}
    uint32_t index=UINT32_MAX;
    if(b->type==104){index=h->event(h->context,b,name);if(index==UINT32_MAX){s->busy=0;*accepted=0;return 0;}}
    if(!s->engine||h->request(h->context,s->engine,b,index))goto failed;
    s->busy=0;*accepted=1;return 0;
failed:s->busy=0;return 2;
}
int uc_audio_source_end(UCAudioSourceController*s,const UCAudioSourceServices*h){
    if(!s||!s->active||s->busy||!h||!h->preflight||!h->close||h->preflight(h->context,s,NULL))return 1;
    s->busy=1;
    if(s->engine&&h->close(h->context,s->engine)){s->busy=0;return 2;}
    s->busy=0;s->active=0;return 0;
}
