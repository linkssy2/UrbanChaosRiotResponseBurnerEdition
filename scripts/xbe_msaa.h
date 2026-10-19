#pragma once
#include "gpu/ext.h"

namespace uc_msaa {
inline uint32_t Samples(const gpu::Target* t) {
    if (!t || !t->res) return 1;
    return t->samples ? t->samples : t->res->GetDesc().SampleDesc.Count;
}
inline bool Primary(const gpu::Target& t,uint32_t width,uint32_t height) {
    return t.gw==width && t.gh==height && width && height && t.range==1 &&
        (t.depth || t.fmt==DXGI_FORMAT_B8G8R8A8_UNORM);
}
inline bool FormatSupports(ID3D12Device* dev,DXGI_FORMAT format,D3D12_FORMAT_SUPPORT1 bits) {
    D3D12_FEATURE_DATA_FORMAT_SUPPORT q{format};
    return SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&q,sizeof(q))) &&
        (q.Support1&bits)==bits;
}
inline uint32_t Support(ID3D12Device* dev) {
    uint32_t mask=1u<<1;
    if(!FormatSupports(dev,DXGI_FORMAT_B8G8R8A8_UNORM,D3D12_FORMAT_SUPPORT1(
           D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD|D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE)) ||
       !FormatSupports(dev,DXGI_FORMAT_R16G16B16A16_UNORM,D3D12_FORMAT_SUPPORT1(
           D3D12_FORMAT_SUPPORT1_RENDER_TARGET|D3D12_FORMAT_SUPPORT1_BLENDABLE|
           D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD|D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE)) ||
       !FormatSupports(dev,DXGI_FORMAT_R24_UNORM_X8_TYPELESS,D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD) ||
       !FormatSupports(dev,DXGI_FORMAT_X24_TYPELESS_G8_UINT,D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD) ||
       !FormatSupports(dev,DXGI_FORMAT_R32_FLOAT,D3D12_FORMAT_SUPPORT1_RENDER_TARGET))return mask;
    for(uint32_t n:{2u,4u}) {
        bool ok=true;
        for(auto format:{DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_D24_UNORM_S8_UINT,
                         DXGI_FORMAT_R16G16B16A16_UNORM}) {
            D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS q{format,n,D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,0};
            ok &= SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,&q,sizeof(q))) && q.NumQualityLevels;
        }
        if(ok)mask|=1u<<n;
    }
    return mask;
}
inline D3D12_SHADER_RESOURCE_VIEW_DESC DepthView(bool stencil) {
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};
    v.Format=stencil?DXGI_FORMAT_X24_TYPELESS_G8_UINT:DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DMS;
    v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    return v;
}
// The original postprocess reads one depth/stencil value per pixel. Preserve a
// corresponding sample pair; averaging the packed bytes invents unrelated Z.
inline constexpr char kDepthRead[] = R"(
Texture2DMS<float> depth : register(t0);
Texture2DMS<uint2> stencil : register(t1);
float4 vs(uint id : SV_VertexID) : SV_Position {
    float2 uv=float2((id<<1)&2,id&2);
    return float4(uv*float2(2,-2)+float2(-1,1),0,1);
}
float psmain(float4 p : SV_Position) : SV_Target { return depth.Load(int2(p.xy),0); }
float4 packed(float4 p : SV_Position) : SV_Target {
    uint z=(uint)round(saturate(depth.Load(int2(p.xy),0))*16777215.0);
    return float4(z&255,(z>>8)&255,z>>16,stencil.Load(int2(p.xy),0).g&255)/255.0;
}
)";
} // namespace uc_msaa
