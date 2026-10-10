#include "tone_history.h"
#include "../point_light_pixel.h"
#include <d3dcompiler.h>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
static void check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Tone/history operation failed");}
void NativeToneHistory::open(ID3D11Device *d,ID3D11DeviceContext *c,ID3D11Texture2D *source,
    ID3D11Texture2D *previous,const std::string &pixel_source){
    if(device_||!d||!c||!source||c->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)throw std::invalid_argument("Invalid tone/history owner");
    ComPtr<ID3D11Device>owner;c->GetDevice(&owner);if(owner.Get()!=d)throw std::invalid_argument("Foreign tone/history context");
    D3D11_TEXTURE2D_DESC td{};source->GetDesc(&td);
    auto valid=[&](ID3D11Texture2D *p){D3D11_TEXTURE2D_DESC v{};p->GetDesc(&v);p->GetDevice(&owner);return owner.Get()==d&&v.Width==td.Width&&v.Height==td.Height&&v.Format==DXGI_FORMAT_R8G8B8A8_UNORM&&v.MipLevels==1&&v.ArraySize==1&&v.SampleDesc.Count==1&&(v.BindFlags&D3D11_BIND_SHADER_RESOURCE);};
    UCEnvironmentToneDrawInput input{td.Width,td.Height,{0,0,(float)td.Width,(float)td.Height},0,previous!=nullptr};UCEnvironmentToneDrawPlan plan;
    if(!valid(source)||(previous&&!valid(previous))||uc_environment_tone_draw(&input,&plan))throw std::invalid_argument("Unsupported tone/history images");
    device_=d;context_=c;source_=source;previous_=previous;width_=td.Width;height_=td.Height;
    check(d->CreateShaderResourceView(source,nullptr,&source_view_));if(previous)check(d->CreateShaderResourceView(previous,nullptr,&previous_view_));
    td.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;td.Usage=D3D11_USAGE_DEFAULT;td.CPUAccessFlags=td.MiscFlags=0;
    for(unsigned i=0;i<2;i++){check(d->CreateTexture2D(&td,nullptr,&images_[i]));check(d->CreateRenderTargetView(images_[i].Get(),nullptr,&targets_[i]));}
    check(d->CreateShaderResourceView(images_[0].Get(),nullptr,&tone_view_));td.Width=256;td.Height=1;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    check(d->CreateTexture2D(&td,nullptr,&lookup_));check(d->CreateShaderResourceView(lookup_.Get(),nullptr,&lookup_view_));
    const std::string code=pixel_source+R"(
cbuffer Params:register(b0){float4 dimensions;float4 amount;};
struct Input{float3 position:POSITION;float2 uv:TEXCOORD0;};
struct Pixel{float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
Pixel vertex(Input v){Pixel p;p.position=float4(v.position.x/dimensions.x*2-1,1-v.position.y/dimensions.y*2,0,1);p.uv=v.uv;return p;}
Texture2D<float4> current:register(t0);Texture2D<float4> other:register(t1);
float4 lut_pixel(Pixel p):SV_TARGET{
    uint3 index=(uint3)round(current.Load(int3((int2)p.position.xy,0)).rgb*255);
    return float4(other.Load(int3(index.r,0,0)).r,other.Load(int3(index.g,0,0)).g,other.Load(int3(index.b,0,0)).b,0);
}
float4 history_pixel(Pixel p):SV_TARGET{return environment_history(current.Load(int3((int2)p.position.xy,0)),other.Load(int3((int2)p.position.xy,0)),amount);}
)";
    const unsigned flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;
    ComPtr<ID3DBlob>vs,error;check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"vertex","vs_5_0",flags,0,&vs,&error));check(d->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex_));
    for(unsigned i=0;i<2;i++){ComPtr<ID3DBlob>ps;check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,i?"history_pixel":"lut_pixel","ps_5_0",flags,0,&ps,&error));check(d->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel_[i]));}
    const D3D11_INPUT_ELEMENT_DESC e[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
    check(d->CreateInputLayout(e,2,vs->GetBufferPointer(),vs->GetBufferSize(),&layout_));
    D3D11_BUFFER_DESC b{};b.ByteWidth=80;b.BindFlags=D3D11_BIND_VERTEX_BUFFER;check(d->CreateBuffer(&b,nullptr,&vertices_));b.ByteWidth=32;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(d->CreateBuffer(&b,nullptr,&constants_));
    D3D11_BLEND_DESC blend{};blend.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;check(d->CreateBlendState(&blend,&blend_));
    D3D11_DEPTH_STENCIL_DESC depth{};check(d->CreateDepthStencilState(&depth,&depth_));D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;check(d->CreateRasterizerState(&raster,&raster_));
}
void NativeToneHistory::draw(unsigned pass,const UCEnvironmentToneDrawPlan &plan){
    float params[8]={(float)width_,(float)height_,0,0};uint32_t packed;
    if(!uc_point_light_constant(plan.history_constant,&packed))throw std::runtime_error("Unsupported history constant");
    for(unsigned k=0;k<4;k++)params[4+k]=(float)((packed>>(k==3?24:16-k*8))&255)/255.f;
    context_->UpdateSubresource(constants_.Get(),0,nullptr,params,0,0);ID3D11Buffer *cb=constants_.Get(),*vb=vertices_.Get();unsigned stride=20,offset=0;
    context_->IASetInputLayout(layout_.Get());context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    context_->VSSetShader(vertex_.Get(),nullptr,0);context_->PSSetShader(pixel_[pass].Get(),nullptr,0);context_->GSSetShader(nullptr,nullptr,0);context_->HSSetShader(nullptr,nullptr,0);context_->DSSetShader(nullptr,nullptr,0);
    context_->VSSetConstantBuffers(0,1,&cb);context_->PSSetConstantBuffers(0,1,&cb);context_->OMSetBlendState(blend_.Get(),nullptr,~0u);context_->OMSetDepthStencilState(depth_.Get(),0);context_->RSSetState(raster_.Get());
    D3D11_VIEWPORT viewport{0,0,(float)width_,(float)height_,0,1};context_->RSSetViewports(1,&viewport);
    ID3D11RenderTargetView *rt=targets_[pass].Get();ID3D11ShaderResourceView *views[]={pass?tone_view_.Get():source_view_.Get(),pass?(plan.bind_previous?previous_view_.Get():nullptr):lookup_view_.Get()};
    context_->OMSetRenderTargets(1,&rt,nullptr);context_->PSSetShaderResources(0,2,views);context_->Draw(4,0);
    views[0]=views[1]=nullptr;context_->PSSetShaderResources(0,2,views);context_->OMSetRenderTargets(0,nullptr,nullptr);
}
void NativeToneHistory::apply(const float view[4],const unsigned char rgb[768],void *opaque,float (*history)(void *)){
    if(!raster_||!view||!rgb||!history)throw std::invalid_argument("Invalid tone/history request");
    UCEnvironmentToneDrawInput input{width_,height_,{view[0],view[1],view[2],view[3]},0,previous_!=nullptr};UCEnvironmentToneDrawPlan plan;
    if(uc_environment_tone_draw(&input,&plan)||plan.vertices[1].position[0]<plan.vertices[0].position[0]||plan.vertices[2].position[1]<plan.vertices[0].position[1])throw std::invalid_argument("Unsupported tone/history rectangle");
    unsigned char rgba[1024];for(unsigned i=0;i<256;i++){rgba[i*4]=rgb[i];rgba[i*4+1]=rgb[256+i];rgba[i*4+2]=rgb[512+i];rgba[i*4+3]=0;}
    UCEnvironmentToneVertex vertices[]={plan.vertices[0],plan.vertices[1],plan.vertices[3],plan.vertices[2]};
    ID3D11ShaderResourceView *nulls[]={nullptr,nullptr};context_->PSSetShaderResources(0,2,nulls);context_->OMSetRenderTargets(0,nullptr,nullptr);
    context_->UpdateSubresource(lookup_.Get(),0,nullptr,rgba,sizeof rgba,sizeof rgba);context_->UpdateSubresource(vertices_.Get(),0,nullptr,vertices,0,0);
    context_->CopyResource(images_[0].Get(),source_.Get());draw(0,plan);
    input.history=history(opaque);
    if(uc_environment_tone_draw(&input,&plan)||(plan.request_previous&&!plan.bind_previous))throw std::runtime_error("Unsupported history service result after colour draw");
    context_->CopyResource(images_[1].Get(),images_[0].Get());draw(1,plan);
}
