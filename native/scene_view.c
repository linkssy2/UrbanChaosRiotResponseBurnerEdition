#include "scene_view.h"
#include "firstperson_projection.h"
#include "point_light_receiver.h"
#include <string.h>
#include <xmmintrin.h>
static int perspective(UCSceneProjection*p,float fov,const UCSceneViewServices*s,float*out){
    void *viewport=p->viewport;
    float aspect=viewport?s->aspect(s->owner,viewport):1.33333337306976318359375f;
    return uc_firstperson_projection(fov,aspect,p->parameters.near_z,p->parameters.far_z,
        p->parameters.depth_low,p->parameters.depth_high,out)==0;
}
static int cached(UCSceneProjection*p,const UCSceneViewServices*s,float*out){
    if(p->dirty){float value[16];if(!perspective(p,p->fov,s,value))return 0;
        memcpy(p->cache,value,64);p->dirty=0;}
    memcpy(out,p->cache,64);return 1;
}
int uc_scene_view_construct(UCSceneView*v,UCSceneProjection*p,uint32_t clear,
                           uint32_t flags,const UCSceneViewServices*s){
    static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    if(!v||!s||!s->preflight||!s->fallback_viewport||
       (p&&(!s->camera_world||!s->aspect))||(_mm_getcsr()&0xE040u)||
       !s->preflight(s->owner,p))return 1;
    v->parameters.near_z=.1f;v->parameters.far_z=1;
    v->parameters.depth_low=0;v->parameters.depth_high=1;
    v->parameters.flags[0]=1;v->parameters.flags[1]=0;v->clear=clear;
    if(!p){v->viewport=s->fallback_viewport(s->owner);
        memcpy(v->world,identity,64);memcpy(v->inverse,identity,64);
        memcpy(v->projection,identity,64);memcpy(v->alternate,identity,64);v->flags=0;
    }else{
        const float *world;v->viewport=p->viewport;
        if(!v->viewport)v->viewport=s->fallback_viewport(s->owner);
        world=s->camera_world(s->owner);if(!world)return 2;
        memcpy(v->world,world,64);memcpy(v->inverse,v->world,64);
        if(!uc_point_light_inverse(v->inverse,v->inverse))return 2;
        if(!cached(p,s,v->projection))return 2;
        memcpy(&v->parameters,&p->parameters,sizeof(v->parameters));
        if(p->alternate_enabled){if(!perspective(p,p->alternate_fov,s,v->alternate))return 2;}
        else if(!cached(p,s,v->alternate))return 2;
        v->flags=flags;
    }
    return 0;
}
