#include "light_fitting.h"
#include "model_shadow_casters.h"
#include <DirectXMath.h>
#include <cmath>
#include <cstring>
#include <string>
using namespace DirectX;
static XMMATRIX matrix(const float*p){return XMLoadFloat4x4((const XMFLOAT4X4*)p);}
static bool rigid(const float*m){
    for(unsigned i=0;i<16;i++)if(!std::isfinite(m[i])||std::fabs(m[i])>1e6f)return false;
    if(m[3]!=0||m[7]!=0||m[11]!=0||std::fabs(m[15]-1)>.000001f)return false;
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++){float d=0;for(unsigned k=0;k<3;k++)d+=m[4*i+k]*m[4*j+k];if(std::fabs(d-(i==j?1.f:0.f))>.0001f)return false;}
    return XMVectorGetX(XMMatrixDeterminant(matrix(m)))>0;
}
int NativeLightFitting::construct(NativeRenderer&r,NativeMissionWorld&w,uint32_t i,const UCArchive*a,const uint8_t hash[32],const std::filesystem::path&catalog,NativeIntegrationLighting&lights,UCGameRandom&random){
    if(ready_||entity.aux||i>=w.source_count()||!a||!hash||random.first>=250||random.second>=250)return 1;
    const auto&m=w.metadata(i);const auto*d=m.definition;
    if(!d||std::strcmp(d->class_name,"LightFitting")||!m.asset_present||*w.active_descriptor()!=&m||!rigid(d->transform)||d->script_count||m.condition_count)return 1;
    const uint8_t*cursor=*w.property_cursor(i);UCLightFittingParameters p;
    if(uc_light_fitting_parameters(d,&cursor,&p)||p.health!=100||p.value!=1||p.linked_entity<0||!lights.fitting_base((uint32_t)p.linked_entity,base_intensity_))return 1;
    const bool off=p.initial_state==1&&p.flicker_mode==2&&std::isfinite(p.period)&&p.period>0;
    if(!off&&(p.initial_state||p.flicker_mode||p.period!=1))return 1;
    std::string name;for(const char*c=d->asset_name;*c;c++){
        if(*c=='/'||*c=='\\'){if(name.empty()||name.back()!='\\')name+='\\';}
        else name+=(*c>='A'&&*c<='Z')?(char)(*c+'a'-'A'):*c;
    }
    const bool fire=name=="assets_maps\\lightfittings\\01-01\\fire.mtxt";
    if(off){if(!fire&&name!="assets_maps\\lightfittings\\01-01\\emergency.mtxt")return 1;}
    else if(name!="assets_maps\\lightfittings\\01-01\\hlight.mtxt"&&name!="assets_maps\\lightfittings\\01-01\\slamp.mtxt"&&name!="assets_maps\\lightfittings\\01-01\\warew.mtxt")return 1;
    std::string collision=name.substr(0,name.find_last_of('.'))+".mtxthk";BytesOwner bytes;auto entry=find_resource(a,collision.c_str());UCModelCollisionAsset asset{};
    if(entry==SIZE_MAX||uc_archive_read(a,entry,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK||uc_model_collision_load(bytes.value.data,bytes.value.size,1,&asset))return 1;
    const uint32_t groups=asset.group_count;UCModelCollisionShape shape{};unsigned found=0;bool bad=groups!=(fire?1u:3u);
    for(uint32_t n=0;n<asset.shape_count&&!bad;n++)if(asset.shapes[n].group==0){bad=asset.shapes[n].object||uc_rigid_prop_box(&asset.shapes[n],&shape);found++;}
    uc_model_collision_free(&asset);if(bad||found!=(fire?0u:1u))return 1;
    if(uc_light_fitting_values(groups,1,1,off?0.f:1.f,base_intensity_,&values_))return 1;
    entry=find_resource(a,"Banks\\Objects\\LightFitting.amp");
    if(entry==SIZE_MAX||uc_archive_read(a,entry,UC_AR_LOCAL_PAYLOAD,&sound_bank_.value)!=UC_AR_OK||!sound_bank_.value.size)return 1;
    try{
        if(fire){
            // This exact Fire group has a real model but no ordinary-pass batch.
            // Keep the shared drawable-model loader's stricter guard unchanged.
            BytesOwner model_bytes;entry=find_resource(a,d->asset_name);
            if(entry==SIZE_MAX||uc_archive_read(a,entry,UC_AR_LOCAL_PAYLOAD,&model_bytes.value)!=UC_AR_OK||uc_model_load_group(model_bytes.value.data,model_bytes.value.size,0,&model_.source))return 1;
            const auto&m0=model_.source;if(m0.vertex_count!=24||m0.index_count!=96||m0.batch_count!=1||m0.bone_count!=2||m0.batches[0].light_index!=UINT16_MAX||m0.batches[0].flags[0]||m0.batches[0].flags[1])return 1;
            model_.materials=load_materials(catalog,hash,a,m0.material_names,m0.material_count,UC_NATIVE_MATERIAL_GEOMETRY_SKIN3,false,r);
            const uint8_t draw=1;if(uc_model_render_init(&m0,&draw,&model_.prepared)||model_.prepared.index_count)return 1;
        }else model_.load(r,a,hash,catalog,d->asset_name,nullptr,nullptr,nullptr,values_.group);
    }catch(...){return 1;}
    uint32_t model_groups;std::memcpy(&model_groups,model_.source.source_header+8,4);if(model_groups!=groups)return 1;
    for(const auto&material:model_.materials)if(!material.supported())return 1;
    if(uc_entity_health_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return 2;
    *w.property_cursor(i)=cursor;parameters_=p;entity.actor.health9C=p.health;if(p.health>entity.actor.maximumA0)entity.actor.maximumA0=p.health;
    if(uc_light_fitting_delay(&random,&initial_delay_))return 2;
    entity.model=&model_;entity.flags=(entity.flags&~0x100u)|(values_.group?0x100u:0);
    float identity[16]{};for(unsigned n=0;n<4;n++)identity[n*5]=1;
    try{if(fire){if(uc_model_render_update(&model_.prepared,nullptr,identity,nullptr,nullptr))return 2;}else model_.bind_pose(r,identity);}catch(...){return 2;}
    const XMMATRIX swap=XMMatrixSet(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1);
    XMMATRIX world=swap*matrix(entity.transform)*swap;world.r[3]=XMVectorMultiply(world.r[3],XMVectorSet(.01f,.01f,.01f,1));
    if(found){UCSolidBlockGeometry b{};XMMATRIX composed=matrix(shape.transform)*world;XMStoreFloat3((XMFLOAT3*)b.center,composed.r[3]);XMStoreFloat4((XMFLOAT4*)b.rotation,XMQuaternionNormalize(XMQuaternionRotationMatrix(composed)));std::memcpy(b.half,shape.dimensions,12);b.radius=shape.radius;boxes_.push_back(b);shapes_.push_back(shape);}
    lighting_=&lights;if(!lights.set_fitting_intensity((uint32_t)p.linked_entity,values_.intensity))return 2;
    ready_=true;return 0;
}
int NativeLightFitting::update(){
    if(!ready_)return 1;
    if((entity.flags&0x100)!=(values_.group?0x100u:0)||entity.actor.health9C!=parameters_.health||entity.helper)return 2;
    return uc_entity_fresh_aux_clear(entity.aux)?2:0;
}
void NativeLightFitting::draw(NativeRenderer&r,const float*vp,float exposure,NativeModelLighting *lighting,const NativeLightingFrame *frame){if(!ready_||!model_.prepared.index_count)return;XMFLOAT4X4 m;XMStoreFloat4x4(&m,matrix(entity.transform)*matrix(vp));const float tint[4]={1,1,1,1};if(lighting&&frame)model_.draw_lit(r,*lighting,*frame,entity.transform,&m._11,tint,nullptr,exposure);else model_.draw(r,&m._11,tint,nullptr,exposure);}
void NativeLightFitting::collect_shadows(NativeRenderer&r,NativeModelLighting&lighting,const NativeLightingFrame&frame,NativeModelShadowCasters&casters,float){
    if(!ready_||!NativeModelShadowCasters::eligible(model_,entity.secondary))return;model_.present(r,1);
    casters.add(model_,entity.transform,entity.secondary,lighting.prepare_membership(model_,frame,entity.transform),frame);
}
int NativeLightFitting::close_detached(){
    // PC shutdown restores the immutable authored lighting policy; no claim of
    // original manager unregister/176430 empty-contribution behavior.
    if(lighting_&&!lighting_->set_fitting_intensity((uint32_t)parameters_.linked_entity,base_intensity_))return 1;
    lighting_=nullptr;ready_=false;entity.model=nullptr;boxes_.clear();shapes_.clear();
    if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;return 0;
}
