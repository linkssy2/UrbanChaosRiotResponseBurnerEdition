// Experimental compact attribute decoding, independently exercised by readback.
// Typed R32_UINT avoids RAW/STRUCTURED misc flags on CPU-writable buffers.
// Every captured element has padded storage for bounded aligned word loads.
#ifdef DRAW_VERTEX
Buffer<uint> packed_vertices : register(t7);
cbuffer PackedLayout : register(b2) {
#else
Buffer<uint> packed_vertices : register(t0);
cbuffer PackedLayout : register(b0) {
#endif
    uint4 attribute_layout[16];
    uint4 attribute_defaults[16];
#if defined(DRAW_VERTEX) && SPLIT_PACKED_METADATA
};
cbuffer PackedParameters : register(b3) {
#endif
    uint packed_stride, packed_base, packed_count, output_base;
};
uint packed_byte(uint address) {
    return (packed_vertices.Load(address >> 2) >> ((address & 3u)*8u)) & 255u;
}
uint packed_word(uint address) {
    uint value=0;
#if RAW_SOURCE_STREAM
    // Raw source formats may be unaligned. Never load past the padded span.
    if(address&3u){
        uint b0=packed_byte(address),b1=packed_byte(address+1);
        uint b2=packed_byte(address+2),b3=packed_byte(address+3);
        value=b0|(b1<<8)|(b2<<16)|(b3<<24);
    }else value=packed_vertices.Load(address>>2);
#else
    value=packed_vertices.Load(address>>2);
#endif
    return value;
}
#if RAW_PACKET_STREAM && RAW_PACKET_VERTEX_PLAN && defined(DRAW_VERTEX)
// One packet/index selection per vertex, shared by all authored attributes.
// Inputs remain owned raw packets; this changes no resource or descriptor ABI.
uint3 raw_packet_vertex(uint vertex) {
    uint lo=0,hi=packed_stride;
    [loop]while(lo+1<hi){uint mid=(lo+hi)/2;uint first=packed_vertices.Load(packed_base/4+mid*4);if(first<=vertex)lo=mid;else hi=mid;}
    uint directory=packed_base/4+lo*4;
    uint first_vertex=packed_vertices.Load(directory);
    uint header=packed_base+packed_vertices.Load(directory+2);
    uint data_base=packed_base+packed_vertices.Load(directory+3);
    uint index_offset=packed_vertices.Load(header/4+128);
    uint index=vertex-first_vertex;
    [branch]if(index_offset!=0xffffffffu){
        uint offset=index_offset&0x7fffffffu;
        if(index_offset&0x80000000u){uint pair=packed_vertices.Load((data_base+offset)/4+index/2);index=(pair>>((index&1u)*16u))&65535u;}
        else index=packed_vertices.Load((data_base+offset)/4+index);
    }
    return uint3(header,data_base,index);
}
#endif
uint4 unpack_attribute(uint vertex,uint attribute
#if RAW_PACKET_STREAM && RAW_PACKET_VERTEX_PLAN && defined(DRAW_VERTEX)
    ,uint3 prepared
#endif
) {
#if RAW_PACKET_STREAM
#if RAW_PACKET_VERTEX_PLAN && defined(DRAW_VERTEX)
    uint header=prepared.x,data_base=prepared.y;
#else
    // Binary search ordered packet ranges. No per-vertex CPU reference table.
    uint lo=0,hi=packed_stride;
    [loop]while(lo+1<hi){uint mid=(lo+hi)/2;uint first=packed_vertices.Load(packed_base/4+mid*4);if(first<=vertex)lo=mid;else hi=mid;}
    uint directory=packed_base/4+lo*4;
    uint first_vertex=packed_vertices.Load(directory);
    uint header=packed_base+packed_vertices.Load(directory+2);
    uint data_base=packed_base+packed_vertices.Load(directory+3);
#endif
    uint d=header/4+attribute*4;
    uint4 desc=uint4(packed_vertices.Load(d),packed_vertices.Load(d+1),packed_vertices.Load(d+2),packed_vertices.Load(d+3));
    uint default_address=header/4+64+attribute*4;
    uint4 default_value=uint4(packed_vertices.Load(default_address),packed_vertices.Load(default_address+1),packed_vertices.Load(default_address+2),packed_vertices.Load(default_address+3));
#elif PACKED_LAYOUT_SPECIALIZED
    uint4 desc=specialized_layout[attribute];
#else
    uint4 desc=attribute_layout[attribute];
#endif
    uint type=desc.y&15u,n=(desc.y>>4)&15u;
#if RAW_PACKET_STREAM
    if(!n)return default_value;
#if RAW_PACKET_VERTEX_PLAN && defined(DRAW_VERTEX)
    uint index=prepared.z;
#else
    uint index_offset=packed_vertices.Load(header/4+128);
    uint index=vertex-first_vertex;
    // Compact packets are already in output order. Keep this a branch so the
    // sentinel never becomes a speculative out-of-range buffer address.
    [branch]if(index_offset!=0xffffffffu){
        uint offset=index_offset&0x7fffffffu;
        if(index_offset&0x80000000u){
            uint pair=packed_vertices.Load((data_base+offset)/4+index/2);
            index=(pair>>((index&1u)*16u))&65535u;
        }else index=packed_vertices.Load((data_base+offset)/4+index);
    }
#endif
    uint address=data_base+desc.x+(index-desc.w)*desc.z;
#else
    if(!n)return attribute_defaults[attribute];
#if RAW_SOURCE_STREAM
    uint index=packed_vertices.Load((packed_base+packed_stride)/4+vertex);
    uint address=packed_base+desc.x+(index-desc.w)*desc.z;
#else
    uint address=packed_base+vertex*packed_stride+desc.x;
#if PACKED_PARTICLE_STREAM
    address=packed_base+(vertex/6u)*packed_stride+desc.x;
#endif
#endif
#endif
    uint4 result=uint4(0,0,0,0x3f800000);
    if(type==2){
        [unroll]for(uint c=0;c<4;++c)if(c<n){
            uint source=address+c*4;
#if PACKED_PARTICLE_STREAM
            if(desc.x+c*4==20u){
                uint k=vertex%6u;
                uint corner=k==3u?0u:k==4u?2u:k==5u?3u:k;
                source=packed_base+(vertex/6u)*packed_stride+52u+corner*4u;
            }
#endif
            result[c]=packed_word(source);
        }
        return result;
    }
    if(type==6){
        uint bits=packed_word(address);
        int x=asint(bits<<21)>>21,y=asint(bits<<10)>>21,z=asint(bits)>>22;
        return asuint(float4(max(-1.0f,x/1023.0f),max(-1.0f,y/1023.0f),max(-1.0f,z/511.0f),1));
    }
    [unroll]for(uint c=0;c<4;++c)if(c<n){
        float value;
        if(type==0||type==4)value=packed_byte(address+c)/255.0f;
        else{
            uint bits=packed_byte(address+c*2)|(packed_byte(address+c*2+1)<<8);
            int signed_value=asint(bits<<16)>>16;
            value=type==1?max(-1.0f,signed_value/32767.0f):(float)signed_value;
        }
        result[c]=asuint(value);
    }
    if(type==0&&n>=3){uint swap=result.x;result.x=result.z;result.z=swap;}
    return result;
}
#ifndef DRAW_VERTEX
RWStructuredBuffer<uint4> unpacked_vertices : register(u0);
[numthreads(32,1,1)]void main(uint3 thread:SV_DispatchThreadID){
    if(thread.x>=packed_count)return;
    [unroll]for(uint a=0;a<16;++a)unpacked_vertices[(output_base+thread.x)*16+a]=unpack_attribute(thread.x,a);
}
#endif
