#include "control_quad_bridge.h"
#include "decal_quads.h"
#include "particle_quads.h"
#include <d3d11_1.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace {
void require(bool v,const char *message){if(!v)throw std::invalid_argument(message);}
void check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Native quad D3D11.1 initialization failed");}
void error_text(char *out,size_t capacity,const char *message){if(out&&capacity){size_t n=std::min(capacity-1,strlen(message));memcpy(out,message,n);out[n]=0;}}
std::string shader(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary|std::ios::ate);require(bool(f),"Missing native quad shader");const auto n=f.tellg();require(n>0&&n<=1024*1024,"Invalid native quad shader size");std::string s(size_t(n),'\0');f.seekg(0);f.read(s.data(),n);require(bool(f),"Cannot read native quad shader");return s;}
struct SavedPipeline{ID3D11DeviceContext1 *context;ComPtr<ID3DDeviceContextState> prior;SavedPipeline(ID3D11DeviceContext1*c,ID3DDeviceContextState*s):context(c){c->SwapDeviceContextState(s,&prior);}~SavedPipeline(){context->ClearState();context->SwapDeviceContextState(prior.Get(),nullptr);}};
void owner(ID3D11DeviceChild *object,ID3D11Device *device){if(object){ComPtr<ID3D11Device>d;object->GetDevice(&d);require(d.Get()==device,"Foreign native quad resource");}}
ComPtr<ID3D11Resource> target(ID3D11View *view,ID3D11Device *device,const D3D11_VIEWPORT &vp,D3D11_TEXTURE2D_DESC &desc){
    ComPtr<ID3D11Resource>r;if(!view)return r;owner(view,device);view->GetResource(&r);ComPtr<ID3D11Texture2D>t;require(SUCCEEDED(r.As(&t)),"Unsupported native quad target");t->GetDesc(&desc);
    require((desc.SampleDesc.Count==1||desc.SampleDesc.Count==2||desc.SampleDesc.Count==4)&&vp.TopLeftX+vp.Width<=float(desc.Width)&&vp.TopLeftY+vp.Height<=float(desc.Height),"Unsupported native quad target extent or MSAA");return r;
}
}
struct UCNativeControlQuads {
    DWORD thread=GetCurrentThreadId();ComPtr<ID3D11Device>device;ComPtr<ID3D11DeviceContext1>context;ComPtr<ID3DDeviceContextState>pipeline;
    NativeDecalQuads decals;NativeParticleQuads textured;
    UCNativeControlQuads(ID3D11Device*d,ID3D11DeviceContext*c,const wchar_t *path):device(d){
        require(d&&c&&path,"Missing native quad owner");ComPtr<ID3D11Device>actual;c->GetDevice(&actual);require(actual.Get()==d&&c->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Invalid native quad context");
        check(c->QueryInterface(IID_PPV_ARGS(&context)));ComPtr<ID3D11Device1>d1;check(d->QueryInterface(IID_PPV_ARGS(&d1)));const auto level=d->GetFeatureLevel();D3D_FEATURE_LEVEL selected;
        check(d1->CreateDeviceContextState(0,&level,1,D3D11_SDK_VERSION,__uuidof(ID3D11Device),&selected,&pipeline));
        const std::filesystem::path p(path);decals.open(d,c,shader(p/L"base_alpha_vertex.hlsl"));textured.open(d,c,shader(p/L"particle_vertex.hlsl"),shader(p/L"particle.hlsl"));
    }
};
static_assert(sizeof(UCNativeControlDecalVertex)==sizeof(NativeDecalVertex)&&sizeof(UCNativeControlDecalView)==sizeof(NativeDecalView)&&sizeof(UCNativeControlTexturedQuadView)==sizeof(NativeParticleQuadView),"Native quad ABI");
template<class Vertex,class View,class Submit>static int draw(UCNativeControlQuads*s,const Vertex*v,uint32_t count,const View*view,
    const UCNativeControlParticleDraw*d,ID3D11RasterizerState*raster,char *error,size_t capacity,Submit submit){
    error_text(error,capacity,"");bool submitted=false;
    try{
        require(s&&s->thread==GetCurrentThreadId(),"Native quad owner thread mismatch");
        if(FAILED(s->device->GetDeviceRemovedReason())){error_text(error,capacity,"Native quad device removed");return -1;}
        require(view&&d&&d->target&&d->texture&&d->sampler&&raster&&count<=128&&(!count||v),"Invalid native quad draw");
        const auto &vp=d->viewport;const float values[]={vp.TopLeftX,vp.TopLeftY,vp.Width,vp.Height,vp.MinDepth,vp.MaxDepth};for(float x:values)require(std::isfinite(x),"Invalid native quad viewport");
        require(vp.TopLeftX>=0&&vp.TopLeftY>=0&&vp.Width>0&&vp.Height>0&&vp.MinDepth>=0&&vp.MaxDepth<=1&&vp.MinDepth<=vp.MaxDepth,"Unsupported native quad viewport");
        for(float f:d->blend_factor)require(std::isfinite(f),"Invalid native quad blend factor");owner(d->blend,s->device.Get());owner(d->depth_state,s->device.Get());owner(raster,s->device.Get());
        D3D11_RENDER_TARGET_VIEW_DESC rt{};d->target->GetDesc(&rt);require((rt.ViewDimension==D3D11_RTV_DIMENSION_TEXTURE2D&&!rt.Texture2D.MipSlice)||rt.ViewDimension==D3D11_RTV_DIMENSION_TEXTURE2DMS,"Unsupported native quad RTV");
        if(d->depth){D3D11_DEPTH_STENCIL_VIEW_DESC dt{};d->depth->GetDesc(&dt);require((dt.ViewDimension==D3D11_DSV_DIMENSION_TEXTURE2D&&!dt.Texture2D.MipSlice)||dt.ViewDimension==D3D11_DSV_DIMENSION_TEXTURE2DMS,"Unsupported native quad DSV");}
        D3D11_TEXTURE2D_DESC color_desc{},depth_desc{};auto color=target(d->target,s->device.Get(),vp,color_desc),depth=target(d->depth,s->device.Get(),vp,depth_desc);
        require(!depth||(color_desc.Width==depth_desc.Width&&color_desc.Height==depth_desc.Height&&color_desc.SampleDesc.Count==depth_desc.SampleDesc.Count&&color_desc.SampleDesc.Quality==depth_desc.SampleDesc.Quality),"Native quad target/depth size mismatch");
        ComPtr<ID3D11Resource>image;d->texture->GetResource(&image);require(image!=color&&image!=depth,"Native quad target feedback");
        D3D11_SHADER_RESOURCE_VIEW_DESC desc{};d->texture->GetDesc(&desc);const auto f=desc.Format;
        require(f==DXGI_FORMAT_BC1_UNORM||f==DXGI_FORMAT_BC2_UNORM||f==DXGI_FORMAT_BC3_UNORM||f==DXGI_FORMAT_R8G8B8A8_UNORM||f==DXGI_FORMAT_B8G8R8A8_UNORM,"Unsupported native quad texture format");
        for(uint32_t i=0;i<count*4;i++){const auto *p=v[i].position,*r=view->clip_rows[3];float w=((p[0]*r[0]+p[1]*r[1])+p[2]*r[2])+r[3];require(std::isfinite(w)&&std::abs(w)>=0x1p-64f&&std::abs(w)<=0x1p64f,"Unsupported CONTROL quad clip W");}
        ComPtr<ID3D11Predicate>predicate;BOOL value=FALSE;s->context->GetPredication(&predicate,&value);require(!predicate,"Native quad insertion has predication");
        SavedPipeline saved(s->context.Get(),s->pipeline.Get());s->context->OMSetRenderTargets(1,&d->target,d->depth);s->context->RSSetViewports(1,&vp);s->context->RSSetState(raster);
        s->context->OMSetBlendState(d->blend,d->blend_factor,d->sample_mask);s->context->OMSetDepthStencilState(d->depth_state,d->stencil_ref);submit();submitted=count!=0;
        if(FAILED(s->device->GetDeviceRemovedReason())){error_text(error,capacity,"Native quad device failed during submission");return -1;}return 1;
    }catch(const std::exception&e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown native quad failure");}
    return submitted||(s&&s->thread==GetCurrentThreadId()&&FAILED(s->device->GetDeviceRemovedReason()))?-1:0;
}
extern "C" UCNativeControlQuads *uc_native_control_quads_create(ID3D11Device*d,ID3D11DeviceContext*c,const wchar_t*p,char*e,size_t n){error_text(e,n,"");try{return new UCNativeControlQuads(d,c,p);}catch(const std::exception&x){error_text(e,n,x.what());}catch(...){error_text(e,n,"Unknown native quad initialization failure");}return nullptr;}
extern "C" int uc_native_control_quads_decal(UCNativeControlQuads*s,const UCNativeControlDecalVertex*v,uint32_t count,const UCNativeControlDecalView*view,const UCNativeControlParticleDraw*d,ID3D11RasterizerState*r,char*e,size_t n){
    return draw(s,v,count,view,d,r,e,n,[&]{NativeDecalView q;memcpy(&q,view,sizeof q);s->decals.draw(reinterpret_cast<const NativeDecalVertex*>(v),count,q,d->texture,d->sampler);});
}
extern "C" int uc_native_control_quads_textured(UCNativeControlQuads*s,const UCParticleQuadVertex*v,uint32_t count,const UCNativeControlTexturedQuadView*view,const UCNativeControlParticleDraw*d,ID3D11RasterizerState*r,char*e,size_t n){
    return draw(s,v,count,view,d,r,e,n,[&]{D3D11_RASTERIZER_DESC rd;r->GetDesc(&rd);require(rd.FillMode==D3D11_FILL_SOLID&&rd.CullMode==D3D11_CULL_NONE&&!rd.DepthBias&&!rd.DepthBiasClamp&&!rd.SlopeScaledDepthBias&&rd.DepthClipEnable&&!rd.ScissorEnable,"Unsupported native textured quad rasterizer");NativeParticleQuadView q;memcpy(&q,view,sizeof q);s->textured.draw(v,count,q,d->texture,d->sampler);});
}
extern "C" void uc_native_control_quads_destroy(UCNativeControlQuads*s){delete s;}
