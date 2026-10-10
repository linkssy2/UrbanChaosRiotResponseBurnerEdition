#include "scene_model_render.h"
#include <math.h>
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static int finite_values(const float*p,unsigned n){if(!p)return 0;for(unsigned i=0;i<n;++i)if(!isfinite(p[i]))return 0;return 1;}
static int range(uint32_t start,uint32_t count,uint32_t total){return start<=total&&count<=total-start;}
int uc_scene_model_render_init(const UCSceneModels*models,uint32_t index,
    const void*data,size_t bytes,uint32_t(*flags)(void*,uint32_t),void*context,UCSceneModelRender*out){
    if(!models||!models->scene||!models->items||index>=models->count||!data||!out)return 0;
    const UCScene*s=models->scene;const UCSceneModel*a=models->items+index;const uint8_t*geo=data;
    if(a->source_offset>bytes||a->source_bytes>bytes-a->source_offset||
       a->group_offset<a->source_offset||a->group_offset-a->source_offset>a->source_bytes||
       a->source_bytes-(a->group_offset-a->source_offset)<168||
       !range(a->first_batch,a->batch_count,s->batch_count)||!range(a->first_material,a->material_count,s->material_count)||
       (a->batch_count&&(!s->batches||!flags))||!finite_values((const float*)&a->bounds,12))return 0;
    const uint8_t*g=geo+a->group_offset,*node=g+56;
    if(word(g)!=1||word(node+4)||word(node+20)||word(node+24)||word(node+28)||
       word(node+12)!=a->batch_count||memcmp(g+4,a->bounds.minimum,12)||memcmp(g+16,a->bounds.maximum,12))return 0;
    for(uint32_t i=0;i<a->batch_count;++i){
        const UCSceneBatch*b=s->batches+a->first_batch+i;
        if(b->node_flags||b->material_index<a->first_material||b->material_index-a->first_material>=a->material_count)return 0;
    }
    UCSceneModelRender r={0};r.scene=s;r.asset=a;r.bounds=a->bounds;r.flags=0x400;r.indexed=1;r.alpha_all_bit40=1;
    for(unsigned i=0;i<16;++i)r.world[i]=i%5==0?1.0f:0.0f;
    for(uint32_t i=0;i<a->batch_count;++i){
        const UCSceneBatch*b=s->batches+a->first_batch+i;
        if(b->light_index==UINT16_MAX){uint32_t f=flags(context,b->material_index);if(f&1){r.alpha_present=1;if(!(f&0x40))r.alpha_all_bit40=0;}}
    }
    *out=r;return 1;
}
static int multiply(float out[16],const float a[16],const float b[16]){
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
        out[r*4+c]=(a[r*4]*b[c]+a[r*4+1]*b[4+c])+(a[r*4+2]*b[8+c]+a[r*4+3]*b[12+c]);
    return finite_values(out,16);
}
int uc_scene_model_collect(UCSceneModelRender*s,const UCSceneModelCollectView*v,
    const UCSceneModelCollectHooks*h,void*context){
    if(!s||!v||!h||!s->scene||!s->asset||s->indexed!=1||s->modifier||
       s->bounds_valid>1||s->alpha_present>1||s->alpha_all_bit40>1)return 0;
    if(v->filter_present&&!(v->filter_flags&3))return 1;
    if(!h->append||(v->manager&&!h->membership)||!finite_values(s->world,16)||
       !finite_values((const float*)&s->bounds,12)||!finite_values(v->clip_matrix,16))return 0;
    float matrix[16],view[16],depth=0;uint32_t visible;
    if(!multiply(matrix,s->world,v->clip_matrix)||uc_model_visibility(matrix,&s->bounds,&visible)!=UC_MODEL_OK)return 0;
    int draw=visible||(s->flags&0x40);
    if(draw){
        if(!finite_values(v->view_matrix,16)||!finite_values(v->projection_matrix,16)||
           !multiply(view,s->world,v->view_matrix)||!multiply(matrix,view,v->projection_matrix))return 0;
        depth=(s->bounds.center[0]*matrix[2]+s->bounds.center[1]*matrix[6])+(s->bounds.center[2]*matrix[10]+matrix[14]);
        depth+=s->flags&0x10?-1000000000.0f:(s->flags&0x20?1000000000.0f:0.0f);
        if(!isfinite(depth))return 0;
    }
    s->bounds_valid=1;if(v->reset_visibility)s->visibility_subject=NULL;
    if(!draw)return 1;
    UCSceneModelRecord record={s,0,1,0x3D,depth};h->append(context,&record);
    if(s->alpha_present){
        record.partition=UINT32_MAX;record.group=s->flags&0x80?6:(v->group_mode?4:5);
        record.flags=0x13D|(s->alpha_all_bit40?0x200:0);record.depth=0.0f-depth;h->append(context,&record);
    }
    if(v->manager)h->membership(context,s,v->manager);
    return 1;
}
