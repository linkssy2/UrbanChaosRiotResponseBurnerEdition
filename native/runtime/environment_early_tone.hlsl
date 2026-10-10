/* Fixed retail4F6AE0: source alpha carries an additive lighting contribution.
 * Constants are already packed by9C830: reciprocal RGB in constant4, and
 * desaturation weights RGB/amount A in constant0. UNORM samples/constants. */
float4 environment_early_tone(float4 source,float4 reciprocal,float4 desaturation){
    precise float3 base=clamp(source.a*reciprocal.rgb+source.rgb,-1,1);
    precise float3 weighted=base*desaturation.rgb;
    precise float gray=clamp((weighted.x+weighted.y)+weighted.z,-1,1);
    precise float3 retained=base*(1-saturate(desaturation.a));
    return float4(saturate(retained+gray),0);
}
