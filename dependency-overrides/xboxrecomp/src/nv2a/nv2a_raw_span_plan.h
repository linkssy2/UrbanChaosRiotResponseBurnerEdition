#ifndef NV2A_RAW_SPAN_PLAN_H
#define NV2A_RAW_SPAN_PLAN_H
#include <stdlib.h>
#include <string.h>
#include "nv2a_raw_packets.h"

/* Optional private descriptor cache, not a guest-data cache. The immediate
 * context owner must reset this whenever its fetch plan is invalidated/reset.
 * A revision identifies immutable entries/defaults only inside that ownership
 * interval. Synthetic/public plans must not borrow the private owner's cache. */
typedef struct NvRawSpanPlan {
    const NvVertexFetchPlan *owner;
    const uint8_t *ram;
    uint64_t revision, end, threshold;
    uint32_t begin, stride, maximum;
    unsigned known, eligible, count;
    uint8_t attributes[16];
    NvRawStream pattern;
} NvRawSpanPlan;

static int nv_raw_span_plan_enabled(void){
    static int enabled=-1;
    if(enabled<0){const char*s=getenv("UCRR_CPU_BUNDLE_RAW_PLAN");enabled=s&&s[0]=='1'&&!s[1];}
    return enabled;
}
static void nv_raw_span_plan_reset(NvRawSpanPlan*c){if(c)c->known=0;}

static int nv_raw_span_plan_build(NvRawSpanPlan*c,const NvVertexFetchPlan*p){
    struct Entry {uint32_t address,bytes;} ordered[16],v;
    memset(c,0,sizeof(*c));
    if(!p||!p->valid||!p->ram||!p->revision||!p->count||p->count>16)return 0;
    c->owner=p;c->ram=p->ram;c->revision=p->revision;c->count=p->count;c->known=1;
    c->stride=p->entry[0].stride;c->maximum=UINT32_MAX;
    for(unsigned i=0;i<p->count;i++){
        const NvVertexFetchEntry*e=&p->entry[i];unsigned bytes=nv2a_vertex_attribute_bytes(e->format);
        if(e->attribute>=16||!bytes||c->pattern.attribute[e->attribute][1]||e->stride!=c->stride)return 0;
        if((uint64_t)e->address+bytes>UINT32_MAX)return 0;
        ordered[i]=(struct Entry){e->address,bytes};c->attributes[i]=(uint8_t)e->attribute;
        c->pattern.attribute[e->attribute][1]=e->format;
        c->pattern.attribute[e->attribute][2]=e->stride;
        if(e->max_index<c->maximum)c->maximum=e->max_index;
    }
    for(unsigned i=1;i<p->count;i++){
        v=ordered[i];unsigned j=i;
        while(j&&ordered[j-1].address>v.address){ordered[j]=ordered[j-1];--j;}ordered[j]=v;
    }
    c->begin=ordered[0].address;c->end=(uint64_t)c->begin+ordered[0].bytes;
    for(unsigned i=1;i<p->count;i++){
        if(ordered[i].address>c->end){
            if(!c->stride)return 0;
            uint64_t gap=(uint64_t)ordered[i].address-c->end;
            uint64_t threshold=(gap+c->stride-1)/c->stride;
            if(threshold>c->threshold)c->threshold=threshold;
        }
        uint64_t end=(uint64_t)ordered[i].address+ordered[i].bytes;
        if(end>c->end)c->end=end;
    }
    for(unsigned i=0;i<p->count;i++)c->pattern.attribute[p->entry[i].attribute][0]=p->entry[i].address-c->begin;
    memcpy(c->pattern.defaults,p->defaults,sizeof(c->pattern.defaults));
    c->pattern.spans=1;c->eligible=1;return 1;
}

/* Same byte/header contract as the reference capture. Eligibility is restricted
 * to one exact connected union of equal-stride source intervals. All checks
 * precede destination writes, and every successful call copies fresh RAM and
 * fresh indices. Mixed/disconnected layouts use the unchanged reference path. */
