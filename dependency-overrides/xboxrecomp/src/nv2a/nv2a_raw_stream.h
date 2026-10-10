#ifndef NV2A_RAW_STREAM_H
#define NV2A_RAW_STREAM_H
#include "nv2a_vertex_fetch_plan.h"
#include "nv2a_index_bounds.h"
/* Experimental packet-owned raw spans. Never retain guest pointers after return.
 * Caller supplies immutable indices for this call and an explicit upload budget.
 * GPU pulls original indices; CPU does no per-vertex packing or hash remapping. */
typedef struct NvRawStream {
    uint32_t attribute[16][4],defaults[16][4];
    uint32_t indices,base,count,output_base; /* GPU cbuffer ABI, 528 bytes */
    uint32_t bytes,spans;
} NvRawStream;
#define NV_RAW_INDEX16 0x80000000u
static uint32_t nv_raw_index_read(const void*indices,unsigned i,unsigned width){
    if(width==2){uint16_t value;memcpy(&value,(const uint8_t*)indices+(size_t)i*2,2);return value;}
    uint32_t value;memcpy(&value,(const uint8_t*)indices+(size_t)i*4,4);return value;
}
/* Private proof for one callback-free raw_capture_packet closure. Never retain
 * this across guest execution or reuse it after changing the indexed bytes.
 * Identity checks prevent accidentally applying another packet's bounds; all
 * attribute/RAM/span/capacity checks still run in the consumer. */
typedef struct NvRawIndexBounds {
    const void *indices;
    unsigned width,count;
    uint32_t low,high;
} NvRawIndexBounds;
static int nv_raw_index_preflight(const void*indices,unsigned width,unsigned count,
    uint32_t maximum,NvRawIndexBounds*result){
    uint32_t low=UINT32_MAX,high=0;
    if(!indices||!result||!count||count>16384||(width!=2&&width!=4))return 0;
    if(width==2&&count>=16){
        nv_index_bounds(indices,width,count,&low,&high);
        if(high>maximum)return 0;
    }else{
        /* Keep the original scalar failure prefix: stop at the first invalid
         * index, without reading any later (possibly inaccessible) bytes. */
        for(unsigned i=0;i<count;i++){
            uint32_t index=nv_raw_index_read(indices,i,width);
            if(index>maximum)return 0;
            if(index<low)low=index;if(index>high)high=index;
        }
    }
    *result=(NvRawIndexBounds){indices,width,count,low,high};return 1;
}
static int nv_raw_stream_capture_width_owned(const NvVertexFetchPlan*p,size_t ram_bytes,
    const void*indices,unsigned width,unsigned count,uint8_t*out,size_t capacity,NvRawStream*result,
    const NvRawIndexBounds*bounds){
    struct Span {uint64_t lo,hi;uint32_t offset;} spans[16],v;
    uint64_t starts[16]; unsigned ns=0; uint32_t low=UINT32_MAX,high=0;
    NvRawStream r={0};
    if(!p||!p->valid||!p->ram||!p->count||p->count>16||!indices||!count||count>16384||!out||!result||(width!=2&&width!=4))return 0;
    if(bounds){
        if(bounds->indices!=indices||bounds->width!=width||bounds->count!=count||bounds->low>bounds->high)return 0;
        low=bounds->low;high=bounds->high;
    }else if(width==2 && count>=16)nv_index_bounds(indices,2,count,&low,&high);
    else if(width==2){
        for(unsigned i=0;i<count;i++){uint32_t index=nv_raw_index_read(indices,i,2);if(index<low)low=index;if(index>high)high=index;}
    }else{
        /* Keep the existing compiler-optimized 32-bit loop: the explicit SSE2
         * candidate regressed large 32-bit packets in the offline comparison. */
        for(unsigned i=0;i<count;i++){uint32_t index=nv_raw_index_read(indices,i,4);if(index<low)low=index;if(index>high)high=index;}
    }
    for(unsigned i=0;i<p->count;i++){
        const NvVertexFetchEntry*e=&p->entry[i];unsigned bytes=nv2a_vertex_attribute_bytes(e->format);
        if(e->attribute>=16||!bytes||r.attribute[e->attribute][1]||high>e->max_index)return 0;
        uint64_t lo=(uint64_t)e->address+(uint64_t)low*e->stride;
        uint64_t hi=(uint64_t)e->address+(uint64_t)high*e->stride+bytes;
        if(hi>ram_bytes||hi>UINT32_MAX||hi<lo)return 0;
        starts[i]=lo;spans[ns++]=(struct Span){lo,hi,0};
        r.attribute[e->attribute][1]=e->format;r.attribute[e->attribute][2]=e->stride;r.attribute[e->attribute][3]=low;
    }
    for(unsigned i=1;i<ns;i++){v=spans[i];unsigned j=i;while(j&&spans[j-1].lo>v.lo){spans[j]=spans[j-1];--j;}spans[j]=v;}
    unsigned merged=0;
    for(unsigned i=0;i<ns;i++){
        if(merged&&spans[i].lo<=spans[merged-1].hi){if(spans[i].hi>spans[merged-1].hi)spans[merged-1].hi=spans[i].hi;}
        else spans[merged++]=spans[i];
    }
    uint64_t total=0;
    for(unsigned i=0;i<merged;i++){spans[i].offset=(uint32_t)total;total+=(spans[i].hi-spans[i].lo+3)&~3ULL;if(total>UINT32_MAX)return 0;}
    r.indices=(uint32_t)total;total+=((uint64_t)count*width+3)&~3ULL;
    if(total>capacity||total>=NV_RAW_INDEX16)return 0;
    for(unsigned i=0;i<p->count;i++)for(unsigned j=0;j<merged;j++)if(starts[i]>=spans[j].lo&&starts[i]<spans[j].hi){
        r.attribute[p->entry[i].attribute][0]=spans[j].offset+(uint32_t)(starts[i]-spans[j].lo);break;
    }
    /* All checks precede writes. Only destination padding is initialized. */
    for(unsigned i=0;i<merged;i++){
        size_t bytes=(size_t)(spans[i].hi-spans[i].lo);
        memcpy(out+spans[i].offset,p->ram+(size_t)spans[i].lo,bytes);
        memset(out+spans[i].offset+bytes,0,((bytes+3)&~(size_t)3)-bytes);
    }
    memcpy(out+r.indices,indices,(size_t)count*width);
    memset(out+r.indices+(size_t)count*width,0,((count*width+3)&~3u)-count*width);
    if(width==2)r.indices|=NV_RAW_INDEX16;
    memcpy(r.defaults,p->defaults,sizeof(r.defaults));r.count=count;r.bytes=(uint32_t)total;r.spans=merged;*result=r;return 1;
}
static int nv_raw_stream_capture_width(const NvVertexFetchPlan*p,size_t ram_bytes,
    const void*indices,unsigned width,unsigned count,uint8_t*out,size_t capacity,NvRawStream*result){
    return nv_raw_stream_capture_width_owned(p,ram_bytes,indices,width,count,out,capacity,result,NULL);
}
static int nv_raw_stream_capture(const NvVertexFetchPlan*p,size_t ram_bytes,
    const uint32_t*indices,unsigned count,uint8_t*out,size_t capacity,NvRawStream*result){
    return nv_raw_stream_capture_width(p,ram_bytes,indices,4,count,out,capacity,result);
}
#endif
