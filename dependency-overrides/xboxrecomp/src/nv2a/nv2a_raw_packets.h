#ifndef NV2A_RAW_PACKETS_H
#define NV2A_RAW_PACKETS_H
#include <stdlib.h>
#include "nv2a_raw_stream.h"
#include "nv2a_vertex_snapshot.h"
#define NV_RAW_PACKET_LIMIT 128u
#define NV_RAW_DIRECTORY_BYTES (NV_RAW_PACKET_LIMIT*16u)
#define NV_RAW_STREAM_HEADER_BYTES ((unsigned)sizeof(NvRawStream))
/* Compact rows already follow output order. The GPU derives their row index;
 * no identity-index payload or CPU identity-table construction is needed. */
#define NV_RAW_IMPLICIT_INDICES UINT32_MAX
typedef struct NvRawPackets {unsigned bytes,packets,vertices;} NvRawPackets;
static void nv_raw_packets_begin(NvRawPackets*p){p->bytes=NV_RAW_DIRECTORY_BYTES;p->packets=p->vertices=0;}
/* Concatenate owned packet ranges without decoding vertices or expanding indices.
 * Source/destination arenas must be disjoint. Offsets inside NvRawStream stay
 * relative to that packet's payload; only directory offsets need relocation.
 * Preflight is transactional, including rejection of malformed directories. */
static int nv_raw_packets_join(NvRawPackets*dst,uint8_t*out,size_t capacity,
    const NvRawPackets*src,const uint8_t*in,unsigned first,unsigned packets){
    if(!dst||!out||!src||!in||out==in||!packets||first>=src->packets||
       src->packets>NV_RAW_PACKET_LIMIT||packets>src->packets-first||
       dst->packets>NV_RAW_PACKET_LIMIT-packets||dst->bytes<NV_RAW_DIRECTORY_BYTES||
       src->bytes<NV_RAW_DIRECTORY_BYTES||dst->bytes>capacity)return 0;
    unsigned bytes=0,vertices=0,source_first=0;
    for(unsigned i=0;i<packets;i++){
        uint32_t e[4];NvRawStream r;
        memcpy(e,in+(first+i)*16,16);
        if(e[2]<NV_RAW_DIRECTORY_BYTES||e[2]>src->bytes||
           src->bytes-e[2]<sizeof(r)||e[3]!=e[2]+sizeof(r))return 0;
        memcpy(&r,in+e[2],sizeof(r));
        if(!e[1]||e[1]!=r.count||e[1]>16384-vertices||
           e[3]>src->bytes||r.bytes>src->bytes-e[3]||(r.bytes&3))return 0;
        if(!i)source_first=e[0];
        if(e[0]!=source_first+vertices||source_first>16384-vertices-e[1])return 0;
        unsigned span=(unsigned)sizeof(r)+r.bytes;
        if(span>capacity-dst->bytes-bytes)return 0;
        bytes+=span;vertices+=e[1];
    }
    if(dst->vertices>16384-vertices)return 0;
    for(unsigned i=0;i<packets;i++){
        uint32_t e[4];NvRawStream r;memcpy(e,in+(first+i)*16,16);memcpy(&r,in+e[2],sizeof(r));
        unsigned span=(unsigned)sizeof(r)+r.bytes,header=dst->bytes;
        memcpy(out+header,in+e[2],span);
        e[0]=dst->vertices;e[2]=header;e[3]=header+(unsigned)sizeof(r);
        memcpy(out+dst->packets*16,e,16);
        dst->bytes+=span;dst->vertices+=e[1];++dst->packets;
    }
    return 1;
}
/* Append once per callback-free published packet, never once per draw end.
 * Directory entries: first output ordinal, count, metadata offset, data offset.
 * Vertex addressing, index remapping and default selection happen on the GPU.
 * On failure no published directory/counters or earlier snapshot changes. */
