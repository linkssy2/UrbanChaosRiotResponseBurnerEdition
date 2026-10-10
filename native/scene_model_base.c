#include "scene_model_base.h"
#include <limits.h>
static int range(uint32_t first,uint32_t count,uint32_t limit){return first<=limit&&count<=limit-first;}
int uc_scene_model_base(const UCSceneModelRender*m,uint32_t partition,uint8_t override,
    uint32_t*skipped,const UCSceneModelBaseHooks*h,void*c){
    if(!m||!m->scene||!m->asset||m->indexed!=1||m->modifier||(partition&&partition!=UINT32_MAX)||
       !skipped||!h||!h->stream||!h->material_flags||!h->draw||(!partition&&!override&&m->visibility_subject&&!h->visible))return 0;
    const UCScene*a=m->scene;const UCSceneModel*n=m->asset;
    if(n->batch_count>INT32_MAX||!range(n->first_batch,n->batch_count,a->batch_count)||
       !range(n->first_vertex,n->vertex_count,a->vertex_count)||!range(n->first_index,n->index_count,a->index_count)||
       !range(n->first_material,n->material_count,a->material_count)||(n->vertex_count&&!a->vertices)||
       (n->index_count&&!a->indices)||(n->batch_count&&!a->batches))return 0;
    for(uint32_t i=0;i<n->batch_count;i++){const UCSceneBatch*b=a->batches+n->first_batch+i;
        if(b->node_flags||!range(b->material_index-n->first_material,1,n->material_count)||!range(b->first_index-n->first_index,b->index_count,n->index_count))return 0;}
    if(!partition&&!override&&!(m->visibility_subject&&h->visible(c,m->visibility_subject))){++*skipped;return 1;}
    const UCSceneModelRender*stream=n->vertex_count?m:NULL;
    h->stream(c,1,stream,4);
    if(n->batch_count){
        h->stream(c,0,stream,16);h->stream(c,2,stream,8);h->stream(c,4,NULL,8);h->stream(c,5,stream,8);
        for(uint32_t i=0;i<n->batch_count;i++){const UCSceneBatch*b=a->batches+n->first_batch+i;
            if(b->light_index==UINT16_MAX&&b->flags[1]&&(h->material_flags(c,b->material_index)&1u)==(uint32_t)(partition!=0))h->draw(c,m,b);
        }
    }
    return 1;
}
