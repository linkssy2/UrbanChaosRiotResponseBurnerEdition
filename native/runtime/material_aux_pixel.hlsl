/* Fixed full4F6360, selected by8FF30 for a static header9 image with no header8.
 * Texture0 is already sampled UNORM; color is actual packed constant2 UNORM.
 * Original two combiner stages add RGB and emit alpha0. No fog input is read. */
float4 material_aux_pixel(float4 sample0,float4 color){
 precise float3 sum=color.rgb+sample0.rgb;
 return float4(saturate(sum),0);
}
