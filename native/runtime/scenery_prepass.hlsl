Texture2D<float4> auxiliary:register(t0);
StructuredBuffer<uint> normal_palette:register(t1);
SamplerState sampler0:register(s0);
cbuffer Matrix:register(b0){float4 rows[4];};
cbuffer Pixel:register(b1){float4 color;float4 normal_weight;};
struct RasterVertex{float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
RasterVertex vertex(float3 position:POSITION,float2 uv:TEXCOORD0){
    RasterVertex r;
#ifdef AUXILIARY
    MaterialAuxVertex v=material_aux_vertex(position,uv,rows[0],rows[1],rows[2],rows[3],1,0);
    r.position=v.position.clip;r.uv=v.uv.xy;
#else
    r.position=base_alpha_vertex(position,rows[0],rows[1],rows[2],rows[3],1,0).clip;r.uv=0;
#endif
    return r;
}
float4 pixel(RasterVertex p):SV_TARGET{
#ifdef NORMAL
    uint index=(uint)round(auxiliary.Sample(sampler0,p.uv).r*255),packed=normal_palette[index];
    float4 texel=float4((packed>>16)&255,(packed>>8)&255,packed&255,packed>>24)/255;
    return base_alpha_normal(texel,normal_weight,color);
#elif defined(AUXILIARY)
    return material_aux_pixel(auxiliary.Sample(sampler0,p.uv),color);
#else
    return base_alpha_untextured(color,normal_weight.a,0);
#endif
}
