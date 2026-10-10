/* Fixed basic diffuse4F5DC0. No runtime shader/register interpreter.
 * Resources contain original A3B10/A4540 texels, decoded before filtering.
 * Incoming vectors are the qualified static/skinned light vertex outputs.
 * Caller supplies actual receiver constants, material and draw-state owners. */
TextureCube<float4> normalization_cube : register(t0);
Texture3D<float4> attenuation_volume : register(t1);
SamplerState cube_sampler : register(s0);
SamplerState volume_sampler : register(s1);
cbuffer LightMaterial : register(b0) { float4 light_color; float4 material_scalar; };
struct LightPixelInput {
    float4 position : SV_POSITION;
    float3 attenuation : TEXCOORD0;
    float3 direction : TEXCOORD1;
    float3 normal : TEXCOORD2;
};
float4 light_basic(float3 direction,float3 normal,float attenuation,float3 color,float scalar) {
    precise float product=direction.x*normal.x+direction.y*normal.y;
    product=clamp(product+direction.z*normal.z,-1.0,1.0);
    precise float3 lit=clamp(attenuation*color,-1.0,1.0);
    lit=clamp(product*lit,-1.0,1.0);
    precise float gate=clamp(product+-scalar,-1.0,1.0);
    gate=max(gate,0.0);gate=clamp((gate+gate)*2.0,-1.0,1.0);
    return float4(saturate(clamp(lit*gate,-1.0,1.0)),0.0);
}
float4 main(LightPixelInput input) : SV_TARGET {
    float attenuation=attenuation_volume.SampleLevel(volume_sampler,input.attenuation,0).a;
    float3 direction=normalization_cube.SampleLevel(cube_sampler,input.direction,0).rgb;
    float3 normal=normalization_cube.SampleLevel(cube_sampler,input.normal,0).rgb;
    return light_basic(direction,normal,attenuation,light_color.rgb,material_scalar.a);
}
