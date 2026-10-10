#include "material_aux_prepass.h"
#include "point_light_pixel.h"
#include <math.h>
#include <string.h>
int uc_material_aux_prepass_prepare(const UCMaterialAsset*m,UCMaterialAuxPrepass*out){
 if(!m||!out||m->header[9]==255||m->header[9]>=m->dependency_count||
    !m->dependencies||!m->dependencies[m->header[9]].kind)return 0;
 /* Read-only adapter for the SAME selected setup; no second resource owner. */
 UCMaterialAsset view=*m;view.header[9]=255;UCMaterialAuxPrepass p={0};
 if(!uc_material_prepass_prepare(&view,&p.setup))return 0;
 p.resource_index=m->header[9];*out=p;return 1;
}
int uc_material_aux_prepass_draw(const UCMaterialAuxPrepass*p,const void*batch,uint32_t flag,
 const UCTextureAsset*image,const float rgb[3],const UCMaterialAuxPrepassHooks*h,void*c,
 const float tint[4],float(*scale)(void*)){
 float rgba[4];uint32_t packed;
 if(!p||p->setup.cull_none>1||!batch||!image||!image->storage||!image->mip_count||
    image->mip_count>UC_TEXTURE_MAX_MIPS||(image->source_flags&0xFF00)||!rgb||!h||
    !h->vertex||!h->texture||!h->pixel||!h->constant||!h->transform||!h->cull||!h->draw)return 0;
 memcpy(rgba,rgb,12);rgba[3]=1;if(!uc_point_light_constant(rgba,&packed))return 0;
 if(p->setup.emission){if(!tint||!scale)return 0;for(unsigned i=0;i<4;++i)if(!isfinite(tint[i]))return 0;}
 h->vertex(c,(uint8_t)flag!=0);h->texture(c,image,0);h->pixel(c);
 if(p->setup.emission){
  float input=scale(c),emission=(float)p->setup.emission*(1.0f/255.0f),added=emission/input;
  for(unsigned i=0;i<4;++i)rgba[i]=rgba[i]*tint[i]+added;
  if(!uc_point_light_constant(rgba,&packed))return 0;
 }
 h->constant(c,2,packed);h->transform(c);if(p->setup.cull_none)h->cull(c,0);
 h->draw(c,batch);if(p->setup.cull_none)h->cull(c,0x900);return 1;
}
