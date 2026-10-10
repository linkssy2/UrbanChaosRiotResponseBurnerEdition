#ifndef NV2A_GEOMETRY_CERTIFICATE_PROBE_H
#define NV2A_GEOMETRY_CERTIFICATE_PROBE_H
/* Diagnostic observer only: the caller always uses the original successful
 * raw packet. Include after NvRawStream/NvVertexFetchPlan. No GPU ownership,
 * guest writes, capture substitution or pre-capture hit claim. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resource_revisions.h"
#include "xbox_memory_layout.h"
#define NV_GCERT_WAYS 64u
#define NV_GCERT_PACKET_BYTES 32768u
#define NV_GCERT_STRIDE 64u
#define NV_GCERT_OBSERVATIONS 65536u
#define NV_GCERT_NEW_REGIONS 8u
#define NV_GCERT_ATTEMPTS 32u
#define NV_GCERT_REGION_BYTES 65536u
#define NV_GCERT_TOTAL_PAGES 262144u
#define NV_GCERT_FIELDS(X) \
 X(calls) X(selected) X(capped) X(observed) X(owner_errors) X(invalid) X(oversize) \
 X(first_keys) X(key_repeats) X(payload_repeats) X(replacements) X(all_owned) X(incomplete) \
 X(register_attempts) X(borrowed_spans) X(registration_denied) X(arm_failed) X(native_busy) \
 X(published) X(clean_repeats) X(dirty_unchanged) X(changed) X(false_clean) \
 X(lighting_observed) X(lighting_clean_repeats) X(meaningful_admitted_bytes) X(prefix_vertices)
typedef struct NvGCertSpan {uint32_t lo,bytes,offset;} NvGCertSpan;
typedef struct NvGCertKey {
    uint64_t ram,ram_bytes;
    uint32_t entries,width;
    NvVertexFetchEntry entry[16];
    NvRawStream raw;
} NvGCertKey;
typedef struct NvGCertEntry {
    uint64_t hash,age,epoch;
    NvGCertKey key;
    unsigned owners,owner[16];uint64_t token[16];
    uint8_t data[NV_GCERT_PACKET_BYTES];
} NvGCertEntry;
typedef struct NvGCertRegistration {
    uint32_t lo,bytes,id,status;uint64_t epoch;
} NvGCertRegistration;
static struct {
    unsigned active,finished,allocation_failed,invalid_run,used,new_regions,page_bytes,registrations;
    DWORD owner;uint64_t start,end,age;
#define NV_GCERT_MEMBER(n) uint64_t n;
    NV_GCERT_FIELDS(NV_GCERT_MEMBER)
#undef NV_GCERT_MEMBER
    uint64_t status[UC_RESOURCE_HANDLER_FAILED+1];
    NvGCertRegistration registration[NV_GCERT_ATTEMPTS];
    struct {uint32_t lo,bytes,id;} region[NV_GCERT_NEW_REGIONS];
    NvGCertEntry *entry;
} nv_gcert;

static uint64_t nv_gcert_hash(uint64_t h,const void *data,size_t bytes){
    const uint8_t *p=(const uint8_t*)data;
    for(size_t i=0;i<bytes;++i){h^=p[i];h*=UINT64_C(1099511628211);}return h;
}
/* Reconstruct only an already successful packet, using its owned indices.
 * All validation precedes any additional geometry RAM read or registration. */
