#include "animation_skeleton.h"
#include <math.h>
#include <string.h>

static int valid(const UCAnimationSkeleton *s) {
 return s&&s->bone_count&&s->bone_count<=4096&&s->enabled&&
  (s->root_bone==-1||(s->root_bone>=0&&(uint32_t)s->root_bone<s->bone_count));
}
UCAnimationStatus uc_animation_skeleton_finish(const UCAnimationSkeleton *s,const UCAnimationBlendMath *math,
 UCAnimationBlend *bones,UCAnimationKey *scratch) {
 if(!valid(s)||!math||!bones||!scratch)return UC_ANIMATION_ARGUMENT;
 int root=s->root_bone>=0&&s->enabled[s->root_bone];float root_limit=0,root_scale=0;
 for(uint32_t i=0;i<s->bone_count;i++) {
  const UCAnimationBlend *b=bones+i;
  if(b->count>b->capacity||(b->capacity&&!b->contributions)||!isfinite(b->accumulated)||b->accumulated<0||b->accumulated>1)
   return UC_ANIMATION_ARGUMENT;
  UCAnimationBlend result=*b;
  if(root&&i==(uint32_t)s->root_bone) {
   uc_animation_blend_stage(&result,1);root_limit=result.limit;root_scale=result.scale;
   /* Appended neutral is consumed first. Its accepted weight is exactly the
    * uncovered prior weight, so use that neutral prior without a temp node.
    * Saturated roots append nothing and retain their original prior pose. */
   if(root_scale!=0)result.pose=(UCAnimationKey){{0,0,0,-1},{0,0,0}};
  }
  UCAnimationStatus status=uc_animation_blend_finish(&result,math);
  if(status!=UC_ANIMATION_OK)return status;
  scratch[i]=result.pose;
 }
 if(root){bones[s->root_bone].limit=root_limit;bones[s->root_bone].scale=root_scale;}
 for(uint32_t i=0;i<s->bone_count;i++){bones[i].pose=scratch[i];bones[i].count=0;bones[i].accumulated=0;}
 return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_skeleton_locals(const UCAnimationSkeleton *s,const UCAnimationBlend *bones,
 float (*local)[16],float (*scratch)[16]) {
 if(!valid(s)||!bones||!local||!scratch||!s->translation_scale||!s->translation_offset)return UC_ANIMATION_ARGUMENT;
 memset(scratch[0],0,sizeof(scratch[0]));scratch[0][0]=scratch[0][6]=s->mirrored?-1.0f:1.0f;
 scratch[0][9]=scratch[0][15]=1;
 for(uint32_t i=1;i<s->bone_count;i++) {
  if(!s->enabled[i])continue;
  float scale=s->translation_scale[i];if(!isfinite(scale))return UC_ANIMATION_ARGUMENT;
  UCAnimationKey key=bones[i].pose;
  for(unsigned j=0;j<3;j++) {
   key.translation[j]*=scale;
   if(!isfinite(key.translation[j]))return UC_ANIMATION_UNSUPPORTED;
  }
  UCAnimationStatus status=uc_animation_blend_matrix(&key,scratch[i]);
  if(status!=UC_ANIMATION_OK)return status;
  if((int32_t)i!=s->root_bone)for(unsigned j=0;j<3;j++) {
   float offset=s->translation_offset[i][j];if(!isfinite(offset))return UC_ANIMATION_ARGUMENT;
   scratch[i][12+j]=offset+scratch[i][12+j];
   if(!isfinite(scratch[i][12+j]))return UC_ANIMATION_UNSUPPORTED;
  }
 }
 for(uint32_t i=0;i<s->bone_count;i++)if(!i||s->enabled[i])memcpy(local[i],scratch[i],sizeof(local[i]));
 return UC_ANIMATION_OK;
}
