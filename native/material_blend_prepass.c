#include "material_blend_prepass.h"
#include "point_light_pixel.h"
#include <math.h>
#include <string.h>
static int literal(const uint8_t*p,uint8_t op,uint32_t v){
    if(p[0]!=op)return 0;
    for(unsigned i=0;i<4;++i){uint32_t x;memcpy(&x,p+1+i*4,4);if(x!=v)return 0;}return 1;
}
static int setting(const uint8_t*p,uint8_t op,uint32_t v,uint32_t state){
    return literal(p,op,v)&&literal(p+17,0x82,state);
}
int uc_material_blend_prepass_prepare(const UCMaterialAsset*m,UCMaterialPrepass*out){
    static const uint8_t tail[10]={3,0,0,1,255,255,255,1,0,0};
    const uint8_t*p;uint32_t n;int cull=0;
    if(!m||!out||memcmp(m->header+4,tail,10)||m->source_flags!=3||m->effective_flags!=0x143||
       m->dependency_count!=1||!m->dependencies||!m->dependencies[0].kind||m->pass_count!=1||!m->passes)return 0;
    n=m->setup_program[0].size;p=m->setup_program[0].data;
    if(!p||(n!=104&&n!=138&&n!=172&&n!=206)||
       ((uint32_t)m->header[0]|((uint32_t)m->header[1]<<8))!=n)return 0;
    if(n==138||n==206){if(p[0]!=0xC1||!literal(p+17,0x86,0))return 0;p+=34;n-=34;}
    if(n==172){if(!setting(p,0xC1,0,0x93))return 0;p+=34;n-=34;cull=1;}
    if((!setting(p,0xC0,0x303,0x3F)&&!setting(p,0xC0,1,0x3F))||
       !setting(p+34,0xC0,0x302,0x3E)||!setting(p+68,0xC0,1,0x3B)||p[102]!=2)return 0;
    p+=103;n-=103;
    if(cull){if(!setting(p,0xC0,0x900,0x93))return 0;p+=34;n-=34;}
    if(n!=1||*p!=4)return 0;
    n=m->setup_program[1].size;p=m->setup_program[1].data;
    if(n!=(cull?69u:0u)||((uint32_t)m->header[2]|((uint32_t)m->header[3]<<8))!=n)return 0;
    if(n&&(!p||!setting(p,0xC1,0,0x93)||p[34]!=2||!setting(p+35,0xC0,0x900,0x93)))return 0;
    *out=(UCMaterialPrepass){(uint8_t)cull,255};return 1;
}
int uc_material_blend_prepass_draw(const UCMaterialPrepass*p,const void*batch,uint32_t flag,
    const float rgb[3],const UCMaterialPrepassHooks*h,void(*blend)(void*,uint32_t,uint32_t),
    void*c,const float tint[4],float(*scale)(void*)){
    float rgba[4];uint32_t packed;
    if(!p||p->cull_none>1||p->emission!=255||!batch||!rgb||!h||!h->vertex||!h->pixel||
       !h->constant||!h->transform||!h->cull||!h->draw||!blend||!tint||!scale)return 0;
    memcpy(rgba,rgb,12);rgba[3]=1;
    if(!uc_point_light_constant(rgba,&packed))return 0;
    for(unsigned i=0;i<4;++i)if(!isfinite(tint[i]))return 0;
    blend(c,0x40344,0x302);blend(c,0x40348,0x303);
    return uc_material_prepass_draw_emission(p,batch,flag,rgb,h,c,tint,scale);
}
