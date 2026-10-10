/* Fixed17-instruction4EB158. Supplied once-rounded per-particle sin/cos,
 * original retained view/bases, corner0..3 and already decoded ARGB color.
 * No shader VM or trigonometric approximation in the frame loop. */
struct ParticleBillboardVertex {float4 clip;float4 screen;float4 color;float4 uv;float fog;};
ParticleBillboardVertex particle_billboard_vertex(float3 center,float radius,float sine,float cosine,
    float4 color,uint corner,float4 row0,float4 row1,float4 row2,float4 row3,
    float4 up,float4 right,float3 scale,float3 offset){
    static const float2 sides[4]={float2(-1,-1),float2(1,-1),float2(1,1),float2(-1,1)};
    static const float2 uv[4]={float2(0,0),float2(1,0),float2(1,1),float2(0,1)};
    precise float x=sides[corner].x*radius,y=sides[corner].y*radius,z=0.0*radius;
    precise float rx=(x*cosine+y*sine)+z*0.0;
    precise float ry=(x*-sine+y*cosine)+z*0.0;
    precise float4 horizontal=right*rx;
    precise float4 p=(up*ry+horizontal)+float4(center,1);
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
    ParticleBillboardVertex result;result.clip=clip;result.screen=float4(screen,clip.w);
    result.color=color;result.uv=float4(uv[corner],0,1);result.fog=clip.w;return result;
}
