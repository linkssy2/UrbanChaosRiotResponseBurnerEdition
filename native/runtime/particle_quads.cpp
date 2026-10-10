#include "particle_quads.h"
#include <d3dcompiler.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
static void check(HRESULT h){if(FAILED(h))throw std::runtime_error("Particle GPU operation failed");}
void NativeParticleQuads::open(ID3D11Device *device,ID3D11DeviceContext *context,const std::string &vs,const std::string &ps){
    ComPtr<ID3D11Device> owner;if(context)context->GetDevice(&owner);
    if(device_||!device||!context||owner.Get()!=device||context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)
        throw std::invalid_argument("Unsupported particle GPU owner");
    device_=device;context_=context;
    const std::string code=vs+ps+R"(
cbuffer Params:register(b0){float4 rows[4];float4 fog;uint alpha_test;uint texture_alpha_kill;uint2 padding;};
struct Input{float3 position:POSITION;float4 color:COLOR0;float2 uv:TEXCOORD0;};
struct Pixel{float4 position:SV_POSITION;float4 color:COLOR0;float2 uv:TEXCOORD0;};
Pixel vertex(Input v){
 ParticleMeshVertex p=particle_mesh_vertex(v.position,v.color.bgra,v.uv,rows[0],rows[1],rows[2],rows[3],float3(1,1,1),float3(0,0,0));
 Pixel result;result.position=p.clip;result.color=p.color;result.uv=p.uv.xy;return result;
}
Texture2D<float4> image:register(t0);SamplerState image_sampler:register(s0);
float4 pixel(Pixel p):SV_TARGET{
 float4 texel=image.Sample(image_sampler,p.uv);
 if(texture_alpha_kill&&texel.a==0)discard;
 float4 result=particle_textured(texel,p.color,fog);
 if(alpha_test&&result.a<1.0/255.0)discard;
 return result;
}
float4 heat_pixel(Pixel p):SV_TARGET{
 float4 texel=image.Sample(image_sampler,p.uv);
 if(texture_alpha_kill&&texel.a==0)discard;
 float4 result=particle_heat_mask(texel,p.color,fog);
 if(alpha_test&&result.a<1.0/255.0)discard;
 return result;
})";
    ComPtr<ID3DBlob>vb,pb,error;unsigned flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;
    check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"vertex","vs_5_0",flags,0,&vb,&error));
    check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"pixel","ps_5_0",flags,0,&pb,&error));
    check(device->CreateVertexShader(vb->GetBufferPointer(),vb->GetBufferSize(),nullptr,&vertex_));check(device->CreatePixelShader(pb->GetBufferPointer(),pb->GetBufferSize(),nullptr,&pixel_));
    pb.Reset();check(D3DCompile(code.data(),code.size(),nullptr,nullptr,nullptr,"heat_pixel","ps_5_0",flags,0,&pb,&error));check(device->CreatePixelShader(pb->GetBufferPointer(),pb->GetBufferSize(),nullptr,&heat_pixel_));
    const D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R8G8B8A8_UNORM,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0}};
    check(device->CreateInputLayout(elements,3,vb->GetBufferPointer(),vb->GetBufferSize(),&layout_));
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=512*sizeof(UCParticleQuadVertex);bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;bd.Usage=D3D11_USAGE_DYNAMIC;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;check(device->CreateBuffer(&bd,nullptr,&vertices_));
    uint16_t indices[128*6];for(unsigned i=0;i<128;i++){const unsigned corners[6]={0,1,2,0,2,3};for(unsigned k=0;k<6;k++)indices[i*6+k]=uint16_t(i*4+corners[k]);}
    bd.ByteWidth=sizeof indices;bd.BindFlags=D3D11_BIND_INDEX_BUFFER;bd.Usage=D3D11_USAGE_IMMUTABLE;bd.CPUAccessFlags=0;D3D11_SUBRESOURCE_DATA data{indices,0,0};check(device->CreateBuffer(&bd,&data,&indices_));
    bd.ByteWidth=sizeof(NativeParticleQuadView);bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;bd.Usage=D3D11_USAGE_DEFAULT;check(device->CreateBuffer(&bd,nullptr,&constants_));
    D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;rd.MultisampleEnable=TRUE;check(device->CreateRasterizerState(&rd,&raster_));
}
void NativeParticleQuads::draw(const UCParticleQuadVertex *v,unsigned count,const NativeParticleQuadView &view,ID3D11ShaderResourceView *texture,ID3D11SamplerState *sampler,bool heat_mask){
    submit(v,count,view,texture,sampler,heat_mask,false);
}
void NativeParticleQuads::draw_triangles(const UCParticleQuadVertex *v,unsigned count,const NativeParticleQuadView &view,ID3D11ShaderResourceView *texture,ID3D11SamplerState *sampler){
    submit(v,count,view,texture,sampler,false,true);
}
void NativeParticleQuads::submit(const UCParticleQuadVertex *v,unsigned count,const NativeParticleQuadView &view,ID3D11ShaderResourceView *texture,ID3D11SamplerState *sampler,bool heat_mask,bool triangles){
    const unsigned vertex_count=count*(triangles?3u:4u);
    if(!raster_||count>128||(count&&!v)||!texture||!sampler||view.alpha_test>1||view.texture_alpha_kill>1)
        throw std::invalid_argument("Unsupported particle batch");
    for(const auto &row:view.clip_rows)for(float f:row)if(!std::isfinite(f))throw std::invalid_argument("Invalid particle matrix");
    for(float f:view.fog)if(!std::isfinite(f)||f<0||f>1)throw std::invalid_argument("Invalid particle fog");
    for(unsigned i=0;i<vertex_count;i++){
        for(float f:v[i].position)if(!std::isfinite(f))throw std::invalid_argument("Invalid particle position");
        for(float f:v[i].uv)if(!std::isfinite(f))throw std::invalid_argument("Invalid particle UV");
        for(unsigned k=0;k<4;k++){
            const float *r=view.clip_rows[k],*p=v[i].position;
            float clip=((p[0]*r[0]+p[1]*r[1])+p[2]*r[2])+r[3];
            if(!std::isfinite(clip))throw std::invalid_argument("Invalid particle clip position");
        }
    }
    ComPtr<ID3D11Device>a,b;texture->GetDevice(&a);sampler->GetDevice(&b);D3D11_SHADER_RESOURCE_VIEW_DESC sd;texture->GetDesc(&sd);
    if(a!=device_||b!=device_||sd.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D)throw std::invalid_argument("Invalid particle resource");
    if(!count)return;
    D3D11_MAPPED_SUBRESOURCE map{};check(context_->Map(vertices_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map));memcpy(map.pData,v,vertex_count*sizeof(*v));context_->Unmap(vertices_.Get(),0);
    context_->UpdateSubresource(constants_.Get(),0,nullptr,&view,0,0);
    context_->VSSetShader(vertex_.Get(),nullptr,0);context_->PSSetShader(heat_mask?heat_pixel_.Get():pixel_.Get(),nullptr,0);context_->GSSetShader(nullptr,nullptr,0);context_->HSSetShader(nullptr,nullptr,0);context_->DSSetShader(nullptr,nullptr,0);
    context_->IASetInputLayout(layout_.Get());context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context_->IASetIndexBuffer(indices_.Get(),DXGI_FORMAT_R16_UINT,0);
    ID3D11Buffer *vb=vertices_.Get(),*cb=constants_.Get();unsigned stride=sizeof(*v),offset=0;context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);context_->VSSetConstantBuffers(0,1,&cb);context_->PSSetConstantBuffers(0,1,&cb);
    context_->PSSetShaderResources(0,1,&texture);context_->PSSetSamplers(0,1,&sampler);context_->RSSetState(raster_.Get());if(triangles)context_->Draw(vertex_count,0);else context_->DrawIndexed(count*6,0,0);
    texture=nullptr;context_->PSSetShaderResources(0,1,&texture);
}
