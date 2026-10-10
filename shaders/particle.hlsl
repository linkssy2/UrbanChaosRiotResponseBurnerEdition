/* Original ordinary textured-particle combiner4EB270. Texture sampling and
 * original per-fragment fog factor/color are supplied by the draw owner.
 * Blend, alpha test, depth, heat-haze and framebuffer ownership are separate. */
float4 particle_textured(float4 texel,float4 vertex_color,float4 fog) {
    precise float3 base=clamp(texel.rgb,-1,1);
    precise float3 colored=clamp(base*vertex_color.rgb,-1,1);
    precise float alpha=clamp(vertex_color.a*texel.a,-1,1);
    precise float factor=max(fog.a,0);
    precise float3 rgb=0+factor*max(colored,0)+(1-factor)*max(fog.rgb,0);
    return float4(saturate(rgb),max(alpha,0));
}
/* Heat-mask combiner4EB360. The draw owner must supply the original heat
 * vertex packing and heat target; this is not the screen-space composite. */
float4 particle_heat_mask(float4 texel,float4 vertex_color,float4 fog) {
    vertex_color.a=vertex_color.b;
    return particle_textured(texel,vertex_color,fog);
}
