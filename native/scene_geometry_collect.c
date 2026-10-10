#include "scene_geometry_collect.h"
#include <stddef.h>
static int sector_valid(const UCSceneGeometrySector *s){
    if(!s||!s->identity||s->count>4096||(s->count>0&&!s->models))return 0;
    for(int32_t i=0;i<s->count;++i)if(!s->models[i])return 0;
    return 1;
}
static void sector_collect(UCSceneGeometrySector *s,void *c,void *m,
    const UCSceneGeometryHooks *h,void *o){
    h->append(o,c,s->identity,0,0,0,0.0f);
    for(int32_t i=0;i<s->count;++i)h->model(o,s->models[i],c,m);
}
int uc_scene_sector_collect(UCSceneGeometrySector *s,void *c,void *m,
    const UCSceneGeometryHooks *h,void *o){
    if(!c||!h||!h->append||!h->model||!sector_valid(s))return 1;
    sector_collect(s,c,m,h,o);return 0;
}
int uc_scene_geometry_collect(UCSceneGeometry *g,uint8_t *mode,void *c,
    void *m,const UCSceneGeometryHooks *h,void *o){
    const UCSceneGeometryConfig *config;
    if(!g||!g->identity||!mode||!c||!h||!h->camera||!h->config||
       !h->filter_flags||!h->append||!h->model||g->background||g->occluder||
       g->count>4096||(g->count>0&&!g->sectors))return 1;
    for(int32_t i=0;i<g->count;++i)if(!sector_valid(g->sectors+i))return 1;
    (void)h->camera(o);
    h->append(o,c,g->identity,0,7,0x51,-1.0f);
    config=h->config(o,c);
    if(config->filter){
        config=h->config(o,c);
        if(!(h->filter_flags(o,config->filter)&1))return 0;
    }
    *mode=1;
    for(int32_t i=0;i<g->count;++i)
        if(g->enabled)sector_collect(g->sectors+i,c,m,h,o);
    *mode=0;return 0;
}
