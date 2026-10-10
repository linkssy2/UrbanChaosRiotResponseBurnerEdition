#ifndef NV2A_VERTEX_SNAPSHOT_H
#define NV2A_VERTEX_SNAPSHOT_H
#include "nv2a_vertex_fetch_plan.h"
/* Experimental owned stream. Four-byte alignment permits bounded word loads
 * on the GPU; disabled attributes live in draw metadata, not every vertex.
 * The owner must retain the layout until all captured vertices are consumed. */
typedef struct NvVertexSnapshotLayout {
    uint32_t attribute[16][4]; /* packed offset, format, unused, unused */
    uint32_t defaults[16][4];
    uint32_t stride;
} NvVertexSnapshotLayout;
#if defined(_MSC_VER)
static __forceinline
#else
static inline
#endif
void nv_vertex_snapshot_copy(uint8_t *out,const uint8_t *in,unsigned bytes){
    /* Legal element lengths only. Constant-size unaligned-safe copies avoid
     * a general CRT call per attribute, without reading source padding. */
    switch(bytes){
    case 1:memcpy(out,in,1);break;case 2:memcpy(out,in,2);break;
    case 3:memcpy(out,in,2);memcpy(out+2,in+2,1);break;
    case 4:memcpy(out,in,4);break;case 6:memcpy(out,in,4);memcpy(out+4,in+4,2);break;
    case 8:memcpy(out,in,8);break;case 12:memcpy(out,in,8);memcpy(out+8,in+8,4);break;
    case 16:memcpy(out,in,16);break;
    }
}
#if defined(_MSC_VER)
static __forceinline
#else
static inline
#endif
void nv_vertex_snapshot_copy_padded(uint8_t *out,const uint8_t *in,unsigned bytes){
    /* Exactly bytes source bytes, exactly round_up(bytes,4) destination bytes.
     * Zero-extension initializes padding without clearing the whole vertex. */
    uint32_t value;uint16_t half;
    switch(bytes){
    case 1:value=in[0];memcpy(out,&value,4);break;
    case 2:memcpy(&half,in,2);value=half;memcpy(out,&value,4);break;
    case 3:memcpy(&half,in,2);value=half|((uint32_t)in[2]<<16);memcpy(out,&value,4);break;
    case 4:memcpy(out,in,4);break;
    case 6:memcpy(out,in,4);memcpy(&half,in+4,2);value=half;memcpy(out+4,&value,4);break;
    case 8:memcpy(out,in,8);break;
    case 12:memcpy(out,in,8);memcpy(out+8,in+8,4);break;
    case 16:memcpy(out,in,16);break;
    case 32:memcpy(out,in,32);break;
    default:if(bytes>16)memcpy(out,in,bytes);break; /* validated merged ranges */
    }
}
static int nv_vertex_snapshot_layout(NvVertexSnapshotLayout *out,const NvVertexFetchPlan *p){
    /* Validate in attribute-only scratch. Defaults need neither zeroing nor
     * an intermediate copy; publish them once after all rejection checks. */
    uint32_t attributes[16][4]={0};
    unsigned stride=0;
    if(!p->valid||p->count>16)return 0;
    for(unsigned i=0;i<p->count;++i){
        const NvVertexFetchEntry *e=&p->entry[i];
        unsigned bytes=nv2a_vertex_attribute_bytes(e->format);
        if(!bytes||e->attribute>=16||attributes[e->attribute][1])return 0;
        attributes[e->attribute][0]=stride;
        attributes[e->attribute][1]=e->format;
        stride+=(bytes+3u)&~3u;
    }
    /* A defaults-only vertex needs a valid, nonzero resource extent. */
    memcpy(out->attribute,attributes,sizeof(attributes));
    memcpy(out->defaults,p->defaults,sizeof(out->defaults));
    out->stride=stride?stride:4;
    return 1;
}
static int nv_vertex_snapshot_capture(const NvVertexFetchPlan *p,
    const NvVertexSnapshotLayout *layout,uint32_t index,uint8_t *out,size_t capacity){
    if(!p->valid||!p->ram||p->count>16||!layout->stride||layout->stride>256||capacity<layout->stride)return 0;
    /* Check the entire operation before output writes. Ownership requires a
     * layout built from this plan and a refresh on format/default changes. */
    for(unsigned i=0;i<p->count;++i){
        const NvVertexFetchEntry *e=&p->entry[i];
        if(e->attribute>=16||index>e->max_index)return 0;
        unsigned off=layout->attribute[e->attribute][0];
        unsigned bytes=nv2a_vertex_attribute_bytes(e->format);
        if(!bytes||layout->attribute[e->attribute][1]!=e->format||off>layout->stride||bytes>layout->stride-off)return 0;
    }
    memset(out,0,layout->stride);
    for(unsigned i=0;i<p->count;++i){
        const NvVertexFetchEntry *e=&p->entry[i];
        nv_vertex_snapshot_copy(out+layout->attribute[e->attribute][0],p->ram+e->address+index*e->stride,nv2a_vertex_attribute_bytes(e->format));
    }
    return 1;
}
/* Decoder-owned copy descriptors. They may outlive an index-only method, never
 * a fetch-plan revision. Source bytes remain mutable and are copied each time. */
