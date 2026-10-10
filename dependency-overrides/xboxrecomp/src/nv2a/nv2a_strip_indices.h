#ifndef NV2A_STRIP_INDICES_H
#define NV2A_STRIP_INDICES_H
/* Raw, already-owned vertex records remain in draw order. Build only the small
 * strip-restart index stream on the GPU, then retain it by exact ordered lengths.
 * Immediate-context owner only; no guest pointers, CPU expansion, readback,
 * waits, topology changes, or draw submission are performed here. */
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define NV_STRIP_INDICES_MAX_DRAWS 8u
#define NV_STRIP_INDICES_MAX_VERTICES 16384u
#define NV_STRIP_INDICES_CACHE_SIZE 32u
#define NV_STRIP_INDICES_EXPANDED_SIZE 256u
#define NV_STRIP_INDICES_BUCKETS 512u
/* The fixture substitutes a constant to prove exact-key collision handling. */
#ifndef NV_STRIP_INDICES_KEY_HASH
#define NV_STRIP_INDICES_KEY_HASH(h) (h)
#endif

typedef struct NvStripIndexEntry {
    ID3D11Buffer *buffer;
    unsigned lengths[NV_STRIP_INDICES_MAX_DRAWS], count, index_count;
    uint64_t used;
    uint64_t hash;
    unsigned short hash_next,hash_prev,lru_next,lru_prev;
} NvStripIndexEntry;
typedef struct NvStripIndices {
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    ID3D11ComputeShader *shader;
    ID3D11Buffer *params;
    NvStripIndexEntry entries[NV_STRIP_INDICES_EXPANDED_SIZE];
    unsigned short buckets[NV_STRIP_INDICES_BUCKETS],lru_first,lru_last;
    unsigned policy_initialized,expanded,resident;
    uint64_t clock,hits,builds;
    unsigned failed;
} NvStripIndices;
typedef struct NvStripIndexParams {
    unsigned range[NV_STRIP_INDICES_MAX_DRAWS][4];
    unsigned info[4];
} NvStripIndexParams;

static uint64_t nv_strip_indices_hash(const unsigned *lengths,unsigned count) {
    uint64_t hash=UINT64_C(14695981039346656037)^count;
    for(unsigned n=0;n<count;++n) {
        hash^=lengths[n];hash*=UINT64_C(1099511628211);
    }
    /* Mix the high bits into the bucket selector, including structured lengths. */
    hash^=hash>>32;
    return NV_STRIP_INDICES_KEY_HASH(hash);
}

/* Links are entry+1, with zero denoting no link. Only the immediate-context
 * owner mutates them. A failed build never changes the directory or LRU. */
static void nv_strip_indices_lru_unlink(NvStripIndices *cache,unsigned slot) {
    NvStripIndexEntry *entry=&cache->entries[slot];
    if(entry->lru_prev) cache->entries[entry->lru_prev-1].lru_next=entry->lru_next;
    else cache->lru_first=entry->lru_next;
    if(entry->lru_next) cache->entries[entry->lru_next-1].lru_prev=entry->lru_prev;
    else cache->lru_last=entry->lru_prev;
}
static void nv_strip_indices_lru_append(NvStripIndices *cache,unsigned slot) {
    NvStripIndexEntry *entry=&cache->entries[slot];
    entry->lru_prev=cache->lru_last;entry->lru_next=0;
    if(cache->lru_last) cache->entries[cache->lru_last-1].lru_next=(unsigned short)(slot+1);
    else cache->lru_first=(unsigned short)(slot+1);
    cache->lru_last=(unsigned short)(slot+1);
}
static void nv_strip_indices_hash_unlink(NvStripIndices *cache,unsigned slot) {
    NvStripIndexEntry *entry=&cache->entries[slot];
    if(entry->hash_prev) cache->entries[entry->hash_prev-1].hash_next=entry->hash_next;
    else cache->buckets[entry->hash&(NV_STRIP_INDICES_BUCKETS-1)]=entry->hash_next;
    if(entry->hash_next) cache->entries[entry->hash_next-1].hash_prev=entry->hash_prev;
}
static void nv_strip_indices_hash_insert(NvStripIndices *cache,unsigned slot,uint64_t hash) {
    NvStripIndexEntry *entry=&cache->entries[slot];
    unsigned bucket=(unsigned)(hash&(NV_STRIP_INDICES_BUCKETS-1));
    entry->hash=hash;entry->hash_prev=0;entry->hash_next=cache->buckets[bucket];
    if(entry->hash_next) cache->entries[entry->hash_next-1].hash_prev=(unsigned short)(slot+1);
    cache->buckets[bucket]=(unsigned short)(slot+1);
    nv_strip_indices_lru_append(cache,slot);
}

