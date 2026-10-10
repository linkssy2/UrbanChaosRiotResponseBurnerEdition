#include "glow_filter.h"
#include "../environment_glow_filter.h"
#include "../point_light_pixel.h"
#include <d3dcompiler.h>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
static void check(HRESULT r){if(FAILED(r))throw std::runtime_error("Glow GPU operation failed");}
void NativeGlowFilter::open(ID3D11Device *device,ID3D11DeviceContext *context,
    unsigned width,unsigned height,const std::string &source,float sigma){
    if(device_||!device||!context||context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)
        throw std::invalid_argument("Unsupported glow owner");
    ComPtr<ID3D11Device> actual;context->GetDevice(&actual);
    UCEnvironmentGlowKernel kernel;UCEnvironmentGlowFilterPlan plan;
    if(actual.Get()!=device||uc_environment_glow_reset(&kernel)||
       uc_environment_glow_filter(&kernel,sigma,width,height,&plan))
        throw std::invalid_argument("Unsupported glow dimensions or arithmetic");
    device_=device;context_=context;width_=width;height_=height;
    const char *wrapper=R"(
cbuffer Params:register(b0){float4 dimensions;float4 near_weight;float4 far_weight;};
struct Input {float3 position:POSITION;float2 uv0:TEXCOORD0;float2 uv1:TEXCOORD1;float2 uv2:TEXCOORD2;float2 uv3:TEXCOORD3;};
struct Pixel {float4 position:SV_POSITION;float2 uv0:TEXCOORD0;float2 uv1:TEXCOORD1;float2 uv2:TEXCOORD2;float2 uv3:TEXCOORD3;};
Pixel vertex(Input v){Pixel p;p.position=float4(v.position.x/dimensions.x*2-1,1-v.position.y/dimensions.y*2,0,1);
 p.uv0=v.uv0/dimensions.xy;p.uv1=v.uv1/dimensions.xy;p.uv2=v.uv2/dimensions.xy;p.uv3=v.uv3/dimensions.xy;return p;}
Texture2D<float4> original:register(t0);SamplerState linear_clamp:register(s0);
float4 pixel(Pixel p):SV_TARGET{return environment_filter(original.SampleLevel(linear_clamp,p.uv0,0),
 original.SampleLevel(linear_clamp,p.uv1,0),original.SampleLevel(linear_clamp,p.uv2,0),original.SampleLevel(linear_clamp,p.uv3,0),near_weight,far_weight);}
)";
    const std::string code=source+wrapper;ComPtr<ID3DBlob>vs,ps,error;
    const unsigned flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;
    check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"vertex","vs_5_0",flags,0,&vs,&error));
    check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"pixel","ps_5_0",flags,0,&ps,&error));
    check(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex_));
    check(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel_));
    const D3D11_INPUT_ELEMENT_DESC input[]={
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",1,DXGI_FORMAT_R32G32_FLOAT,0,20,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",2,DXGI_FORMAT_R32G32_FLOAT,0,28,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",3,DXGI_FORMAT_R32G32_FLOAT,0,36,D3D11_INPUT_PER_VERTEX_DATA,0}};
    check(device->CreateInputLayout(input,5,vs->GetBufferPointer(),vs->GetBufferSize(),&layout_));
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=sizeof(plan.vertices);bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA data{plan.vertices,0,0};check(device->CreateBuffer(&bd,&data,&vertices_));
    float parameters[12]={(float)width,(float)height,0,0};
    for(unsigned i=0;i<2;i++){
        uint32_t packed;if(!uc_point_light_constant(plan.constants[i],&packed))throw std::runtime_error("Glow packing failed");
        for(unsigned k=0;k<3;k++)parameters[4+i*4+k]=float((packed>>(16-k*8))&255)/255.f;
        parameters[7+i*4]=float(packed>>24)/255.f;
    }
    bd.ByteWidth=sizeof(parameters);bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;data.pSysMem=parameters;check(device->CreateBuffer(&bd,&data,&constants_));
    D3D11_TEXTURE2D_DESC td{};td.Width=width;td.Height=height;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
    for(unsigned i=0;i<2;i++){check(device->CreateTexture2D(&td,nullptr,&image_[i]));check(device->CreateShaderResourceView(image_[i].Get(),nullptr,&view_[i]));check(device->CreateRenderTargetView(image_[i].Get(),nullptr,&target_[i]));}
    D3D11_BLEND_DESC blend{};auto &b=blend.RenderTarget[0];b.BlendEnable=TRUE;b.SrcBlend=b.SrcBlendAlpha=D3D11_BLEND_ONE;b.DestBlend=b.DestBlendAlpha=D3D11_BLEND_SRC_ALPHA;b.BlendOp=b.BlendOpAlpha=D3D11_BLEND_OP_ADD;b.RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;check(device->CreateBlendState(&blend,&blend_));
    D3D11_DEPTH_STENCIL_DESC depth{};check(device->CreateDepthStencilState(&depth,&depth_));
    D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;check(device->CreateRasterizerState(&raster,&raster_));
    D3D11_SAMPLER_DESC sampler{};sampler.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sampler.MaxLOD=0;check(device->CreateSamplerState(&sampler,&sampler_));
}
void NativeGlowFilter::apply(ID3D11Texture2D *input){
    if(!sampler_||!input||input==image_[0].Get()||input==image_[1].Get())throw std::invalid_argument("Unsupported glow input");
    D3D11_TEXTURE2D_DESC td{};input->GetDesc(&td);ComPtr<ID3D11Device> owner;input->GetDevice(&owner);
    if(owner.Get()!=device_.Get()||td.Width!=width_||td.Height!=height_||td.Format!=DXGI_FORMAT_R8G8B8A8_UNORM||td.MipLevels!=1||td.ArraySize!=1||td.SampleDesc.Count!=1)
        throw std::invalid_argument("Glow input mismatch");
    ID3D11ShaderResourceView *null_view=nullptr;context_->PSSetShaderResources(0,1,&null_view);context_->OMSetRenderTargets(0,nullptr,nullptr);
    context_->CopyResource(image_[0].Get(),input);
    context_->VSSetShader(vertex_.Get(),nullptr,0);context_->PSSetShader(pixel_.Get(),nullptr,0);context_->GSSetShader(nullptr,nullptr,0);context_->HSSetShader(nullptr,nullptr,0);context_->DSSetShader(nullptr,nullptr,0);
    context_->IASetInputLayout(layout_.Get());context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ID3D11Buffer *cb=constants_.Get(),*vb=vertices_.Get();unsigned stride=44,offset=0;context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);context_->VSSetConstantBuffers(0,1,&cb);context_->PSSetConstantBuffers(0,1,&cb);
    ID3D11SamplerState *sampler=sampler_.Get();context_->PSSetSamplers(0,1,&sampler);context_->OMSetBlendState(blend_.Get(),nullptr,~0u);context_->OMSetDepthStencilState(depth_.Get(),0);context_->RSSetState(raster_.Get());
    D3D11_VIEWPORT viewport{0,0,(float)width_,(float)height_,0,1};context_->RSSetViewports(1,&viewport);
    for(unsigned pass=0;pass<2;pass++){
        const unsigned src=pass,dst=1-pass;context_->PSSetShaderResources(0,1,&null_view);context_->OMSetRenderTargets(0,nullptr,nullptr);context_->CopyResource(image_[dst].Get(),image_[src].Get());
        ID3D11RenderTargetView *rt=target_[dst].Get();ID3D11ShaderResourceView *view=view_[src].Get();context_->OMSetRenderTargets(1,&rt,nullptr);context_->PSSetShaderResources(0,1,&view);context_->Draw(4,pass*4);
    }
    context_->PSSetShaderResources(0,1,&null_view);context_->OMSetRenderTargets(0,nullptr,nullptr);
}
