#include "animating_prop.h"
UCAnimationStatus uc_animating_prop_controller(const UCAnimationClip *clip,UCAnimationController *out){
    UCAnimationController s;UCAnimationStatus result=uc_animation_controller_init(clip,0xc0000009u,&s);
    if(!out)return UC_ANIMATION_ARGUMENT;if(result!=UC_ANIMATION_OK)return result;
    s.repeats=0;s.blend_out=-1.f;*out=s;return UC_ANIMATION_OK;
}
int uc_animating_prop_idle(uint8_t done,uint8_t loop,uint8_t *finished,int(*apply)(void *),void *owner){
    if(!finished||(loop&&!apply))return 1;if(!done)return 0;
    if(!*finished)*finished=1;if(loop){if(apply(owner))return 2;*finished=0;}return 0;
}
