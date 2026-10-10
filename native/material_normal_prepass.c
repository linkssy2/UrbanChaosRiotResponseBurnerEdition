#include "material_normal_prepass.h"
#include "point_light_pixel.h"
#include <math.h>
#include <string.h>
int uc_material_normal_prepass_prepare(const UCMaterialAsset*m,UCMaterialNormalPrepass*out){
 if(!m||!out||(m->effective_flags&~10u)||m->header[8]==255||m->header[8]>=m->dependency_count||
    m->header[9]!=255||!m->dependencies||!m->dependencies[m->header[8]].kind)return 0;
 UCMaterialAsset view=*m;view.header[8]=255;view.effective_flags&=~8u;UCMaterialNormalPrepass p={0};
 if(!uc_material_prepass_prepare(&view,&p.setup))return 0;
 p.resource_index=m->header[8];*out=p;return 1;
}
int uc_material_normal_prepass_draw(const UCMaterialNormalPrepass*p,const void*batch,uint32_t flag,
 const UCTextureAsset*image,const float rgb[3],UCMaterialNormalCache cache,
 const UCMaterialNormalPrepassHooks*h,void*c,const float tint[4],float(*scale)(void*)){
 float rgba[4],weight;uint32_t packed;uint8_t normal;
 if(!p||p->setup.cull_none>1||!batch||!image||!image->storage||!image->mip_count||
    image->mip_count>UC_TEXTURE_MAX_MIPS||(image->source_flags&0xFF00)||!rgb||
    !cache.flags||!cache.weight||!isfinite(*cache.weight)||!h||!h->vertex||!h->texture||
    !h->pixel||!h->constant||!h->transform||!h->cull||!h->draw||!h->weight)return 0;
 memcpy(rgba,rgb,12);rgba[3]=1;if(!uc_point_light_constant(rgba,&packed))return 0;
 if(p->setup.emission){if(!tint||!scale)return 0;for(unsigned i=0;i<4;++i)if(!isfinite(tint[i]))return 0;}
 weight=h->weight(c);if(!isfinite(weight))return 0;normal=weight!=0;
 h->vertex(c,(uint8_t)(((uint8_t)flag!=0)|(normal?16:0)));
 if(normal)h->texture(c,image,0);h->pixel(c,normal);
 if(normal){
  float value[4]={0,1,0,0};
  if(!(*cache.flags&1)){*cache.flags|=1;*cache.weight=h->weight(c);}
  value[3]=*cache.weight;if(!uc_point_light_constant(value,&packed))return 0;
  h->constant(c,1,packed);
 }
 if(p->setup.emission){
  float input=scale(c),emission=(float)p->setup.emission*(1.0f/255.0f),added=emission/input;
  for(unsigned i=0;i<4;++i)rgba[i]=rgba[i]*tint[i]+added;
 }
 if(!uc_point_light_constant(rgba,&packed))return 0;
 h->constant(c,2,packed);h->transform(c);if(p->setup.cull_none)h->cull(c,0);
 h->draw(c,batch);if(p->setup.cull_none)h->cull(c,0x900);return 1;
}
