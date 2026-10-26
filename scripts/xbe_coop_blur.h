#pragma once
#include <cstdint>

namespace uc_coop_blur {
struct Region { uint32_t top=0,bottom=0;float constants[8]{}; };
inline bool Plan(uint32_t width,uint32_t height,uint32_t guest_width,uint32_t guest_height,unsigned slot,Region& out){
    if(!width||height<2||!guest_width||!guest_height||slot>=2)return false;
    out.top=slot?height/2:0;out.bottom=slot?height:height/2;
    out.constants[0]=0;out.constants[1]=float(out.top)/height;
    out.constants[2]=1;out.constants[3]=float(out.bottom-out.top)/height;
    out.constants[4]=1.5f/guest_width;out.constants[5]=1.5f/guest_height;
    out.constants[6]=out.constants[7]=0;
    return true;
}
inline constexpr char kShader[]=R"(
Texture2D image : register(t0);
SamplerState linear_clamp : register(s0);
cbuffer Params : register(b1) { float4 rect; float4 step; };
struct Vertex { float4 position:SV_Position;float2 uv:TEXCOORD0; };
Vertex vs(uint id:SV_VertexID){
    Vertex o;o.uv=float2((id<<1)&2,id&2);
    o.position=float4(o.uv*float2(2,-2)+float2(-1,1),0,1);return o;
}
float4 psmain(Vertex input):SV_Target{
    uint width,height;image.GetDimensions(width,height);
    float2 inset=.5/float2(width,height);
    float2 minimum=rect.xy+inset,maximum=rect.xy+rect.zw-inset;
    float2 center=rect.xy+input.uv*rect.zw;
    float3 color=0;
    [unroll]for(int y=-1;y<=1;++y)[unroll]for(int x=-1;x<=1;++x){
        float weight=(x==0?2:1)*(y==0?2:1);
        color+=image.SampleLevel(linear_clamp,clamp(center+float2(x,y)*step.xy,minimum,maximum),0).rgb*weight;
    }
    return float4(color/16,image.SampleLevel(linear_clamp,clamp(center,minimum,maximum),0).a);
}
)";
}
