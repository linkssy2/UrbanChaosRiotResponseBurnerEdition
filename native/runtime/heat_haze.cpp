#include "heat_haze.h"
#include "../heat_distortion.h"
#include <d3dcompiler.h>
#include <cstring>
#include <stdexcept>
static void check(HRESULT h){if(FAILED(h))throw std::runtime_error("Heat composition GPU operation failed");}
void NativeHeatHaze::open(NativeRenderer&r,const std::string&source){
    if(device_||r.width<2||r.height<2||r.width>4096||r.height>4096)throw std::invalid_argument("Heat composition dimensions");
    device_=r.device;context_=r.context;width_=r.width;height_=r.height;
    if(uc_heat_distortion_table(table_)||uc_heat_distortion_grid(&previous_,0,table_,grid_))throw std::runtime_error("Heat phase initialization");
    D3D11_TEXTURE2D_DESC td{};td.Width=width_;td.Height=height_;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;
    td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    check(device_->CreateTexture2D(&td,nullptr,&mask_));check(device_->CreateRenderTargetView(mask_.Get(),nullptr,&mask_target_));check(device_->CreateShaderResourceView(mask_.Get(),nullptr,&mask_view_));
    td.BindFlags=D3D11_BIND_SHADER_RESOURCE;check(device_->CreateTexture2D(&td,nullptr,&snapshot_));check(device_->CreateShaderResourceView(snapshot_.Get(),nullptr,&snapshot_view_));
    td.Width=width_/2;td.Height=height_/2;td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    check(device_->CreateTexture2D(&td,nullptr,&small_));check(device_->CreateRenderTargetView(small_.Get(),nullptr,&small_target_));
    check(device_->CreateTexture2D(&td,nullptr,&composed_));check(device_->CreateRenderTargetView(composed_.Get(),nullptr,&composed_target_));check(device_->CreateShaderResourceView(composed_.Get(),nullptr,&composed_view_));
    filter_.open(device_.Get(),context_.Get(),td.Width,td.Height,source,1.25f);
    const char*code=R"(
cbuffer Params:register(b0){float4 dimensions;};
struct Input{float3 position:POSITION;float2 uv:TEXCOORD0;};
struct Pixel{float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
Pixel vertex(Input p){Pixel q;q.position=float4(p.position.x/dimensions.x*2-1,1-p.position.y/dimensions.y*2,0,1);q.uv=p.uv/dimensions.zw;return q;}
Texture2D<float4> source:register(t0);SamplerState image_sampler:register(s0);
float4 pixel(Pixel p):SV_TARGET{return source.SampleLevel(image_sampler,p.uv,0);}
float4 copy_pixel(Pixel p):SV_TARGET{return float4(source.SampleLevel(image_sampler,p.uv,0).rgb,0);}
)";
    ComPtr<ID3DBlob>vs,ps,error;unsigned flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;
    check(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"vertex","vs_5_0",flags,0,&vs,&error));check(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"pixel","ps_5_0",flags,0,&ps,&error));
    check(device_->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex_));check(device_->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel_));
    ps.Reset();check(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"copy_pixel","ps_5_0",flags,0,&ps,&error));check(device_->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&copy_pixel_));
    const D3D11_INPUT_ELEMENT_DESC layout[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
    check(device_->CreateInputLayout(layout,2,vs->GetBufferPointer(),vs->GetBufferSize(),&layout_));
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=sizeof(UCHeatComposition);bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;check(device_->CreateBuffer(&bd,nullptr,&vertices_));bd.ByteWidth=16;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(device_->CreateBuffer(&bd,nullptr,&constants_));
    for(unsigned i=0;i<4;i++){D3D11_BLEND_DESC desc{};auto&b=desc.RenderTarget[0];b.BlendEnable=i==2;b.SrcBlend=b.SrcBlendAlpha=D3D11_BLEND_SRC_ALPHA;b.DestBlend=b.DestBlendAlpha=D3D11_BLEND_INV_SRC_ALPHA;b.BlendOp=b.BlendOpAlpha=D3D11_BLEND_OP_ADD;
        b.RenderTargetWriteMask=i==0?15:i==1?8:i==2?7:10;check(device_->CreateBlendState(&desc,&blend_[i]));}
    D3D11_SAMPLER_DESC sampler{};sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sampler.MaxLOD=0;
    for(unsigned i=0;i<2;i++){sampler.Filter=i?D3D11_FILTER_MIN_MAG_MIP_POINT:D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;check(device_->CreateSamplerState(&sampler,&sampler_[i]));}
    D3D11_DEPTH_STENCIL_DESC depth{};check(device_->CreateDepthStencilState(&depth,&depth_));D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;check(device_->CreateRasterizerState(&raster,&raster_));
}
void NativeHeatHaze::begin(NativeRenderer&r){
    if(!device_||pending_||r.device.Get()!=device_.Get()||r.width!=width_||r.height!=height_)throw std::logic_error("Heat frame owner");
    UINT count=1;context_->RSGetViewports(&count,&viewport_);if(count!=1)throw std::logic_error("Heat viewport owner");
    const float clear[4]{};context_->ClearRenderTargetView(mask_target_.Get(),clear);pending_=false;
}
void NativeHeatHaze::bind_mask(NativeRenderer&r){
    ID3D11RenderTargetView*rt=mask_target_.Get();context_->OMSetRenderTargets(1,&rt,r.depth_view.Get());context_->OMSetBlendState(blend_[3].Get(),nullptr,~0u);pending_=true;
}
void NativeHeatHaze::draw(unsigned pass,const UCHeatComposition&,ID3D11ShaderResourceView*source,ID3D11RenderTargetView*target,unsigned w,unsigned h){
    float dimensions[4]={(float)w,(float)h,(float)(pass==2?width_/2:width_),(float)(pass==2?height_/2:height_)};context_->UpdateSubresource(constants_.Get(),0,nullptr,dimensions,0,0);
    context_->VSSetShader(vertex_.Get(),nullptr,0);context_->PSSetShader(pass?pixel_.Get():copy_pixel_.Get(),nullptr,0);context_->GSSetShader(nullptr,nullptr,0);context_->HSSetShader(nullptr,nullptr,0);context_->DSSetShader(nullptr,nullptr,0);
    context_->IASetInputLayout(layout_.Get());context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);ID3D11Buffer*vb=vertices_.Get(),*cb=constants_.Get();UINT stride=20,offset=0;context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);context_->VSSetConstantBuffers(0,1,&cb);context_->PSSetConstantBuffers(0,1,&cb);
    ID3D11SamplerState*sampler=sampler_[pass==2?1:0].Get();context_->PSSetSamplers(0,1,&sampler);context_->OMSetBlendState(blend_[pass].Get(),nullptr,~0u);context_->OMSetDepthStencilState(depth_.Get(),0);context_->RSSetState(raster_.Get());
    D3D11_VIEWPORT viewport{0,0,(float)w,(float)h,0,1};context_->RSSetViewports(1,&viewport);context_->OMSetRenderTargets(1,&target,nullptr);context_->PSSetShaderResources(0,1,&source);
    if(pass==2)for(unsigned col=0;col<19;col++)context_->Draw(30,8+30*col);else context_->Draw(4,4*pass);
    source=nullptr;context_->PSSetShaderResources(0,1,&source);context_->OMSetRenderTargets(0,nullptr,nullptr);
}
void NativeHeatHaze::finish(NativeRenderer&r,float phase){
    if(!pending_)return;
    const float view[4]={viewport_.TopLeftX,viewport_.TopLeftY,viewport_.Width,viewport_.Height};UCHeatComposition plan{};
    if(uc_heat_distortion_grid(&previous_,phase,table_,grid_)||uc_heat_composition(width_,height_,view,grid_,&plan))throw std::runtime_error("Heat draw plan");
    ID3D11ShaderResourceView*none=nullptr;context_->PSSetShaderResources(0,1,&none);context_->OMSetRenderTargets(0,nullptr,nullptr);context_->CopyResource(snapshot_.Get(),r.color.Get());context_->UpdateSubresource(vertices_.Get(),0,nullptr,&plan,0,0);
    draw(0,plan,snapshot_view_.Get(),small_target_.Get(),width_/2,height_/2);filter_.apply(small_.Get());context_->CopyResource(composed_.Get(),filter_.horizontal_output());
    draw(1,plan,mask_view_.Get(),composed_target_.Get(),width_/2,height_/2);draw(2,plan,composed_view_.Get(),r.target.Get(),width_,height_);
    ID3D11RenderTargetView*rt=r.target.Get();context_->OMSetRenderTargets(1,&rt,r.depth_view.Get());context_->RSSetViewports(1,&viewport_);pending_=false;
}