typedef struct NvVertexSnapshotCapturePlan {
    uint64_t revision;
    const uint8_t *ram;
    unsigned valid,count,stride,max_index,copy_kind;
    struct {uint32_t address,stride,offset,bytes;} entry[16];
} NvVertexSnapshotCapturePlan;
static int nv_vertex_snapshot_build_capture(NvVertexSnapshotCapturePlan *out,
    const NvVertexFetchPlan *p,NvVertexSnapshotLayout *layout){
    /* Single decoder owner: publish valid only after all descriptors are ready.
     * Unused entries are never consumed, so do not clear/copy a temporary plan. */
    out->valid=0;
    if(!p->ram||!p->valid||p->count>16)return 0;
    /* Build GPU layout and copy ranges together. Each format is decoded once;
     * no second attribute walk or temporary layout-to-plan reconstruction. */
    memset(layout->attribute,0,sizeof(layout->attribute));
    unsigned packed_stride=0;
    out->revision=p->revision;out->ram=p->ram;out->count=0;
    out->max_index=UINT32_MAX;
    for(unsigned i=0;i<p->count;++i){
        const NvVertexFetchEntry *e=&p->entry[i];
        unsigned bytes=nv2a_vertex_attribute_bytes(e->format),offset=packed_stride;
        if(!bytes||e->attribute>=16||layout->attribute[e->attribute][1])return 0;
        layout->attribute[e->attribute][0]=offset;
        layout->attribute[e->attribute][1]=e->format;
        packed_stride+=(bytes+3u)&~3u;
        unsigned j=out->count;
        /* Merge only exact contiguous ranges, with identical index stepping
         * and no padding on either side. All original limits still contribute
         * below. Never include holes, source padding or another vertex. */
        if(j && !(bytes&3u) && !(out->entry[j-1].bytes&3u) &&
           out->entry[j-1].stride==e->stride &&
           (uint64_t)out->entry[j-1].address+out->entry[j-1].bytes==e->address &&
           out->entry[j-1].offset+out->entry[j-1].bytes==offset){
            out->entry[j-1].bytes+=bytes;
        }else{
            out->entry[j].address=e->address;out->entry[j].stride=e->stride;
            out->entry[j].offset=offset;out->entry[j].bytes=bytes;++out->count;
        }
        if(e->max_index<out->max_index)out->max_index=e->max_index;
    }
    memcpy(layout->defaults,p->defaults,sizeof(layout->defaults));
    layout->stride=packed_stride?packed_stride:4;
    out->stride=layout->stride;
    /* Select only by validated copy widths; addresses, offsets and strides
     * stay dynamic. Unknown layouts keep the general bounded copier. */
    out->copy_kind=0;
    static const unsigned patterns[6][7]={
        {6,6,4,16,8,6,2},{3,6,6,2},{6,6,4,4,8,6,2},
        {3,16,16,8},{3,16,4,8},{1,12}};
    for(unsigned kind=0;kind<6;++kind)if(out->count==patterns[kind][0]){
        unsigned n=0;for(;n<out->count;++n)if(out->entry[n].bytes!=patterns[kind][n+1])break;
        if(n==out->count){out->copy_kind=kind+1;break;}
    }
    out->valid=1;return 1;
}
static int nv_vertex_snapshot_prepare(NvVertexSnapshotCapturePlan *out,
    const NvVertexFetchPlan *p,const NvVertexSnapshotLayout *layout){
    NvVertexSnapshotLayout expected;
    if(!nv_vertex_snapshot_build_capture(out,p,&expected))return 0;
    if(memcmp(&expected,layout,sizeof(expected))){out->valid=0;return 0;}
    return 1;
}
static int nv_vertex_snapshot_capture_prepared(const NvVertexSnapshotCapturePlan *c,
    const NvVertexFetchPlan *p,uint32_t index,uint8_t *out,size_t capacity){
    /* The minimum of all source limits proves every access before any write.
     * Original plan builder already bounds multiplication and the whole element.
     * Owners invalidate p at guest/register reentry and non-index methods. */
    if(!c->valid||!p->valid||c->revision!=p->revision||c->ram!=p->ram||
        index>c->max_index||capacity<c->stride)return 0;
    if(!c->count){uint32_t zero=0;memcpy(out,&zero,4);}
    for(unsigned i=0;i<c->count;++i){
        uint8_t*dst=out+c->entry[i].offset;
        const uint8_t*src=c->ram+c->entry[i].address+index*c->entry[i].stride;
        unsigned n=c->entry[i].bytes;
        nv_vertex_snapshot_copy_padded(dst,src,n);
    }
    return 1;
}
/* Plan-private fixed-width kernels avoid per-attribute width dispatch. No
 * index/data reuse, overreading source padding, or shortened ownership scope.
 * copy_kind is published only by private plan construction/classification,
 * never from guest memory. */
