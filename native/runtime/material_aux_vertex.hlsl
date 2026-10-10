/* Compile after base_alpha_vertex.hlsl. Actual seven-instruction4EB9C0 reuses
 * the position chain and additionally MOVs float2 v3 (with default z0/w1) to
 * texture output0. Declared v4 is not read by this program. */
struct MaterialAuxVertex {BaseAlphaVertex position;float4 uv;};
MaterialAuxVertex material_aux_vertex(float3 p,float2 uv,float4 row0,float4 row1,
 float4 row2,float4 row3,float3 scale,float3 offset){
 MaterialAuxVertex result;
 result.position=base_alpha_vertex(p,row0,row1,row2,row3,scale,offset);
 result.uv=float4(uv,0,1);return result;
}
