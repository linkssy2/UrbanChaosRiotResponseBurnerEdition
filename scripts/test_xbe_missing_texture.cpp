// Production key generation and HLSL, including a real WARP pixel witness.
#undef NDEBUG
#define NOMINMAX
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "gpu/shaders.h"
using Microsoft::WRL::ComPtr;
// ORIGINAL_KEY

static void Check(HRESULT hr) { assert(SUCCEEDED(hr)); }
static ComPtr<ID3DBlob> Compile(const std::string& source, const char* profile) {
    ComPtr<ID3DBlob> code, error;
    auto hr=D3DCompile(source.data(),source.size(),"missing-texture",nullptr,nullptr,"main",profile,
                      D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);
    if(FAILED(hr)&&error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);
    Check(hr);return code;
}

static void Keys() {
    using namespace gpu;
    unsigned cases=0;
    for(unsigned stage=0;stage<4;++stage)for(unsigned op=2;op<=26;++op)
    for(unsigned a:{0u,1u,2u,3u,5u})for(unsigned b:{0u,1u,2u})for(unsigned c:{1u,2u})
    for(unsigned modifier:{0u,0x10u,0x20u,0x30u})for(unsigned kind=0;kind<4;++kind)
    for(unsigned result:{1u,5u}){
        State s{};TexInfo textures[4]{};textures[stage].kind=uint8_t(kind);
        for(auto& t:s.tss){t[kTssColorOp]=2;t[kTssColorArg1]=1;}
        auto& t=s.tss[stage];t[kTssColorOp]=t[kTssAlphaOp]=op;
        t[kTssColorArg1]=t[kTssAlphaArg1]=a|modifier;
        t[kTssColorArg2]=t[kTssAlphaArg2]=b|modifier;
        t[kTssColorArg0]=t[kTssAlphaArg0]=c|modifier;
        t[kTssResultArg]=result;
        auto expected=BeforeKey(s,textures);
        // Independent operand-use table: selection, ternary, binary; alpha-only
        // operations 18..21 select their first operand. Modifiers keep identity.
        const unsigned rgb_mask=op==2?1:op==3?2:(op==23||op==24)?7:3;
        const unsigned alpha_mask=op>=18&&op<=21?1:rgb_mask;
        const unsigned texture_mask=(a==2?1:0)|(b==2?2:0)|(c==2?4:0);
        if(!kind)for(unsigned channel:{0u,4u}){
            if(op==14||op==16||((channel?alpha_mask:rgb_mask)&texture_mask)){
                expected.ff[stage][channel]=2;expected.ff[stage][channel+1]=1;
            }
        }
        assert(MakePixelKey(s,textures)==expected);++cases;
    }
    State s{};TexInfo textures[4]{};
    assert(MakePixelKey(s,textures)==BeforeKey(s,textures)); // disabled stages
    s.ps=std::make_shared<PixelShader>();
    s.ps->def[8]=0x00000004u; // final RGB from diffuse
    s.ps->def[9]=0x00001400u; // final alpha from diffuse
    s.tss[0][kTssColorOp]=s.tss[0][kTssAlphaOp]=2;
    s.tss[0][kTssColorArg1]=s.tss[0][kTssAlphaArg1]=2;
    const auto programmable=MakePixelKey(s,textures);
    assert(programmable==BeforeKey(s,textures)); // programmable combiners ignore FF state
    assert(programmable.programmable && programmable.final_abcd==s.ps->def[8] && programmable.final_efg==s.ps->def[9]);
    std::printf("PASS %u fixed-function keys: every stage, operand/modifier, alpha, result and texture kind; disabled and programmable paths unchanged\n",cases);
}