static int nv_gcert_spans(const NvVertexFetchPlan *p,size_t ram_bytes,
    const NvRawStream *r,const uint8_t *data,size_t data_bytes,unsigned width,
    NvGCertSpan out[16],unsigned *nspans){
    struct Span {uint64_t lo,hi;uint32_t offset;} spans[16],v;
    if(!p||!p->valid||!p->ram||p->ram!=xbox_GetMemoryBase()||!p->count||p->count>16||
       !r||!data||!r->count||r->count>16384||(width!=2&&width!=4)||r->base||r->output_base||
       r->bytes>data_bytes||r->bytes>NV_GCERT_PACKET_BYTES||r->indices==UINT32_MAX||
       !!(r->indices&NV_RAW_INDEX16)!=(width==2))return 0;
    uint32_t at=r->indices&~NV_RAW_INDEX16;
    size_t index_bytes=(size_t)r->count*width;
    if(at>r->bytes||index_bytes>r->bytes-at||at+((index_bytes+3)&~(size_t)3)!=r->bytes)return 0;
    uint32_t low=UINT32_MAX,high=0,attributes=0;
    for(unsigned i=0;i<r->count;++i){uint32_t x=nv_raw_index_read(data+at,i,width);if(x<low)low=x;if(x>high)high=x;}
    for(unsigned i=0;i<p->count;++i){const NvVertexFetchEntry *e=&p->entry[i];
        unsigned bytes=nv2a_vertex_attribute_bytes(e->format);
        if(e->attribute>=16||!bytes||(attributes&(1u<<e->attribute))||high>e->max_index)return 0;
        attributes|=1u<<e->attribute;
        uint64_t lo=(uint64_t)e->address+(uint64_t)low*e->stride;
        uint64_t hi=(uint64_t)e->address+(uint64_t)high*e->stride+bytes;
        if(hi<=lo||hi>ram_bytes||hi>UINT32_MAX)return 0;
        spans[i]=(struct Span){lo,hi,0};
    }
    for(unsigned i=1;i<p->count;++i){v=spans[i];unsigned j=i;
        while(j&&spans[j-1].lo>v.lo){spans[j]=spans[j-1];--j;}spans[j]=v;}
    unsigned n=0;
    for(unsigned i=0;i<p->count;++i){
        if(n&&spans[i].lo<=spans[n-1].hi){if(spans[i].hi>spans[n-1].hi)spans[n-1].hi=spans[i].hi;}
        else spans[n++]=spans[i];
    }
    uint64_t total=0;
    for(unsigned i=0;i<n;++i){spans[i].offset=(uint32_t)total;total+=(spans[i].hi-spans[i].lo+3)&~UINT64_C(3);}
    if(n!=r->spans||total!=at||memcmp(r->defaults,p->defaults,sizeof(r->defaults)))return 0;
    for(unsigned a=0;a<16;++a)if(!(attributes&(1u<<a))){
        static const uint32_t zero[4];if(memcmp(r->attribute[a],zero,sizeof(zero)))return 0;}
    for(unsigned i=0;i<p->count;++i){const NvVertexFetchEntry *e=&p->entry[i];
        uint64_t lo=(uint64_t)e->address+(uint64_t)low*e->stride;unsigned j=0;
        for(;j<n;++j)if(lo>=spans[j].lo&&lo<spans[j].hi)break;
        const uint32_t *a=r->attribute[e->attribute];
        if(j==n||a[0]!=spans[j].offset+lo-spans[j].lo||a[1]!=e->format||a[2]!=e->stride||a[3]!=low)return 0;
    }
    for(unsigned i=0;i<n;++i){size_t bytes=(size_t)(spans[i].hi-spans[i].lo);
        for(size_t j=bytes;j<((bytes+3)&~(size_t)3);++j)if(data[spans[i].offset+j])return 0;
        out[i]=(NvGCertSpan){(uint32_t)spans[i].lo,(uint32_t)bytes,spans[i].offset};
    }
    for(size_t i=index_bytes;i<((index_bytes+3)&~(size_t)3);++i)if(data[at+i])return 0;
    *nspans=n;return 1;
}
static unsigned nv_gcert_owner(NvGCertSpan span){
    if(span.lo<0x1000000u){++nv_gcert.registration_denied;return 0;}
    unsigned id=uc_resource_find_existing(span.lo,span.bytes);
    if(id){++nv_gcert.borrowed_spans;return id;}
    uint64_t hi=((uint64_t)span.lo+span.bytes+4095)&~UINT64_C(4095);
    uint32_t lo=span.lo&~4095u;uint64_t bytes=hi-lo;
    if(lo<0x1000000u||bytes>NV_GCERT_REGION_BYTES||
       nv_gcert.new_regions>=NV_GCERT_NEW_REGIONS||bytes>NV_GCERT_TOTAL_PAGES-nv_gcert.page_bytes){
        ++nv_gcert.registration_denied;return 0;}
    uint64_t epoch=uc_resource_native_epoch();if(!epoch){++nv_gcert.native_busy;return 0;}
    unsigned slot=0;
    for(;slot<nv_gcert.registrations;++slot){NvGCertRegistration *r=&nv_gcert.registration[slot];
        if(r->lo==lo&&r->bytes==bytes){
            if(r->status!=UC_RESOURCE_NATIVE_BUSY||r->epoch==epoch)return 0;
            break;
        }
    }
    if(nv_gcert.register_attempts>=NV_GCERT_ATTEMPTS){++nv_gcert.registration_denied;return 0;}
    if(slot==nv_gcert.registrations)++nv_gcert.registrations;
    ++nv_gcert.register_attempts;
    UcResourceRegistration got=uc_resource_register_result(lo,(size_t)bytes);
    nv_gcert.registration[slot]=(NvGCertRegistration){lo,(uint32_t)bytes,got.id,got.status,got.native_epoch};
    if(got.status<=UC_RESOURCE_HANDLER_FAILED)++nv_gcert.status[got.status];
    else {nv_gcert.invalid_run=1;return 0;}
    if(!got.id)return 0;
    /* Same renderer owner, no callback between lookup and registration. A
     * tagged lighting borrower is existing ownership, never a new region. */
    if(got.id&0x80000000u){++nv_gcert.borrowed_spans;return got.id;}
    for(unsigned i=0;i<nv_gcert.new_regions;++i)if(nv_gcert.region[i].id==got.id)return got.id;
    unsigned n=nv_gcert.new_regions++;
    nv_gcert.region[n].lo=lo;nv_gcert.region[n].bytes=(uint32_t)bytes;nv_gcert.region[n].id=got.id;
    nv_gcert.page_bytes+=(unsigned)bytes;nv_gcert.meaningful_admitted_bytes+=span.bytes;
    return got.id;
}
static int nv_gcert_clean(const NvGCertEntry *e){
    if(!e->epoch||!e->owners||e->epoch!=uc_resource_native_epoch())return 0;
    for(unsigned i=0;i<e->owners;++i)if(!uc_resource_clean(e->owner[i],e->token[i]))return 0;
    return e->epoch==uc_resource_native_epoch();
}
static void nv_gcert_publish(NvGCertEntry *e,const NvVertexFetchPlan *p,
    const uint8_t *data,const NvGCertSpan spans[16],unsigned n){
    e->epoch=0;e->owners=0;
    uint64_t epoch=uc_resource_native_epoch();if(!epoch){++nv_gcert.native_busy;return;}
    for(unsigned i=0;i<n;++i){unsigned id=nv_gcert_owner(spans[i]);
        if(!id){++nv_gcert.incomplete;return;}
        unsigned j=0;for(;j<e->owners;++j)if(e->owner[j]==id)break;
        if(j==e->owners)e->owner[e->owners++]=id;
    }
    ++nv_gcert.all_owned;
    for(unsigned i=0;i<e->owners;++i){e->token[i]=uc_resource_arm(e->owner[i]);
        if(!e->token[i]){++nv_gcert.arm_failed;return;}}
#ifdef NV_GCERT_TEST_POINT
    NV_GCERT_TEST_POINT(1);
#endif
    memcpy(e->data,data,e->key.raw.bytes);
    for(unsigned i=0;i<n;++i)if(memcmp(e->data+spans[i].offset,p->ram+spans[i].lo,spans[i].bytes))return;
#ifdef NV_GCERT_TEST_POINT
    NV_GCERT_TEST_POINT(2);
#endif
    if(epoch!=uc_resource_native_epoch())return;
    e->epoch=epoch;if(!nv_gcert_clean(e)){e->epoch=0;return;}++nv_gcert.published;
}
/* One existing post-success hook. Snapshot all packet-local bytes before the
 * arena moves; do not pass seed/compact packets or borrowed guest indices. */
