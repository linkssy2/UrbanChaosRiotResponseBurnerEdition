#pragma once
#include <cmath>
#include <cstdint>

namespace uc_weapon_wheel_blur {
// One final presentation draw: no source writes, additional target, or scene copy.
inline bool Plan(uint32_t guest_width,uint32_t guest_height,const float uv[4],bool gamma,float (&out)[8]) {
    if(!guest_width||!guest_height)return false;
    for(unsigned i=0;i<4;++i)if(!std::isfinite(uv[i]))return false;
    if(uv[0]<0||uv[1]<0||uv[2]<=0||uv[3]<=0||uv[0]+uv[2]>1||uv[1]+uv[3]>1)return false;
    for(unsigned i=0;i<4;++i)out[i]=uv[i];
    out[4]=3.f*uv[2]/guest_width;out[5]=3.f*uv[3]/guest_height;
    out[6]=gamma?1.f:0.f;out[7]=0;return true;
}
inline constexpr char kShader[]=R"(
cbuffer B : register(b1) { float4 uvrect; float4 opts; };
Texture2D img : register(t0);
Texture2D lut : register(t1);
SamplerState ls : register(s0);
SamplerState ps : register(s1);
struct O { float4 p : SV_Position; float2 uv : TEXCOORD0; };
O vs(uint id : SV_VertexID) {
    O o;float2 t=float2((id<<1)&2,id&2);
    o.p=float4(t*float2(2,-2)+float2(-1,1),0,1);
    o.uv=uvrect.xy+t*uvrect.zw;return o;
}
float4 psmain(O i) : SV_Target {
    uint width,height;img.GetDimensions(width,height);
    float2 inset=.5/float2(width,height);
    float2 low=uvrect.xy+inset,high=max(low,uvrect.xy+uvrect.zw-inset);
    const float weight[5]={1,4,6,4,1};float3 color=0;
    [unroll] for(int y=-2;y<=2;++y) [unroll] for(int x=-2;x<=2;++x)
        color+=weight[x+2]*weight[y+2]*img.SampleLevel(ls,clamp(i.uv+float2(x,y)*opts.xy,low,high),0).rgb;
    color/=256;
    if(opts.z>0)color=float3(lut.SampleLevel(ps,float2(color.r,.5),0).r,
        lut.SampleLevel(ps,float2(color.g,.5),0).g,lut.SampleLevel(ps,float2(color.b,.5),0).b);
    return float4(color,1);
}
)";
}
