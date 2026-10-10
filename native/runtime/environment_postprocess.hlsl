/* Fixed retail glow/history pixel programs. Inputs are already sampled and
 * constants are source-packed UNORM bytes, not unquantized authored floats.
 * Target copies, texture coordinates, sampling and blending belong to callers. */
float4 environment_threshold(float4 source,float4 threshold) {
    return float4(saturate(source.rgb-threshold.rgb),1);
}
float4 environment_filter(float4 a,float4 b,float4 c,float4 d,float4 near_weight,float4 far_weight) {
    precise float3 near_pair=clamp(a.rgb*near_weight.rgb+b.rgb*near_weight.rgb,-1,1);
    precise float3 far_pair=clamp(c.rgb*far_weight.rgb+d.rgb*far_weight.rgb,-1,1);
    return float4(saturate(near_pair+far_pair),near_weight.a);
}
float4 environment_composite(float4 source,float4 gain) {
    precise float4 product=source*gain;
    return saturate(product*4);
}
float4 environment_history(float4 current,float4 previous,float4 amount) {
    precise float3 a=amount.rgb*previous.rgb;
    precise float3 b=(1-amount.rgb)*current.rgb;
    return float4(saturate(a+b),0);
}
