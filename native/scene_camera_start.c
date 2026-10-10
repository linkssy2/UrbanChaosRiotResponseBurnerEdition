#include "scene_camera_start.h"
int uc_scene_camera_start(UCSceneCamera*c,const int32_t*id,void*owner,
    UCEntityNames*(*current)(void*),UCSceneCameraStack*stack){
    if(!c||!id||!current||!stack||(c->mode!=0&&c->mode!=2))return 1;
    if(c->mode==2)return 0;
    UCEntityNames*names=current(owner);if(uc_entity_names_bind(names,"CAMERA",*id))return 2;
    names=current(owner);if(uc_entity_names_bind(names,"SHAKEABLE_CAMERA",*id))return 2;
    if(c->source_id!=-1)c->mode=2;if(c->target_id!=-1)c->mode=2;
    if(c->mode!=2){c->mode=1;if(uc_scene_camera_stack_activate(stack,c,-1))return 2;}
    return 0;
}
