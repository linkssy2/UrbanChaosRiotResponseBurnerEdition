#include "material_prepass.h"
#include "point_light_pixel.h"
#include <math.h>
#include <string.h>
static int literal(const uint8_t *p,uint8_t op,uint32_t value){
    if(p[0]!=op)return 0;
    for(unsigned i=0;i<4;++i){uint32_t x;memcpy(&x,p+1+i*4,4);if(x!=value)return 0;}return 1;
}
static int setting(const uint8_t*p,uint8_t op,uint32_t value,uint8_t action,uint32_t state){
    return literal(p,op,value)&&literal(p+17,action,state);
}
int uc_material_prepass_prepare(const UCMaterialAsset*m,UCMaterialPrepass*out){
    UCMaterialPrepass p={0};UCMaterialSpan s;
    if(!m||!out||(m->effective_flags&~2u)||m->header[8]!=255||m->header[9]!=255)return 0;
    p.emission=m->header[10];
    s=m->setup_program[m->setup_program[0].size?0:1];
    if(!s.size){*out=p;return 1;}
    if(!s.data||m->pass_count!=1||!m->passes)return 0;
    /* Original bool1 discards literal float4 / pixel-constant0 setup completely. */
    if(s.size>=34&&s.data[0]==0xC1&&literal(s.data+17,0x86,0)){s.data+=34;s.size-=34;}
    if(s.size==1&&s.data[0]==2){*out=p;return 1;}
    if(s.size==69&&s.data[34]==2){
        if(setting(s.data,0xC1,0,0x82,0x93)&&setting(s.data+35,0xC0,0x900,0x82,0x93))p.cull_none=1;
        else if(setting(s.data,0xC1,3,0x85,1)&&setting(s.data+35,0xC0,1,0x85,1)){}
        else return 0;
    }else if(s.size==137&&s.data[68]==2&&
        setting(s.data,0xC1,3,0x85,0)&&setting(s.data+34,0xC1,3,0x85,1)&&
        setting(s.data+69,0xC0,1,0x85,0)&&setting(s.data+103,0xC0,1,0x85,1)){}
    else return 0;
    *out=p;return 1;
}
int uc_material_prepass_draw(const UCMaterialPrepass*p,const void*batch,uint32_t flag,
    const float rgb[3],const UCMaterialPrepassHooks*h,void*c){
    if(!p||p->emission)return 0;
    return uc_material_prepass_draw_emission(p,batch,flag,rgb,h,c,NULL,NULL);
}
int uc_material_prepass_draw_emission(const UCMaterialPrepass*p,const void*batch,uint32_t flag,
    const float rgb[3],const UCMaterialPrepassHooks*h,void*c,const float tint[4],float(*scale)(void*)){
    float rgba[4];uint32_t packed;
    if(!p||p->cull_none>1||!batch||!rgb||
       !h||!h->vertex||!h->pixel||!h->constant||!h->transform||!h->cull||!h->draw)return 0;
    memcpy(rgba,rgb,12);rgba[3]=1;
    if(!uc_point_light_constant(rgba,&packed))return 0;
    if(p->emission){if(!tint||!scale)return 0;for(unsigned i=0;i<4;++i)if(!isfinite(tint[i]))return 0;}
    h->vertex(c,(uint8_t)flag!=0);h->pixel(c);
    if(p->emission){
        float input=scale(c),emission=(float)p->emission*(1.0f/255.0f),added=emission/input;
        for(unsigned i=0;i<4;++i)rgba[i]=rgba[i]*tint[i]+added;
        if(!uc_point_light_constant(rgba,&packed))return 0;
    }
    h->constant(c,2,packed);h->transform(c);
    if(p->cull_none)h->cull(c,0);
    h->draw(c,batch);
    if(p->cull_none)h->cull(c,0x900);
    return 1;
}

