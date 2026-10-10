#include "scene_camera_base.h"
#include "scene_projection_begin.h"
#include "bullet_initialize.h"
int uc_scene_camera_base_construct(const UCEntityFreshContext*c,const UCEntityFreshView*v,
    UCSceneProjection*p,UCBulletMotion*m,void *(*viewport)(void *),void*owner){
    float quaternion[4];int result;
    if(!p||!m||!viewport)return 1;
    result=uc_entity_fresh_construct_parent(c,v);if(result)return result;
    if(uc_scene_projection_begin(p,viewport,owner)||
       uc_bullet_frame_quaternion(v->transform,quaternion)||
       uc_bullet_motion_reset(v->transform+12,quaternion,m))return 2;
    return 0;
}