static void nv_geometry_certificate_observe(const NvVertexFetchPlan *p,size_t ram_bytes,
    const NvRawStream *r,const uint8_t *data,size_t data_bytes,unsigned width,
    unsigned prefix_vertices,int inside_lighting){
    if(!nv_gcert.active||nv_gcert.invalid_run)return;
    DWORD saved=GetLastError();
    if(nv_gcert.owner!=GetCurrentThreadId()){++nv_gcert.owner_errors;goto done;}
    if((nv_gcert.calls++%NV_GCERT_STRIDE)!=0)goto done;
    ++nv_gcert.selected;
    if(nv_gcert.selected>NV_GCERT_OBSERVATIONS){++nv_gcert.capped;goto done;}
    if(!r||r->bytes>NV_GCERT_PACKET_BYTES){++nv_gcert.oversize;goto done;}
    NvGCertSpan spans[16];unsigned n=0;
    if(!nv_gcert_spans(p,ram_bytes,r,data,data_bytes,width,spans,&n)){++nv_gcert.invalid;goto done;}
    ++nv_gcert.observed;nv_gcert.lighting_observed+=!!inside_lighting;nv_gcert.prefix_vertices+=prefix_vertices;
    NvGCertKey key={0};key.ram=(uintptr_t)p->ram;key.ram_bytes=ram_bytes;key.entries=p->count;key.width=width;
    memcpy(key.entry,p->entry,p->count*sizeof(*p->entry));key.raw=*r;
    uint32_t at=r->indices&~NV_RAW_INDEX16;size_t index_bytes=(size_t)r->count*width;
    uint64_t hash=nv_gcert_hash(UINT64_C(14695981039346656037),&key,sizeof(key));
    hash=nv_gcert_hash(hash,data+at,index_bytes);if(!hash)hash=1;
    unsigned found=NV_GCERT_WAYS,victim=0;uint64_t oldest=UINT64_MAX;
    for(unsigned i=0;i<NV_GCERT_WAYS;++i){NvGCertEntry *e=&nv_gcert.entry[i];
        if(e->hash==hash&&!memcmp(&e->key,&key,sizeof(key))&&!memcmp(e->data+at,data+at,index_bytes)){found=i;break;}
        if(e->age<oldest){oldest=e->age;victim=i;}
    }
    if(found==NV_GCERT_WAYS){NvGCertEntry *e=&nv_gcert.entry[victim];
        if(e->hash)++nv_gcert.replacements;else ++nv_gcert.used;
        ++nv_gcert.first_keys;memset(e,0,sizeof(*e));e->hash=hash;e->key=key;e->age=++nv_gcert.age;
        memcpy(e->data,data,r->bytes);goto done;
    }
    NvGCertEntry *e=&nv_gcert.entry[found];e->age=++nv_gcert.age;++nv_gcert.key_repeats;
    int clean=nv_gcert_clean(e),equal=!memcmp(e->data,data,r->bytes);
    /* Bracket equality: a write during the diagnostic comparison cannot be
     * misreported as a false-clean contract violation. */
    clean=clean&&nv_gcert_clean(e);
    if(clean&&!equal){++nv_gcert.false_clean;nv_gcert.invalid_run=1;goto done;}
    if(equal){++nv_gcert.payload_repeats;
        if(clean){++nv_gcert.clean_repeats;nv_gcert.lighting_clean_repeats+=!!inside_lighting;goto done;}
        if(e->epoch)++nv_gcert.dirty_unchanged;
        nv_gcert_publish(e,p,data,spans,n);
    }else {++nv_gcert.changed;e->epoch=0;memcpy(e->data,data,r->bytes);}
done:SetLastError(saved);
}
void uc_geometry_certificate_boundary(int start,uint64_t qpc){
    const char *s=getenv("UCRR_GEOMETRY_CERTIFICATE_PROBE");
    if(!s||strcmp(s,"1")||nv_gcert.finished)return;
    DWORD saved=GetLastError();
    if(start){
        if(nv_gcert.active)goto done;
        nv_gcert.owner=GetCurrentThreadId();nv_gcert.start=qpc;
        nv_gcert.entry=(NvGCertEntry*)calloc(NV_GCERT_WAYS,sizeof(*nv_gcert.entry));
        nv_gcert.allocation_failed=!nv_gcert.entry;nv_gcert.active=!!nv_gcert.entry;goto done;
    }
    if(nv_gcert.owner!=GetCurrentThreadId()){++nv_gcert.owner_errors;goto done;}
    nv_gcert.active=0;nv_gcert.finished=1;nv_gcert.end=qpc;
    char path[128];snprintf(path,sizeof(path),"geometry-certificate-%lu-%llu.json",GetCurrentProcessId(),(unsigned long long)nv_gcert.start);
    FILE *f=fopen(path,"wx");
    if(f){
        fprintf(f,"{\"schema\":1,\"scope\":\"Intrusive post-capture certificate observations; every original capture/draw retained; not FPS or pre-capture reuse\",\"pid\":%lu,\"owner_tid\":%lu,\"start_qpc\":%llu,\"end_qpc\":%llu,\"sample_stride\":%u,\"observation_cap\":%u,\"ways\":%u,\"packet_byte_cap\":%u,\"new_region_cap\":%u,\"attempt_cap\":%u,\"page_byte_cap\":%u,\"used\":%u,\"new_regions\":%u,\"new_page_bytes\":%u,\"allocation_failed\":%u,\"invalid_run\":%u",
            GetCurrentProcessId(),nv_gcert.owner,(unsigned long long)nv_gcert.start,(unsigned long long)nv_gcert.end,
            NV_GCERT_STRIDE,NV_GCERT_OBSERVATIONS,NV_GCERT_WAYS,NV_GCERT_PACKET_BYTES,NV_GCERT_NEW_REGIONS,NV_GCERT_ATTEMPTS,NV_GCERT_TOTAL_PAGES,
            nv_gcert.used,nv_gcert.new_regions,nv_gcert.page_bytes,nv_gcert.allocation_failed,nv_gcert.invalid_run);
#define NV_GCERT_JSON(n) fprintf(f,",\"" #n "\":%llu",(unsigned long long)nv_gcert.n);
        NV_GCERT_FIELDS(NV_GCERT_JSON)
#undef NV_GCERT_JSON
        fprintf(f,",\"registration_status\":[");
        for(unsigned i=0;i<=UC_RESOURCE_HANDLER_FAILED;++i)fprintf(f,"%s%llu",i?",":"",(unsigned long long)nv_gcert.status[i]);
        fprintf(f,"],\"new_region_ranges\":[");
        for(unsigned i=0;i<nv_gcert.new_regions;++i)fprintf(f,"%s{\"offset\":%u,\"bytes\":%u,\"owner\":%u}",i?",":"",nv_gcert.region[i].lo,nv_gcert.region[i].bytes,nv_gcert.region[i].id);
        fprintf(f,"]}\n");fclose(f);
    }
    free(nv_gcert.entry);nv_gcert.entry=NULL;
done:SetLastError(saved);
}
#endif
