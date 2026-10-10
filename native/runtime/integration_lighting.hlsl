// Basic point-light formulas/resources remain owned by the qualified endpoint.
#define main source_light_main
#include "point_light.hlsl"
#undef main
cbuffer Receiver : register(b1) {
    row_major float4x4 view_projection;
    float4 attenuation_rows[3];
    float4 light_center;
};
cbuffer Shadow : register(b2) { float4 shadow_parameters; };
TextureCube<float> shadow_map : register(t2);
SamplerComparisonState shadow_sampler : register(s2);
Texture2D<float4> albedo_map : register(t3);
SamplerState albedo_sampler : register(s3);
cbuffer Albedo : register(b3) { float4 albedo_constant; float4 albedo_options; };
struct Vertex { float3 position:POSITION; uint packed_normal:NORMAL; float2 uv:TEXCOORD; float4 color:COLOR; uint2 packed_basis:TANGENT; };
struct Pixel {
    float4 position:SV_POSITION; float3 attenuation:TEXCOORD0;
    float3 direction:TEXCOORD1; float3 normal:TEXCOORD2; float2 uv:TEXCOORD3; float3 albedo:COLOR;
};
float normal_component(uint bits,uint width) {
    int sign=1<<(width-1); int value=int(bits&((1u<<width)-1u));
    if(value&sign)value-=sign*2;
    return value==-sign?-1.0:float(value)/float(sign-1);
}
float3 unpack_normal(uint packed){return float3(normal_component(packed,11),normal_component(packed>>11,11),normal_component(packed>>22,10));}
float3 tangent_light(Vertex v,float3 light){
    float3 normal=unpack_normal(v.packed_normal),basis5=unpack_normal(v.packed_basis.x),basis6=unpack_normal(v.packed_basis.y);
    precise float3 a=light*basis6,b=light*normal,c=light*(-basis5);
    return float3((a.x+a.y)+a.z,(b.x+b.y)+b.z,(c.x+c.y)+c.z);
}
Pixel VS(Vertex v) {
    Pixel o; o.position=mul(float4(v.position,1),view_projection);o.uv=v.uv;o.albedo=v.color.bgr;
    o.attenuation=float3(dot(float4(v.position,1),attenuation_rows[0]),
        dot(float4(v.position,1),attenuation_rows[1]),dot(float4(v.position,1),attenuation_rows[2]));
    o.direction=light_center.xyz-v.position;
    o.normal=unpack_normal(v.packed_normal);
    return o;
}
struct NormalPixel {float4 position:SV_POSITION;float3 attenuation:TEXCOORD0;float3 direction:TEXCOORD1;float3 normal:TEXCOORD2;float2 uv:TEXCOORD3;float3 albedo:COLOR;float3 tangent_direction:TEXCOORD4;};
NormalPixel normal_vertex(Vertex v,bool directional){
    Pixel p=VS(v);NormalPixel o;o.position=p.position;o.attenuation=p.attenuation;o.direction=p.direction;o.normal=p.normal;o.uv=p.uv;o.albedo=p.albedo;
    o.tangent_direction=tangent_light(v,directional?light_center.xyz:p.direction);return o;
}
NormalPixel VSNormal(Vertex v){return normal_vertex(v,false);}
NormalPixel VSNormalDirectional(Vertex v){return normal_vertex(v,true);}
float shadow_visibility(float3 direction) {
    float3 distance=abs(direction);
    float z=max(max(distance.x,distance.y),distance.z);
    float n=shadow_parameters.x,f=shadow_parameters.y;
    float scale=n*f/(f-n);
    // PCF samples adjacent texel centers, not this receiver's exact location.
    // Correct their receiver-plane depth range; a constant bias alone makes
    // sloping surfaces repeatedly shadow themselves as the camera moves.
    // Cube face coordinates span[-1,1], and the owned maps are512px per face.
    float3 plane=cross(ddx(direction),ddy(direction));
    float3 slope=abs(plane);
    float major=distance.x>=distance.y&&distance.x>=distance.z?slope.x:
                distance.y>=distance.z?slope.y:slope.z;
    float footprint=(2.0/512.0)*scale*(slope.x+slope.y+slope.z-major)/
                    max(abs(dot(plane,direction)),1.e-20);
    float depth=f/(f-n)-scale/max(z,n)-shadow_parameters.z-footprint;
    return shadow_map.SampleCmpLevelZero(shadow_sampler,-direction,depth);
}
float4 PS(Pixel input):SV_TARGET {
    LightPixelInput source;source.position=input.position;source.attenuation=input.attenuation;source.direction=input.direction;source.normal=input.normal;
    float4 color=source_light_main(source);
    float3 albedo=albedo_constant.rgb;
    if(albedo_options.x!=0)albedo=saturate(albedo_map.Sample(albedo_sampler,input.uv).rgb*albedo);
    color.rgb*=albedo*input.albedo;
    if(shadow_parameters.w!=0) {
        color.rgb*=shadow_visibility(input.direction);
    }
    return color;
}
// Source-ordered native composition writes illumination BEFORE the material.
// No base texture, vertex albedo or exposure multiplier is applied twice.
float4 PSNative(Pixel input):SV_TARGET {
    LightPixelInput source;source.position=input.position;source.attenuation=input.attenuation;source.direction=input.direction;source.normal=input.normal;
    float4 color=source_light_main(source);
    if(shadow_parameters.w!=0)color.rgb*=shadow_visibility(input.direction);
    return color;
}
float4 PSAmbient(Pixel input):SV_TARGET {return light_color;}
// Original 8FF30 auxiliary definition 4F6360: add the authored emissive image
// to packed ambient C2, before the ordinary material multiplies illumination.
float4 PSAuxiliaryAmbient(Pixel input):SV_TARGET {
    return float4(saturate(light_color.rgb+albedo_map.Sample(albedo_sampler,input.uv).rgb),0);
}
float4 PSDirectional(Pixel input):SV_TARGET {
    float3 normal=normalization_cube.SampleLevel(cube_sampler,input.normal,0).rgb;
    precise float d=light_center.x*normal.x+light_center.y*normal.y;
    d=clamp(d+light_center.z*normal.z,-1.0,1.0);
    return float4(saturate(clamp(d*light_color.rgb,-1.0,1.0)),0);
}

