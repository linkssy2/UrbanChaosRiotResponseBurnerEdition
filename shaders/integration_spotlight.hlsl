// Original spot VS4F14A0 and PS4F5DC0. Texture coordinates remain homogeneous
// until pixel sampling, matching the original PROJECT3D texture operation.
#define VS point_VS
#define PS point_PS
#include "integration_lighting.hlsl"
#undef VS
#undef PS
cbuffer SpotProjection : register(b4) { float4 spot_rows[4]; };
struct SpotPixel {
    float4 position:SV_POSITION;float4 attenuation:TEXCOORD0;
    float3 direction:TEXCOORD1;float3 normal:TEXCOORD2;float2 uv:TEXCOORD3;float3 albedo:COLOR;
};
SpotPixel VS(Vertex v){
    SpotPixel o;o.position=mul(float4(v.position,1),view_projection);o.uv=v.uv;o.albedo=v.color.bgr;
    [unroll]for(uint i=0;i<4;i++)o.attenuation[i]=dot(float4(v.position,1),spot_rows[i]);
    o.direction=light_center.xyz-v.position;
    o.normal=unpack_normal(v.packed_normal);
    return o;
}
struct NormalSpotPixel {float4 position:SV_POSITION;float4 attenuation:TEXCOORD0;float3 direction:TEXCOORD1;float3 normal:TEXCOORD2;float2 uv:TEXCOORD3;float3 albedo:COLOR;float3 tangent_direction:TEXCOORD4;};
NormalSpotPixel VSNormalSpot(Vertex v){
    SpotPixel p=VS(v);NormalSpotPixel o;o.position=p.position;o.attenuation=p.attenuation;o.direction=p.direction;o.normal=p.normal;o.uv=p.uv;o.albedo=p.albedo;
    o.tangent_direction=tangent_light(v,p.direction);return o;
}
float4 PS(SpotPixel input):SV_TARGET{
    float attenuation=attenuation_volume.SampleLevel(volume_sampler,input.attenuation.xyz/input.attenuation.w,0).a;
    float3 direction=normalization_cube.SampleLevel(cube_sampler,input.direction,0).rgb;
    float3 normal=normalization_cube.SampleLevel(cube_sampler,input.normal,0).rgb;
    float4 color=light_basic(direction,normal,attenuation,light_color.rgb,material_scalar.a);
    float3 albedo=albedo_constant.rgb;
    if(albedo_options.x!=0)albedo=saturate(albedo_map.Sample(albedo_sampler,input.uv).rgb*albedo);
    color.rgb*=albedo*input.albedo;return color;
}
float4 PSNativeSpot(SpotPixel input):SV_TARGET{
    float attenuation=attenuation_volume.SampleLevel(volume_sampler,input.attenuation.xyz/input.attenuation.w,0).a;
    float3 direction=normalization_cube.SampleLevel(cube_sampler,input.direction,0).rgb;
    float3 normal=normalization_cube.SampleLevel(cube_sampler,input.normal,0).rgb;
    return light_basic(direction,normal,attenuation,light_color.rgb,material_scalar.a);
}

float4 PSNormalSpot(NormalSpotPixel input):SV_TARGET {
    float attenuation=attenuation_volume.SampleLevel(volume_sampler,input.attenuation.xyz/input.attenuation.w,0).a;
    float3 direction=normalization_cube.SampleLevel(cube_sampler,input.tangent_direction,0).rgb;
    float3 normal=albedo_map.Sample(albedo_sampler,input.uv).rgb*2-1;
    return light_basic(direction,normal,attenuation,light_color.rgb,material_scalar.a);
}

float4 PSMaskedSpot(NormalSpotPixel input):SV_TARGET {
    return masked_light(albedo_map.Sample(albedo_sampler,input.uv),normalization_cube.SampleLevel(cube_sampler,input.tangent_direction,0),attenuation_volume.SampleLevel(volume_sampler,input.attenuation.xyz/input.attenuation.w,0).a);
}

struct SpecularSpotPixel {float4 position:SV_POSITION;float4 attenuation:TEXCOORD0;float3 direction:TEXCOORD1;float3 normal:TEXCOORD2;float3 reflected:TEXCOORD4;};
SpecularSpotPixel VSSpecularSpot(Vertex v){
    SpotPixel p=VS(v);SpecularSpotPixel o;o.position=p.position;o.attenuation=p.attenuation;o.direction=p.direction;o.normal=p.normal;o.reflected=reflected_eye(v);return o;
}
float4 PSSpecularSpot(SpecularSpotPixel input):SV_TARGET{
    float3 n=normalization_cube.SampleLevel(cube_sampler,input.normal,0).rgb,d=normalization_cube.SampleLevel(cube_sampler,input.direction,0).rgb;
    float3 reflection=normalization_cube.SampleLevel(cube_sampler,input.reflected,0).rgb;
    return specular_point(d,n,reflection,attenuation_volume.SampleLevel(volume_sampler,input.attenuation.xyz/input.attenuation.w,0).a);
}
