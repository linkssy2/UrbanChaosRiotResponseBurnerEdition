#include "control_particle_bridge.h"
#include "particle_billboards.h"
#include <d3d11_1.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message){if(!condition)throw std::invalid_argument(message);}
void check(HRESULT result){if(FAILED(result))throw std::runtime_error("Particle bridge D3D11.1 initialization failed");}
void error_text(char *out,size_t capacity,const char *message){
    if(out&&capacity){const size_t n=std::min(capacity-1,strlen(message));memcpy(out,message,n);out[n]=0;}
}
std::string read_shader(const std::filesystem::path &path){
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    require(bool(file),"Missing particle bridge shader");const auto size=file.tellg();
    require(size>0&&size<=1024*1024,"Invalid particle bridge shader size");
    std::string result(size_t(size),'\0');file.seekg(0);file.read(result.data(),size);require(bool(file),"Cannot read particle bridge shader");return result;
}
struct ScopedPipeline {
    ID3D11DeviceContext1 *context;ComPtr<ID3DDeviceContextState> prior;
    ScopedPipeline(ID3D11DeviceContext1*c,ID3DDeviceContextState*s):context(c){context->SwapDeviceContextState(s,&prior);}
    ~ScopedPipeline(){context->ClearState();context->SwapDeviceContextState(prior.Get(),nullptr);}
};
void validate_owner(ID3D11DeviceChild *object,ID3D11Device *device){
    if(object){ComPtr<ID3D11Device> owner;object->GetDevice(&owner);require(owner.Get()==device,"Particle resource belongs to another device");}
}
ComPtr<ID3D11Resource> validate_target(ID3D11View *view,ID3D11Device *device,const D3D11_VIEWPORT &viewport){
    ComPtr<ID3D11Resource> resource;if(!view)return resource;
    validate_owner(view,device);view->GetResource(&resource);ComPtr<ID3D11Texture2D> texture;
    require(SUCCEEDED(resource.As(&texture)),"Unsupported particle target resource");
    D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);
    require((desc.SampleDesc.Count==1||desc.SampleDesc.Count==2||desc.SampleDesc.Count==4)&&viewport.TopLeftX+viewport.Width<=float(desc.Width)&&
        viewport.TopLeftY+viewport.Height<=float(desc.Height),"Unsupported particle target extent or MSAA");return resource;
}
}
struct UCNativeControlParticles {
    DWORD thread=GetCurrentThreadId();
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext1> context;
    ComPtr<ID3DDeviceContextState> pipeline;
    NativeParticleBillboards renderer;
    UCNativeControlParticles(ID3D11Device *d,ID3D11DeviceContext *c,const wchar_t *shaders):device(d){
        require(d&&c&&shaders,"Missing particle bridge owner or shader directory");
        ComPtr<ID3D11Device> owner;c->GetDevice(&owner);
        require(owner.Get()==d&&c->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Unsupported particle bridge owner");
        check(c->QueryInterface(IID_PPV_ARGS(&context)));ComPtr<ID3D11Device1> device1;check(d->QueryInterface(IID_PPV_ARGS(&device1)));
        const auto level=d->GetFeatureLevel();D3D_FEATURE_LEVEL selected{};
        check(device1->CreateDeviceContextState(0,&level,1,D3D11_SDK_VERSION,__uuidof(ID3D11Device),&selected,&pipeline));
        const std::filesystem::path directory(shaders);
        renderer.open(d,c,read_shader(directory/L"particle_billboard.hlsl"),read_shader(directory/L"particle.hlsl"));
    }
};
static_assert(sizeof(UCNativeControlParticleView)==sizeof(NativeParticleBillboardView)&&sizeof(UCParticleBillboard)==28,"Particle bridge layouts");
extern "C" UCNativeControlParticles *uc_native_control_particles_create(ID3D11Device *device,ID3D11DeviceContext *context,
    const wchar_t *shaders,char *error,size_t capacity){
    error_text(error,capacity,"");try{return new UCNativeControlParticles(device,context,shaders);}
    catch(const std::exception&e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown particle bridge initialization failure");}return nullptr;
}
extern "C" int uc_native_control_particles_draw(UCNativeControlParticles *state,const UCParticleBillboard *sprites,uint32_t count,
    const UCNativeControlParticleView *view,const UCNativeControlParticleDraw *draw,char *error,size_t capacity){
    error_text(error,capacity,"");bool submitted=false;
    try{
        require(state&&state->thread==GetCurrentThreadId(),"Particle bridge owner thread mismatch");
        if(FAILED(state->device->GetDeviceRemovedReason())){error_text(error,capacity,"Particle bridge device removed");return UC_CONTROL_PARTICLE_FATAL;}
        require(view&&draw&&draw->target&&draw->texture&&draw->sampler&&count<=39&&(!count||sprites),"Invalid particle bridge draw");
        const auto &v=draw->viewport;const float values[]={v.TopLeftX,v.TopLeftY,v.Width,v.Height,v.MinDepth,v.MaxDepth};
        for(float f:values)require(std::isfinite(f),"Invalid particle viewport");
        require(v.TopLeftX>=0&&v.TopLeftY>=0&&v.Width>0&&v.Height>0&&v.MinDepth>=0&&v.MaxDepth<=1&&v.MinDepth<=v.MaxDepth,"Unsupported particle viewport");
        for(float f:draw->blend_factor)require(std::isfinite(f),"Invalid particle blend factor");
        validate_owner(draw->blend,state->device.Get());validate_owner(draw->depth_state,state->device.Get());
        D3D11_RENDER_TARGET_VIEW_DESC color_desc{};draw->target->GetDesc(&color_desc);
        require((color_desc.ViewDimension==D3D11_RTV_DIMENSION_TEXTURE2D&&color_desc.Texture2D.MipSlice==0)||color_desc.ViewDimension==D3D11_RTV_DIMENSION_TEXTURE2DMS,"Unsupported particle color target view");
        if(draw->depth){D3D11_DEPTH_STENCIL_VIEW_DESC depth_desc{};draw->depth->GetDesc(&depth_desc);
            require((depth_desc.ViewDimension==D3D11_DSV_DIMENSION_TEXTURE2D&&depth_desc.Texture2D.MipSlice==0)||depth_desc.ViewDimension==D3D11_DSV_DIMENSION_TEXTURE2DMS,"Unsupported particle depth target view");}
        auto target=validate_target(draw->target,state->device.Get(),v),depth=validate_target(draw->depth,state->device.Get(),v);
        if(depth){ComPtr<ID3D11Texture2D> color_texture,depth_texture;target.As(&color_texture);depth.As(&depth_texture);
            D3D11_TEXTURE2D_DESC a{},b{};color_texture->GetDesc(&a);depth_texture->GetDesc(&b);
            require(a.Width==b.Width&&a.Height==b.Height&&a.SampleDesc.Count==b.SampleDesc.Count&&a.SampleDesc.Quality==b.SampleDesc.Quality,"Particle target/depth dimensions differ");}
        ComPtr<ID3D11Resource> texture;draw->texture->GetResource(&texture);
        require(texture!=target&&texture!=depth,"Particle texture aliases its target");
        ComPtr<ID3D11Predicate> predicate;BOOL predicate_value=FALSE;state->context->GetPredication(&predicate,&predicate_value);
        require(!predicate,"Particle insertion has active predication");
        NativeParticleBillboardView native_view;memcpy(&native_view,view,sizeof native_view);
        ScopedPipeline saved(state->context.Get(),state->pipeline.Get());
        state->context->OMSetRenderTargets(1,&draw->target,draw->depth);state->context->RSSetViewports(1,&v);
        state->context->OMSetBlendState(draw->blend,draw->blend_factor,draw->sample_mask);
        state->context->OMSetDepthStencilState(draw->depth_state,draw->stencil_ref);
        // The existing owner validates all four corners once, before Map/Draw.
        state->renderer.draw(sprites,count,native_view,draw->texture,draw->sampler,false,true);
        submitted=count!=0;
        if(FAILED(state->device->GetDeviceRemovedReason())){error_text(error,capacity,"Particle bridge device failed during draw");return UC_CONTROL_PARTICLE_FATAL;}
        return UC_CONTROL_PARTICLE_SUBMITTED;
    }catch(const std::exception&e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown particle bridge draw failure");}
    if(submitted||(state&&state->thread==GetCurrentThreadId()&&FAILED(state->device->GetDeviceRemovedReason())))return UC_CONTROL_PARTICLE_FATAL;
    return UC_CONTROL_PARTICLE_REJECTED;
}
extern "C" void uc_native_control_particles_destroy(UCNativeControlParticles *state){delete state;}
