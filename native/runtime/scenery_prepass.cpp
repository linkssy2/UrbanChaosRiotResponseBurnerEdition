#include "scenery_prepass.h"
#include "assets.h"
extern "C" {
#include "normal_palette.h"
#include "texture_asset/texture_asset.h"
}
#include <d3dcompiler.h>
#include <stdexcept>
static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
static void checked(HRESULT value){if(FAILED(value))throw std::runtime_error("Scenery GPU operation failed");}
static std::string text(const std::filesystem::path&p){auto b=read_file(p,1024*1024);return{b.begin(),b.end()};}
static ComPtr<ID3DBlob>shader(const std::string&s,const char*entry,const char*profile){
    ComPtr<ID3DBlob>b,error;HRESULT h=D3DCompile(s.data(),s.size(),nullptr,nullptr,nullptr,entry,profile,
        D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&error);
    if(FAILED(h))throw std::runtime_error(error?std::string((char*)error->GetBufferPointer(),error->GetBufferSize()):"Scenery shader failed");return b;
}
NativeSceneryPrepass::NativeSceneryPrepass(NativeRenderer&r,const std::filesystem::path&dir):renderer_(r){
    require(r.device&&r.context,"Scenery requires a live renderer");auto*d=r.device.Get();
    auto source=text(dir/"base_alpha_vertex.hlsl")+text(dir/"base_alpha_pixel.hlsl")+
        text(dir/"material_aux_vertex.hlsl")+text(dir/"material_aux_pixel.hlsl")+text(dir/"scenery_prepass.hlsl");
    for(unsigned i=0;i<2;i++){
        auto b=shader(std::string(i?"#define AUXILIARY\n":"")+source,"vertex","vs_5_0");
        checked(d->CreateVertexShader(b->GetBufferPointer(),b->GetBufferSize(),nullptr,&vertex_[i]));
        if(i){const D3D11_INPUT_ELEMENT_DESC e[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
            checked(d->CreateInputLayout(e,2,b->GetBufferPointer(),b->GetBufferSize(),&layout_));}
    }
    for(unsigned i=0;i<3;i++){
        auto b=shader(std::string(i==2?"#define NORMAL\n#define AUXILIARY\n":i?"#define AUXILIARY\n":"")+source,"pixel","ps_5_0");
        checked(d->CreatePixelShader(b->GetBufferPointer(),b->GetBufferSize(),nullptr,&pixel_[i]));
    }
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=64;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    checked(d->CreateBuffer(&bd,nullptr,&matrix_));bd.ByteWidth=32;checked(d->CreateBuffer(&bd,nullptr,&constants_));
    uint32_t values[256];uc_normal_palette(values);bd.ByteWidth=sizeof(values);bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    bd.Usage=D3D11_USAGE_IMMUTABLE;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=4;
    D3D11_SUBRESOURCE_DATA data{values,0,0};checked(d->CreateBuffer(&bd,&data,&palette_));checked(d->CreateShaderResourceView(palette_.Get(),nullptr,&palette_view_));
    D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sd.MaxLOD=D3D11_FLOAT32_MAX;
    checked(d->CreateSamplerState(&sd,&sampler_));
    for(unsigned i=0;i<2;i++)for(unsigned j=0;j<2;j++){
        D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=i?D3D11_CULL_NONE:D3D11_CULL_BACK;
        rd.FrontCounterClockwise=FALSE;rd.DepthClipEnable=!j;checked(d->CreateRasterizerState(&rd,&raster_[i][j]));
    }
}
void NativeSceneryPrepass::mesh(const NativeGpuMesh&m,const float rows[16]){
    require(m.vertices&&m.indices&&rows,"Scenery mesh/matrix missing");mesh_=&m;
    renderer_.context->UpdateSubresource(matrix_.Get(),0,nullptr,rows,0,0);
}
void NativeSceneryPrepass::vertex(bool auxiliary){renderer_.context->VSSetShader(vertex_[auxiliary].Get(),nullptr,0);}
void NativeSceneryPrepass::pixel(unsigned family){
    require(family<3,"Scenery pixel family");auto*c=renderer_.context.Get();ComPtr<ID3D11PixelShader>current;
    c->PSGetShader(&current,nullptr,nullptr);
    if(current.Get()!=pixel_[family].Get()){
        // All three retail definitions initialize their combiner constants to zero.
        // Observe the live binding: an ordinary pass may have replaced our shader.
        for(float&v:pixel_constants_)v=0;
        c->UpdateSubresource(constants_.Get(),0,nullptr,pixel_constants_,0,0);
    }
    c->PSSetShader(pixel_[family].Get(),nullptr,0);
}
void NativeSceneryPrepass::constant(unsigned slot,uint32_t argb){
    require(slot==1||slot==2,"Scenery constant slot");float*p=pixel_constants_+(slot==1?4:0);
    p[0]=float((argb>>16)&255)/255.f;p[1]=float((argb>>8)&255)/255.f;p[2]=float(argb&255)/255.f;p[3]=float(argb>>24)/255.f;
    renderer_.context->UpdateSubresource(constants_.Get(),0,nullptr,pixel_constants_,0,0);
}
void NativeSceneryPrepass::texture(const UCTextureAsset&t){
    auto it=textures_.find(&t);
    if(it==textures_.end()){
        ComPtr<ID3D11ShaderResourceView>view;
        if(t.format!=UC_TEXTURE_P8_UINT)view=renderer_.upload(t);
        else{
            require(t.storage&&t.width&&t.height&&t.width<=16384&&t.height<=16384&&t.mip_count&&t.mip_count<=UC_TEXTURE_MAX_MIPS,"Scenery palette image domain");
            D3D11_SUBRESOURCE_DATA levels[UC_TEXTURE_MAX_MIPS]{};unsigned w=t.width,h=t.height;
            for(unsigned i=0;i<t.mip_count;i++){
                const auto&m=t.mips[i];auto base=uintptr_t(t.storage),ptr=uintptr_t(m.data);
                require(m.width==w&&m.height==h&&m.row_pitch==w&&m.slice_pitch==w*h&&ptr>=base&&ptr-base<=t.storage_bytes&&m.slice_pitch<=t.storage_bytes-(ptr-base),"Scenery palette mip bounds");
                levels[i]={m.data,m.row_pitch,m.slice_pitch};w=w>1?w/2:1;h=h>1?h/2:1;
            }
            D3D11_TEXTURE2D_DESC td{};td.Width=t.width;td.Height=t.height;td.MipLevels=t.mip_count;td.ArraySize=1;td.SampleDesc.Count=1;
            td.Format=DXGI_FORMAT_R8_UNORM;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;ComPtr<ID3D11Texture2D>image;
            checked(renderer_.device->CreateTexture2D(&td,levels,&image));checked(renderer_.device->CreateShaderResourceView(image.Get(),nullptr,&view));
        }
        it=textures_.emplace(&t,std::move(view)).first;
    }
    auto*view=it->second.Get();renderer_.context->PSSetShaderResources(0,1,&view);
}
void NativeSceneryPrepass::cull(uint32_t value){require(value==0||value==0x900,"Scenery cull mode");cull_=value;renderer_.context->RSSetState(raster_[!cull_][depth_clamp_].Get());}
void NativeSceneryPrepass::depth_clamp(bool value){depth_clamp_=value;cull(cull_);}
void NativeSceneryPrepass::bind_palette(){auto*p=palette_view_.Get();renderer_.context->PSSetShaderResources(1,1,&p);}
void NativeSceneryPrepass::draw(uint32_t first,uint32_t count){
    require(mesh_&&count&&first<=mesh_->index_count&&count<=mesh_->index_count-first,"Scenery draw range");
    auto*c=renderer_.context.Get();UINT stride=sizeof(NativeVertex),offset=0;auto*v=mesh_->vertices.Get();
    c->IASetInputLayout(layout_.Get());c->IASetVertexBuffers(0,1,&v,&stride,&offset);c->IASetIndexBuffer(mesh_->indices.Get(),DXGI_FORMAT_R32_UINT,0);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);auto*m=matrix_.Get();auto*p=constants_.Get();auto*s=sampler_.Get();
    c->VSSetConstantBuffers(0,1,&m);c->PSSetConstantBuffers(1,1,&p);c->PSSetSamplers(0,1,&s);c->DrawIndexed(count,first,0);
}
