#ifndef NV2A_GEOMETRY_CENSUS_H
#define NV2A_GEOMETRY_CENSUS_H
/* Diagnostic only: reconstruct and check the exact successful raw-copy union.
 * Descriptor/index recurrence is NOT immutable content or an eligible cache hit.
 * Existing-owner lookup never registers, arms, or changes memory protections. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource_revisions.h"
#include "guest_lighting_coverage.h"
#define NV_GC_SLOTS 8192u
#define NV_GC_BYTES (32u*1024u*1024u)
#define NV_GC_STRIDE 64u
typedef struct NvGeometryKey {
    uint64_t hash,ram,ram_bytes;
    uint32_t count,width,entries,index_offset;
    NvVertexFetchEntry entry[16];
    float defaults[16][4];
} NvGeometryKey;
static struct {
    unsigned active,finished,used,index_bytes,allocation_failed;
    DWORD owner;
    uint64_t start,end,calls,raw,raw_rejected,planned_option,bytes,sampled,span_errors;
    uint64_t lighting_raw,lighting_sampled,lighting_bytes,lighting_owned,lighting_repeats;
    uint64_t spans,covered_spans,source_bytes,covered_bytes,all_owned;
    uint64_t repeats,repeat_owned,repeat_bytes,overflow,owner_errors;
    unsigned examples;
    struct {uint32_t lo,bytes,owner;} example[64];
    NvGeometryKey *keys;
    uint8_t *indices;
} nv_geometry_census;
static uint64_t nv_gc_hash(uint64_t h,const void*p,size_t n){
    const uint8_t*s=(const uint8_t*)p;
    for(size_t i=0;i<n;i++){h^=s[i];h*=UINT64_C(1099511628211);}return h;
}
void uc_geometry_census_boundary(int start,uint64_t qpc){
    const char*s=getenv("UCRR_LIGHTING_LIST_PROBE");
    if(!s||strcmp(s,"1")||nv_geometry_census.finished)return;
    if(start){
        if(nv_geometry_census.active)return;
        nv_geometry_census.owner=GetCurrentThreadId();nv_geometry_census.start=qpc;
        nv_geometry_census.keys=(NvGeometryKey*)calloc(NV_GC_SLOTS,sizeof(NvGeometryKey));
        nv_geometry_census.indices=(uint8_t*)malloc(NV_GC_BYTES);
        nv_geometry_census.allocation_failed=!nv_geometry_census.keys||!nv_geometry_census.indices;
        nv_geometry_census.active=1;return;
    }
    if(!nv_geometry_census.active)return;
    if(nv_geometry_census.owner!=GetCurrentThreadId()){++nv_geometry_census.owner_errors;return;}
    nv_geometry_census.active=0;nv_geometry_census.finished=1;nv_geometry_census.end=qpc;
    char path[128];snprintf(path,sizeof(path),"geometry-census-%lu-%llu.json",GetCurrentProcessId(),(unsigned long long)nv_geometry_census.start);
    FILE*f=fopen(path,"wx");if(!f)return;
    fprintf(f,"{\"version\":1,\"sample_stride\":%u,\"dictionary_capacity\":%u,\"index_capacity\":%u",NV_GC_STRIDE,NV_GC_SLOTS,NV_GC_BYTES);
#define GC_FIELD(n) fprintf(f,",\"" #n "\":%llu",(unsigned long long)nv_geometry_census.n)
    GC_FIELD(start);GC_FIELD(end);GC_FIELD(calls);GC_FIELD(raw);GC_FIELD(raw_rejected);GC_FIELD(planned_option);GC_FIELD(bytes);
    GC_FIELD(lighting_raw);GC_FIELD(lighting_sampled);GC_FIELD(lighting_bytes);GC_FIELD(lighting_owned);GC_FIELD(lighting_repeats);
    GC_FIELD(sampled);GC_FIELD(span_errors);GC_FIELD(spans);GC_FIELD(covered_spans);GC_FIELD(source_bytes);GC_FIELD(covered_bytes);
    GC_FIELD(all_owned);GC_FIELD(repeats);GC_FIELD(repeat_owned);GC_FIELD(repeat_bytes);GC_FIELD(overflow);GC_FIELD(owner_errors);
    GC_FIELD(used);GC_FIELD(index_bytes);GC_FIELD(allocation_failed);
#undef GC_FIELD
    fprintf(f,",\"span_examples\":[");
    for(unsigned i=0;i<nv_geometry_census.examples;i++)fprintf(f,"%s{\"offset\":%u,\"bytes\":%u,\"owner\":%u}",i?",":"",
        nv_geometry_census.example[i].lo,nv_geometry_census.example[i].bytes,nv_geometry_census.example[i].owner);
    fprintf(f,"]}\n");fclose(f);
    free(nv_geometry_census.keys);free(nv_geometry_census.indices);
    nv_geometry_census.keys=NULL;nv_geometry_census.indices=NULL;
}
static void nv_geometry_census_observe(const NvVertexFetchPlan*p,size_t ram_bytes,
    const void*indices,unsigned width,unsigned count,const NvRawStream*r,int planned){
    if(!nv_geometry_census.active)return;
    if(nv_geometry_census.owner!=GetCurrentThreadId()){++nv_geometry_census.owner_errors;return;}
    ++nv_geometry_census.calls;
    if(!r){++nv_geometry_census.raw_rejected;return;}
    int lighting=uc_guest_light_in_traversal();
    ++nv_geometry_census.raw;nv_geometry_census.planned_option+=!!planned;nv_geometry_census.bytes+=r->bytes;
    if(lighting){++nv_geometry_census.lighting_raw;nv_geometry_census.lighting_bytes+=r->bytes;}
    if((nv_geometry_census.raw-1)%NV_GC_STRIDE||nv_geometry_census.allocation_failed)return;
    ++nv_geometry_census.sampled;
    nv_geometry_census.lighting_sampled+=lighting;
    struct Span{uint64_t lo,hi;uint32_t offset;} spans[16],v;
    uint32_t low=UINT32_MAX,high=0;unsigned ns=0;
    for(unsigned i=0;i<count;i++){uint32_t x=nv_raw_index_read(indices,i,width);if(x<low)low=x;if(x>high)high=x;}
    for(unsigned i=0;i<p->count;i++){
        const NvVertexFetchEntry*e=&p->entry[i];
        spans[ns++]=(struct Span){(uint64_t)e->address+(uint64_t)low*e->stride,
            (uint64_t)e->address+(uint64_t)high*e->stride+nv2a_vertex_attribute_bytes(e->format),0};
    }
    for(unsigned i=1;i<ns;i++){v=spans[i];unsigned j=i;while(j&&spans[j-1].lo>v.lo){spans[j]=spans[j-1];--j;}spans[j]=v;}
    unsigned merged=0;uint64_t total=0,source=0;int owned=1;
    for(unsigned i=0;i<ns;i++){
        if(merged&&spans[i].lo<=spans[merged-1].hi){if(spans[i].hi>spans[merged-1].hi)spans[merged-1].hi=spans[i].hi;}
        else spans[merged++]=spans[i];
    }
    for(unsigned i=0;i<merged;i++){
        if(spans[i].hi>ram_bytes||spans[i].hi<=spans[i].lo){++nv_geometry_census.span_errors;return;}
        spans[i].offset=(uint32_t)total;total+=(spans[i].hi-spans[i].lo+3)&~UINT64_C(3);
    }
    if(total!=(r->indices&~NV_RAW_INDEX16)||merged!=r->spans||r->count!=count){++nv_geometry_census.span_errors;return;}
    for(unsigned i=0;i<p->count;i++){
        const NvVertexFetchEntry*e=&p->entry[i];uint64_t lo=(uint64_t)e->address+(uint64_t)low*e->stride;unsigned j=0;
        for(;j<merged;j++)if(lo>=spans[j].lo&&lo<spans[j].hi)break;
        const uint32_t*a=r->attribute[e->attribute];
        if(j==merged||a[0]!=spans[j].offset+lo-spans[j].lo||a[1]!=e->format||a[2]!=e->stride||a[3]!=low){++nv_geometry_census.span_errors;return;}
    }
    for(unsigned i=0;i<merged;i++){
        size_t bytes=(size_t)(spans[i].hi-spans[i].lo);unsigned id=uc_resource_find_existing((size_t)spans[i].lo,bytes);
        unsigned example=0;for(;example<nv_geometry_census.examples;example++)
            if(nv_geometry_census.example[example].lo==spans[i].lo&&nv_geometry_census.example[example].bytes==bytes)break;
        if(example==nv_geometry_census.examples&&example<64){
            nv_geometry_census.example[example].lo=(uint32_t)spans[i].lo;
            nv_geometry_census.example[example].bytes=(uint32_t)bytes;
            nv_geometry_census.example[example].owner=id;++nv_geometry_census.examples;
        }
        ++nv_geometry_census.spans;source+=bytes;
        if(id){++nv_geometry_census.covered_spans;nv_geometry_census.covered_bytes+=bytes;}else owned=0;
    }
    nv_geometry_census.source_bytes+=source;nv_geometry_census.all_owned+=owned;
    nv_geometry_census.lighting_owned+=owned&&lighting;
    NvGeometryKey key={0};key.ram=(uintptr_t)p->ram;key.ram_bytes=ram_bytes;key.count=count;key.width=width;key.entries=p->count;
    memcpy(key.entry,p->entry,p->count*sizeof(*p->entry));memcpy(key.defaults,p->defaults,sizeof(key.defaults));
    uint64_t h=nv_gc_hash(UINT64_C(14695981039346656037),&key,sizeof(key));
    h=nv_gc_hash(h,indices,(size_t)count*width);if(!h)h=1;
    for(unsigned probe=0;probe<NV_GC_SLOTS;probe++){
        NvGeometryKey*k=&nv_geometry_census.keys[(h+probe)&(NV_GC_SLOTS-1)];
        if(!k->hash){
            unsigned n=count*width;if(n>NV_GC_BYTES-nv_geometry_census.index_bytes)break;
            *k=key;k->hash=h;k->index_offset=nv_geometry_census.index_bytes;
            memcpy(nv_geometry_census.indices+k->index_offset,indices,n);nv_geometry_census.index_bytes+=n;++nv_geometry_census.used;return;
        }
        if(k->hash==h&&k->ram==key.ram&&k->ram_bytes==key.ram_bytes&&k->count==count&&k->width==width&&k->entries==p->count&&
            !memcmp(k->entry,key.entry,sizeof(key.entry))&&!memcmp(k->defaults,key.defaults,sizeof(key.defaults))&&
            !memcmp(nv_geometry_census.indices+k->index_offset,indices,(size_t)count*width)){
            ++nv_geometry_census.repeats;nv_geometry_census.repeat_owned+=owned;nv_geometry_census.repeat_bytes+=source;
            nv_geometry_census.lighting_repeats+=lighting;return;
        }
    }
    ++nv_geometry_census.overflow;
}
#endif