// Original static mode1 normal-map programs4F5370/4F5EB0. The inherited
// full-native parent owns post-opaque depth fog and specular-enable=0.
float4 PSNormalAmbient(Pixel input):SV_TARGET {
    float3 normal=albedo_map.Sample(albedo_sampler,input.uv).rgb*2-1;
    precise float d=clamp(normal.y,-1.0,1.0);
    d=clamp(d*material_scalar.a+(1-saturate(material_scalar.a)),-1.0,1.0);
    return float4(saturate(max(d,0)*light_color.rgb),0);
}
float4 PSNormal(NormalPixel input):SV_TARGET {
    float attenuation=attenuation_volume.SampleLevel(volume_sampler,input.attenuation,0).a;
    float3 direction=normalization_cube.SampleLevel(cube_sampler,input.tangent_direction,0).rgb;
    float3 normal=albedo_map.Sample(albedo_sampler,input.uv).rgb*2-1;
    float4 color=light_basic(direction,normal,attenuation,light_color.rgb,material_scalar.a);
    if(shadow_parameters.w!=0)color.rgb*=shadow_visibility(input.direction);return color;
}
float4 PSNormalDirectional(NormalPixel input):SV_TARGET {
    float4 direction=normalization_cube.SampleLevel(cube_sampler,input.tangent_direction,0);
    float3 normal=albedo_map.Sample(albedo_sampler,input.uv).rgb*2-1;
    precise float d=normal.x*direction.x+normal.y*direction.y;d=clamp(d+normal.z*direction.z,-1.0,1.0);
    precise float3 lit=clamp(d*light_color.rgb,-1.0,1.0);lit=clamp(lit*direction.a,-1.0,1.0);
    return float4(saturate(lit),0);
}

