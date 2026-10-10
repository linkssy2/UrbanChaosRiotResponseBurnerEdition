#include "control_lighting_bridge.h"
#include "control_spotlight_bridge.h"
#include "integration_lighting.h"
#include "integration_shadows.h"
#include "assets.h"
#include <d3d11_1.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
void check(HRESULT hr){require(SUCCEEDED(hr),"Native lighting D3D11.1 resource failed");}
void error_text(char *out,size_t capacity,const char *message){
    if(out&&capacity){const size_t n=std::min(capacity-1,strlen(message));memcpy(out,message,n);out[n]=0;}
}
struct SceneOwner{UCScene value{};~SceneOwner(){uc_scene_free(&value);}};
struct ModelsOwner{UCSceneModels value{};~ModelsOwner(){uc_scene_models_free(&value);}};
struct LevelOwner{UCLevel value{};~LevelOwner(){uc_level_free(&value);}};
struct ScopedPipeline {
    ID3D11DeviceContext1 *context;ComPtr<ID3DDeviceContextState> prior;
    ScopedPipeline(ID3D11DeviceContext1*c,ID3DDeviceContextState*s):context(c){context->SwapDeviceContextState(s,&prior);}
    ~ScopedPipeline(){context->ClearState();context->SwapDeviceContextState(prior.Get(),nullptr);}
};
DXGI_SAMPLE_DESC validate_target(ID3D11View *view,ID3D11Device *device,const D3D11_VIEWPORT &viewport){
    require(view!=nullptr,"Missing native lighting target");
    ComPtr<ID3D11Device> owner;view->GetDevice(&owner);require(owner.Get()==device,"Lighting target belongs to another device");
    ComPtr<ID3D11Resource> resource;view->GetResource(&resource);
    ComPtr<ID3D11Texture2D> texture;check(resource.As(&texture));D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);
    require((desc.SampleDesc.Count==1||desc.SampleDesc.Count==2||desc.SampleDesc.Count==4)&&viewport.TopLeftX>=0&&viewport.TopLeftY>=0&&viewport.Width>0&&viewport.Height>0&&
        viewport.TopLeftX+viewport.Width<=float(desc.Width)&&viewport.TopLeftY+viewport.Height<=float(desc.Height)&&
        viewport.MinDepth==0&&viewport.MaxDepth==1,"Unsupported native lighting target extent/MSAA/depth range");
    return desc.SampleDesc;
}
}
struct UCNativeControlLighting {
    DWORD thread=GetCurrentThreadId();
    NativeRenderer renderer;
    ComPtr<ID3D11DeviceContext1> context;
    ComPtr<ID3DDeviceContextState> pipeline;
    SceneOwner scene;ModelsOwner models;LevelOwner level;
    ArchiveOwner archive{nullptr,uc_archive_close};BytesOwner geometry;
    std::vector<NativeMaterial> materials;
    NativeGpuMesh mesh;
    std::unique_ptr<NativeIntegrationLighting> lighting;
    std::unique_ptr<NativeIntegrationShadows> shadows;
    std::vector<NativeLightingPoint> points;
    UCNativeControlLighting(ID3D11Device *device,ID3D11DeviceContext *borrowed,
        const std::filesystem::path &geo,const std::filesystem::path &catalog,const std::filesystem::path &shaders){
        renderer.attach(device,borrowed);check(borrowed->QueryInterface(IID_PPV_ARGS(&context)));
        ComPtr<ID3D11Device1> device1;check(device->QueryInterface(IID_PPV_ARGS(&device1)));
        const auto feature=device->GetFeatureLevel();D3D_FEATURE_LEVEL selected{};
        check(device1->CreateDeviceContextState(0,&feature,1,D3D11_SDK_VERSION,__uuidof(ID3D11Device),&selected,&pipeline));
        uint8_t hash[32];archive=read_archive(geo,hash);
        const size_t index=find_resource(archive.get(),geo.filename().string().c_str());
        require(index!=SIZE_MAX&&uc_archive_read(archive.get(),index,UC_AR_LOCAL_PAYLOAD,&geometry.value)==UC_AR_OK,"Missing hybrid GEO entry");
        require(uc_scene_load_models(geometry.value.data,geometry.value.size,&scene.value,&models.value)==UC_SCENE_OK,"Unsupported hybrid GEO layout");
        auto ent=geo;ent.replace_extension(".ent");uint8_t ent_hash[32];auto entities=read_archive(ent,ent_hash);BytesOwner data;
        const size_t entity=find_resource(entities.get(),ent.filename().string().c_str());
        require(entity!=SIZE_MAX&&uc_archive_read(entities.get(),entity,UC_AR_LOCAL_PAYLOAD,&data.value)==UC_AR_OK&&
            uc_level_load(data.value.data,data.value.size,&level.value)==UC_LEVEL_OK,"Unsupported hybrid ENT layout");
        materials=load_materials(catalog,hash,archive.get(),scene.value.material_names,scene.value.material_count,
            UC_NATIVE_MATERIAL_GEOMETRY_STATIC,true,renderer);
        std::vector<NativeVertex> vertices(scene.value.vertex_count);
        for(size_t i=0;i<vertices.size();i++){memcpy(vertices[i].position,scene.value.vertices[i].position,12);
            memcpy(vertices[i].uv,scene.value.vertices[i].uv,8);vertices[i].color=scene.value.vertices[i].source_color;}
        mesh=renderer.upload(vertices.data(),vertices.size(),scene.value.indices,scene.value.index_count);
        lighting=std::make_unique<NativeIntegrationLighting>(renderer,scene.value,models.value,materials,
            geometry.value.data,geometry.value.size,level.value,archive.get(),shaders);
        wchar_t shadow_batch[2];
        const bool batch=GetEnvironmentVariableW(L"UCRR_NATIVE_SHADOW_BATCH_RANGES",shadow_batch,2)==1&&shadow_batch[0]==L'1';
        shadows=std::make_unique<NativeIntegrationShadows>(renderer,scene.value,mesh,materials,std::max(1u,lighting->counts().shadow_eligible),batch);
        points.reserve(1024);
    }
};
extern "C" UCNativeControlLighting *uc_native_control_lighting_create(ID3D11Device *device,ID3D11DeviceContext *context,
    const wchar_t *geo,const wchar_t *catalog,const wchar_t *shaders,char *error,size_t capacity){
    error_text(error,capacity,"");
    try{require(geo&&catalog&&shaders,"Missing native lighting asset paths");return new UCNativeControlLighting(device,context,geo,catalog,shaders);}
    catch(const std::exception&e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown native lighting initialization failure");}return nullptr;
}
extern "C" int uc_native_control_lighting_draw(UCNativeControlLighting *state,ID3D11RenderTargetView *target,ID3D11DepthStencilView *depth,
    const D3D11_VIEWPORT *viewport,const float vp[16],const float eye[3],float exposure,const UCNativeControlLight *lights,
    size_t count,uint32_t shadow_enabled,UCNativeControlLightingCounts *counts,char *error,size_t capacity){
    error_text(error,capacity,"");if(counts)*counts={};
    try{
        require(state&&state->thread==GetCurrentThreadId(),"Native lighting owner thread mismatch");
        require(viewport&&vp&&eye&&std::isfinite(exposure)&&exposure>0&&shadow_enabled<=1&&count<=1024&&(!count||lights),"Invalid native lighting frame");
        for(unsigned i=0;i<16;i++)require(std::isfinite(vp[i]),"Invalid native lighting view matrix");
        for(unsigned i=0;i<3;i++)require(std::isfinite(eye[i]),"Invalid native lighting eye");
        auto color_samples=validate_target(target,state->renderer.device.Get(),*viewport);auto depth_samples=validate_target(depth,state->renderer.device.Get(),*viewport);
        require(color_samples.Count==depth_samples.Count&&color_samples.Quality==depth_samples.Quality,"Lighting color/depth sample mismatch");
        ComPtr<ID3D11Predicate> predicate;BOOL value=FALSE;state->context->GetPredication(&predicate,&value);
        require(!predicate,"Native lighting insertion has active predication");
        UCNativeControlLightingCounts result{};result.supplied_lights=uint32_t(count);state->points.clear();
        for(size_t i=0;i<count;i++){
            const auto&source=lights[i];require(source.source_index>=-1&&source.shadows<=1&&source.specular<=1,"Invalid native lighting source identity/flags");
            NativeLightingPoint p{};p.source_index=uint32_t(source.source_index);memcpy(p.position,source.position,12);
            p.radius=source.radius;memcpy(p.color,source.color,16);p.intensity=source.intensity;
            p.shadows=source.shadows!=0;p.specular=source.specular!=0;
            if(source.source_index==-1){result.transient_shadow_omissions+=p.shadows;result.transient_specular_omissions+=p.specular;p.shadows=p.specular=false;}
            state->points.push_back(p);
        }
        state->lighting->set_frame_lights(state->points.data(),state->points.size());
        ScopedPipeline saved(state->context.Get(),state->pipeline.Get());
        state->context->OMSetRenderTargets(1,&target,depth);state->context->RSSetViewports(1,viewport);
        state->shadows->begin_frame();
        state->lighting->draw(state->renderer,vp,eye,exposure,true,
            shadow_enabled?NativeIntegrationShadows::prepare:nullptr,state->shadows.get());
        const auto&c=state->lighting->counts();result.drawn_lights=c.drawn_lights;result.drawn_batches=c.drawn_batches;
        result.shadowed_lights=c.shadowed_lights;result.unsupported_materials=c.unsupported_materials;result.excluded_specular_batches=c.excluded_specular_batches;
        if(counts)*counts=result;return 1;
    }catch(const std::exception&e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown native lighting frame failure");}return 0;
}
extern "C" void uc_native_control_lighting_destroy(UCNativeControlLighting *state){delete state;}
extern "C" int uc_native_control_lighting_set_casters(UCNativeControlLighting *state,const NativeShadowCaster *casters,
    uint32_t count,char *error,size_t capacity){
    error_text(error,capacity,"");
    try{
        require(state&&state->thread==GetCurrentThreadId(),"Native shadow caster owner thread mismatch");
        state->shadows->set_dynamic_casters(casters,count);return 1;
    }catch(const std::exception&e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown native shadow caster failure");}return 0;
}
extern "C" int uc_native_control_spotlight_draw(UCNativeControlLighting *state,ID3D11RenderTargetView *target,
    ID3D11DepthStencilView *depth,const D3D11_VIEWPORT *viewport,const float vp[16],float exposure,
    const UCNativeControlSpot *spots,size_t count,uint32_t *batches,char *error,size_t capacity){
    error_text(error,capacity,"");if(batches)*batches=0;
    try{
        require(state&&state->thread==GetCurrentThreadId(),"Native spotlight owner thread mismatch");
        require(viewport&&vp&&std::isfinite(exposure)&&exposure>0,"Invalid native spotlight frame");
        for(unsigned i=0;i<16;i++)require(std::isfinite(vp[i]),"Invalid native spotlight view matrix");
        auto color_samples=validate_target(target,state->renderer.device.Get(),*viewport);auto depth_samples=validate_target(depth,state->renderer.device.Get(),*viewport);
        require(color_samples.Count==depth_samples.Count&&color_samples.Quality==depth_samples.Quality,"Lighting color/depth sample mismatch");
        ComPtr<ID3D11Predicate> predicate;BOOL value=FALSE;state->context->GetPredication(&predicate,&value);
        require(!predicate,"Native spotlight insertion has active predication");
        ScopedPipeline saved(state->context.Get(),state->pipeline.Get());
        state->context->OMSetRenderTargets(1,&target,depth);state->context->RSSetViewports(1,viewport);
        const auto result=state->lighting->draw_spots(state->renderer,vp,exposure,spots,count);
        if(batches)*batches=result;return 1;
    }catch(const std::exception&e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown native spotlight failure");}return 0;
}