static int nv_raw_packets_append_width_owned(NvRawPackets*a,const NvVertexFetchPlan*p,size_t ram_size,
    const void*indices,unsigned width,unsigned count,uint8_t*arena,size_t capacity,size_t packet_budget,
    const NvRawIndexBounds*bounds){
    if(!a||!arena||a->packets>=NV_RAW_PACKET_LIMIT||a->vertices>16384||count>16384-a->vertices||
       a->bytes<NV_RAW_DIRECTORY_BYTES||a->bytes>capacity||capacity-a->bytes<NV_RAW_STREAM_HEADER_BYTES)return 0;
    size_t room=capacity-a->bytes-NV_RAW_STREAM_HEADER_BYTES;if(room>packet_budget)room=packet_budget;
    NvRawStream raw;unsigned header=a->bytes,data=header+NV_RAW_STREAM_HEADER_BYTES;
    if(!nv_raw_stream_capture_width_owned(p,ram_size,indices,width,count,arena+data,room,&raw,bounds))return 0;
    if(raw.bytes>UINT32_MAX-data)return 0;
    memcpy(arena+header,&raw,sizeof(raw));
    uint32_t entry[4]={a->vertices,count,header,data};memcpy(arena+a->packets*16,entry,16);
    a->bytes=data+raw.bytes;a->vertices+=count;++a->packets;return 1;
}
static int nv_raw_packets_append_width(NvRawPackets*a,const NvVertexFetchPlan*p,size_t ram_size,
    const void*indices,unsigned width,unsigned count,uint8_t*arena,size_t capacity,size_t packet_budget){
    return nv_raw_packets_append_width_owned(a,p,ram_size,indices,width,count,arena,capacity,packet_budget,NULL);
}
static int nv_raw_packets_append(NvRawPackets*a,const NvVertexFetchPlan*p,size_t ram_size,
    const uint32_t*indices,unsigned count,uint8_t*arena,size_t capacity,size_t packet_budget){
    return nv_raw_packets_append_width(a,p,ram_size,indices,4,count,arena,capacity,packet_budget);
}
static int nv_raw_direct_seed_enabled(void){
    static int enabled=-1;
    if(enabled<0){const char*s=getenv("UCRR_RAW_DIRECT_SEED");enabled=s&&s[0]=='1'&&!s[1];}
    return enabled;
}
/* One/two-row seed from the already owned compact snapshot. The private owner
 * supplies rows zero/one with an ascending tightly packed layout and disjoint arena.
 * Preserve the generic seed's exact header, explicit indices and padding;
 * no synthetic fetch plan, source-span sorting or guest-memory access is needed.
 * Other row mappings/layouts retain the existing generic seed path. */
static int nv_raw_packets_seed_first_owned(NvRawPackets*a,const NvVertexSnapshotLayout*layout,
    const uint8_t*row,size_t row_bytes,unsigned rows,uint8_t*arena,size_t capacity){
    if(!a||!layout||!row||!arena||rows<1||rows>2||a->packets||a->vertices||a->bytes!=NV_RAW_DIRECTORY_BYTES||
       !layout->stride||layout->stride>256||row_bytes<(size_t)rows*layout->stride||
       capacity<a->bytes||capacity-a->bytes<sizeof(NvRawStream)+(size_t)rows*(layout->stride+4))return 0;
    NvRawStream raw={0};unsigned offset=0,previous_end=0,spans=0;
    for(unsigned attr=0;attr<16;attr++)if(layout->attribute[attr][1]){
        unsigned format=layout->attribute[attr][1],bytes=nv2a_vertex_attribute_bytes(format);
        if(!bytes||layout->attribute[attr][0]!=offset||offset>layout->stride||bytes>layout->stride-offset)return 0;
        if(!spans||offset!=previous_end)++spans;
        raw.attribute[attr][0]=offset;raw.attribute[attr][1]=format;
        raw.attribute[attr][2]=layout->stride;
        previous_end=offset+bytes;offset+=(bytes+3u)&~3u;
    }
    if(!spans||offset!=layout->stride)return 0;
    memcpy(raw.defaults,layout->defaults,sizeof(raw.defaults));
    raw.indices=rows*offset;raw.count=rows;raw.bytes=rows*(offset+4);raw.spans=rows==2?1:spans;
    unsigned header=a->bytes,data=header+(unsigned)sizeof(raw);
    /* All rejection checks precede every destination write. Recreate padding
     * explicitly, even if an owned source's unused bytes contain old values. */
    if(rows==2){
        /* Every attribute interval extends by one complete stride, making the
         * two-row union connected. The generic source-span copier preserves
         * interior padding; only the final trailing padding is zero-filled. */
        size_t copied=(size_t)layout->stride+previous_end;
        memcpy(arena+data,row,copied);memset(arena+data+copied,0,raw.indices-copied);
    }else if(spans==1&&previous_end==layout->stride)memcpy(arena+data,row,layout->stride);
    else for(unsigned attr=0;attr<16;attr++)if(raw.attribute[attr][1]){
        unsigned at=raw.attribute[attr][0];
        nv_vertex_snapshot_copy_padded(arena+data+at,row+at,nv2a_vertex_attribute_bytes(raw.attribute[attr][1]));
    }
    uint32_t indices[2]={0,1};memcpy(arena+data+raw.indices,indices,rows*4u);
    memcpy(arena+header,&raw,sizeof(raw));
    uint32_t entry[4]={0,rows,header,data};memcpy(arena,entry,sizeof(entry));
    a->bytes=data+raw.bytes;a->vertices=rows;a->packets=1;return 1;
}
/* A sparse packet need not unpack earlier raw packets. Copy only its vertices
 * into one compact snapshot using the same GPU directory/attribute ABI.
 * c/layout must be the paired private descriptors built from p. No guest
 * callbacks occur during capture; failed preflight leaves all bytes unchanged. */
