#include "scene_projection_begin.h"
int uc_scene_projection_begin(UCSceneProjection*p,void *(*current)(void *),void*context){
    if(!p||!current)return 1;
    p->parameters.near_z=25;p->parameters.far_z=50000;
    p->parameters.depth_low=0;p->parameters.depth_high=1;
    p->parameters.flags[0]=1;p->parameters.flags[1]=0;p->dirty=1;
    p->viewport=current(context);p->alternate_enabled=0;return 0;
}
