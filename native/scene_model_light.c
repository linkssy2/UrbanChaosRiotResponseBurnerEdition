#include "scene_model_light.h"
#include <limits.h>
#include <string.h>
static int range(uint32_t start,uint32_t count,uint32_t limit){return start<=limit&&count<=limit-start;}
static void bias(const UCSceneModelLightState*s,const UCSceneModelLightHooks*h,void*c,uint32_t value){
    float unit=-(float)value,slope=unit*.25f;uint32_t u,v;
    memcpy(&u,&unit,4);memcpy(&v,&slope,4);
    h->render_state(c,0x4D,v);h->render_state(c,0x4E,u);
    h->render_state(c,0x4F,value!=0);h->render_state(c,0x50,value!=0);h->render_state(c,0x51,value!=0);
    *s->depth_bias=value;
}
int uc_scene_model_light(const UCSceneModelRender*m,const void*light,uint32_t partition,
    const UCSceneModelLightState*s,const UCSceneModelLightHooks*h,void*c){
    const UCScene*a;const UCSceneModel*n;uint16_t index;
    if(!m||!m->scene||!m->asset||m->indexed!=1||m->modifier||!light||
       (partition&&partition!=UINT32_MAX)||!s||!s->rejected||!s->submitted||!s->depth_bias||
       s->rejected==s->submitted||s->rejected==s->depth_bias||s->submitted==s->depth_bias||
       !h||!h->visible||!h->light_index||!h->stream||!h->render_state||!h->material_flags||!h->draw)return 0;
    a=m->scene;n=m->asset;
    if(n->batch_count>INT32_MAX||!range(n->first_batch,n->batch_count,a->batch_count)||
       !range(n->first_vertex,n->vertex_count,a->vertex_count)||
       !range(n->first_index,n->index_count,a->index_count)||
       !range(n->first_material,n->material_count,a->material_count)||
       (n->vertex_count&&!a->vertices)||(n->index_count&&!a->indices)||(n->batch_count&&!a->batches))return 0;
    for(uint32_t i=0;i<n->batch_count;++i){
        const UCSceneBatch*b=a->batches+n->first_batch+i;
        if(!range(b->material_index-n->first_material,1,n->material_count)||
           !range(b->first_index-n->first_index,b->index_count,n->index_count))return 0;
    }
    if(m->flags&4)return 1;
    if(!partition&&(!m->visibility_subject||!h->visible(c,m->visibility_subject))){++*s->rejected;return 1;}
    index=(uint16_t)h->light_index(c,light);
    h->stream(c,0,n->vertex_count?m:NULL,16);
    h->stream(c,2,n->vertex_count?m:NULL,8);
    h->stream(c,3,n->vertex_count?m:NULL,8);
    h->stream(c,4,NULL,8);
    if(m->flags&0x400)bias(s,h,c,3);
    ++*s->submitted;
    for(uint32_t i=0;i<n->batch_count;++i){
        const UCSceneBatch*b=a->batches+n->first_batch+i;
        if(b->light_index==index&&b->flags[1]&&
           (h->material_flags(c,b->material_index)&1u)==(uint32_t)(partition!=0))h->draw(c,m,b,light,0,NULL);
    }
    if(m->flags&0x400)bias(s,h,c,0);
    return 1;
}
