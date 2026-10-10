#include "animation_playback.h"
#include <math.h>
#include <string.h>

UCAnimationStatus uc_animation_playback_sample(const UCAnimationController *controller,
    UCAnimationSample *sample,const uint8_t *enabled,uint32_t bones,UCAnimationBlend *blend) {
 if(!controller)return UC_ANIMATION_ARGUMENT;
 if(!(controller->flags&0x80000000u)||controller->weight<=0)return UC_ANIMATION_OK;
 if(!sample||!enabled||!blend||!bones||bones>4096||
    !isfinite(controller->weight)||!isfinite(controller->rate)||controller->rate<0||
    !isfinite(controller->phase)||controller->phase<0||controller->phase>=1||
    !sample->clip.source_tracks||!sample->clip.keys||!sample->clip.key_times||
    !sample->history||!sample->omitted||!sample->lower||!sample->upper||
    !sample->clip.track_count||sample->clip.track_count>4096)return UC_ANIMATION_ARGUMENT;
 uint16_t needed[4096];memset(needed,0,bones*sizeof(*needed));
 for(uint32_t t=0;t<sample->clip.track_count;t++) {
  int32_t id=sample->clip.source_tracks[t];uint32_t bone=(uint32_t)(id<0?~id:id);
  if(bone>=bones)return UC_ANIMATION_ARGUMENT;
  if(enabled[bone])needed[bone]+=2;
 }
 for(uint32_t b=0;b<bones;b++) {
  if(blend[b].count>blend[b].capacity||(blend[b].capacity&&!blend[b].contributions)||
     !isfinite(blend[b].accumulated)||blend[b].accumulated<0||blend[b].accumulated>1)
   return UC_ANIMATION_ARGUMENT;
  if(needed[b]>blend[b].capacity-blend[b].count)return UC_ANIMATION_LIMIT;
 }
 UCAnimationStatus status=uc_animation_sample_phase(sample,controller->phase);
 if(status!=UC_ANIMATION_OK)return status;
 for(uint32_t b=0;b<bones;b++)uc_animation_blend_stage(blend+b,controller->weight);
 float factor=uc_animation_controller_root_factor(controller);
 for(uint32_t t=0;t<sample->clip.track_count;t++) {
  int32_t id=sample->clip.source_tracks[t];uint32_t bone=(uint32_t)(id<0?~id:id);
  if(!enabled[bone])continue;
  float root=(int32_t)t==sample->clip.root_track?factor:1.0f;
  if(sample->fraction==0)uc_animation_blend_add(blend+bone,sample->lower+t,controller->weight,root);
  else {
   uc_animation_blend_add(blend+bone,sample->lower+t,(1.0f-sample->fraction)*controller->weight,root);
   uc_animation_blend_add(blend+bone,sample->upper+t,sample->fraction*controller->weight,root);
  }
 }
 return UC_ANIMATION_OK;
}