static int nv_raw_stream_capture_width_planned(NvRawSpanPlan*c,const NvVertexFetchPlan*p,size_t ram_bytes,
    const void*indices,unsigned width,unsigned count,uint8_t*out,size_t capacity,NvRawStream*result,
    const NvRawIndexBounds*bounds){
    if(!c||!p||!p->valid||!p->ram||!p->revision||!p->count||p->count>16||
       !indices||!count||count>16384||!out||!result||(width!=2&&width!=4))
        return nv_raw_stream_capture_width_owned(p,ram_bytes,indices,width,count,out,capacity,result,bounds);
    if(!c->known||c->owner!=p||c->ram!=p->ram||c->revision!=p->revision||c->count!=p->count)
        nv_raw_span_plan_build(c,p);
    if(!c->eligible)return nv_raw_stream_capture_width_owned(p,ram_bytes,indices,width,count,out,capacity,result,bounds);
    uint32_t low=UINT32_MAX,high=0;
    if(bounds){
        if(bounds->indices!=indices||bounds->width!=width||bounds->count!=count||bounds->low>bounds->high)return 0;
        low=bounds->low;high=bounds->high;
    }else if(width==2&&count>=16)nv_index_bounds(indices,2,count,&low,&high);
    else for(unsigned i=0;i<count;i++){
        uint32_t index=nv_raw_index_read(indices,i,width);
        if(index<low)low=index;if(index>high)high=index;
    }
    if(high>c->maximum)return 0;
    if((uint64_t)high-low<c->threshold){
        NvRawIndexBounds checked={indices,width,count,low,high};
        return nv_raw_stream_capture_width_owned(p,ram_bytes,indices,width,count,out,capacity,result,&checked);
    }
    uint64_t lo=(uint64_t)c->begin+(uint64_t)low*c->stride;
    uint64_t hi=c->end+(uint64_t)high*c->stride;
    if(hi>ram_bytes||hi>UINT32_MAX||hi<lo)return 0;
    uint64_t bytes=hi-lo,aligned=(bytes+3)&~3ULL;
    uint64_t index_bytes=(uint64_t)count*width,total=aligned+((index_bytes+3)&~3ULL);
    if(total>capacity||total>=NV_RAW_INDEX16)return 0;
    NvRawStream raw=c->pattern;
    for(unsigned i=0;i<c->count;i++)raw.attribute[c->attributes[i]][3]=low;
    raw.indices=(uint32_t)aligned;raw.count=count;raw.bytes=(uint32_t)total;
    memcpy(out,p->ram+(size_t)lo,(size_t)bytes);
    memset(out+(size_t)bytes,0,(size_t)(aligned-bytes));
    memcpy(out+(size_t)aligned,indices,(size_t)index_bytes);
    memset(out+(size_t)(aligned+index_bytes),0,(size_t)(total-aligned-index_bytes));
    if(width==2)raw.indices|=NV_RAW_INDEX16;
    *result=raw;return 1;
}

/* Intentional counterpart of nv_raw_packets_append_width_owned. Keep the
 * original directory/publish ordering; only the capture consumer differs. */
static int nv_raw_packets_append_width_planned(NvRawSpanPlan*c,NvRawPackets*a,const NvVertexFetchPlan*p,size_t ram_size,
    const void*indices,unsigned width,unsigned count,uint8_t*arena,size_t capacity,size_t packet_budget,
    const NvRawIndexBounds*bounds){
    if(!a||!arena||a->packets>=NV_RAW_PACKET_LIMIT||a->vertices>16384||count>16384-a->vertices||
       a->bytes<NV_RAW_DIRECTORY_BYTES||a->bytes>capacity||capacity-a->bytes<NV_RAW_STREAM_HEADER_BYTES)return 0;
    size_t room=capacity-a->bytes-NV_RAW_STREAM_HEADER_BYTES;if(room>packet_budget)room=packet_budget;
    NvRawStream raw;unsigned header=a->bytes,data=header+NV_RAW_STREAM_HEADER_BYTES;
    if(!nv_raw_stream_capture_width_planned(c,p,ram_size,indices,width,count,arena+data,room,&raw,bounds))return 0;
    if(raw.bytes>UINT32_MAX-data)return 0;
    memcpy(arena+header,&raw,sizeof(raw));
    uint32_t entry[4]={a->vertices,count,header,data};memcpy(arena+a->packets*16,entry,16);
    a->bytes=data+raw.bytes;a->vertices+=count;++a->packets;return 1;
}
#endif
