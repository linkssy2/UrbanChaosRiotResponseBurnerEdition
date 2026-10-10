#include "scene_ambient.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace {
void require(bool ok){if(!ok)throw std::runtime_error("Native scene ambient source domain");}
uint32_t word(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
float scalar(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
void unpack(uint32_t color,float *rgba){for(unsigned k=0;k<4;k++)rgba[k]=float((color>>(k==3?24:16-k*8))&255)*(1.f/255.f);}
}
NativeSceneAmbient::NativeSceneAmbient(const UCLevel&level,const void*geo,size_t size){
    float base[4];unpack(color_,base);require(uc_environment_ambient_init(&owner_,base)!=0);
    for(uint32_t i=0;i<level.entity_count;i++){
        const auto&e=level.entities[i];if(strcmp(e.class_name,"AmbientRegionController"))continue;
        require(e.extra.size==32&&e.properties.size==16);uint16_t type,count;
        memcpy(&type,e.extra.data+4,2);memcpy(&count,e.extra.data+6,2);require(type==1&&!count);
        Region r;float p[4];memcpy(p,e.properties.data,16);memcpy(r.bounds,e.extra.data+8,24);
        require(uc_environment_ambient_region_init(&r.state,p,p+3)!=0);regions_.push_back(r);
    }
    require(geo&&size>=464);const auto*bytes=(const uint8_t*)geo;uint32_t count=word(bytes+32);
    require(count<=65536&&size>=464+size_t(count)*176);
    for(uint32_t i=0;i<count;i++){
        const auto*p=bytes+464+size_t(i)*176;if(word(p)!=3)continue;
        Radial r;unpack(word(p+80),r.state.color);r.state.multiplier=scalar(p+96);r.state.attenuation=scalar(p+84);
        for(unsigned k=0;k<3;k++)r.center[k]=scalar(p+64+k*4);r.center[3]=1;radials_.push_back(r);
    }
    // Point/spot virtual+90 is exact zero. Directional is outside this manager
    // query list; only source radial contributions change the ambient sample.
    query_.reserve(radials_.size());for(auto&r:radials_)query_.push_back({3,&r.state,r.center});
}
void NativeSceneAmbient::step(const float position[3],float delta,uint32_t mode){
    require(position&&std::isfinite(delta)&&delta>=0);for(unsigned k=0;k<3;k++)require(std::isfinite(position[k]));
    memcpy(position_,position,12);delta_=delta;mode_=mode;
    static const UCEnvironmentAmbientRegionHooks region_hooks={
        [](void*p){auto&s=*(NativeSceneAmbient*)p;uint32_t inside=0;auto&b=s.regions_[s.selected_].bounds;require(uc_environment_ambient_region_contains(b,b+3,s.position_,&inside)!=0);return inside;},
        [](void*p,float*out){memcpy(out,((NativeSceneAmbient*)p)->position_,12);},
        [](void*p,float*out){float rgba[4];unpack(((NativeSceneAmbient*)p)->color_,rgba);memcpy(out,rgba,12);},
        [](void*p,const float*v){require(uc_environment_ambient_set(&((NativeSceneAmbient*)p)->owner_,UC_AMBIENT_INITIAL,v)!=0);},
        [](void*p,const float*v){require(uc_environment_ambient_set(&((NativeSceneAmbient*)p)->owner_,UC_AMBIENT_TARGET,v)!=0);}};
    static const UCEnvironmentAmbientHooks hooks={[](void*p)->void*{return p;},[](void*p){return ((NativeSceneAmbient*)p)->delta_;},
        [](void*p){return ((NativeSceneAmbient*)p)->mode_;},[](void*p,void*,uint32_t color){((NativeSceneAmbient*)p)->color_=color;}};
    for(selected_=0;selected_<regions_.size();selected_++)require(uc_environment_ambient_region_update(&regions_[selected_].state,&region_hooks,this)!=0);
    require(uc_environment_ambient_update(&owner_,&hooks,this)!=0);
}
void NativeSceneAmbient::sample(const float eye[3],float exposure,float out[4])const{
    require(out&&std::isfinite(exposure)&&exposure>0);uint32_t color;
    require(!uc_scene_ambient_query(color_,query_.data(),uint32_t(query_.size()),eye,&color));
    float rgba[4];unpack(color,rgba);for(unsigned k=0;k<4;k++)out[k]=rgba[k]/exposure;
}
