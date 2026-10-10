extern "C" {
#include "../particle_billboard.h"
}
#include "particle_billboards.h"
#include <d3dcompiler.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
static_assert(sizeof(UCParticleBillboard)==28&&sizeof(NativeParticleBillboardView)==144,"GPU instance/constant layout");
static void check(HRESULT h){if(FAILED(h)){char message[80];sprintf_s(message,"Particle billboard GPU error 0x%08lX",(unsigned long)h);throw std::runtime_error(message);}}
void NativeParticleBillboards::open(ID3D11Device *device,ID3D11DeviceContext *context,const std::string &vs,const std::string &ps){
    ComPtr<ID3D11Device> owner;if(context)context->GetDevice(&owner);
    if(device_||!device||!context||owner.Get()!=device||context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)throw std::invalid_argument("Unsupported billboard GPU owner");
    device_=device;context_=context;
    const std::string code=vs+ps+R"(
cbuffer Params:register(b0){float4 rows[4];float4 up;float4 right;float4 fog_color;
 float slope;float bias;uint fog_enabled;uint alpha_test;uint texture_alpha_kill;uint3 padding;};
struct Input{float4 center_radius:POSITION;float2 trig:TEXCOORD0;float4 color:COLOR0;};
struct Pixel{float4 position:SV_POSITION;float4 color:COLOR0;float2 uv:TEXCOORD0;float fog:TEXCOORD1;};
Pixel vertex(Input v,uint id:SV_VertexID){
 static const uint corners[6]={0,1,2,0,2,3};
 ParticleBillboardVertex p=particle_billboard_vertex(v.center_radius.xyz,v.center_radius.w,v.trig.x,v.trig.y,
  v.color.bgra,corners[id],rows[0],rows[1],rows[2],rows[3],up,right,float3(1,1,1),float3(0,0,0));
 precise float factor=(bias+p.fog*slope)-1;
 Pixel r;r.position=p.clip;r.color=p.color;r.uv=p.uv.xy;r.fog=fog_enabled?factor:1;return r;
}
Texture2D<float4> image:register(t0);SamplerState image_sampler:register(s0);
float4 pixel(Pixel p):SV_TARGET{
 float4 texel=image.Sample(image_sampler,p.uv);if(texture_alpha_kill&&texel.a==0)discard;
 float4 result=particle_textured(texel,p.color,float4(fog_color.rgb,saturate(p.fog)));
 if(alpha_test&&result.a<1.0/255.0)discard;return result;
}
float4 heat_pixel(Pixel p):SV_TARGET{
 float4 texel=image.Sample(image_sampler,p.uv);if(texture_alpha_kill&&texel.a==0)discard;
 float4 result=particle_heat_mask(texel,p.color,float4(fog_color.rgb,saturate(p.fog)));
 if(alpha_test&&result.a<1.0/255.0)discard;return result;
})";
    ComPtr<ID3DBlob>vb,pb,error;unsigned flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;
    check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"vertex","vs_5_0",flags,0,&vb,&error));check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"pixel","ps_5_0",flags,0,&pb,&error));
    check(device->CreateVertexShader(vb->GetBufferPointer(),vb->GetBufferSize(),nullptr,&vertex_));check(device->CreatePixelShader(pb->GetBufferPointer(),pb->GetBufferSize(),nullptr,&pixel_));
    pb.Reset();check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"heat_pixel","ps_5_0",flags,0,&pb,&error));check(device->CreatePixelShader(pb->GetBufferPointer(),pb->GetBufferSize(),nullptr,&heat_pixel_));
    const D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_INSTANCE_DATA,1},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_INSTANCE_DATA,1},{"COLOR",0,DXGI_FORMAT_R8G8B8A8_UNORM,0,24,D3D11_INPUT_PER_INSTANCE_DATA,1}};
    check(device->CreateInputLayout(elements,3,vb->GetBufferPointer(),vb->GetBufferSize(),&layout_));D3D11_BUFFER_DESC bd{};bd.ByteWidth=39*sizeof(UCParticleBillboard);bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;bd.Usage=D3D11_USAGE_DYNAMIC;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;check(device->CreateBuffer(&bd,nullptr,&instances_));
    bd.ByteWidth=sizeof(NativeParticleBillboardView);bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;bd.Usage=D3D11_USAGE_DEFAULT;bd.CPUAccessFlags=0;check(device->CreateBuffer(&bd,nullptr,&constants_));D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;rd.MultisampleEnable=TRUE;check(device->CreateRasterizerState(&rd,&raster_));
}
void NativeParticleBillboards::draw(const UCParticleBillboard *sprites,unsigned count,const NativeParticleBillboardView &view,ID3D11ShaderResourceView *texture,ID3D11SamplerState *sampler,bool heat_mask,bool control_clip){
    if(!raster_||count>39||(count&&!sprites)||!texture||!sampler||view.fog_enabled>1||view.alpha_test>1||view.texture_alpha_kill>1)throw std::invalid_argument("Unsupported billboard batch");
    UCParticleBillboardView v{};memcpy(v.clip_rows,view.clip_rows,sizeof v.clip_rows);memcpy(v.up,view.up,sizeof v.up);memcpy(v.right,view.right,sizeof v.right);v.viewport_scale[0]=v.viewport_scale[1]=v.viewport_scale[2]=1;
    for(unsigned k=0;k<24;k++)if(!std::isfinite(((const float*)&v)[k]))throw std::invalid_argument("Invalid billboard view");
    for(float f:view.fog_color)if(!std::isfinite(f)||f<0||f>1)throw std::invalid_argument("Invalid billboard fog color");
    if(!std::isfinite(view.fog_slope)||!std::isfinite(view.fog_bias))throw std::invalid_argument("Invalid billboard fog coefficients");
    for(unsigned i=0;i<count;i++)for(unsigned corner=0;corner<4;corner++){
        UCParticleBillboardVertex vertex;if(!uc_particle_billboard_clip(sprites+i,corner,&v,&vertex))throw std::invalid_argument("Invalid billboard geometry");
        // CONTROL's original adapter uses bounded RCC then screen-to-clip.
        // Outside this interval homogeneous direct rendering is not equivalent.
        if(control_clip&&(std::abs(vertex.clip[3])<0x1p-64f||std::abs(vertex.clip[3])>0x1p64f))throw std::invalid_argument("Unsupported CONTROL billboard clip W");
        if(view.fog_enabled&&!std::isfinite((view.fog_bias+vertex.fog*view.fog_slope)-1))throw std::invalid_argument("Invalid billboard fog factor");
    }
    ComPtr<ID3D11Device>a,b;texture->GetDevice(&a);sampler->GetDevice(&b);D3D11_SHADER_RESOURCE_VIEW_DESC sd;texture->GetDesc(&sd);
    bool format=sd.Format==DXGI_FORMAT_BC1_UNORM||sd.Format==DXGI_FORMAT_BC2_UNORM||sd.Format==DXGI_FORMAT_BC3_UNORM||sd.Format==DXGI_FORMAT_B8G8R8A8_UNORM||sd.Format==DXGI_FORMAT_R8G8B8A8_UNORM;
    if(a!=device_||b!=device_||sd.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D||!format)throw std::invalid_argument("Invalid billboard resource");
    if(!count)return;
    D3D11_MAPPED_SUBRESOURCE map{};check(context_->Map(instances_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map));memcpy(map.pData,sprites,count*sizeof(*sprites));context_->Unmap(instances_.Get(),0);context_->UpdateSubresource(constants_.Get(),0,nullptr,&view,0,0);
    context_->VSSetShader(vertex_.Get(),nullptr,0);context_->PSSetShader(heat_mask?heat_pixel_.Get():pixel_.Get(),nullptr,0);context_->GSSetShader(nullptr,nullptr,0);context_->HSSetShader(nullptr,nullptr,0);context_->DSSetShader(nullptr,nullptr,0);context_->IASetInputLayout(layout_.Get());context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context_->IASetIndexBuffer(nullptr,DXGI_FORMAT_UNKNOWN,0);
    ID3D11Buffer *vb=instances_.Get(),*cb=constants_.Get();unsigned stride=sizeof(*sprites),offset=0;context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);context_->VSSetConstantBuffers(0,1,&cb);context_->PSSetConstantBuffers(0,1,&cb);context_->PSSetShaderResources(0,1,&texture);context_->PSSetSamplers(0,1,&sampler);context_->RSSetState(raster_.Get());context_->DrawInstanced(6,count,0,0);texture=nullptr;context_->PSSetShaderResources(0,1,&texture);
}
