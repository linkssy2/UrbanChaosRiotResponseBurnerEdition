#include "decal_quads.h"
#include <d3dcompiler.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
static void check(HRESULT h){if(FAILED(h))throw std::runtime_error("Decal GPU operation failed");}
void NativeDecalQuads::open(ID3D11Device*d,ID3D11DeviceContext*c,const std::string&position){
    ComPtr<ID3D11Device>owner;if(c)c->GetDevice(&owner);
    if(device_||!d||!c||owner.Get()!=d||c->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)throw std::invalid_argument("Invalid decal GPU owner");
    device_=d;context_=c;
    const std::string code=position+R"(
cbuffer Params:register(b0){float4 rows[4],tint,exposure;uint alpha_test,alpha_greater,alpha_reference,padding;};
struct Input{float3 position:POSITION;float4 color:COLOR0;float2 uv:TEXCOORD0;};
struct Pixel{float4 position:SV_POSITION;float4 color:COLOR0;float2 uv:TEXCOORD0;};
Pixel vertex(Input v){
 Pixel r;r.position=base_alpha_vertex(v.position,rows[0],rows[1],rows[2],rows[3],float3(1,1,1),float3(0,0,0)).clip;
 precise float4 color=v.color*exposure;color*=tint;
 r.color=floor(saturate(color)*255+.5)/255;r.uv=v.uv;return r;
}
Texture2D<float4> image:register(t0);SamplerState image_sampler:register(s0);
float4 pixel(Pixel p):SV_TARGET{
 float4 result=saturate(image.Sample(image_sampler,p.uv)*p.color);
 if(alpha_test&&(alpha_greater?result.a<=alpha_reference/255.0:result.a<alpha_reference/255.0))discard;
 return result;
})";
    ComPtr<ID3DBlob>vs,ps,error;unsigned flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;
    check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"vertex","vs_5_0",flags,0,&vs,&error));
    check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"pixel","ps_5_0",flags,0,&ps,&error));
    check(d->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex_));check(d->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel_));
    const D3D11_INPUT_ELEMENT_DESC e[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,28,D3D11_INPUT_PER_VERTEX_DATA,0}};
    check(d->CreateInputLayout(e,3,vs->GetBufferPointer(),vs->GetBufferSize(),&layout_));
    D3D11_BUFFER_DESC b{};b.ByteWidth=512*sizeof(NativeDecalVertex);b.BindFlags=D3D11_BIND_VERTEX_BUFFER;b.Usage=D3D11_USAGE_DYNAMIC;b.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;check(d->CreateBuffer(&b,nullptr,&vertices_));
    uint16_t ix[128*6];for(unsigned i=0;i<128;i++){const unsigned corners[]={0,1,2,0,2,3};for(unsigned k=0;k<6;k++)ix[i*6+k]=(uint16_t)(i*4+corners[k]);}
    b.ByteWidth=sizeof ix;b.BindFlags=D3D11_BIND_INDEX_BUFFER;b.Usage=D3D11_USAGE_IMMUTABLE;b.CPUAccessFlags=0;D3D11_SUBRESOURCE_DATA data{ix,0,0};check(d->CreateBuffer(&b,&data,&indices_));
    b.ByteWidth=sizeof(NativeDecalView);b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;b.Usage=D3D11_USAGE_DEFAULT;check(d->CreateBuffer(&b,nullptr,&constants_));
}
void NativeDecalQuads::draw(const NativeDecalVertex*v,unsigned count,const NativeDecalView&s,ID3D11ShaderResourceView*texture,ID3D11SamplerState*sampler){
    if(!constants_||count>128||(count&&!v)||!texture||!sampler||s.alpha_test>1||s.alpha_greater>1||s.alpha_reference>255)throw std::invalid_argument("Invalid decal batch");
    for(const auto&row:s.clip_rows)for(float x:row)if(!std::isfinite(x))throw std::invalid_argument("Invalid decal matrix");
    for(unsigned k=0;k<4;k++)if(!std::isfinite(s.tint[k])||!std::isfinite(s.exposure[k]))throw std::invalid_argument("Invalid decal color state");
    for(unsigned i=0;i<count*4;i++){
        for(float x:v[i].position)if(!std::isfinite(x))throw std::invalid_argument("Invalid decal position");
        for(float x:v[i].uv)if(!std::isfinite(x))throw std::invalid_argument("Invalid decal UV");
        for(unsigned k=0;k<4;k++){
            float color=v[i].color[k]*s.exposure[k];color*=s.tint[k];
            const float*r=s.clip_rows[k],*p=v[i].position;float clip=((p[0]*r[0]+p[1]*r[1])+p[2]*r[2])+r[3];
            if(!std::isfinite(color)||!std::isfinite(clip)||(k==3&&clip==0))throw std::invalid_argument("Invalid decal vertex output");
        }
    }
    ComPtr<ID3D11Device>a,b;texture->GetDevice(&a);sampler->GetDevice(&b);D3D11_SHADER_RESOURCE_VIEW_DESC desc;texture->GetDesc(&desc);
    if(a!=device_||b!=device_||desc.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D)throw std::invalid_argument("Invalid decal resource");
    if(!count)return;
    D3D11_MAPPED_SUBRESOURCE map{};check(context_->Map(vertices_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map));memcpy(map.pData,v,count*4*sizeof(*v));context_->Unmap(vertices_.Get(),0);
    context_->UpdateSubresource(constants_.Get(),0,nullptr,&s,0,0);
    context_->VSSetShader(vertex_.Get(),nullptr,0);context_->PSSetShader(pixel_.Get(),nullptr,0);context_->GSSetShader(nullptr,nullptr,0);context_->HSSetShader(nullptr,nullptr,0);context_->DSSetShader(nullptr,nullptr,0);
    context_->IASetInputLayout(layout_.Get());context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context_->IASetIndexBuffer(indices_.Get(),DXGI_FORMAT_R16_UINT,0);
    ID3D11Buffer*vb=vertices_.Get(),*cb=constants_.Get();unsigned stride=sizeof(*v),offset=0;context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);context_->VSSetConstantBuffers(0,1,&cb);context_->PSSetConstantBuffers(0,1,&cb);
    context_->PSSetShaderResources(0,1,&texture);context_->PSSetSamplers(0,1,&sampler);context_->DrawIndexed(count*6,0,0);texture=nullptr;context_->PSSetShaderResources(0,1,&texture);
}
