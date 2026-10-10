#include "scene_portal_collect.h"
#include <stddef.h>
int uc_scene_portal_collect(UCSceneGeometry *g,const int32_t *roots,uint8_t *mode,
    void *c,void *m,const UCScenePortalCollectHooks *h,void *ctx,uint32_t budget){
    if(!g||!g->identity||!g->occluder||g->background||!roots||!mode||!c||!h||
       !h->geometry.camera||!h->geometry.config||!h->geometry.filter_flags||
       !h->geometry.append||!h->geometry.model||!h->occlusion||
       !h->matrices.context||!h->matrices.upload||!budget||g->count>4096||
       (g->count>0&&!g->sectors))return 1;
    UCScenePortalOwner *p=g->occluder;
    if(p->failed||!p->graph||p->graph->active!=1||!p->graph->rooms||
       !p->graph->room_count||p->graph->room_count>256||!p->start||!p->planes||!p->visible||
       p->visible->failed||!p->movement_ambiguity||*roots<0||(uint32_t)*roots>p->graph->room_count)return 1;
    for(uint32_t i=0;i<p->graph->room_count;i++)if(!p->graph->rooms[i])return 1;
    for(int32_t i=0;i<g->count;i++){
        UCSceneGeometrySector *s=g->sectors+i;
        if(!s->identity||s->count>4096||(s->count>0&&!s->models))return 1;
        for(int32_t j=0;j<s->count;j++)if(!s->models[j])return 1;
    }
    UCSceneMatrixProvider *provider=h->geometry.camera(ctx);
    h->geometry.append(ctx,c,g->identity,0,7,0x51,-1.f);
    const UCSceneGeometryConfig *config=h->geometry.config(ctx,c);
    if(config->filter){config=h->geometry.config(ctx,c);
        if(!(h->geometry.filter_flags(ctx,config->filter)&1))return 0;}
    *mode=1;
    config=h->geometry.config(ctx,c);
    uint8_t occlusion=h->occlusion(ctx,config);
    if(occlusion){
        if(uc_scene_matrix_flush(provider,&h->matrices))goto failed;
        if(p->active&&!p->freeze){
            if(uc_scene_portal_move(p->graph,provider->camera,p->start,p->movement_ambiguity)||
               uc_scene_portal_visibility(p->graph,p->start,p->planes,
                  provider->camera,provider->view,provider->composite,
                  p->visible,budget))goto failed;
        }
        h->geometry.append(ctx,c,g->identity,1,6,0x10,1000000000.f);
    }
    for(int32_t i=0;i<g->count;i++){
        if(occlusion&&i<*roots&&p->active&&!p->graph->rooms[i]->flags[1])continue;
        if(g->enabled&&uc_scene_sector_collect(g->sectors+i,c,m,&h->geometry,ctx))goto failed;
    }
    *mode=0;return 0;
failed:p->failed=1;return 2;
}
