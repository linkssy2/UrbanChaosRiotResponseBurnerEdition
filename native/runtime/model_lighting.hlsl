#include "control_model.hlsl"
// Match the depth-producing NativeRenderer VS matrix contraction exactly.
ModelPixel VSNative(ModelVertex input){
 ModelPixel o=VS(input);
 if(mode.y==1&&mode.x==0){precise float3 light=center.xyz;o.direction=float3(dot(light,input.basis6),dot(light,input.normal),dot(light,-input.basis5));}
 if(mode.x==1)o.attenuation.w=1;
 float4x4 clip_matrix=transpose(float4x4(clip_rows[0],clip_rows[1],clip_rows[2],clip_rows[3]));o.position=mul(float4(input.position,1),clip_matrix);return o;
}
float4 PSNative(ModelPixel input):SV_TARGET {
 if(mode.y==1&&mode.x==0){
  float4 texel=model_texture.Sample(model_sampler,input.uv);if(texel.a==0)discard;
  float4 direction=normalization_cube.SampleLevel(cube_sampler,input.direction,0);
  // Original4F5460 preserves normalization-cube alpha as a separate stage.
  precise float3 lit=clamp(direction.y*model_color.rgb,-1.0,1.0);
  lit=clamp(lit*direction.a,-1.0,1.0);lit=clamp(lit*texel.a,-1.0,1.0);
  return float4(saturate(max(lit,0)*texel.rgb),0);
 }
 return PS(input);
}
struct AmbientPixel {float4 position:SV_POSITION;float2 uv:TEXCOORD;float4 color:COLOR;};
float4 Ambient(AmbientPixel input):SV_TARGET {
 if(mode.y==2)return float4(saturate(model_color.rgb+model_texture.Sample(model_sampler,input.uv).rgb),0);
 return model_color;
}
