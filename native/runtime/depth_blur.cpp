#include "depth_blur.h"
#include "glow_filter.h"
#include "../environment_fog.h"
#include <d3dcompiler.h>
#include <array>
#include <cstring>
#include <stdexcept>
static void check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Depth blur GPU operation failed");}
struct NativeDepthBlur::State {
 NativeGlowFilter filter;
 ComPtr<ID3D11Texture2D> snapshot,downsampled,lookup;
 ComPtr<ID3D11ShaderResourceView> source,blurred,table;
 ComPtr<ID3D11RenderTargetView> small_target;
 ComPtr<ID3D11VertexShader> vertex;
 ComPtr<ID3D11PixelShader> sample,composite;
 ComPtr<ID3D11Buffer> constants;
 ComPtr<ID3D11DepthStencilState> depth;
 ComPtr<ID3D11BlendState> blend;
 ComPtr<ID3D11SamplerState> linear,point;
 std::array<uint8_t,65536>alpha{};float cached[5]{};bool valid=false;
 ID3D11Texture2D*identity=nullptr;unsigned w=0,h=0;
};
NativeDepthBlur::NativeDepthBlur(const std::string&s):shader_(s){}
NativeDepthBlur::~NativeDepthBlur()=default;
void NativeDepthBlur::draw(NativeRenderer&r,float m22,float m32,const UCDepthBlurSettings&settings){
 if(!settings.enabled)return;
 if(!state_||state_->identity!=r.color.Get()||state_->w!=r.width||state_->h!=r.height){
  UCDepthBlurPlan plan{};if(uc_depth_blur_plan(r.width,r.height,&plan))throw std::invalid_argument("Depth blur viewport");
  auto s=std::make_unique<State>();s->w=r.width;s->h=r.height;s->identity=r.color.Get();
  D3D11_TEXTURE2D_DESC td{};r.color->GetDesc(&td);
  if(td.Format!=DXGI_FORMAT_R8G8B8A8_UNORM||td.SampleDesc.Count!=1)throw std::invalid_argument("Depth blur color surface");
  td.BindFlags=D3D11_BIND_SHADER_RESOURCE;check(r.device->CreateTexture2D(&td,nullptr,&s->snapshot));check(r.device->CreateShaderResourceView(s->snapshot.Get(),nullptr,&s->source));
  td.Width=r.width/2;td.Height=r.height/2;td.BindFlags|=D3D11_BIND_RENDER_TARGET;
  check(r.device->CreateTexture2D(&td,nullptr,&s->downsampled));check(r.device->CreateRenderTargetView(s->downsampled.Get(),nullptr,&s->small_target));
  s->filter.open(r.device.Get(),r.context.Get(),td.Width,td.Height,shader_,plan.sigma);check(r.device->CreateShaderResourceView(s->filter.output(),nullptr,&s->blurred));
  td.Width=td.Height=256;td.Format=DXGI_FORMAT_R8_UNORM;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
  check(r.device->CreateTexture2D(&td,nullptr,&s->lookup));check(r.device->CreateShaderResourceView(s->lookup.Get(),nullptr,&s->table));
  const char*code=R"(
cbuffer Params:register(b0){float4 extent;};
float4 vs(uint id:SV_VertexID):SV_POSITION{float2 p=float2((id<<1)&2,id&2);return float4(p*float2(2,-2)+float2(-1,1),0,1);}
Texture2D<float4> image:register(t0);Texture2D<float> depth:register(t1);Texture2D<float> lut:register(t2);SamplerState sampling:register(s0);
float4 copy(float4 p:SV_POSITION):SV_TARGET{return float4(image.SampleLevel(sampling,p.xy/extent.xy,0).rgb,1);}
float4 composite(float4 p:SV_POSITION):SV_TARGET{
 uint z=(uint)round(saturate(depth.Load(int3((int2)p.xy,0)))*16777215.0);
 float amount=lut.Load(int3((z>>8)&255,(z>>16)&255,0));
 return float4(image.SampleLevel(sampling,p.xy/extent.xy,0).rgb,amount);
})";
  auto compile=[&](const char*entry,const char*target){ComPtr<ID3DBlob>b,e;check(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,entry,target,D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&e));return b;};
  auto vs=compile("vs","vs_5_0"),ps=compile("copy","ps_5_0"),blend=compile("composite","ps_5_0");
  check(r.device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&s->vertex));check(r.device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&s->sample));check(r.device->CreatePixelShader(blend->GetBufferPointer(),blend->GetBufferSize(),nullptr,&s->composite));
  D3D11_BUFFER_DESC bd{};bd.ByteWidth=16;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(r.device->CreateBuffer(&bd,nullptr,&s->constants));
  D3D11_DEPTH_STENCIL_DESC ds{};check(r.device->CreateDepthStencilState(&ds,&s->depth));
  D3D11_BLEND_DESC bs{};auto&b=bs.RenderTarget[0];b.BlendEnable=TRUE;b.SrcBlend=D3D11_BLEND_SRC_ALPHA;b.DestBlend=D3D11_BLEND_INV_SRC_ALPHA;b.BlendOp=D3D11_BLEND_OP_ADD;b.SrcBlendAlpha=D3D11_BLEND_ONE;b.DestBlendAlpha=D3D11_BLEND_ZERO;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;b.RenderTargetWriteMask=7;check(r.device->CreateBlendState(&bs,&s->blend));
  D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=0;check(r.device->CreateSamplerState(&sd,&s->linear));sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;check(r.device->CreateSamplerState(&sd,&s->point));
  state_=std::move(s);
 }
 auto&s=*state_;const float cache[5]={settings.start,settings.end,settings.amplitude,m22,m32};
 if(!s.valid||memcmp(cache,s.cached,sizeof cache)){
  if(uc_environment_depth_lookup(cache[0],cache[1],cache[2],m22,m32,s.alpha.data())!=UC_FOG_OK)throw std::invalid_argument("Unsupported depth blur projection/range");
  r.context->UpdateSubresource(s.lookup.Get(),0,nullptr,s.alpha.data(),256,65536);memcpy(s.cached,cache,sizeof cache);s.valid=true;
 }
 ID3D11ShaderResourceView*empty[3]{};r.context->PSSetShaderResources(0,3,empty);r.context->OMSetRenderTargets(0,nullptr,nullptr);r.context->CopyResource(s.snapshot.Get(),r.color.Get());
 auto draw=[&](unsigned w,unsigned h,ID3D11RenderTargetView*target,ID3D11PixelShader*pixel,ID3D11ShaderResourceView*image,ID3D11SamplerState*sampler,ID3D11BlendState*blend){
  const float extent[4]={(float)w,(float)h,0,0};r.context->UpdateSubresource(s.constants.Get(),0,nullptr,extent,0,0);ID3D11Buffer*cb=s.constants.Get();
  r.context->IASetInputLayout(nullptr);r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);r.context->VSSetShader(s.vertex.Get(),nullptr,0);r.context->PSSetShader(pixel,nullptr,0);r.context->GSSetShader(nullptr,nullptr,0);r.context->PSSetConstantBuffers(0,1,&cb);
  r.context->OMSetRenderTargets(1,&target,nullptr);r.context->OMSetDepthStencilState(s.depth.Get(),0);r.context->OMSetBlendState(blend,nullptr,~0u);r.context->RSSetState(r.rasterizer.Get());D3D11_VIEWPORT v{0,0,(float)w,(float)h,0,1};r.context->RSSetViewports(1,&v);
  ID3D11ShaderResourceView*views[3]={image,pixel==s.composite.Get()?r.depth_resource.Get():nullptr,pixel==s.composite.Get()?s.table.Get():nullptr};r.context->PSSetShaderResources(0,3,views);r.context->PSSetSamplers(0,1,&sampler);r.context->Draw(3,0);r.context->PSSetShaderResources(0,3,empty);r.context->OMSetRenderTargets(0,nullptr,nullptr);
 };
 draw(s.w/2,s.h/2,s.small_target.Get(),s.sample.Get(),s.source.Get(),s.linear.Get(),nullptr);s.filter.apply(s.downsampled.Get());
 draw(s.w,s.h,r.target.Get(),s.composite.Get(),s.blurred.Get(),s.point.Get(),s.blend.Get());
 ID3D11RenderTargetView*target=r.target.Get();r.context->OMSetRenderTargets(1,&target,r.depth_view.Get());r.context->IASetInputLayout(r.layout.Get());r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);ID3D11Buffer*cb=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&cb);r.context->PSSetConstantBuffers(0,1,&cb);
}