static int nv_raw_packets_append_compact(NvRawPackets*a,const NvVertexFetchPlan*p,
    const NvVertexSnapshotCapturePlan*c,const NvVertexSnapshotLayout*layout,
    const uint32_t*indices,unsigned count,uint8_t*arena,size_t capacity){
    if(!a||!p||!c||!layout||!indices||!arena||!count||count>16384||
       !p->valid||!c->valid||!p->ram||c->ram!=p->ram||c->revision!=p->revision||
       !layout->stride||layout->stride>256||(layout->stride&3)||c->stride!=layout->stride||
       a->packets>=NV_RAW_PACKET_LIMIT||a->vertices>16384||count>16384-a->vertices||
       a->bytes<NV_RAW_DIRECTORY_BYTES||a->bytes>capacity)return 0;
    size_t row_bytes=(size_t)count*layout->stride;
    size_t data_bytes=row_bytes;
    if(NV_RAW_STREAM_HEADER_BYTES+data_bytes>capacity-a->bytes||
       NV_RAW_STREAM_HEADER_BYTES+data_bytes>UINT32_MAX-a->bytes)return 0;
    for(unsigned i=0;i<count;i++)if(indices[i]>c->max_index)return 0;
    NvRawStream r={0};
    for(unsigned attr=0;attr<16;attr++)if(layout->attribute[attr][1]){
        unsigned off=layout->attribute[attr][0],fmt=layout->attribute[attr][1];
        unsigned bytes=nv2a_vertex_attribute_bytes(fmt);
        if(!bytes||off>layout->stride||bytes>layout->stride-off)return 0;
        r.attribute[attr][0]=off;r.attribute[attr][1]=fmt;
        r.attribute[attr][2]=layout->stride;r.attribute[attr][3]=0;
    }
    memcpy(r.defaults,layout->defaults,sizeof(r.defaults));
    r.indices=NV_RAW_IMPLICIT_INDICES;r.count=count;r.bytes=(uint32_t)data_bytes;r.spans=1;
    unsigned header=a->bytes,data=header+NV_RAW_STREAM_HEADER_BYTES;
    for(unsigned i=0;i<count;i++){
        /* Preflight above proves every prepared capture check. */
        nv_vertex_snapshot_capture_direct(c,p,indices[i],arena+data+(size_t)i*layout->stride,layout->stride);
    }
    memcpy(arena+header,&r,sizeof(r));
    uint32_t entry[4]={a->vertices,count,header,data};memcpy(arena+a->packets*16,entry,16);
    a->bytes=data+r.bytes;a->vertices+=count;++a->packets;return 1;
}
#endif
