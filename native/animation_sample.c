#include "animation_sample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int explicit_key(const UCAnimationSample *s,uint32_t f,uint32_t t) {
 size_t stride=(s->clip.track_count+7u)/8;
 return !(s->omitted[(size_t)f*stride+t/8]&(1u<<(t%8)));
}
void uc_animation_sample_reset(UCAnimationSample *s) {
 if(!s)return;
 s->interval=INT32_MIN+1;s->fraction=0;
 for(uint32_t t=0;t<s->clip.track_count;t++)
  s->history[t]=(UCAnimationRotationHistory){s->clip.key_count,0,0,0};
}
void uc_animation_sample_free(UCAnimationSample *s) {
 if(!s)return;
 uc_animation_clip_free(&s->clip);free(s->omitted);free(s->history);
 free(s->lower);free(s->upper);memset(s,0,sizeof(*s));
}
UCAnimationStatus uc_animation_sample_load(const void *p,size_t n,UCAnimationSample *out) {
 UCAnimationSample s={0};UCAnimationStatus status;size_t bytes;
 if(!out)return UC_ANIMATION_ARGUMENT;
 memset(out,0,sizeof(*out));status=uc_animation_clip_load(p,n,&s.clip);
 if(status!=UC_ANIMATION_OK)return status;
 bytes=(size_t)((s.clip.track_count+7u)/8)*s.clip.key_count;
 s.omitted=(uint8_t *)calloc(bytes,1);
 s.history=(UCAnimationRotationHistory *)calloc(s.clip.track_count,sizeof(*s.history));
 s.lower=(UCAnimationKey *)calloc(s.clip.track_count,sizeof(*s.lower));
 s.upper=(UCAnimationKey *)calloc(s.clip.track_count,sizeof(*s.upper));
 if(!s.omitted||!s.history||!s.lower||!s.upper){uc_animation_sample_free(&s);return UC_ANIMATION_MEMORY;}
 if(s.clip.source_flags&8)
  memcpy(s.omitted,(const uint8_t *)p+42+(size_t)s.clip.track_count*4,bytes);
 uc_animation_sample_reset(&s);*out=s;return UC_ANIMATION_OK;
}
static void seek(UCAnimationSample *s,uint32_t frame) {
 const int forward=(int32_t)frame==s->interval+1;
 const int reverse=(int32_t)frame==s->interval-1;
 uint32_t count=s->clip.key_count;
 /* ponytail: scan authored keys per affected track, as the original does;
  * add precomputed explicit-key links only if playback profiling requires it. */
 for(uint32_t t=0;t<s->clip.track_count;t++) {
  UCAnimationRotationHistory *h=s->history+t;
  if(forward) {
   if(h->next_key<=frame){h->lower_sample=h->upper_sample;h->lower_key=h->next_key;}
  } else if(reverse) {
   if(h->lower_key>frame){h->upper_sample=h->lower_sample;h->next_key=h->lower_key;}
  } else if(h->lower_key>frame||h->next_key<=frame) {
   h->lower_key=frame+1;h->next_key=frame;
  }
  if(!forward && h->lower_key>frame) {
   uint32_t f=frame;
   while(f && !explicit_key(s,f,t))--f;
   h->lower_key=f;h->lower_sample=f;
  }
  if(!reverse) {
   uint32_t next=frame+1;
   if(explicit_key(s,next,t)) {
    if(forward||h->next_key<=frame)h->upper_sample=next;
    if(h->next_key<=frame)h->next_key=next;
   } else if(!forward)h->upper_sample=h->lower_sample;
   if(h->next_key<=frame) {
    for(next=frame+2;next<count&&!explicit_key(s,next,t);next++){}
    h->next_key=next;
   }
  }
 }
 s->interval=(int32_t)frame;
}
UCAnimationStatus uc_animation_sample_phase(UCAnimationSample *s,float phase) {
 uint32_t low=0,high;
 if(!s||!s->clip.keys||!s->history||!s->omitted||!s->lower||!s->upper||
    !isfinite(phase)||phase<0||phase>=1)return UC_ANIMATION_ARGUMENT;
 high=s->clip.key_count-1;
 while(low+1<high){uint32_t mid=low+(high-low)/2;if(s->clip.key_times[mid]<=phase)low=mid;else high=mid;}
 if((int32_t)low!=s->interval)seek(s,low);
 s->fraction=(phase-s->clip.key_times[low])/(s->clip.key_times[low+1]-s->clip.key_times[low]);
 for(uint32_t t=0;t<s->clip.track_count;t++) {
  const UCAnimationRotationHistory *h=s->history+t;size_t tracks=s->clip.track_count;
  s->lower[t]=s->clip.keys[(size_t)low*tracks+t];
  s->upper[t]=s->clip.keys[(size_t)(low+1)*tracks+t];
  memcpy(s->lower[t].rotation,s->clip.keys[(size_t)h->lower_sample*tracks+t].rotation,16);
  memcpy(s->upper[t].rotation,s->clip.keys[(size_t)h->upper_sample*tracks+t].rotation,16);
 }
 return UC_ANIMATION_OK;
}
