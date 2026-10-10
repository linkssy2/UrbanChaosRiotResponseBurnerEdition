/* Fixed ordinary textured-particle vertex program4EAFE0, eight instructions.
 * Input positions and UVs come from the original quad/line consumer; color is
 * already decoded from packed ARGB. clip_rows are retained c8..c11; viewport
 * values are original constants58/59. Finite inputs/nonzero clipW and finite
 * outputs are required. PC rendering consumes clip directly; screen preserves
 * the original viewport result for comparison. This program writes no fog.
 * No command decoding or guest shader bytecode is used by this implementation. */
struct ParticleMeshVertex {float4 clip;float4 screen;float4 color;float4 uv;};
ParticleMeshVertex particle_mesh_vertex(float3 position,float4 color,float2 uv,
    float4 row0,float4 row1,float4 row2,float4 row3,float3 scale,float3 offset){
    ParticleMeshVertex result;
    precise float4 p=float4(position,1);
    precise float4 clip;
    clip.x=((p.x*row0.x+p.y*row0.y)+p.z*row0.z)+p.w*row0.w;
    clip.y=((p.x*row1.x+p.y*row1.y)+p.z*row1.z)+p.w*row1.w;
    clip.z=((p.x*row2.x+p.y*row2.y)+p.z*row2.z)+p.w*row2.w;
    clip.w=((p.x*row3.x+p.y*row3.y)+p.z*row3.z)+p.w*row3.w;
    precise float reciprocal=1/clip.w;
    precise float magnitude=clamp(abs(reciprocal),5.421010862427522e-20,1.8446744073709552e19);
    reciprocal=clip.w<0?-magnitude:magnitude;
    precise float3 scaled=clip.xyz*scale;
    precise float3 screen=scaled*reciprocal+offset;
    result.clip=clip;result.screen=float4(screen,clip.w);
    result.color=color;result.uv=float4(uv,0,1);return result;
}
