#include "model_render.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int eligible(const UCModelBatch *b){return b->light_index==UINT16_MAX&&b->flags[1]!=0;}
void uc_model_render_free(UCModelRender *r){
 if(!r)return;free(r->vertices);free(r->vectors);free(r->indices);free(r->skinned_source);free(r->ranges);free(r->draw);free(r->world_valid);free(r->world);free(r->bone_rows);memset(r,0,sizeof(*r));
}
static UCModelRenderStatus initialize(const UCModel *m,const uint8_t *draw,UCModelRender *out,uint8_t lighting){
 if(!m||!draw||!out||!m->bones||!m->vertices||!m->skin||!m->indices||!m->batches||
    (m->palette_update_count&&!m->palette_updates)||!m->bone_count||m->bone_count>4096||
    !m->vertex_count||m->vertex_count>1000000||m->index_count>8000000||m->batch_count>100000||m->palette_update_count>100000)
  return UC_MODEL_RENDER_ARGUMENT;
 UCModelRender r={0};r.model=m;r.lighting=lighting;
 for(uint32_t i=0;i<m->batch_count;i++){
  const UCModelBatch *b=m->batches+i;
  if(draw[i]>1||b->material_index>=m->material_count||b->first_index>m->index_count||
     b->index_count>m->index_count-b->first_index||b->index_count%3||
     b->first_palette_update>m->palette_update_count||b->palette_update_count>m->palette_update_count-b->first_palette_update)
   return UC_MODEL_RENDER_ARGUMENT;
  if(!lighting&&!eligible(b))continue;
  for(uint32_t j=0;j<b->palette_update_count;j++){
   const UCModelPaletteUpdate *p=m->palette_updates+b->first_palette_update+j;
   if(p->bone>=m->bone_count||p->row_offset< -96||p->row_offset>93)return UC_MODEL_RENDER_ARGUMENT;
  }
  if(!draw[i]||!b->index_count)continue;
  if(b->index_count>8000000-r.vertex_count)return UC_MODEL_RENDER_LIMIT;
  r.vertex_count+=b->index_count;r.range_count++;
  for(uint32_t j=0;j<b->index_count;j++){
   uint32_t v=m->indices[b->first_index+j];if(v>=m->vertex_count)return UC_MODEL_RENDER_ARGUMENT;
   for(unsigned k=0;k<3;k++)if(!isfinite(m->vertices[v].position[k])||m->skin[v].palette_rows[k]< -96||m->skin[v].palette_rows[k]>93)return UC_MODEL_RENDER_ARGUMENT;
   for(unsigned k=0;k<2;k++)if(!isfinite(m->vertices[v].uv[k]))return UC_MODEL_RENDER_ARGUMENT;
  }
 }
 r.index_count=r.vertex_count;
 size_t bytes=(size_t)r.vertex_count*(sizeof(*r.vertices)+sizeof(*r.indices)+(lighting?sizeof(*r.vectors):0))+
  (size_t)m->vertex_count*sizeof(uint32_t)+(size_t)r.range_count*sizeof(*r.ranges)+m->batch_count+(size_t)m->bone_count*113;
 if(bytes>256u*1024u*1024u)return UC_MODEL_RENDER_LIMIT;
 r.vertices=(UCModelRenderVertex*)calloc(r.vertex_count?r.vertex_count:1,sizeof(*r.vertices));
 if(lighting)r.vectors=(UCModelRenderVectors*)calloc(r.vertex_count?r.vertex_count:1,sizeof(*r.vectors));
 r.indices=(uint32_t*)calloc(r.index_count?r.index_count:1,sizeof(*r.indices));
 r.skinned_source=(uint32_t*)malloc((size_t)m->vertex_count*sizeof(uint32_t));
 r.ranges=(UCModelRenderRange*)calloc(r.range_count?r.range_count:1,sizeof(*r.ranges));
 r.draw=(uint8_t*)malloc(m->batch_count?m->batch_count:1);
 r.world_valid=(uint8_t*)malloc(m->bone_count);
 r.world=(float(*)[16])malloc((size_t)m->bone_count*64);r.bone_rows=(float(*)[12])malloc((size_t)m->bone_count*48);
 if(!r.vertices||(lighting&&!r.vectors)||!r.indices||!r.skinned_source||!r.ranges||!r.draw||!r.world_valid||!r.world||!r.bone_rows){uc_model_render_free(&r);return UC_MODEL_RENDER_MEMORY;}
 memcpy(r.draw,draw,m->batch_count);uint32_t at=0,range=0;
 for(uint32_t i=0;i<m->batch_count;i++){
  const UCModelBatch *b=m->batches+i;if((!lighting&&!eligible(b))||!draw[i]||!b->index_count)continue;
  r.ranges[range++]=(UCModelRenderRange){at,b->index_count,b->material_index,i};
  for(uint32_t j=0;j<b->index_count;j++,at++){
   const UCSceneVertex *v=m->vertices+m->indices[b->first_index+j];
   memcpy(r.vertices[at].uv,v->uv,8);r.vertices[at].color=v->source_color;r.indices[at]=at;
  }
 }
 *out=r;return UC_MODEL_RENDER_OK;
}
UCModelRenderStatus uc_model_render_init(const UCModel*m,const uint8_t*d,UCModelRender*r){return initialize(m,d,r,0);}
UCModelRenderStatus uc_model_render_init_lighting(const UCModel*m,const uint8_t*d,UCModelRender*r){return initialize(m,d,r,1);}
static float component(uint32_t raw,unsigned width){int sign=1<<(width-1),n=(int)(raw&((1u<<width)-1));if(n&sign)n-=2*sign;return n==-sign?-1.f:(float)n/(float)(sign-1);}
static void skin_vector(uint32_t packed,const uint8_t bytes[2],const float rows[3][12],float out[3]){
 float n[3]={component(packed,11),component(packed>>11,11),component(packed>>22,10)};
 float w[3]={bytes[0]/255.f,bytes[1]/255.f,0};w[2]=1.f-(w[0]+w[1]);
 for(unsigned r=0;r<3;r++){float b[3];for(unsigned k=0;k<3;k++){unsigned i=r*4+k;float a=w[0]*rows[0][i];float c=w[1]*rows[1][i]+a;b[k]=w[2]*rows[2][i]+c;}out[r]=n[0]*b[0]+n[1]*b[1]+n[2]*b[2];}
}
static UCModelRenderStatus prepare_vertices(UCModelRender*r,const UCModelRenderPalette*prior_palette){
 const UCModel*m=r->model;
 if(prior_palette)r->palette=*prior_palette;else memset(&r->palette,0,sizeof(r->palette));
 uint32_t at=0;
 for(uint32_t i=0;i<m->batch_count;i++){
  const UCModelBatch *b=m->batches+i;if(!r->lighting&&!eligible(b))continue;
  for(uint32_t j=0;j<b->palette_update_count;j++){
   const UCModelPaletteUpdate *p=m->palette_updates+b->first_palette_update+j;unsigned row=(unsigned)(96+p->row_offset);
   if(!r->world_valid[p->bone])return UC_MODEL_RENDER_POSE;
   memcpy(r->palette.rows+row,r->bone_rows[p->bone],48);memset(r->palette.valid+row,1,3);
  }
  if(!r->draw[i])continue;
  memset(r->skinned_source,0xff,(size_t)m->vertex_count*sizeof(uint32_t));
  for(uint32_t j=0;j<b->index_count;j++,at++){
   uint32_t v=m->indices[b->first_index+j];const UCModelSkin *skin=m->skin+v;float rows[3][12];
   uint32_t previous=r->skinned_source[v];
   if(previous!=UINT32_MAX){
    memcpy(r->vertices[at].position,r->vertices[previous].position,12);
    if(r->lighting)r->vectors[at]=r->vectors[previous];
    continue;
   }
   r->skinned_source[v]=at;
   for(unsigned k=0;k<3;k++){
    unsigned row=(unsigned)(96+skin->palette_rows[k]);
    for(unsigned a=0;a<3;a++){
     if(!r->palette.valid[row+a])return UC_MODEL_RENDER_PALETTE;
     for(unsigned c=0;c<4;c++)if(!isfinite(r->palette.rows[row+a][c]))return UC_MODEL_RENDER_PALETTE;
    }
    memcpy(rows[k],r->palette.rows+row,48);
   }
   uc_model_skin_position(m->vertices[v].position,skin->weight_bytes,rows,r->vertices[at].position);
   for(unsigned k=0;k<3;k++)if(!isfinite(r->vertices[at].position[k]))return UC_MODEL_RENDER_POSE;
   if(r->lighting){
    UCModelRenderVectors *vout=r->vectors+at;const UCSceneVertex *vin=m->vertices+v;
    skin_vector(vin->packed_normal,skin->weight_bytes,rows,vout->normal);
    skin_vector(vin->packed_basis[0],skin->weight_bytes,rows,vout->basis5);
    skin_vector(vin->packed_basis[1],skin->weight_bytes,rows,vout->basis6);
    for(unsigned k=0;k<3;k++)if(!isfinite(vout->normal[k])||!isfinite(vout->basis5[k])||!isfinite(vout->basis6[k]))return UC_MODEL_RENDER_POSE;
   }
  }
 }
 r->ready=1;return UC_MODEL_RENDER_OK;
}
UCModelRenderStatus uc_model_render_update(UCModelRender *r,const float (*local)[16],const float root[16],
 const float (*prior_world)[16],const UCModelRenderPalette *prior_palette){
 if(!r)return UC_MODEL_RENDER_ARGUMENT;r->ready=0;
 if(!r->model||!root||!r->world||!r->world_valid||!r->bone_rows)return UC_MODEL_RENDER_ARGUMENT;
 const UCModel *m=r->model;UCModelPoseStatus status;
 if(prior_world){
  if(prior_world!=r->world)memcpy(r->world,prior_world,(size_t)m->bone_count*64);
  status=uc_model_pose_world_retained(m,local,root,r->world);memset(r->world_valid,1,m->bone_count);
 }else status=uc_model_pose_world_partial(m,local,root,r->world,r->world_valid);
 if(status!=UC_POSE_OK||uc_model_pose_palette_selected(m,r->world,r->world_valid,r->bone_rows)!=UC_POSE_OK)return UC_MODEL_RENDER_POSE;
 return prepare_vertices(r,prior_palette);
}
UCModelRenderStatus uc_model_render_update_lighting(UCModelRender*r,const UCModelRender*pose,const UCModelRenderPalette*seed){
 if(!r)return UC_MODEL_RENDER_ARGUMENT;r->ready=0;
 if(!r->lighting||!r->model||!pose||!pose->ready||pose->model!=r->model||!r->vectors||r==pose)return UC_MODEL_RENDER_ARGUMENT;
 memcpy(r->world,pose->world,(size_t)r->model->bone_count*64);
 memcpy(r->world_valid,pose->world_valid,r->model->bone_count);
 memcpy(r->bone_rows,pose->bone_rows,(size_t)r->model->bone_count*48);
 return prepare_vertices(r,seed);
}
