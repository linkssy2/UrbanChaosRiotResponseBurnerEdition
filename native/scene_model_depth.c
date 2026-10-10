#include "scene_model_depth.h"
#include <limits.h>
#include <string.h>
static int range(uint32_t first,uint32_t count,uint32_t limit){return first<=limit&&count<=limit-first;}
static void bias(const UCSceneModelDepthState*s,const UCSceneModelDepthHooks*h,void*c,uint32_t value){
    float unit=-(float)value,slope=unit*.25f;uint32_t u,v;memcpy(&u,&unit,4);memcpy(&v,&slope,4);
    h->render_state(c,0x4D,v);h->render_state(c,0x4E,u);h->render_state(c,0x4F,value!=0);h->render_state(c,0x50,value!=0);h->render_state(c,0x51,value!=0);*s->depth_bias=value;
}
int uc_scene_model_depth(UCSceneModelRender*m,uint32_t query,uint32_t partition,uint32_t special,const void*pass,const UCSceneModelDepthState*s,const UCSceneModelDepthHooks*h,void*c){
    const UCScene*a;const UCSceneModel*n;uint8_t q=(uint8_t)query,sp=(uint8_t)special;
    if(!m||!m->scene||!m->asset||m->indexed!=1||m->modifier||(partition&&partition!=UINT32_MAX)||
       !s||!s->submitted||!s->depth_bias||s->submitted==s->depth_bias||!h||!h->query_begin||!h->query_end||
       !h->stream||!h->render_state||!h->material_flags||!h->draw)return 0;
    a=m->scene;n=m->asset;
    if(n->batch_count>INT32_MAX||!range(n->first_batch,n->batch_count,a->batch_count)||
       !range(n->first_vertex,n->vertex_count,a->vertex_count)||!range(n->first_index,n->index_count,a->index_count)||
       !range(n->first_material,n->material_count,a->material_count)||(n->vertex_count&&!a->vertices)||
       (n->index_count&&!a->indices)||(n->batch_count&&!a->batches))return 0;
    for(uint32_t i=0;i<n->batch_count;++i){const UCSceneBatch*b=a->batches+n->first_batch+i;
        if(!range(b->material_index-n->first_material,1,n->material_count)||!range(b->first_index-n->first_index,b->index_count,n->index_count))return 0;}
    if(q&&!partition)m->visibility_subject=(const void*)(uintptr_t)h->query_begin(c);
    ++*s->submitted;
    if(n->batch_count){
        h->stream(c,0,n->vertex_count?m:NULL,16);h->stream(c,2,n->vertex_count?m:NULL,8);h->stream(c,4,NULL,8);
        for(uint32_t i=0;i<n->batch_count;++i){const UCSceneBatch*b=a->batches+n->first_batch+i;int extra=b->light_index!=UINT16_MAX;
            if(extra&&!s->indexed_extra)continue;
            if(extra)bias(s,h,c,1);
            if((!q&&!sp)||b->flags[1]){uint32_t flags=h->material_flags(c,b->material_index);
                if(!q||(flags&1u)==(uint32_t)(partition!=0))h->draw(c,m,b,sp!=0,pass);}
            if(extra)bias(s,h,c,0);
        }
    }
    if(q&&!partition)h->query_end(c,(uint32_t)(uintptr_t)m->visibility_subject);
    return 1;
}
