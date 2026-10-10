#include "point_light_ready.h"
#include <math.h>
#include <string.h>
int uc_point_light_manager_begin(UCPointLightPipeline *p,const UCPointLightResources *r){
    if(!p||!r)return 0;
    p->color_mask=0x1010101;p->blend_enabled=1;p->source_factor=1;p->destination_factor=1;
    p->depth_bias=-100.f;p->slope_bias=-.5f;p->depth_write=0;
    for(unsigned i=0;i<4;i++)p->palette[i]=r->palette;
    return 1;
}
int uc_point_light_manager_end(UCPointLightPipeline *p){
    if(!p)return 0;p->color_mask=0x10101;p->depth_bias=0;p->slope_bias=0;p->depth_write=1;return 1;
}
int uc_point_light_ready(UCPointLightState *s,const float world[16],const float center[4],
    UCPointLightGeometry *g,uint8_t *mode,UCPointLightPipeline *p,
    const UCPointLightResources *r,uint8_t debug,int *ready){
    if(!s||!g||!mode||!p||!r||!ready||debug||!isfinite(s->attenuation)||!isfinite(s->multiplier))return 0;
    for(unsigned i=0;i<4;i++)if(!isfinite(s->color[i]))return 0;
    UCPointLightState updated=*s;UCPointLightGeometry geometry=*g;
    if(!uc_point_light_geometry_update(&updated,world,center,&geometry))return 0;
    int active=s->attenuation!=0&&s->multiplier>1.f/256.f&&
        (s->color[0]!=0||s->color[1]!=0||s->color[2]!=0||s->color[3]!=0)&&s->enabled;
    *s=updated;*g=geometry;
    if(active){
        p->texture[1]=r->volume;p->texture[2]=p->texture[3]=r->cube;
        p->coordinate_source[2]=p->coordinate_source[3]=0xE0000000;
        for(unsigned i=1;i<4;i++)for(unsigned j=0;j<3;j++)p->address[i][j]=3;
        *mode=0;p->transform_flags[1]=4;p->alpha_test=0;
    }
    *ready=active;return 1;
}
int uc_point_light_end(UCPointLightPipeline *p){
    if(!p)return 0;p->texture[1]=NULL;p->transform_flags[1]=0;p->alpha_test=0;
    for(unsigned i=0;i<4;i++){p->coordinate_source[i]=0;for(unsigned j=0;j<3;j++)p->address[i][j]=1;}
    return 1;
}