static void nv_strip_indices_shutdown(NvStripIndices *cache) {
    if (!cache) return;
    for (unsigned n=0;n<NV_STRIP_INDICES_EXPANDED_SIZE;++n)
        if (cache->entries[n].buffer) ID3D11Buffer_Release(cache->entries[n].buffer);
    if (cache->params) ID3D11Buffer_Release(cache->params);
    if (cache->shader) ID3D11ComputeShader_Release(cache->shader);
    if (cache->context) ID3D11DeviceContext_Release(cache->context);
    if (cache->device) ID3D11Device_Release(cache->device);
    memset(cache,0,sizeof(*cache));
}

/* Optional startup prewarm. Failure is sticky until shutdown so an unsupported
 * device or compiler cannot trigger per-draw retries. Resource-allocation misses
 * below are not sticky and leave every already-cached entry intact. */
static int nv_strip_indices_prewarm(NvStripIndices *cache,ID3D11Device *device) {
    static const char source[]=
        "cbuffer R:register(b0){uint4 ranges[8];uint4 info;}\n"
        "RWBuffer<uint> indices:register(u0);\n"
        "[numthreads(64,1,1)]void main(uint3 tid:SV_DispatchThreadID){\n"
        "uint i=tid.x;if(i>=info.x)return;uint value=0xffffffff;\n"
        "[unroll]for(uint n=0;n<8;++n){uint4 r=ranges[n];\n"
        "if(i>=r.x&&i<r.y)value=r.z+i-r.x;}indices[i]=value;}\n";
    ID3DBlob *code=NULL,*errors=NULL;
    ID3D11ComputeShader *shader=NULL;
    ID3D11Buffer *params=NULL;
    D3D11_BUFFER_DESC desc={0};
    if (!cache||!device||cache->failed) return 0;
    if (!cache->policy_initialized) {
        const char *enabled=getenv("UCRR_STRIP_INDEX_CACHE");
        cache->expanded=enabled&&enabled[0]=='1'&&!enabled[1];
        cache->policy_initialized=1;
    }
    if (cache->device) return cache->device==device&&cache->shader&&cache->params;
    if (ID3D11Device_GetFeatureLevel(device)<D3D_FEATURE_LEVEL_11_0) goto fail;
    if (FAILED(D3DCompile(source,sizeof(source)-1,"raw-strip-indices",NULL,NULL,
            "main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&errors))) goto fail;
    if (FAILED(ID3D11Device_CreateComputeShader(device,ID3D10Blob_GetBufferPointer(code),
            ID3D10Blob_GetBufferSize(code),NULL,&shader))) goto fail;
    desc.ByteWidth=sizeof(NvStripIndexParams);desc.Usage=D3D11_USAGE_DEFAULT;
    desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(ID3D11Device_CreateBuffer(device,&desc,NULL,&params))) goto fail;
    if (errors) ID3D10Blob_Release(errors);
    ID3D10Blob_Release(code);
    cache->shader=shader;cache->params=params;cache->device=device;
    ID3D11Device_AddRef(device);
    return 1;
fail:
    if (code) ID3D10Blob_Release(code);
    if (errors) ID3D10Blob_Release(errors);
    if (shader) ID3D11ComputeShader_Release(shader);
    if (params) ID3D11Buffer_Release(params);
    cache->failed=1;
    return 0;
}

/* Borrowed result: consume/bind before the next call can evict it. D3D11 owns
 * references for queued commands. Cache hits do not touch GPU context state.
 * Output indices use DXGI_FORMAT_R32_UINT / TRIANGLESTRIP, where 0xffffffff
 * terminates a strip and resets winding; no connecting/degenerate triangles. */
static int nv_strip_indices_get(NvStripIndices *cache,ID3D11Device *device,
        ID3D11DeviceContext *context,const unsigned *lengths,unsigned count,
        ID3D11Buffer **buffer,unsigned *index_count) {
    unsigned total=0,slot=0;
    uint64_t oldest=UINT64_MAX,hash=0;
    ID3D11Buffer *made=NULL;
    ID3D11UnorderedAccessView *uav=NULL;
    D3D11_BUFFER_DESC desc={0};
    D3D11_UNORDERED_ACCESS_VIEW_DESC view={0};
    if (buffer) *buffer=NULL;
    if (index_count) *index_count=0;
    if (!cache||!device||!context||!lengths||!buffer||!index_count||count<2||
            count>NV_STRIP_INDICES_MAX_DRAWS||
            (cache->context&&cache->context!=context)||
            (!cache->context&&ID3D11DeviceContext_GetType(context)!=D3D11_DEVICE_CONTEXT_IMMEDIATE)) return 0;
    for (unsigned n=0;n<count;++n) {
        if (lengths[n]<3||lengths[n]>NV_STRIP_INDICES_MAX_VERTICES-total) return 0;
        total+=lengths[n];
    }
    if (!nv_strip_indices_prewarm(cache,device)) return 0;
    if (++cache->clock==0) {
        if(!cache->expanded)
            for (unsigned n=0;n<NV_STRIP_INDICES_CACHE_SIZE;++n) cache->entries[n].used=0;
        cache->clock=1;
    }
    if(cache->expanded) {
        hash=nv_strip_indices_hash(lengths,count);
        unsigned link=cache->buckets[hash&(NV_STRIP_INDICES_BUCKETS-1)];
        while(link) {
            NvStripIndexEntry *entry=&cache->entries[link-1];
            if(entry->hash==hash&&entry->count==count&&
                    !memcmp(entry->lengths,lengths,count*sizeof(*lengths))) {
                ++cache->hits;entry->used=cache->clock;
                if(cache->lru_last!=link) {
                    nv_strip_indices_lru_unlink(cache,link-1);
                    nv_strip_indices_lru_append(cache,link-1);
                }
                *buffer=entry->buffer;*index_count=entry->index_count;return 1;
            }
            link=entry->hash_next;
        }
        slot=cache->resident<NV_STRIP_INDICES_EXPANDED_SIZE?cache->resident:cache->lru_first-1;
    } else for (unsigned n=0;n<NV_STRIP_INDICES_CACHE_SIZE;++n) {
        NvStripIndexEntry *entry=&cache->entries[n];
        if (entry->buffer&&entry->count==count&&
                !memcmp(entry->lengths,lengths,count*sizeof(*lengths))) {
            ++cache->hits;entry->used=cache->clock;*buffer=entry->buffer;*index_count=entry->index_count;
            return 1;
        }
        if (!entry->buffer) {slot=n;oldest=0;}
        else if (entry->used<oldest) {slot=n;oldest=entry->used;}
    }
    /* Context/device mismatch must fail before any resource reaches the context.
     * GetDevice costs only a cache miss, not every repeated batch. */
    ID3D11Device *context_device=NULL;
    ID3D11DeviceContext_GetDevice(context,&context_device);
    int same_device=context_device==device;
    if (context_device) ID3D11Device_Release(context_device);
    if (!same_device) return 0;
    NvStripIndexParams params={0};
    unsigned source_offset=0;
    for (unsigned n=0;n<count;++n) {
        params.range[n][0]=source_offset+n;
        params.range[n][1]=source_offset+n+lengths[n];
        params.range[n][2]=source_offset;source_offset+=lengths[n];
    }
    params.info[0]=total+count-1;
    desc.ByteWidth=params.info[0]*sizeof(unsigned);desc.Usage=D3D11_USAGE_DEFAULT;
    desc.BindFlags=D3D11_BIND_INDEX_BUFFER|D3D11_BIND_UNORDERED_ACCESS;
    if (FAILED(ID3D11Device_CreateBuffer(device,&desc,NULL,&made))) return 0;
    view.Format=DXGI_FORMAT_R32_UINT;view.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;
    view.Buffer.NumElements=params.info[0];
    if (FAILED(ID3D11Device_CreateUnorderedAccessView(device,(ID3D11Resource*)made,&view,&uav))) {
        ID3D11Buffer_Release(made);return 0;
    }
    ID3D11ComputeShader *old_shader=NULL;
    ID3D11ClassInstance *instances[256]={0};UINT instance_count=256;
    ID3D11Buffer *old_params=NULL;
    ID3D11UnorderedAccessView *old_uav=NULL,*none=NULL;
    UINT keep=UINT_MAX;
    ID3D11DeviceContext_CSGetShader(context,&old_shader,instances,&instance_count);
    ID3D11DeviceContext_CSGetConstantBuffers(context,0,1,&old_params);
    ID3D11DeviceContext_CSGetUnorderedAccessViews(context,0,1,&old_uav);
    ID3D11DeviceContext_UpdateSubresource(context,(ID3D11Resource*)cache->params,0,NULL,&params,0,0);
    ID3D11DeviceContext_CSSetShader(context,cache->shader,NULL,0);
    ID3D11DeviceContext_CSSetConstantBuffers(context,0,1,&cache->params);
    ID3D11DeviceContext_CSSetUnorderedAccessViews(context,0,1,&uav,&keep);
    ID3D11DeviceContext_Dispatch(context,(params.info[0]+63)/64,1,1);
    ID3D11DeviceContext_CSSetUnorderedAccessViews(context,0,1,&none,&keep);
    ID3D11DeviceContext_CSSetConstantBuffers(context,0,1,&old_params);
    ID3D11DeviceContext_CSSetUnorderedAccessViews(context,0,1,&old_uav,&keep);
    ID3D11DeviceContext_CSSetShader(context,old_shader,instances,instance_count);
    if (old_shader) ID3D11ComputeShader_Release(old_shader);
    if (old_params) ID3D11Buffer_Release(old_params);
    if (old_uav) ID3D11UnorderedAccessView_Release(old_uav);
    for (UINT n=0;n<instance_count;++n) ID3D11ClassInstance_Release(instances[n]);
    ID3D11UnorderedAccessView_Release(uav);
    NvStripIndexEntry *entry=&cache->entries[slot];
    if(cache->expanded&&entry->buffer) {
        nv_strip_indices_hash_unlink(cache,slot);
        nv_strip_indices_lru_unlink(cache,slot);
    }
    if(cache->expanded&&!entry->buffer) ++cache->resident;
    if (entry->buffer) ID3D11Buffer_Release(entry->buffer);
    memset(entry,0,sizeof(*entry));entry->buffer=made;entry->count=count;
    entry->index_count=params.info[0];entry->used=cache->clock;
    memcpy(entry->lengths,lengths,count*sizeof(*lengths));
    if(cache->expanded) nv_strip_indices_hash_insert(cache,slot,hash);
    if (!cache->context) {cache->context=context;ID3D11DeviceContext_AddRef(context);}
    ++cache->builds;*buffer=made;*index_count=params.info[0];return 1;
}
#endif
