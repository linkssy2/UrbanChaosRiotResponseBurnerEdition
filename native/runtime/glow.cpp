#include "glow.h"
#include "../point_light_pixel.h"
#include <d3dcompiler.h>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
static void check(HRESULT value){if(FAILED(value))throw std::runtime_error("Glow pipeline operation failed");}
void NativeGlow::open(ID3D11Device *device,ID3D11DeviceContext *context,ID3D11Texture2D *source,
    unsigned sw,unsigned sh,const std::string &pixel_source){
    if(device_||!device||!context||!source||context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)
        throw std::invalid_argument("Unsupported glow owner");
    ComPtr<ID3D11Device>a,b;source->GetDevice(&a);context->GetDevice(&b);D3D11_TEXTURE2D_DESC td{};source->GetDesc(&td);
    if(a.Get()!=device||b.Get()!=device||td.Format!=DXGI_FORMAT_R8G8B8A8_UNORM||td.MipLevels!=1||td.ArraySize!=1||td.SampleDesc.Count!=1||!(td.BindFlags&D3D11_BIND_SHADER_RESOURCE))
        throw std::invalid_argument("Unsupported glow source");
    UCEnvironmentGlowOuterInput input{td.Width,td.Height,sw,sh,td.Width,td.Height,{0,0,(float)td.Width,(float)td.Height},0,1};UCEnvironmentGlowOuterPlan plan;
    if(uc_environment_glow_outer(&input,&plan))throw std::invalid_argument("Unsupported glow dimensions");
    device_=device;context_=context;source_=source;width_=td.Width;height_=td.Height;scratch_width_=sw;scratch_height_=sh;
    check(device->CreateShaderResourceView(source,nullptr,&source_view_));
    td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;td.Usage=D3D11_USAGE_DEFAULT;td.CPUAccessFlags=td.MiscFlags=0;
    check(device->CreateTexture2D(&td,nullptr,&output_));check(device->CreateRenderTargetView(output_.Get(),nullptr,&output_target_));
    td.Width=sw;td.Height=sh;check(device->CreateTexture2D(&td,nullptr,&threshold_));check(device->CreateRenderTargetView(threshold_.Get(),nullptr,&threshold_target_));
    filter_.open(device,context,sw,sh,pixel_source);check(device->CreateShaderResourceView(filter_.output(),nullptr,&filtered_view_));
    const char *wrapper=R"(
cbuffer Params:register(b0){float4 dimensions;float4 weight;};
struct Input {float3 position:POSITION;float2 uv:TEXCOORD0;};
struct Pixel {float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
Pixel vertex(Input v){Pixel p;p.position=float4(v.position.x/dimensions.x*2-1,1-v.position.y/dimensions.y*2,0,1);p.uv=v.uv/dimensions.zw;return p;}
Texture2D<float4> original:register(t0);SamplerState linear_clamp:register(s0);
float4 threshold_pixel(Pixel p):SV_TARGET{return environment_threshold(original.SampleLevel(linear_clamp,p.uv,0),weight);}
float4 composite_pixel(Pixel p):SV_TARGET{return environment_composite(original.SampleLevel(linear_clamp,p.uv,0),weight);}
)";
    const std::string code=pixel_source+wrapper;const unsigned flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;
    ComPtr<ID3DBlob>vs,error;check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"vertex","vs_5_0",flags,0,&vs,&error));check(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex_));
    for(unsigned i=0;i<2;i++){
        ComPtr<ID3DBlob>ps;check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,i?"composite_pixel":"threshold_pixel","ps_5_0",flags,0,&ps,&error));check(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel_[i]));
        D3D11_BLEND_DESC blend{};auto &rt=blend.RenderTarget[0];rt.BlendEnable=i;rt.SrcBlend=rt.SrcBlendAlpha=rt.DestBlend=rt.DestBlendAlpha=D3D11_BLEND_ONE;rt.BlendOp=rt.BlendOpAlpha=D3D11_BLEND_OP_ADD;rt.RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;check(device->CreateBlendState(&blend,&blend_[i]));
        D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;raster.ScissorEnable=i;check(device->CreateRasterizerState(&raster,&raster_[i]));
    }
    const D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
    check(device->CreateInputLayout(elements,2,vs->GetBufferPointer(),vs->GetBufferSize(),&layout_));
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=sizeof(plan.vertices);bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;check(device->CreateBuffer(&bd,nullptr,&vertices_));bd.ByteWidth=32;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(device->CreateBuffer(&bd,nullptr,&constants_));
    D3D11_DEPTH_STENCIL_DESC depth{};check(device->CreateDepthStencilState(&depth,&depth_));D3D11_SAMPLER_DESC sampler{};sampler.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sampler.MaxLOD=0;check(device->CreateSamplerState(&sampler,&sampler_));
}
void NativeGlow::draw(unsigned pass,const UCEnvironmentGlowOuterPlan &plan){
    float cb[8]={(float)(pass?width_:scratch_width_),(float)(pass?height_:scratch_height_),(float)(pass?scratch_width_:width_),(float)(pass?scratch_height_:height_)};
    uint32_t packed;if(!uc_point_light_constant(plan.constants[pass],&packed))throw std::runtime_error("Glow constant packing failed");
    for(unsigned k=0;k<3;k++)cb[4+k]=float((packed>>(16-k*8))&255)/255.f;cb[7]=float(packed>>24)/255.f;
    context_->UpdateSubresource(constants_.Get(),0,nullptr,cb,0,0);
    context_->VSSetShader(vertex_.Get(),nullptr,0);context_->PSSetShader(pixel_[pass].Get(),nullptr,0);context_->GSSetShader(nullptr,nullptr,0);context_->HSSetShader(nullptr,nullptr,0);context_->DSSetShader(nullptr,nullptr,0);
    context_->IASetInputLayout(layout_.Get());context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ID3D11Buffer *vb=vertices_.Get(),*c=constants_.Get();unsigned stride=20,offset=0;context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);context_->VSSetConstantBuffers(0,1,&c);context_->PSSetConstantBuffers(0,1,&c);
    ID3D11SamplerState *sampler=sampler_.Get();context_->PSSetSamplers(0,1,&sampler);context_->OMSetBlendState(blend_[pass].Get(),nullptr,~0u);context_->OMSetDepthStencilState(depth_.Get(),0);context_->RSSetState(raster_[pass].Get());
    D3D11_VIEWPORT viewport{0,0,cb[0],cb[1],0,1};context_->RSSetViewports(1,&viewport);
    if(pass){D3D11_RECT scissor{plan.scissor[0],plan.scissor[1],plan.scissor[2],plan.scissor[3]};context_->RSSetScissorRects(1,&scissor);}
    ID3D11RenderTargetView *rt=pass?output_target_.Get():threshold_target_.Get();ID3D11ShaderResourceView *view=pass?filtered_view_.Get():source_view_.Get();context_->OMSetRenderTargets(1,&rt,nullptr);context_->PSSetShaderResources(0,1,&view);context_->Draw(4,pass*4);
    view=nullptr;context_->PSSetShaderResources(0,1,&view);context_->OMSetRenderTargets(0,nullptr,nullptr);
}
void NativeGlow::apply(const UCEnvironmentGlowOuterInput &input){
    UCEnvironmentGlowOuterPlan plan;
    if(!sampler_||input.scene_width!=width_||input.scene_height!=height_||input.scratch_width!=scratch_width_||input.scratch_height!=scratch_height_||uc_environment_glow_outer(&input,&plan)||plan.scissor[2]<plan.scissor[0]||plan.scissor[3]<plan.scissor[1])
        throw std::invalid_argument("Unsupported glow frame");
    ID3D11ShaderResourceView *null_view=nullptr;context_->PSSetShaderResources(0,1,&null_view);context_->OMSetRenderTargets(0,nullptr,nullptr);
    context_->CopyResource(output_.Get(),source_.Get());context_->UpdateSubresource(vertices_.Get(),0,nullptr,plan.vertices,0,0);
    draw(0,plan);filter_.apply(threshold_.Get());draw(1,plan);
}
