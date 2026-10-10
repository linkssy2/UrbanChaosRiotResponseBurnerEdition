#include "point_light.hlsl"
Texture2D<float4> model_texture : register(t2);
SamplerState model_sampler : register(s2);
cbuffer ModelView : register(b1) {
    float4 clip_rows[4];
    float4 attenuation_rows[4];
    float4 center;
    float4 directional;
    float4 model_color;
    float4 model_scalar;
    float4 eye;
    float4 specular;
    uint4 mode;
};
struct ModelVertex { float3 position : POSITION; float3 normal : NORMAL; float2 uv : TEXCOORD0; float3 basis5 : BASIS5; float3 basis6 : BASIS6; };
struct ModelPixel {
    float4 position : SV_POSITION;
    float4 attenuation : TEXCOORD0;
    float3 direction : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 reflected : TEXCOORD3;
    float2 uv : TEXCOORD4;
#ifdef MODEL_SOURCE_CLIP
    float source_white : TEXCOORD5;
#endif
};
ModelPixel VS(ModelVertex input) {
    ModelPixel o;
#ifdef MODEL_SOURCE_CLIP
    o.source_white=1;
#endif
    float4 p=float4(input.position,1);
    precise float4 clip=float4(dot(p,clip_rows[0]),dot(p,clip_rows[1]),dot(p,clip_rows[2]),dot(p,clip_rows[3]));
    o.position=clip;
    o.attenuation=float4(dot(p,attenuation_rows[0]),dot(p,attenuation_rows[1]),dot(p,attenuation_rows[2]),dot(p,attenuation_rows[3]));
    o.direction=center.xyz-input.position;
    if(mode.y==1) {
        precise float3 light=o.direction;
        o.direction=float3(dot(light,input.basis6),dot(light,input.normal),-dot(light,input.basis5));
    }
    o.uv=input.uv;
    o.normal=input.normal;
    precise float3 e=eye.xyz-input.position;
    precise float product=e.x*input.normal.x+e.y*input.normal.y;
    product=product+e.z*input.normal.z;
    o.reflected=(product+product)*input.normal-e;
    return o;
}
// Source row palette is raw c0..c191; skin offsets have already been
// converted from signed offsets to absolute rows by the guest snapshot.
cbuffer ModelPose : register(b2) { float4 pose_rows[192]; uint4 pose_mode; };
struct IndexedModelVertex {
    float3 position : POSITION; uint normal : PACKEDNORMAL;
    float2 uv : TEXCOORD0; uint2 basis : PACKEDBASIS;
    uint rows : SKINROWS; uint weights : SKINWEIGHTS;
};
float3 unpack_model_vector(uint raw) {
    int3 n=int3(int(raw<<21)>>21,int(raw<<10)>>21,int(raw)>>22);
    precise float3 decoded=float3(float(n.x)/1023.0,float(n.y)/1023.0,float(n.z)/511.0);
    return max(decoded,-1.0);
}
float3 pose_model_vector(float3 value,float4 a,float4 b,float4 c) {
    precise float3 first=value.x*float3(a.x,b.x,c.x);
    precise float3 second=value.y*float3(a.y,b.y,c.y)+first;
    precise float3 result=value.z*float3(a.z,b.z,c.z)+second;
    return result;
}
#ifdef MODEL_SOURCE_CLIP
cbuffer SourceProjection : register(b3) {float4 source_rows[4];float4 source_scale;float4 source_bias;float4 source_screen;};
float source_row(float3 p,float4 row){precise float a=p.x*row.x+p.y*row.y;a=a+p.z*row.z;return a+row.w;}
#endif
ModelPixel VSIndexed(IndexedModelVertex input) {
    ModelVertex v;v.position=input.position;v.normal=unpack_model_vector(input.normal);
    v.uv=input.uv;v.basis5=0;v.basis6=0;
    if(mode.y==1){v.basis5=unpack_model_vector(input.basis.x);v.basis6=unpack_model_vector(input.basis.y);}
    if(pose_mode.x){
        uint3 rows=uint3(input.rows&255,(input.rows>>8)&255,(input.rows>>16)&255);
        precise float2 weights=float2(input.weights&255,(input.weights>>8)&255)/255.0;
        precise float w2=1.0-(weights.x+weights.y);
        precise float4 a=(weights.y*pose_rows[rows.y]+weights.x*pose_rows[rows.x])+w2*pose_rows[rows.z];
        precise float4 b=(weights.y*pose_rows[rows.y+1]+weights.x*pose_rows[rows.x+1])+w2*pose_rows[rows.z+1];
        precise float4 c=(weights.y*pose_rows[rows.y+2]+weights.x*pose_rows[rows.x+2])+w2*pose_rows[rows.z+2];
        precise float3 p=pose_model_vector(input.position,a,b,c)+float3(a.w,b.w,c.w);v.position=p;
        v.normal=pose_model_vector(v.normal,a,b,c);
        if(mode.y==1){v.basis5=pose_model_vector(v.basis5,a,b,c);v.basis6=pose_model_vector(v.basis6,a,b,c);}
    }
#ifdef MODEL_SOURCE_CLIP
    ModelPixel o=VS(v);o.source_white=1;
    if(mode.x==1||mode.x==4)o.attenuation.w=1;
    precise float4 q=float4(source_row(v.position,source_rows[0]),source_row(v.position,source_rows[1]),source_row(v.position,source_rows[2]),source_row(v.position,source_rows[3]));
    precise float reciprocal=1.0/q.w;
    float magnitude=clamp(abs(reciprocal),5.421010862427522e-20,1.8446744073709552e19);
    reciprocal=asfloat(asuint(magnitude)|(asuint(reciprocal)&0x80000000));
    precise float3 screen=q.xyz*source_scale.xyz;screen=screen*reciprocal;screen=screen+source_bias.xyz;
    if(!all(isfinite(float4(screen,q.w)))||q.w==0){o=(ModelPixel)0;o.position=float4(2,2,2,1);return o;}
    o.position=float4((screen.x/source_screen.x*2-1)*q.w,(1-screen.y/source_screen.y*2)*q.w,screen.z/source_screen.z*q.w,q.w);
    return o;
#else
    return VS(v);
#endif
}
#ifdef MODEL_SOURCE_CLIP
float4 light_basic_source(float3 direction,float3 normal,float attenuation,float3 color,float scalar,float white) {
    precise float product=direction.x*normal.x+direction.y*normal.y;
    product=clamp(product+direction.z*normal.z,-1.0,1.0);
    precise float3 lit=clamp(attenuation*color,-1.0,1.0);
    lit=clamp(product*lit,-1.0,1.0);lit=clamp(lit*white,-1.0,1.0);
    precise float gate=clamp(product+-scalar,-1.0,1.0);
    gate=max(gate,0.0);gate=clamp((gate+gate)*2.0,-1.0,1.0);
    return float4(saturate(clamp(lit*gate,-1.0,1.0)),0.0);
}
#endif
float4 PS(ModelPixel input) : SV_TARGET {
    if(mode.y==1) {
        // Original4F5FA0: tangent-space Y diffuse, authored mask and albedo.
        float4 texel=model_texture.Sample(model_sampler,input.uv);
        if(texel.a==0)discard; // 905B0 stage0 CONTROL0 alpha-kill, before stencil.
        float a=attenuation_volume.SampleLevel(volume_sampler,input.attenuation.xyz/input.attenuation.w,0).a;
        float d=normalization_cube.SampleLevel(cube_sampler,input.direction,0).g;
        precise float3 lit=clamp(a*model_color.rgb,-1.0,1.0);
        lit=clamp(d*lit,-1.0,1.0);
        lit=clamp(lit*texel.a,-1.0,1.0);
        precise float gate=max(clamp(d-model_scalar.a,-1.0,1.0),0.0);
        gate=clamp((gate+gate)*2.0,-1.0,1.0);
        lit=clamp(lit*gate,-1.0,1.0);
        return float4(saturate(lit*texel.rgb),0);
    }
    float3 n=normalization_cube.SampleLevel(cube_sampler,input.normal,0).rgb;
    if(mode.x==0||mode.x==3) {
        precise float d=n.x*directional.x+n.y*directional.y;
        d=clamp(d+n.z*directional.z,-1.0,1.0);
        float alpha=0;
        if(mode.x==3){
            float3 r=normalization_cube.SampleLevel(cube_sampler,input.reflected,0).rgb;
            precise float q=r.x*directional.x+r.y*directional.y;
            q=clamp(q+r.z*directional.z,-1.0,1.0);
            precise float gate=max(d,0.0);gate=clamp((gate+gate)*2.0,-1.0,1.0);
            alpha=saturate(clamp(q*specular.a,-1.0,1.0)*gate);
        }
        return float4(saturate(clamp(d*model_color.rgb,-1.0,1.0)),alpha);
    }
    float3 t=input.attenuation.xyz;
#ifdef MODEL_SOURCE_CLIP
    t/=input.attenuation.w;
#else
    if(mode.x==2||mode.x==5)t/=input.attenuation.w;
#endif
    float a=attenuation_volume.SampleLevel(volume_sampler,t,0).a;
    float3 d=normalization_cube.SampleLevel(cube_sampler,input.direction,0).rgb;
#ifdef MODEL_SOURCE_CLIP
    float4 output=light_basic_source(d,n,a,model_color.rgb,model_scalar.a,input.source_white);
#else
    float4 output=light_basic(d,n,a,model_color.rgb,model_scalar.a);
#endif
    if(mode.x==4||mode.x==5){
        float3 r=normalization_cube.SampleLevel(cube_sampler,input.reflected,0).rgb;
        precise float q=r.x*d.x+r.y*d.y;q=clamp(q+r.z*d.z,-1.0,1.0);
        q=clamp(q*q,-1.0,1.0);q=clamp(q*q,-1.0,1.0);q=clamp(q*q,-1.0,1.0);
        precise float dot_normal=d.x*n.x+d.y*n.y;dot_normal=clamp(dot_normal+d.z*n.z,-1.0,1.0);
        precise float gate=max(clamp(dot_normal-model_scalar.a,-1.0,1.0),0.0);
        gate=clamp((gate+gate)*2.0,-1.0,1.0);
#ifdef MODEL_SOURCE_CLIP
        precise float source_a=clamp(a*input.source_white,-1.0,1.0);
        precise float attenuation_gate=clamp(source_a*gate,-1.0,1.0);
#else
        precise float attenuation_gate=clamp(a*gate,-1.0,1.0);
#endif
        output.rgb=saturate(clamp(clamp(dot_normal*model_color.rgb,-1.0,1.0)*attenuation_gate,-1.0,1.0));
        output.a=saturate(clamp(clamp(q*specular.a,-1.0,1.0)*attenuation_gate,-1.0,1.0));
    }
    return output;
}
