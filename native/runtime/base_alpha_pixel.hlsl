/* Fixed retail material+20 pixel programs. Inputs are already sampled/packed.
 * Original vertex program, texture stages, fog coordinate and render state
 * belong to the caller; these functions are not the ordinary8FD80 material. */
float4 base_alpha_untextured(float4 color,float alpha_scale,float fog_factor){
    return float4(saturate(color.rgb),saturate((1-saturate(fog_factor))*alpha_scale));
}
float4 base_alpha_constant(float4 color){return float4(saturate(color.rgb),0);}
float4 base_alpha_textured(float4 sample0,float4 color){
    return saturate(sample0*color);
}
float4 base_alpha_normal(float4 sample0,float4 normal_weight,float4 color){
    precise float3 product=(2*sample0.rgb-1)*normal_weight.rgb;
    precise float direction=clamp((product.x+product.y)+product.z,-1,1);
    precise float lit=clamp(direction*normal_weight.a+(1-saturate(normal_weight.a)),-1,1);
    return float4(saturate(max(lit,0)*color.rgb),0);
}
