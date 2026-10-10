/* Fixed six-instruction4EB450. Position-only input; it writes neither color,
 * texture coordinates nor fog. Rows are original raw104..107; screen is the
 * legacy viewport witness, while the PC rasterizer consumes clip directly. */
struct BaseAlphaVertex {float4 clip;float4 screen;};
BaseAlphaVertex base_alpha_vertex(float3 p,float4 row0,float4 row1,float4 row2,float4 row3,
    float3 scale,float3 offset){
    precise float4 clip;
    clip.x=((p.x*row0.x+p.y*row0.y)+p.z*row0.z)+row0.w;
    clip.y=((p.x*row1.x+p.y*row1.y)+p.z*row1.z)+row1.w;
    clip.z=((p.x*row2.x+p.y*row2.y)+p.z*row2.z)+row2.w;
    clip.w=((p.x*row3.x+p.y*row3.y)+p.z*row3.z)+row3.w;
    precise float reciprocal=1/clip.w;
    precise float magnitude=clamp(abs(reciprocal),5.421010862427522e-20,1.8446744073709552e19);
    reciprocal=asfloat(asuint(magnitude)|(asuint(reciprocal)&0x80000000u));
    precise float3 scaled=clip.xyz*scale;
    precise float3 screen=scaled*reciprocal+offset;
    BaseAlphaVertex result;result.clip=clip;result.screen=float4(screen,clip.w);return result;
}
