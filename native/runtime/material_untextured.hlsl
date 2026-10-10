/* Actual static material0: seven-instruction authored VP, no UV output or
 * sampled image. Compile after base_alpha_vertex.hlsl. Color packing remains
 * the renderer's shared NV2A UNORM boundary, before raster interpolation. */
struct MaterialUntexturedVertex {BaseAlphaVertex position;float4 color;};
MaterialUntexturedVertex material_untextured_vertex(float3 p,float4 color,float4 tint,
 float4 row0,float4 row1,float4 row2,float4 row3,float3 scale,float3 offset){
 MaterialUntexturedVertex result;
 result.position=base_alpha_vertex(p,row0,row1,row2,row3,scale,offset);
 result.color=color*tint;return result;
}
/* Complete one-stage authored combiner, including alpha. Inputs are the
 * interpolated packed vertex color and actual packed setup constant0. */
float4 material_untextured_pixel(float4 color,float4 constant0){
 return saturate(color*constant0);
}