/* Private packet-owner kernel; caller must prove plan lifetime, current index
 * and output capacity. No external/general caller may bypass those checks. */
#if defined(_MSC_VER)
static __forceinline
#else
static inline
#endif
int nv_vertex_snapshot_capture_owned(const NvVertexSnapshotCapturePlan*c,
    uint32_t index,uint8_t*out){
#define NV_COPY_AT(I,N) nv_vertex_snapshot_copy_padded(out+c->entry[I].offset,c->ram+c->entry[I].address+(size_t)index*c->entry[I].stride,N)
    switch(c->copy_kind){
    case 1:NV_COPY_AT(0,6);NV_COPY_AT(1,4);NV_COPY_AT(2,16);NV_COPY_AT(3,8);NV_COPY_AT(4,6);NV_COPY_AT(5,2);break;
    case 2:NV_COPY_AT(0,6);NV_COPY_AT(1,6);NV_COPY_AT(2,2);break;
    case 3:NV_COPY_AT(0,6);NV_COPY_AT(1,4);NV_COPY_AT(2,4);NV_COPY_AT(3,8);NV_COPY_AT(4,6);NV_COPY_AT(5,2);break;
    case 4:NV_COPY_AT(0,16);NV_COPY_AT(1,16);NV_COPY_AT(2,8);break;
    case 5:NV_COPY_AT(0,16);NV_COPY_AT(1,4);NV_COPY_AT(2,8);break;
    case 6:NV_COPY_AT(0,12);break;
    default:return 0;
    }
#undef NV_COPY_AT
    return 1;
}
static int nv_vertex_snapshot_capture_direct(const NvVertexSnapshotCapturePlan*c,
    const NvVertexFetchPlan*p,uint32_t index,uint8_t*out,size_t capacity){
    if(!c->copy_kind)return nv_vertex_snapshot_capture_prepared(c,p,index,out,capacity);
    if(!c->valid||!p->valid||c->revision!=p->revision||c->ram!=p->ram||
        index>c->max_index||capacity<c->stride)return 0;
    return nv_vertex_snapshot_capture_owned(c,index,out);
}
/* Reconstruct the ordinary GPU upload staging from owned bytes on fallback.
 * Never reread a guest source that may have changed since command consumption. */
static int nv_vertex_snapshot_expand(const NvVertexSnapshotLayout *layout,
    const uint8_t *bytes,size_t size,float out[16][4]){
    if(!layout->stride||layout->stride>256||size<layout->stride)return 0;
    for(unsigned a=0;a<16;++a){
        unsigned format=layout->attribute[a][1],off=layout->attribute[a][0];
        if(!((format>>4)&15))continue;
        unsigned n=nv2a_vertex_attribute_bytes(format);
        if(!n||off>layout->stride||n>layout->stride-off)return 0;
    }
    memcpy(out,layout->defaults,sizeof(layout->defaults));
    for(unsigned a=0;a<16;++a)if((layout->attribute[a][1]>>4)&15)
        nv2a_vertex_fetch(out[a],layout->attribute[a][1],bytes+layout->attribute[a][0]);
    return 1;
}
#endif
