// Candidate GPU evaluator for the same bounded instruction subset as
// nv2a_vertex_eval.h. Not yet wired into production drawing. No hardware
// precision/fidelity claim; the compute entrypoint exposes every output to tests.
cbuffer GuestProgram : register(b0) {
    float4 constants[192];
#if !COMPACT_SPECIALIZED
    uint4 program[136];
    uint start_pc, vertex_count, reserved0, reserved1;
#endif
};
#ifdef DRAW_VERTEX
#ifdef PACKED_VERTEX_STREAM
// PACKED_VERTEX_HELPERS
#else
#ifdef TYPED_VERTEX_STREAM
Buffer<uint4> vertices : register(t7);
#else
StructuredBuffer<float4> vertices : register(t7);
#endif
#endif
cbuffer DrawConversion : register(b1) { float4 screen_depth; float4 uv_divisor[4]; float4 fog_params; };
#else
StructuredBuffer<float4> vertices : register(t0);
RWStructuredBuffer<float4> results : register(u0);
#endif

float4 masked(float4 old_value, float4 value, uint mask) {
    return float4((mask&8)?value.x:old_value.x,(mask&4)?value.y:old_value.y,
                  (mask&2)?value.z:old_value.z,(mask&1)?value.w:old_value.w);
}
uint component_bits(uint4 bits,uint index){return index==0?bits.x:index==1?bits.y:index==2?bits.z:bits.w;}
bool source_value(out float4 value, uint mux, uint reg, uint swizzle,
                  uint negate, uint vertex, int ci,
                  float4 inputs[16], float4 temp[16]) {
    float4 src=0;value=0;
    if(mux==1)src=temp[reg&15];
    else if(mux==2)src=inputs[vertex&15];
    else if(mux==3 && ci>=0 && ci<192)src=constants[clamp(ci,0,191)];
    else return false;
    uint4 bits=asuint(src);
    value=asfloat(uint4(component_bits(bits,(swizzle>>6)&3),component_bits(bits,(swizzle>>4)&3),
                 component_bits(bits,(swizzle>>2)&3),component_bits(bits,swizzle&3))^(negate?0x80000000u:0u));
    return true;
}
bool evaluate(float4 inputs[16], out float4 outputs[16], out uint written[16]) {
    float4 temp[16];int address=0;
    [unroll]for(uint k=0;k<16;++k){temp[k]=0;outputs[k]=float4(0,0,0,1);written[k]=0;}
    [loop]for(uint pc=start_pc;pc<136;++pc){
        uint4 w=program[pc];
        uint mac=(w.y>>21)&15,ilu=(w.y>>25)&7,vertex=(w.y>>9)&15,r=(w.w>>20)&15;
        int ci=int((w.y>>13)&255)+((w.w&2)?address:0);
        precise float4 a=0,b=0,c=0,m=0,s=0;
        if(mac>13 || ilu>4)return false;
        if(mac && !source_value(a,(w.z>>26)&3,w.z>>28,w.y&255,(w.y>>8)&1,vertex,ci,inputs,temp))return false;
        if(mac>=2 && mac!=3 && mac!=13 && !source_value(b,(w.z>>11)&3,(w.z>>13)&15,(w.z>>17)&255,(w.z>>25)&1,vertex,ci,inputs,temp))return false;
        if((ilu || mac==3 || mac==4) && !source_value(c,(w.w>>28)&3,((w.z&3)<<2)|(w.w>>30),(w.z>>2)&255,(w.z>>10)&1,vertex,ci,inputs,temp))return false;
        switch(mac){
        case 1:m=a;break;case 2:m=a*b;break;case 3:m=a+c;break;case 4:m=a*b+c;break;
        case 5:m=a.x*b.x+a.y*b.y+a.z*b.z;break;
        case 6:m=a.x*b.x+a.y*b.y+a.z*b.z+b.w;break;
        case 7:m=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;break;
        case 8:m=float4(1,a.y*b.y,a.z,b.w);break;
        case 9:m=min(a,b);break;case 10:m=max(a,b);break;
        case 11:m=float4(a.x<b.x,a.y<b.y,a.z<b.z,a.w<b.w);break;
        case 12:m=float4(a.x>=b.x,a.y>=b.y,a.z>=b.z,a.w>=b.w);break;
        }
        if(ilu==1)s=c;
        if(ilu==2 || ilu==3)s=1.0/c.x;
        if(ilu==3){float magnitude=clamp(abs(s.x),5.421010862427522e-20,1.8446744073709552e19);s=asfloat(asuint(magnitude)|(asuint(s.x)&0x80000000));}
        if(ilu==4)s=1.0/sqrt(abs(c.x));
        if(mac==13){if(!isfinite(a.x)||a.x<-192||a.x>192)return false;address=int(floor(a.x));}
        // Paired units read the same old registers; only now commit their writes.
        if(mac && mac!=13 && !(ilu && r==1)){
            uint mask=(w.w>>24)&15;temp[r]=masked(temp[r],m,mask);
            if(r==12){outputs[0]=masked(outputs[0],m,mask);written[0]|=mask;}
        }
        if(ilu){uint dest=mac?1:r,mask=(w.w>>16)&15;temp[dest]=masked(temp[dest],s,mask);
            if(dest==12){outputs[0]=masked(outputs[0],s,mask);written[0]|=mask;}}
        uint mask=(w.w>>12)&15,dest=(w.w>>3)&15;
        if(mask){if(!(w.w&0x800))return false;
            outputs[dest]=masked(outputs[dest],(w.w&4)?s:m,mask);written[dest]|=mask;
            if(dest==0)temp[12]=masked(temp[12],outputs[0],mask);}
        if(w.w&1)return true;
    }
    return false;
}
#ifndef DRAW_VERTEX
[numthreads(32,1,1)]
void main(uint3 thread:SV_DispatchThreadID){
    if(thread.x>=vertex_count)return;
    float4 inputs[16],outputs[16];uint written[16];
    [unroll]for(uint i=0;i<16;++i)inputs[i]=vertices[thread.x*16+i];
    bool valid=evaluate(inputs,outputs,written);
    [unroll]for(uint j=0;j<16;++j)results[thread.x*21+j]=outputs[j];
    [unroll]for(uint q=0;q<4;++q)results[thread.x*21+16+q]=float4(written[q*4],written[q*4+1],written[q*4+2],written[q*4+3]);
    results[thread.x*21+20]=float4(valid,0,0,0);
}
#else
#ifndef MATERIAL_OUTPUTS
#define MATERIAL_OUTPUTS 127
#endif
struct DrawOutput {
    float4 position:SV_POSITION;
    float4 color0:COLOR0;float4 color1:COLOR1;
    float4 tc0:TEXCOORD0;float4 tc1:TEXCOORD1;
    float4 tc2:TEXCOORD2;float4 tc3:TEXCOORD3;float fog:TEXCOORD4;
};
DrawOutput draw_main(uint id:SV_VertexID){
    DrawOutput o=(DrawOutput)0;
    float4 inputs[16],outputs[16];uint written[16];
#if RAW_PACKET_STREAM && RAW_PACKET_VERTEX_PLAN
    uint3 raw_vertex=raw_packet_vertex(id);
#endif
#if defined(TYPED_VERTEX_STREAM) && !defined(PACKED_VERTEX_STREAM)
    // D3D11 SV_VertexID excludes Draw's StartVertexLocation. Carry the base
    // explicitly in the unused conversion word, not in the guest program.
    id+=(uint)screen_depth.w;
#endif
    [unroll]for(uint i=0;i<16;++i){
#ifdef PACKED_VERTEX_STREAM
        inputs[i]=asfloat(unpack_attribute(id,i
#if RAW_PACKET_STREAM && RAW_PACKET_VERTEX_PLAN
            ,raw_vertex
#endif
        ));
#else
#ifdef TYPED_VERTEX_STREAM
        inputs[i]=asfloat(vertices[id*16+i]);
#else
        inputs[i]=vertices[id*16+i];
#endif
#endif
    }
    bool valid=evaluate(inputs,outputs,written);
    float4 p=outputs[0];
    if(!valid || written[0]!=15 || !all(isfinite(p)) || p.w==0){o.position=float4(2,2,2,1);return o;}
    o.position=float4((p.x/screen_depth.x*2-1)*p.w,(1-p.y/screen_depth.y*2)*p.w,p.z/screen_depth.z*p.w,p.w);
    #if MATERIAL_OUTPUTS & 1
    o.color0=floor(saturate(outputs[3])*255+0.5)/255;
    #endif
    // Preserve the authored secondary color for register-combiner V1 reads.
    #if MATERIAL_OUTPUTS & 2
    o.color1=floor(saturate(outputs[4])*255+0.5)/255;
    #endif
    #if MATERIAL_OUTPUTS & 4
    o.tc0=outputs[9]/uv_divisor[0];
    #endif
    #if MATERIAL_OUTPUTS & 8
    o.tc1=outputs[10]/uv_divisor[1];
    #endif
    #if MATERIAL_OUTPUTS & 16
    o.tc2=outputs[11]/uv_divisor[2];
    #endif
    #if MATERIAL_OUTPUTS & 32
    o.tc3=outputs[12]/uv_divisor[3];
    #endif
    o.fog=1;
    // NV097 linear fog: preserve the unbounded vertex factor for interpolation;
    // the combiner clamps it per pixel. fog_params = {bias,slope,mode,enabled}.
    #if MATERIAL_OUTPUTS & 64
    if(fog_params.w!=0 && fog_params.z==9729){
        float distance=outputs[5].x;
        float factor=fog_params.x+distance*fog_params.y-1;
        o.fog=isinf(distance)||isnan(factor)?1:clamp(factor,-3.402823466e+38,3.402823466e+38);
    }
    #endif
    return o;
}
#endif