float4 masked_light(float4 texel,float4 direction,float attenuation){
    if(texel.a==0)discard;
    precise float d=clamp(direction.y,-1.0,1.0);
    precise float3 lit=clamp(attenuation*light_color.rgb,-1.0,1.0);lit=clamp(d*lit,-1.0,1.0);lit=clamp(lit*texel.a,-1.0,1.0);
    precise float gate=clamp(d+-material_scalar.a,-1.0,1.0);gate=max(gate,0);gate=clamp((gate+gate)*2,-1.0,1.0);lit=clamp(lit*gate,-1.0,1.0);
    return float4(saturate(max(lit,0)*texel.rgb),0);
}
float4 PSMasked(NormalPixel input):SV_TARGET {
    float4 color=masked_light(albedo_map.Sample(albedo_sampler,input.uv),normalization_cube.SampleLevel(cube_sampler,input.tangent_direction,0),attenuation_volume.SampleLevel(volume_sampler,input.attenuation,0).a);
    if(shadow_parameters.w!=0)color.rgb*=shadow_visibility(input.direction);return color;
}
float4 PSMaskedDirectional(NormalPixel input):SV_TARGET {
    float4 texel=albedo_map.Sample(albedo_sampler,input.uv);if(texel.a==0)discard;float4 direction=normalization_cube.SampleLevel(cube_sampler,input.tangent_direction,0);
    precise float3 lit=clamp(direction.y*light_color.rgb,-1.0,1.0);lit=clamp(lit*direction.a,-1.0,1.0);lit=clamp(lit*texel.a,-1.0,1.0);
    return float4(saturate(max(lit,0)*texel.rgb),0);
}

// Original basic specular programs4F5550/4F6090 accumulate into alpha.
cbuffer Specular : register(b5) {float4 specular_eye;float4 specular_strength;};
float3 reflected_eye(Vertex v){
    float3 normal=unpack_normal(v.packed_normal);precise float3 e=specular_eye.xyz-v.position;
    precise float product=e.x*normal.x+e.y*normal.y;product=product+e.z*normal.z;
    return (product+product)*normal-e;
}
struct SpecularPixel {float4 position:SV_POSITION;float3 attenuation:TEXCOORD0;float3 direction:TEXCOORD1;float3 normal:TEXCOORD2;float3 reflected:TEXCOORD4;};
SpecularPixel VSSpecular(Vertex v){
    Pixel p=VS(v);SpecularPixel o;o.position=p.position;o.attenuation=p.attenuation;o.direction=p.direction;o.normal=p.normal;o.reflected=reflected_eye(v);return o;
}
float4 specular_point(float3 d,float3 n,float3 reflected,float attenuation){
    precise float q=reflected.x*d.x+reflected.y*d.y;q=clamp(q+reflected.z*d.z,-1.0,1.0);
    q=clamp(q*q,-1.0,1.0);q=clamp(q*q,-1.0,1.0);q=clamp(q*q,-1.0,1.0);
    precise float diff=d.x*n.x+d.y*n.y;diff=clamp(diff+d.z*n.z,-1.0,1.0);
    precise float gate=max(clamp(diff+-material_scalar.a,-1.0,1.0),0);gate=clamp((gate+gate)*2,-1.0,1.0);
    precise float ag=clamp(attenuation*gate,-1.0,1.0);
    return float4(saturate(clamp(clamp(diff*light_color.rgb,-1.0,1.0)*ag,-1.0,1.0)),saturate(clamp(clamp(q*specular_strength.a,-1.0,1.0)*ag,-1.0,1.0)));
}
float4 PSSpecular(SpecularPixel input):SV_TARGET{
    float3 n=normalization_cube.SampleLevel(cube_sampler,input.normal,0).rgb;
    float3 d=normalization_cube.SampleLevel(cube_sampler,input.direction,0).rgb;
    float3 reflection=normalization_cube.SampleLevel(cube_sampler,input.reflected,0).rgb;
    float4 color=specular_point(d,n,reflection,attenuation_volume.SampleLevel(volume_sampler,input.attenuation,0).a);
    if(shadow_parameters.w!=0)color*=shadow_visibility(input.direction);return color;
}
float4 PSSpecularDirectional(SpecularPixel input):SV_TARGET{
    float3 n=normalization_cube.SampleLevel(cube_sampler,input.normal,0).rgb,reflection=normalization_cube.SampleLevel(cube_sampler,input.reflected,0).rgb;
    precise float d=n.x*light_center.x+n.y*light_center.y;d=clamp(d+n.z*light_center.z,-1.0,1.0);
    precise float q=reflection.x*light_center.x+reflection.y*light_center.y;q=clamp(q+reflection.z*light_center.z,-1.0,1.0);
    precise float gate=max(d,0);gate=clamp((gate+gate)*2,-1.0,1.0);
    return float4(saturate(clamp(d*light_color.rgb,-1.0,1.0)),saturate(clamp(q*specular_strength.a,-1.0,1.0)*gate));
}
