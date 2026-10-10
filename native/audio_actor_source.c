#include "audio_actor_source.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
int uc_audio_actor_play(const UCAudioActorView*a,const UCAudioActorBank*b,const char*name,
 const void*argument,float volume,const UCAudioActorServices*h,uint8_t*accepted){
 if(!a||!accepted)return 1;
 if(!b){*accepted=0;return 0;}
 if(!a->helper||!a->position||!b->resource.identity||!b->observer||!name||!h||!h->preflight||!h->allocate||!h->source||
    !h->source->current_audio||!h->source->factory||!h->source->create||!h->source->position||!h->source->event||!h->source->request||
    !isfinite(volume)||volume>32768.f||(_mm_getcsr()&0xE040u)||h->preflight(h->context,a,b))return 1;
 UCAudioSourceController*s=*a->helper;
 if(s&&(!s->active||s->busy))return 1;
 if(!s){s=h->allocate(h->context);if(!s)return 2;memset(s,0,sizeof*s);
  (void)uc_audio_source_begin(s,a->position);s->position.attached_position=a->position;*a->helper=s;}
 s->busy=1;
 if(uc_audio_source_ensure(s,h->source)){s->busy=0;return 2;}
 s->flag25=0;
 if(!s->engine||b->resource.type!=104){s->busy=0;*accepted=0;return 0;}
 uint32_t index=h->source->event(h->source->context,&b->resource,name);
 if(index==UINT32_MAX){s->busy=0;*accepted=0;return 0;}
 UCAudioActorOverride override={argument,volume<0?65535u:(uint16_t)(int32_t)((double)volume*65535.)};
 *b->observer=&override;
 int failed=!s->engine||h->source->request(h->source->context,s->engine,&b->resource,index);
 *b->observer=NULL;s->busy=0;
 if(failed)return 2;*accepted=1;return 0;
}
