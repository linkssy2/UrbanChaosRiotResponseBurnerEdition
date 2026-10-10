#include "early_tone.h"
#include "../point_light_pixel.h"
#include <d3dcompiler.h>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
static void check(HRESULT h){if(FAILED(h))throw std::runtime_error("Early tone device operation failed");}
void NativeEarlyTone::open(ID3D11Device*d,ID3D11DeviceContext*c,ID3D11Texture2D*s,const std::string&pixel){
    if(context_||!d||!c||!s||c->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)throw std::invalid_argument("Invalid early tone owner");
    ComPtr<ID3D11Device>owner;c->GetDevice(&owner);if(owner.Get()!=d)throw std::invalid_argument("Foreign early tone context");
    s->GetDevice(&owner);D3D11_TEXTURE2D_DESC t{};s->GetDesc(&t);
    UCEnvironmentEarlyToneInput in{t.Width,t.Height,{0,0,(float)t.Width,(float)t.Height},1,1};UCEnvironmentEarlyTonePlan plan;
    if(owner.Get()!=d||t.Format!=DXGI_FORMAT_R8G8B8A8_UNORM||t.MipLevels!=1||t.ArraySize!=1||t.SampleDesc.Count!=1||!(t.BindFlags&D3D11_BIND_SHADER_RESOURCE)||uc_environment_early_tone(&in,&plan))throw std::invalid_argument("Unsupported early tone source");
    context_=c;source_=s;width_=t.Width;height_=t.Height;check(d->CreateShaderResourceView(s,nullptr,&source_view_));
    t.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;t.Usage=D3D11_USAGE_DEFAULT;t.CPUAccessFlags=t.MiscFlags=0;
    check(d->CreateTexture2D(&t,nullptr,&output_));check(d->CreateRenderTargetView(output_.Get(),nullptr,&target_));
    const std::string code=pixel+R"(
cbuffer Params:register(b0){float4 dimensions;float4 reciprocal;float4 amount;};
struct Input{float3 position:POSITION;float2 uv:TEXCOORD0;};
struct Pixel{float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
Pixel vertex(Input v){Pixel p;p.position=float4(v.position.x/dimensions.x*2-1,1-v.position.y/dimensions.y*2,0,1);p.uv=v.uv/dimensions.xy;return p;}
Texture2D<float4> source:register(t0);SamplerState point_clamp:register(s0);
float4 shade(Pixel p):SV_TARGET{return environment_early_tone(source.SampleLevel(point_clamp,p.uv,0),reciprocal,amount);}
)";
    const unsigned flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;
    auto compile=[&](const char*entry,const char*target){ComPtr<ID3DBlob>b,error;auto h=D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,entry,target,flags,0,&b,&error);if(FAILED(h))throw std::runtime_error(error?std::string((const char*)error->GetBufferPointer(),error->GetBufferSize()):"Early tone shader compilation failed");return b;};
    auto vs=compile("vertex","vs_5_0"),ps=compile("shade","ps_5_0");
    check(d->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex_));check(d->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel_));
    const D3D11_INPUT_ELEMENT_DESC e[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};check(d->CreateInputLayout(e,2,vs->GetBufferPointer(),vs->GetBufferSize(),&layout_));
    D3D11_BUFFER_DESC b{};b.ByteWidth=80;b.BindFlags=D3D11_BIND_VERTEX_BUFFER;check(d->CreateBuffer(&b,nullptr,&vertices_));b.ByteWidth=48;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(d->CreateBuffer(&b,nullptr,&constants_));
    D3D11_BLEND_DESC blend{};blend.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;check(d->CreateBlendState(&blend,&blend_));D3D11_DEPTH_STENCIL_DESC depth{};check(d->CreateDepthStencilState(&depth,&depth_));
    D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;check(d->CreateRasterizerState(&raster,&raster_));
    D3D11_SAMPLER_DESC sampler{};sampler.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;check(d->CreateSamplerState(&sampler,&sampler_));
}
void NativeEarlyTone::apply(const UCEnvironmentEarlyToneInput&in){
    UCEnvironmentEarlyTonePlan plan;
    if(!sampler_||in.width!=width_||in.height!=height_||uc_environment_early_tone(&in,&plan)||plan.vertices[1].position[0]<plan.vertices[0].position[0]||plan.vertices[2].position[1]<plan.vertices[0].position[1])throw std::invalid_argument("Unsupported early tone rectangle or state");
    float params[12]={(float)width_,(float)height_,0,0};uint32_t packed[2];
    if(!uc_point_light_constant(plan.constant4,packed)||!uc_point_light_constant(plan.constant0,packed+1))throw std::invalid_argument("Unsupported early tone constants");
    for(unsigned i=0;i<2;i++)for(unsigned k=0;k<4;k++)params[4+i*4+k]=(float)((packed[i]>>(k==3?24:16-k*8))&255)/255.f;
    UCEnvironmentToneVertex vertices[]={plan.vertices[0],plan.vertices[1],plan.vertices[3],plan.vertices[2]};
    ID3D11ShaderResourceView*null=nullptr;context_->PSSetShaderResources(0,1,&null);context_->OMSetRenderTargets(0,nullptr,nullptr);context_->CopyResource(output_.Get(),source_.Get());
    context_->UpdateSubresource(vertices_.Get(),0,nullptr,vertices,0,0);context_->UpdateSubresource(constants_.Get(),0,nullptr,params,0,0);
    ID3D11Buffer*vb=vertices_.Get(),*cb=constants_.Get();unsigned stride=20,offset=0;context_->IASetInputLayout(layout_.Get());context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    context_->VSSetShader(vertex_.Get(),nullptr,0);context_->PSSetShader(pixel_.Get(),nullptr,0);context_->GSSetShader(nullptr,nullptr,0);context_->HSSetShader(nullptr,nullptr,0);context_->DSSetShader(nullptr,nullptr,0);context_->VSSetConstantBuffers(0,1,&cb);context_->PSSetConstantBuffers(0,1,&cb);
    context_->OMSetBlendState(blend_.Get(),nullptr,~0u);context_->OMSetDepthStencilState(depth_.Get(),0);context_->RSSetState(raster_.Get());D3D11_VIEWPORT viewport{0,0,(float)width_,(float)height_,0,1};context_->RSSetViewports(1,&viewport);
    ID3D11RenderTargetView*rt=target_.Get();ID3D11ShaderResourceView*s=source_view_.Get();ID3D11SamplerState*ss=sampler_.Get();context_->OMSetRenderTargets(1,&rt,nullptr);context_->PSSetShaderResources(0,1,&s);context_->PSSetSamplers(0,1,&ss);context_->Draw(4,0);context_->PSSetShaderResources(0,1,&null);context_->OMSetRenderTargets(0,nullptr,nullptr);
}
