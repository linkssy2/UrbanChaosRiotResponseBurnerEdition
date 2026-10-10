#include "material_ordinary_alpha.h"
#include "material_blend_prepass.h"
#include "point_light_pixel.h"
#include <string.h>
int uc_material_ordinary_alpha_prepare(const UCMaterialAsset*m,const UCNativeMaterial*n,UCMaterialOrdinaryAlpha*out){
 UCMaterialPrepass base;float rgba[4];uint32_t argb,destination;int constant,cull;
 if(!out||!n||!uc_material_blend_prepass_prepare(m,&base)||
    (m->setup_program[0].size!=104&&m->setup_program[0].size!=172&&m->setup_program[0].size!=206)||n->unsupported_reason||
    n->blend_mode!=UC_NATIVE_MATERIAL_BLEND_ALPHA||n->vertex_exposure!=1||n->geometry||n->alpha_mode||n->color_write||
    n->texture_selection||n->depth_bias||n->address_u||n->address_v||!n->texture_name||
    strcmp(n->texture_name,m->dependencies[0].name)||m->passes[0].dependency_count!=1||m->passes[0].dependency_indices[0])return 0;
 constant=m->setup_program[0].size==206;cull=m->setup_program[0].size!=104;
 if(n->kind!=(constant?UC_NATIVE_MATERIAL_TEXTURE_VERTEX_CONSTANT:UC_NATIVE_MATERIAL_TEXTURE_VERTEX_COLOR)||
    n->cull_mode!=(cull?UC_NATIVE_MATERIAL_CULL_NONE:UC_NATIVE_MATERIAL_CULL_INHERIT)||base.cull_none!=cull)return 0;
 memcpy(&destination,m->setup_program[0].data+(constant?69:cull?35:1),4);if(destination!=0x303)return 0;
 if(constant){
  memcpy(rgba,m->setup_program[0].data+1,16);
  for(unsigned i=0;i<4;++i)if(!(rgba[i]>=0&&rgba[i]<=1))return 0;
  if(rgba[3]!=1||!uc_point_light_constant(rgba,&argb)||argb!=n->constant_argb)return 0;
 }else if(n->constant_argb!=0xffffffffu)return 0;
 *out=(UCMaterialOrdinaryAlpha){n};return 1;
}
int uc_material_ordinary_alpha(const UCMaterialOrdinaryAlpha*p,const void*image,const void*batch,
 UCMaterialOrdinaryAlphaState s,const UCMaterialOrdinaryAlphaHooks*h,void*c){
 UCMaterialEmptyState a=s.common;uint32_t bits;const UCMaterialEmptyContext*x;
 if(!p||!p->material||!image||!batch||!a.alpha_active||!a.bias_bits||!a.dirty||!a.lod_bits||!s.graphics_texture_id||
    a.dirty==a.lod_bits||a.dirty==a.bias_bits||a.lod_bits==a.bias_bits||a.dirty==s.graphics_texture_id||a.lod_bits==s.graphics_texture_id||
    !h||!h->transform||!h->context||!h->texture_id||!h->pixel||!h->constant0||!h->cull||!h->state||!h->texture||!h->vertex||!h->draw)return 0;
 h->transform(c);bits=*a.alpha_active?0:*a.bias_bits;x=h->context(c);if(!x)return 0;
 if(x->override_enabled){x=h->context(c);if(!x)return 0;bits=x->lod_bits;}
 else{uint32_t id=h->texture_id(c,image);if(id==*s.graphics_texture_id)bits=0xC0000000;}
 *a.dirty|=1;*a.lod_bits=bits;
 if(p->material->kind==UC_NATIVE_MATERIAL_TEXTURE_VERTEX_CONSTANT){h->pixel(c,p->material);h->constant0(c,p->material->constant_argb);}
 if(p->material->cull_mode==UC_NATIVE_MATERIAL_CULL_NONE)h->cull(c,0);
 h->state(c,0x40348,0x303);h->state(c,0x40344,0x302);h->state(c,0x40304,1);
 h->texture(c,image,0);h->vertex(c,p->material);h->pixel(c,p->material);h->draw(c,batch,p->material);
 if(p->material->cull_mode==UC_NATIVE_MATERIAL_CULL_NONE)h->cull(c,0x900);
 h->state(c,0x40304,1);h->state(c,0x40344,0x306);h->state(c,0x40348,0);return 1;
}
