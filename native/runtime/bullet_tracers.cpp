#include "bullet_tracers.h"
#include "assets.h"
extern "C" {
#include "../particle_billboard.h"
}
#include "../texture_asset/texture_asset.h"
#include <DirectXMath.h>
#include <cstring>
#include <stdexcept>
using namespace DirectX;
namespace {
void need(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::string shader(const std::filesystem::path&p){auto b=read_file(p,1024*1024);return {b.begin(),b.end()};}
XMMATRIX matrix(const float*p){XMFLOAT4X4 v;memcpy(&v,p,64);return XMLoadFloat4x4(&v);}
void store(float*p,FXMMATRIX m){XMFLOAT4X4 v;XMStoreFloat4x4(&v,m);memcpy(p,&v,64);}
}
void NativeBulletTracers::open(NativeRenderer&r,UCArchiveLookup&lookup,const std::filesystem::path&shaders,const UCBulletTracersServices&services){
    need(!renderer_&&services.listener&&services.flyby,"Tracer owner already open or missing audio services");
    size_t index=SIZE_MAX;need(!uc_archive_lookup(&lookup,"FX\\FX_Tracer\\FX_Tracer_Default.tga",&index)&&index!=SIZE_MAX,"Original tracer texture missing");
    struct Texture{UCTextureAsset value{};~Texture(){uc_texture_free(&value);}}texture;
    need(!uc_texture_load(lookup.archive,index,UC_AR_LOCAL_PAYLOAD,&texture.value),"Original tracer texture unsupported");
    texture_=r.upload(texture.value);triangles_.open(r.device.Get(),r.context.Get(),shader(shaders/"particle_vertex.hlsl"),shader(shaders/"particle.hlsl"));
    D3D11_SAMPLER_DESC d{};d.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;d.AddressU=d.AddressV=d.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;d.MaxLOD=D3D11_FLOAT32_MAX;
    need(SUCCEEDED(r.device->CreateSamplerState(&d,&sampler_)),"Tracer sampler");
    uc_bullet_tracers_init(&state_);services_=services;renderer_=&r;
}
void NativeBulletTracers::append(const float from[3],const float to[3],const float rgba[4],float width){
    need(renderer_&&!uc_bullet_tracers_append(&state_,from,to,rgba,width,&services_),"Tracer append/audio owner6A4C0");
}
void NativeBulletTracers::step(float delta){need(renderer_&&!uc_bullet_tracers_step(&state_,delta),"Tracer update6A3C0");}
void NativeBulletTracers::draw(NativeRenderer&r,const float view[16],const float projection[16]){
    need(renderer_==&r&&view&&projection,"Tracer render owner");if(!state_.count)return;
    float camera[16],clip[16];store(camera,XMMatrixInverse(nullptr,matrix(view)));store(clip,matrix(view)*matrix(projection));
    UCBulletTracerVertex generated[48];need(!uc_bullet_tracers_vertices(&state_,camera+8,generated,48),"Tracer geometry6A000");
    UCParticleQuadVertex vertices[48];const unsigned order[3]={0,2,1};
    for(unsigned i=0;i<state_.count;i++){
        uint32_t color=0;need(uc_particle_color_pack(generated[i*3].rgba,&color)!=0,"Tracer original packed color99980");
        for(unsigned j=0;j<3;j++){auto&v=vertices[i*3+j];const auto&g=generated[i*3+order[j]];memcpy(v.position,g.position,12);memcpy(v.uv,g.uv,8);v.color=color;}
    }
    NativeParticleQuadView frame{};for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)frame.clip_rows[i][j]=clip[j*4+i];frame.fog[3]=1;frame.alpha_test=frame.texture_alpha_kill=1;
    ComPtr<ID3D11BlendState> blend;ComPtr<ID3D11DepthStencilState> depth;ComPtr<ID3D11RasterizerState> raster;float factor[4];UINT mask=0,reference=0;
    r.context->OMGetBlendState(&blend,factor,&mask);r.context->OMGetDepthStencilState(&depth,&reference);r.context->RSGetState(&raster);
    r.context->OMSetBlendState(r.blends[1][0].Get(),nullptr,~0u);r.context->OMSetDepthStencilState(r.transparent_depth.Get(),0);
    triangles_.draw_triangles(vertices,state_.count,frame,texture_.Get(),sampler_.Get());
    r.context->OMSetBlendState(blend.Get(),factor,mask);r.context->OMSetDepthStencilState(depth.Get(),reference);r.context->RSSetState(raster.Get());
    r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);r.context->IASetInputLayout(r.layout.Get());
    ID3D11Buffer*cb=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&cb);r.context->PSSetConstantBuffers(0,1,&cb);
}