static void Pixels() {
    using namespace gpu;
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    Check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
    D3D11_TEXTURE2D_DESC td{};td.Width=td.Height=td.MipLevels=td.ArraySize=1;
    td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> target,readback;Check(device->CreateTexture2D(&td,nullptr,&target));
    td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Check(device->CreateTexture2D(&td,nullptr,&readback));
    ComPtr<ID3D11RenderTargetView> rtv;Check(device->CreateRenderTargetView(target.Get(),nullptr,&rtv));
    auto raw=rtv.Get();context->OMSetRenderTargets(1,&raw,nullptr);
    D3D11_VIEWPORT vp{0,0,1,1,0,1};context->RSSetViewports(1,&vp);
    D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;
    ComPtr<ID3D11RasterizerState> raster;Check(device->CreateRasterizerState(&rd,&raster));context->RSSetState(raster.Get());
    DrawConstants constants{};constants.misc[1]=7;constants.tfactor[0]=.7f;constants.tfactor[1]=.3f;
    constants.tfactor[2]=.1f;constants.tfactor[3]=.5f;
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=sizeof constants;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA initial{&constants};ComPtr<ID3D11Buffer> cb;
    Check(device->CreateBuffer(&bd,&initial,&cb));auto cb_raw=cb.Get();context->PSSetConstantBuffers(1,1,&cb_raw);
    State s{};TexInfo textures[4]{};
    auto& t=s.tss[0];t[kTssColorOp]=t[kTssAlphaOp]=2;t[kTssColorArg1]=t[kTssAlphaArg1]=2;
    const auto hlsl=PixelHlsl(MakePixelKey(s,textures));
    const auto declaration=hlsl.substr(0,hlsl.find("cbuffer VC"));
    const auto vertex=Compile(declaration+R"(
VOut main(uint id:SV_VertexID) {
    VOut o=(VOut)0;float2 p=float2((id<<1)&2,id&2);
    o.pos=float4(p*float2(2,-2)+float2(-1,1),0,1);
    o.d0=float4(.2,.4,.6,.8);o.fogamt=1;return o;
})","vs_5_0");
    ComPtr<ID3D11VertexShader> vs;Check(device->CreateVertexShader(vertex->GetBufferPointer(),vertex->GetBufferSize(),nullptr,&vs));
    context->VSSetShader(vs.Get(),nullptr,0);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    auto draw=[&](const PixelKey& key){
        auto shader=Compile(PixelHlsl(key),"ps_5_0");ComPtr<ID3D11PixelShader> ps;
        Check(device->CreatePixelShader(shader->GetBufferPointer(),shader->GetBufferSize(),nullptr,&ps));
        context->PSSetShader(ps.Get(),nullptr,0);const float sentinel[4]={-1,-1,-1,-1};
        context->ClearRenderTargetView(rtv.Get(),sentinel);context->Draw(3,0);context->CopyResource(readback.Get(),target.Get());
        D3D11_MAPPED_SUBRESOURCE map{};Check(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&map));
        std::array<float,4> result{};std::memcpy(result.data(),map.pData,sizeof result);context->Unmap(readback.Get(),0);return result;
    };
    unsigned witnessed=0;
    for(bool later:{false,true})for(bool temporary:{false,true}){
        auto state=s;
        if(later){state.tss[1]=state.tss[0];state.tss[0][kTssColorArg1]=state.tss[0][kTssAlphaArg1]=3;}
        if(temporary){auto stage=later?1:0;state.tss[stage][kTssResultArg]=5;auto& next=state.tss[stage+1];
            next[kTssColorOp]=next[kTssAlphaOp]=2;next[kTssColorArg1]=next[kTssAlphaArg1]=5;}
        auto after=draw(MakePixelKey(state,textures)),before=draw(BeforeKey(state,textures));
        const std::array<float,4> expected=later?std::array<float,4>{.7f,.3f,.1f,.5f}:std::array<float,4>{.2f,.4f,.6f,.8f};
        for(unsigned i=0;i<4;++i){assert(std::abs(after[i]-expected[i])<1e-6f);assert(before[i]==1.f);}
        ++witnessed;
    }
    std::printf("PASS %u production HLSL WARP pixel witnesses: missing texture preserves current RGB/alpha, later stages and temporary result; old shader returns white\n",witnessed);
}
int main(){Keys();Pixels();}
