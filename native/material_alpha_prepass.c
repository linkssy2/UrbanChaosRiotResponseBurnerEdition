#include "material_alpha_prepass.h"
#include "point_light_pixel.h"
#include <string.h>
int uc_material_alpha_prepass_prepare(const UCMaterialAsset*m,UCMaterialAlphaPrepass*out){
 static const uint8_t header[14]={0,0,0,0,0x21,0,0,1,0,255,0,0,0,0};
 if(!m||!out||m->source_flags!=0x21||m->effective_flags!=0x21||m->dependency_count!=1||
    !m->dependencies||!m->dependencies[0].kind||m->pass_count||m->setup_program[0].size)return 0;
 uint8_t expected[14];memcpy(expected,header,14);size_t n=m->setup_program[1].size;
 if(n!=0&&n!=69)return 0;expected[2]=(uint8_t)n;
 if(memcmp(m->header,expected,14))return 0;
 if(n){const uint8_t*p=m->setup_program[1].data;if(!p||p[0]!=0xC1||p[17]!=0x82||p[34]!=2||p[35]!=0xC0||p[52]!=0x82)return 0;
  for(unsigned i=0;i<4;++i){uint32_t a,b,c,d;memcpy(&a,p+1+i*4,4);memcpy(&b,p+18+i*4,4);memcpy(&c,p+36+i*4,4);memcpy(&d,p+53+i*4,4);if(a||b!=0x93||c!=0x900||d!=0x93)return 0;}}
 *out=(UCMaterialAlphaPrepass){0,(uint8_t)(n!=0)};return 1;
}
int uc_material_alpha_prepass_draw(const UCMaterialAlphaPrepass*p,const void*batch,uint32_t flag,
 const UCTextureAsset*image,const float rgb[3],UCMaterialAlphaState s,const UCMaterialAlphaHooks*h,void*c){
 float color[4];uint32_t packed;
 if(!p||p->resource_index||p->cull_none>1||!batch||!image||!image->storage||!image->mip_count||
    image->mip_count>UC_TEXTURE_MAX_MIPS||(image->source_flags&0xFF00)||!rgb||!s.dirty||!s.texture_transform||
    s.dirty==s.texture_transform||!h||!h->blend||!h->vertex||!h->pixel||!h->texture||
    !h->constant||!h->transform||!h->cull||!h->draw)return 0;
 memcpy(color,rgb,12);color[3]=1;if(!uc_point_light_constant(color,&packed))return 0;
 h->blend(c,0x40344,0x302);h->blend(c,0x40348,0x303);
 h->vertex(c,(uint8_t)(8+((uint8_t)flag!=0)));h->pixel(c);h->texture(c,image,0);
 h->constant(c,2,packed);*s.dirty|=1;*s.texture_transform=4;h->transform(c);
 if(p->cull_none)h->cull(c,0);h->draw(c,batch);if(p->cull_none)h->cull(c,0x900);
 *s.dirty|=1;*s.texture_transform=0;return 1;
}
