#ifndef NV_VERTEX_GPU_RUNTIME_H
#define NV_VERTEX_GPU_RUNTIME_H
#include "nv2a_fixed_equal.h"
#include <stdint.h>
#include <d3dcompiler.h>
#include "nv2a_vertex_gpu_source.h"
#include "nv2a_vertex_specialize.h"
#include "nv2a_vertex_snapshot.h"
#include "nv2a_shader_disk_cache.h"
#include "nv2a_constant_arena.h"
#include "nv2a_vertex_compile_worker.h"
#include "nv2a_compiler_ready_poll.h"
#include "nv2a_shader_compiler_identity.h"
#include "nv2a_geometry_resident.h"
#include "nv2a_host_upload_counters.h"
#include "nv2a_raw_upload_layout.h"
/* Production supplies a capture-window ledger; standalone users stay inert. */
#ifndef NV_HOST_UPLOAD_ACTIVE
#define NV_HOST_UPLOAD_ACTIVE() 0
#endif
#ifndef NV_HOST_UPLOAD_ADD
#define NV_HOST_UPLOAD_ADD(kind,count) ((void)0)
#endif
#ifndef NV_HOST_UPLOAD_ENTER
#define NV_HOST_UPLOAD_ENTER() ((void)0)
#endif
#define NV_RAW_UPLOAD_ADD(g,kind,count) do { if((g)->packed_enabled==2) NV_HOST_UPLOAD_ADD(NV_HOST_UPLOAD_##kind,(count)); } while(0)

/* MSVC may resolve &D3DCompile to the executable's import thunk. Identify the
 * actual compiler DLL export instead; EXE timestamps invalidate caches on every
 * unrelated rebuild. If the expected compiler is unavailable, disable reuse. */
static HMODULE nv_vertex_compiler_contract(uint32_t contract[6]){
    return nv_shader_compiler_identity(contract);
}

typedef struct NvSpecializedEntry {uint32_t p[136][4],layout[16][4];unsigned start,end,layout_valid,outputs;ID3D11VertexShader *shader;} NvSpecializedEntry;
typedef struct NvVertexProgramData {float c[192][4];uint32_t p[136][4],params[4];} NvVertexProgramData;
typedef struct NvResidentProgram {NvVertexProgramData data;ID3D11Buffer *buffer;unsigned valid;} NvResidentProgram;
typedef struct NvVertexGPU {
    NvGeometryResident geometry;
    NvGeometryResident index_geometry;
    NvVertexCompiler compiler;
    unsigned async_compile;
    unsigned fast_ready_poll;
    NvConstantArena arena;
    ID3D11VertexShader*shader;ID3D11Buffer*program,*conversion,*vertices;ID3D11ShaderResourceView*view;
    NvVertexProgramData uploaded_program;
    NvResidentProgram resident_program[8];
    unsigned resident_enabled;
    ID3D11Buffer *resident_selected;
    float uploaded_conversion[6][4];
    unsigned program_valid,conversion_valid;
    unsigned constant_revision_enabled,constant_revision_shadow;
    uint64_t supplied_constant_revision,uploaded_constant_revision;
    unsigned selective_program_copy,shader_lookup_hint;
    unsigned specialize_layout,last_layout_valid;
    unsigned material_outputs_enabled,material_outputs,last_material_outputs;
    uint32_t last_layout[16][4];
    unsigned compact_constants,narrow_indices,last_compact;
    ID3D11Buffer *compact_program;
    unsigned shader_hint[64]; /* checked slot+1; never a shader identity */
    unsigned stream_enabled,vertex_cursor,vertex_first,uploaded_vertices;
    unsigned indexed_enabled,index_cursor,index_first;
    ID3D11Buffer *indices;
    unsigned packed_enabled;
    unsigned raw_packet_count,raw_packet_bytes; /* mode2: packet-owned GPU pull */
    unsigned raw_compact_directory;
    unsigned raw_packet_vertex_plan;
    unsigned dynamic_packed_metadata;
    ID3D11Buffer *packed_metadata;
    unsigned split_metadata,metadata_valid;
    uint32_t uploaded_metadata[32][4];
    ID3D11Buffer *packed_params;
    uint64_t stream_discards,stream_appends;
    uint64_t program_uploads,conversion_uploads,vertex_uploads;
    NvSpecializedEntry specialized[64];
    unsigned specialized_count,specialized_enabled;
    ID3D11VertexShader *active_shader; /* Borrowed from interpreter/cache. */
    ID3D11VertexShader *full_shader; /* Matching constant contract on PS fallback. */
} NvVertexGPU;
static void nv_vertex_gpu_shutdown(NvVertexGPU*g){
    nv_geometry_shutdown(&g->geometry);
    nv_geometry_shutdown(&g->index_geometry);
    nv_vertex_compile_stop(&g->compiler);
    nv_constant_arena_shutdown(&g->arena);
    for(unsigned i=0;i<8;++i)if(g->resident_program[i].buffer)ID3D11Buffer_Release(g->resident_program[i].buffer);
    if(g->packed_params)ID3D11Buffer_Release(g->packed_params);
    if(g->compact_program)ID3D11Buffer_Release(g->compact_program);
    if(g->packed_metadata)ID3D11Buffer_Release(g->packed_metadata);
    if(g->indices)ID3D11Buffer_Release(g->indices);
    for(unsigned i=0;i<g->specialized_count;++i)if(g->specialized[i].shader)ID3D11VertexShader_Release(g->specialized[i].shader);
    if(g->view)ID3D11ShaderResourceView_Release(g->view);if(g->vertices)ID3D11Buffer_Release(g->vertices);
    if(g->program)ID3D11Buffer_Release(g->program);if(g->conversion)ID3D11Buffer_Release(g->conversion);
    if(g->shader)ID3D11VertexShader_Release(g->shader);memset(g,0,sizeof(*g));
}
static int nv_vertex_gpu_init_mode(NvVertexGPU*g,ID3D11Device*dev,unsigned packed){
    if(g->view)return 1;
    g->packed_enabled=packed;
    {const char*s=getenv("UCRR_RAW_COMPACT_DIRECTORY");g->raw_compact_directory=packed==2&&s&&!strcmp(s,"1");}
    {const char*s=getenv("UCRR_RAW_PACKET_VERTEX_PLAN");g->raw_packet_vertex_plan=packed==2&&s&&!strcmp(s,"1");}
    {const char*s=getenv("UCRR_COMPILER_READY_POLL");g->fast_ready_poll=s&&!strcmp(s,"1");}
    {const char*s=getenv("UCRR_ASYNC_VERTEX_COMPILE");g->async_compile=s&&!strcmp(s,"1");}
    {const char*s=getenv("UCRR_RESIDENT_VERTEX_PROGRAM");g->resident_enabled=s&&!strcmp(s,"1");}
    {const char*s=getenv("UCRR_SPECIALIZE_PACKED_LAYOUT");g->specialize_layout=packed&&s&&!strcmp(s,"1");}
    {const char*s=getenv("UCRR_CONSTANT_REVISION");g->constant_revision_enabled=s&&!strcmp(s,"1");
     s=getenv("UCRR_CONSTANT_REVISION_SHADOW");g->constant_revision_shadow=s&&!strcmp(s,"1");
     s=getenv("UCRR_SPLIT_PACKED_METADATA");g->split_metadata=packed&&s&&!strcmp(s,"1");}
    {const char*s=getenv("UCRR_SELECTIVE_PROGRAM_COPY");g->selective_program_copy=s&&!strcmp(s,"1");
     s=getenv("UCRR_VERTEX_SHADER_HINT");g->shader_lookup_hint=s&&!strcmp(s,"1");}
    {const char*s=getenv("UCRR_COMPACT_CONSTANTS");g->compact_constants=s&&!strcmp(s,"1");
     s=getenv("UCRR_NARROW_PACKED_INDICES");g->narrow_indices=packed&&s&&!strcmp(s,"1");}
    {const char*s=getenv("UCRR_PRODUCER_COMPACT_INDICES");if(packed&&s&&!strcmp(s,"1"))g->narrow_indices=1;}
    /* Dynamic SRV no-overwrite is optional, not implied by feature level 11.
     * Typed uint4 loads preserve attribute bits without STRUCTURED misc flags,
     * which cannot be combined with CPU_ACCESS_WRITE. Default path unchanged. */
    const char *stream=getenv("UCRR_VERTEX_STREAM");
    D3D11_FEATURE_DATA_D3D11_OPTIONS options={0};
    g->stream_enabled=stream&&strcmp(stream,"1")==0&&
        SUCCEEDED(ID3D11Device_CheckFeatureSupport(dev,D3D11_FEATURE_D3D11_OPTIONS,&options,sizeof(options)))&&options.MapNoOverwriteOnDynamicBufferSRV;
    if(packed&&!g->stream_enabled)return 0;
    {const char*s=getenv("UCRR_GEOMETRY_RESIDENT");g->geometry.enabled=g->stream_enabled&&s&&!strcmp(s,"1");g->geometry.capacity=16384*256;}
    {const char*s=getenv("UCRR_RAW_RESIDENT_BYTE_BUDGET");nv_geometry_byte_budget(&g->geometry,packed==2&&s&&!strcmp(s,"1"));}
    ID3DBlob*code=NULL,*errors=NULL;D3D_SHADER_MACRO macros[]={{"DRAW_VERTEX","1"},{"PACKED_PARTICLE_STREAM",packed==3?"1":"0"},{"RAW_SOURCE_STREAM",packed==2?"1":"0"},{"RAW_PACKET_STREAM",packed==2?"1":"0"},{"RAW_PACKET_VERTEX_PLAN",g->raw_packet_vertex_plan?"1":"0"},{"SPLIT_PACKED_METADATA",g->split_metadata?"1":"0"},{g->stream_enabled?"TYPED_VERTEX_STREAM":NULL,"1"},{packed?"PACKED_VERTEX_STREAM":NULL,"1"},{NULL,NULL}};
    /* Generic fallback used to compile synchronously at the first level draw
     * every run. Match exact source, macros, flags, entry/profile and compiler
     * identity, just like specialized programs. Invalid bytecode must recompile. */
    ULONGLONG began=GetTickCount64();int cache_hit=0;
    uint32_t contract[6]={D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS,
        g->stream_enabled,packed,0,0,0x05000001+(g->split_metadata?16:0)+(g->raw_packet_vertex_plan?32:0)+(g->packed_enabled==3?64:0)};
    HMODULE compiler=nv_vertex_compiler_contract(contract);
    const char *cache_dir=compiler?getenv("UCRR_SHADER_CACHE_DIR"):NULL;
    size_t cached_size=0;void *cached=nv_shader_disk_load(cache_dir,nv_vertex_shader_source,contract,&cached_size);
    HRESULT hr=E_FAIL;
    if(cached){hr=ID3D11Device_CreateVertexShader(dev,cached,cached_size,NULL,&g->shader);free(cached);cache_hit=SUCCEEDED(hr);}
    if(!cache_hit){
        hr=D3DCompile(nv_vertex_shader_source,strlen(nv_vertex_shader_source),"NV2A vertex candidate",macros,NULL,"draw_main","vs_5_0",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS,0,&code,&errors);
        if(errors){fprintf(stderr,"[GPU-VERTEX-COMPILE] %s\n",(char*)ID3D10Blob_GetBufferPointer(errors));ID3D10Blob_Release(errors);}
        if(SUCCEEDED(hr)){
            hr=ID3D11Device_CreateVertexShader(dev,ID3D10Blob_GetBufferPointer(code),ID3D10Blob_GetBufferSize(code),NULL,&g->shader);
            if(SUCCEEDED(hr))nv_shader_disk_save(cache_dir,nv_vertex_shader_source,contract,ID3D10Blob_GetBufferPointer(code),ID3D10Blob_GetBufferSize(code));
        }
        if(code)ID3D10Blob_Release(code);
    }
    fprintf(stderr,"[GPU-VERTEX-INIT] packed=%u stream=%u cache_hit=%d shader_ms=%llu success=%d\n",packed,g->stream_enabled,cache_hit,(unsigned long long)(GetTickCount64()-began),SUCCEEDED(hr));
    if(FAILED(hr))goto fail;
    g->active_shader=g->full_shader=g->shader;
    {const char *option=getenv("UCRR_SPECIALIZED_VERTICES");g->specialized_enabled=option&&strcmp(option,"1")==0;}
    {const char *option=getenv("UCRR_MATERIAL_OUTPUTS");g->material_outputs_enabled=option&&!strcmp(option,"1");g->material_outputs=g->last_material_outputs=127;}
    D3D11_BUFFER_DESC d={0};d.ByteWidth=(192+136+1)*16;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    if(FAILED(ID3D11Device_CreateBuffer(dev,&d,NULL,&g->program)))goto fail;
    if(g->compact_constants){d.ByteWidth=192*16;if(FAILED(ID3D11Device_CreateBuffer(dev,&d,NULL,&g->compact_program)))goto fail;}
    d.ByteWidth=6*16;if(FAILED(ID3D11Device_CreateBuffer(dev,&d,NULL,&g->conversion)))goto fail;
    if(packed){
        const char *dynamic=getenv("UCRR_DYNAMIC_PACKED_METADATA");
        g->dynamic_packed_metadata=!g->split_metadata&&dynamic&&!strcmp(dynamic,"1");
        d.ByteWidth=(g->split_metadata?32:33)*16;
        if(g->dynamic_packed_metadata){d.Usage=D3D11_USAGE_DYNAMIC;d.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;}
        if(FAILED(ID3D11Device_CreateBuffer(dev,&d,NULL,&g->packed_metadata)))goto fail;
        d.Usage=D3D11_USAGE_DEFAULT;d.CPUAccessFlags=0;
        if(g->split_metadata){d.ByteWidth=16;if(FAILED(ID3D11Device_CreateBuffer(dev,&d,NULL,&g->packed_params)))goto fail;}
    }
    d.ByteWidth=16384*256;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;d.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;d.StructureByteStride=16;
    if(g->stream_enabled){d.Usage=D3D11_USAGE_DYNAMIC;d.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;d.MiscFlags=0;d.StructureByteStride=0;}
    if(FAILED(ID3D11Device_CreateBuffer(dev,&d,NULL,&g->vertices)))goto fail;
    D3D11_SHADER_RESOURCE_VIEW_DESC sd={0};sd.Format=DXGI_FORMAT_R32G32B32A32_UINT;sd.ViewDimension=D3D11_SRV_DIMENSION_BUFFER;sd.Buffer.NumElements=16384*16;
    if(packed){sd.Format=DXGI_FORMAT_R32_UINT;sd.Buffer.NumElements=16384*64;}
    if(FAILED(ID3D11Device_CreateShaderResourceView(dev,(ID3D11Resource*)g->vertices,g->stream_enabled?&sd:NULL,&g->view)))goto fail;
    {const char *indexed=getenv(packed?"UCRR_PACKED_REUSE":"UCRR_INDEXED_GPU");g->indexed_enabled=g->stream_enabled&&indexed&&strcmp(indexed,"1")==0;}
    if(packed==2){g->indexed_enabled=0;g->specialize_layout=0;}
    if(g->indexed_enabled){D3D11_BUFFER_DESC ib={0};ib.ByteWidth=16384*(g->narrow_indices?2:4);ib.Usage=D3D11_USAGE_DYNAMIC;ib.BindFlags=D3D11_BIND_INDEX_BUFFER;ib.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        g->index_geometry.enabled=g->geometry.enabled&&g->narrow_indices;
        g->index_geometry.capacity=16384*2;
        if(FAILED(ID3D11Device_CreateBuffer(dev,&ib,NULL,&g->indices)))goto fail;
        fprintf(stderr,"[INDEXED-GPU] active=1 capacity_indices=16384\n");
    }
    if(stream)fprintf(stderr,"[VERTEX-STREAM] requested=%s active=%u capacity_vertices=16384\n",stream,g->stream_enabled);
    {const char*s=getenv("UCRR_CONSTANT_ARENA");if(packed&&s&&!strcmp(s,"1")&&!g->compact_constants&&!g->split_metadata&&!g->resident_enabled){
        int active=nv_constant_arena_init(&g->arena,dev);fprintf(stderr,"[CONSTANT-ARENA] active=%d\n",active);}}
    return 1;
fail:nv_vertex_gpu_shutdown(g);return 0;
}
static int nv_vertex_gpu_init(NvVertexGPU*g,ID3D11Device*dev){return nv_vertex_gpu_init_mode(g,dev,0);}
/* Relative-address programs use the same bounds-checked evaluator validated
 * by the hardware fixture, but have a separate live-rollout option. */
static int nv_vertex_gpu_supported(const uint32_t p[136][4],unsigned start){
    static int relative=-1;if(relative<0)relative=getenv("UCRR_GPU_VERTEX_RELATIVE")!=NULL;
    for(unsigned pc=start;pc<136;++pc){const uint32_t*w=p[pc];unsigned mac=(w[1]>>21)&15,ilu=(w[1]>>25)&7;
        if(mac>13||ilu>4||((w[1]>>13)&255)>=192)return 0;
        if(!relative&&(mac==13||(w[3]&2)))return 0;
        if(((w[3]>>12)&15)&&!(w[3]&0x800))return 0;
        if(w[3]&1)return 1;
    }return 0;
}
typedef struct NvProgramAdmission {unsigned valid,start;int supported;} NvProgramAdmission;
static void nv_program_admission_write(NvProgramAdmission*a,uint32_t*word,uint32_t value){*word=value;a->valid=0;}
static int nv_program_admission(NvProgramAdmission*a,const uint32_t p[136][4],unsigned start){
    if(!a->valid||a->start!=start){a->supported=nv_vertex_gpu_supported(p,start);a->start=start;a->valid=1;}
    return a->supported;
}
static ID3D11Buffer *nv_vertex_gpu_program(NvVertexGPU*g){return g->last_compact?g->compact_program:(g->resident_selected?g->resident_selected:g->program);}
static int nv_vertex_gpu_resident_upload(NvVertexGPU*g,ID3D11DeviceContext*ctx){
    /* Sampled bits choose a slot only; full initialized payload verifies identity. */
    uint32_t words[4];memcpy(words,g->uploaded_program.c[0],16);
    uint32_t hash=words[0]^(words[1]*33u)^(words[2]*65599u)^words[3]^g->uploaded_program.params[0];
    unsigned bucket=(hash^(hash>>8)^(hash>>16)^(hash>>24))&7u;
    NvResidentProgram *entry=&g->resident_program[bucket];
    if(entry->valid&&!memcmp(&entry->data,&g->uploaded_program,sizeof(entry->data))){g->resident_selected=entry->buffer;return 1;}
    if(!entry->buffer){
        ID3D11Device *dev=NULL;ID3D11DeviceContext_GetDevice(ctx,&dev);
        D3D11_BUFFER_DESC d={0};d.ByteWidth=sizeof(entry->data);d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        HRESULT hr=ID3D11Device_CreateBuffer(dev,&d,NULL,&entry->buffer);ID3D11Device_Release(dev);
        if(FAILED(hr)){g->resident_selected=NULL;return 0;}
    }
    ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)entry->buffer,0,NULL,&g->uploaded_program,0,0);
    NV_RAW_UPLOAD_ADD(g,PROGRAM_UPDATES,1);NV_RAW_UPLOAD_ADD(g,PROGRAM_UPDATE_BYTES,sizeof(entry->data));
    entry->data=g->uploaded_program;entry->valid=1;g->resident_selected=entry->buffer;++g->program_uploads;return 1;
}
static void nv_vertex_gpu_bind_metadata(NvVertexGPU*g,ID3D11DeviceContext*ctx){
    if(g->packed_enabled==2)NV_HOST_UPLOAD_ENTER();
    ID3D11Buffer *buffers[2]={g->packed_metadata,g->packed_params};
    ID3D11DeviceContext_VSSetConstantBuffers(ctx,2,g->split_metadata?2:1,buffers);
    NV_RAW_UPLOAD_ADD(g,METADATA_BIND_CALLS,1);NV_RAW_UPLOAD_ADD(g,METADATA_BIND_BUFFERS,g->split_metadata?2:1);
}
static DXGI_FORMAT nv_vertex_gpu_index_format(NvVertexGPU*g){return g->narrow_indices?DXGI_FORMAT_R16_UINT:DXGI_FORMAT_R32_UINT;}
static ID3D11VertexShader *nv_vertex_gpu_select_layout(NvVertexGPU*g,ID3D11DeviceContext*ctx,const uint32_t p[136][4],unsigned start,const NvVertexSnapshotLayout *layout){
    if(!g->specialized_enabled||!nv_vertex_gpu_supported(p,start))return g->shader;
    if(!g->specialize_layout)layout=NULL;
    unsigned outputs=g->material_outputs_enabled?(g->material_outputs&127):127;
    /* Programs are straight-line and supported() requires an authored final
     * instruction. Source generation consumes exactly [start,end), not the
     * unrelated instruction RAM before start or after that final instruction.
     * Retain every bit within the consumed range, including the final bit. */
    unsigned end=start+1;
    while(end<136&&!(p[end-1][3]&1))++end;
    size_t key_bytes=(end-start)*sizeof(p[0]);
    unsigned bucket=0;
    if(g->shader_lookup_hint){
        bucket=(p[start][0]^p[start][1]^p[end-1][2]^(start*17u)^(end*31u))&63u;
        unsigned slot=g->shader_hint[bucket];
        if(slot&&slot<=g->specialized_count){NvSpecializedEntry*e=&g->specialized[slot-1];
            if(e->outputs==outputs&&e->layout_valid==!!layout&&(!layout||!memcmp(e->layout,layout->attribute,sizeof(e->layout)))&&e->start==start&&e->end==end&&!memcmp(e->p+start,p+start,key_bytes))return e->shader?e->shader:g->shader;
        }
    }
    for(unsigned i=0;i<g->specialized_count;++i){NvSpecializedEntry *e=&g->specialized[i];
        if(e->outputs==outputs&&e->layout_valid==!!layout&&(!layout||!memcmp(e->layout,layout->attribute,sizeof(e->layout)))&&e->start==start&&e->end==end&&!memcmp(e->p+start,p+start,key_bytes)){
            if(g->shader_lookup_hint)g->shader_hint[bucket]=i+1;
            return e->shader?e->shader:g->shader;
        }
    }
    /* Bounded resident cache; failures are cached too. Cold compiles still
     * block, but exact persistent bytecode avoids recompiling on later runs. */
    if(g->specialized_count==64)return g->shader;
    NvSpecializedEntry *e=&g->specialized[g->specialized_count++];
    memcpy(e->p+start,p+start,key_bytes);e->start=start;e->end=end;
    e->outputs=outputs;
    e->layout_valid=!!layout;if(layout)memcpy(e->layout,layout->attribute,sizeof(e->layout));
    if(g->shader_lookup_hint)g->shader_hint[bucket]=g->specialized_count;
    char *source=specialized_vertex_source(nv_vertex_shader_source,p,start);
    if(!source)return g->shader;
    if(outputs!=127){
        char prefix[64];int n=snprintf(prefix,sizeof(prefix),"#define MATERIAL_OUTPUTS %u\n",outputs);
        size_t bytes=strlen(source)+1;char *combined=malloc((size_t)n+bytes);
        if(!combined){free(source);return g->shader;}
        memcpy(combined,prefix,n);memcpy(combined+n,source,bytes);free(source);source=combined;
    }
    if(layout){
        char prefix[2048];size_t used=(size_t)snprintf(prefix,sizeof(prefix),"#define PACKED_LAYOUT_SPECIALIZED 1\nstatic const uint4 specialized_layout[16]={\n");
        for(unsigned i=0;i<16;++i){const uint32_t*a=layout->attribute[i];
            int n=snprintf(prefix+used,sizeof(prefix)-used,"uint4(0x%08Xu,0x%08Xu,0x%08Xu,0x%08Xu)%s\n",a[0],a[1],a[2],a[3],i==15?"};":",");
            if(n<0||(size_t)n>=sizeof(prefix)-used){free(source);return g->shader;}used+=(size_t)n;
        }
        size_t bytes=strlen(source)+1;char *combined=malloc(used+bytes);
        if(!combined){free(source);return g->shader;}memcpy(combined,prefix,used);memcpy(combined+used,source,bytes);free(source);source=combined;
    }
    ID3DBlob *code=NULL,*errors=NULL;D3D_SHADER_MACRO macros[]={{"DRAW_VERTEX","1"},{"PACKED_PARTICLE_STREAM",g->packed_enabled==3?"1":"0"},{"RAW_SOURCE_STREAM",g->packed_enabled==2?"1":"0"},{"RAW_PACKET_STREAM",g->packed_enabled==2?"1":"0"},{"RAW_PACKET_VERTEX_PLAN",g->raw_packet_vertex_plan?"1":"0"},{"SPLIT_PACKED_METADATA",g->split_metadata?"1":"0"},{"COMPACT_SPECIALIZED",g->compact_constants?"1":"0"},{g->stream_enabled?"TYPED_VERTEX_STREAM":NULL,"1"},{g->packed_enabled?"PACKED_VERTEX_STREAM":NULL,"1"},{NULL,NULL}};
    ULONGLONG began=GetTickCount64();
    ID3D11Device *dev=NULL;ID3D11DeviceContext_GetDevice(ctx,&dev);
    uint32_t contract[6]={D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS,
        g->stream_enabled,g->packed_enabled,0,0,(g->compact_constants?0x05000002:0x05000001)+(g->split_metadata?16:0)+(g->raw_packet_vertex_plan?32:0)+(g->packed_enabled==3?64:0)};
    HMODULE compiler=nv_vertex_compiler_contract(contract);
    const char *cache_dir=compiler?getenv("UCRR_SHADER_CACHE_DIR"):NULL;
    if(g->async_compile){
        ID3D11Device_Release(dev);
        if(!nv_vertex_compile_queue_options(&g->compiler,g->specialized_count-1,source,contract,cache_dir,
            g->packed_enabled,g->stream_enabled,g->split_metadata,g->compact_constants,g->raw_packet_vertex_plan))free(source);
        return g->shader;
    }
    size_t cached_size=0;void *cached=nv_shader_disk_load(cache_dir,source,contract,&cached_size);
    if(cached){
        HRESULT cached_hr=ID3D11Device_CreateVertexShader(dev,cached,cached_size,NULL,&e->shader);
        free(cached);
        if(SUCCEEDED(cached_hr)){
            free(source);ID3D11Device_Release(dev);
            fprintf(stderr,"[VERTEX-SPECIALIZE-CACHE] entry=%u hit=1 load_ms=%llu\n",g->specialized_count,(unsigned long long)(GetTickCount64()-began));
            return e->shader;
        }
    }
    HRESULT hr=D3DCompile(source,strlen(source),"specialized NV2A",macros,NULL,"draw_main","vs_5_0",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS,0,&code,&errors);
    if(errors)ID3D10Blob_Release(errors);
    if(SUCCEEDED(hr)){
        hr=ID3D11Device_CreateVertexShader(dev,ID3D10Blob_GetBufferPointer(code),ID3D10Blob_GetBufferSize(code),NULL,&e->shader);
        if(SUCCEEDED(hr))nv_shader_disk_save(cache_dir,source,contract,ID3D10Blob_GetBufferPointer(code),ID3D10Blob_GetBufferSize(code));
    }
    free(source);ID3D11Device_Release(dev);
    if(code)ID3D10Blob_Release(code);
    fprintf(stderr,"[VERTEX-SPECIALIZE] entry=%u start=%u success=%u compile_ms=%llu\n",g->specialized_count,start,e->shader!=NULL,(unsigned long long)(GetTickCount64()-began));
    return e->shader?e->shader:g->shader;
}
static ID3D11VertexShader *nv_vertex_gpu_select(NvVertexGPU*g,ID3D11DeviceContext*ctx,const uint32_t p[136][4],unsigned start){
    return nv_vertex_gpu_select_layout(g,ctx,p,start,NULL);
}
static int nv_vertex_gpu_collect_compiles(NvVertexGPU*g,ID3D11DeviceContext*ctx){
    if(!g->async_compile)return 0;
    if(g->fast_ready_poll&&!nv_compiler_ready_hint(&g->compiler.ready))return 0;
    if(!InterlockedCompareExchange(&g->compiler.ready,0,0))return 0;
    ID3D11Device*dev=NULL;ID3D11DeviceContext_GetDevice(ctx,&dev);int changed=0;
    for(unsigned i=0;i<g->specialized_count;i++){
        NvVertexCompileJob*j=&g->compiler.jobs[i];
        if(InterlockedCompareExchange(&j->state,4,3)!=3)continue;
        InterlockedDecrement(&g->compiler.ready);
        if(j->code&&SUCCEEDED(ID3D11Device_CreateVertexShader(dev,j->code,j->bytes,NULL,&g->specialized[i].shader)))changed=1;
        free(j->code);j->code=NULL;
    }
    ID3D11Device_Release(dev);return changed;
}
static int nv_vertex_gpu_upload_core(NvVertexGPU*g,ID3D11DeviceContext*ctx,const uint32_t p[136][4],unsigned start,const float constants[192][4],const void*attrs,unsigned count,const float conversion[6][4],const NvVertexSnapshotLayout *layout){
    if(g->packed_enabled==2)NV_HOST_UPLOAD_ENTER();
    NV_RAW_UPLOAD_ADD(g,ATTEMPTS,1);
    /* One-shot provenance: direct callers without a renderer token always compare. */
    uint64_t revision=g->constant_revision_enabled?g->supplied_constant_revision:0;
    g->supplied_constant_revision=0;
    if(!g->view||!attrs||!count||count>16384)return 0;
    if((layout!=NULL)!=!!g->packed_enabled)return 0;
    if(layout&&(!layout->stride||layout->stride>256||(layout->stride&3)))return 0;
    unsigned stride=layout?layout->stride:256;
    unsigned units=layout?count*stride:count,capacity=layout?16384*256:16384;
    size_t upload_bytes=(size_t)count*stride;
    if(g->packed_enabled==2){
        if(!g->raw_packet_count||g->raw_packet_count>128||!g->raw_packet_bytes||
           (g->raw_packet_bytes&3)||g->raw_packet_bytes>capacity)return 0;
        units=g->raw_packet_bytes;upload_bytes=units;
    }
    NvRawUploadLayout compact_upload;
    int compact_directory=g->packed_enabled==2&&g->raw_compact_directory&&
        nv_raw_upload_compact(attrs,(unsigned)upload_bytes,g->raw_packet_count,count,&compact_upload);
    if(compact_directory){units=compact_upload.bytes;upload_bytes=units;}
    /* These sums describe the actual host payload, including a compact
     * directory when selected, not the untouched canonical CPU arena size. */
    NV_RAW_UPLOAD_ADD(g,VALIDATED,1);NV_RAW_UPLOAD_ADD(g,PACKETS,g->raw_packet_count);
    NV_RAW_UPLOAD_ADD(g,VERTICES,count);NV_RAW_UPLOAD_ADD(g,REQUEST_BYTES,upload_bytes);
    unsigned first=g->stream_enabled?g->vertex_cursor:0;
    if(first>capacity-units)first=0;
    unsigned resident_offset=0;
    int measure_geometry=g->packed_enabled==2&&NV_HOST_UPLOAD_ACTIVE();
    uint64_t geometry_misses=measure_geometry?g->geometry.misses:0;
    int geometry_hit=g->stream_enabled&&(compact_directory?
        nv_geometry_find_parts(&g->geometry,compact_upload.directory,compact_upload.directory_bytes,
            compact_upload.payload,compact_upload.payload_bytes,&resident_offset):
        nv_geometry_find(&g->geometry,attrs,(unsigned)upload_bytes,&resident_offset));
    if(measure_geometry){
        if(geometry_hit){NV_RAW_UPLOAD_ADD(g,RESIDENT_HITS,1);NV_RAW_UPLOAD_ADD(g,RESIDENT_BYTES,upload_bytes);}
        else if(g->stream_enabled&&g->geometry.misses!=geometry_misses)NV_RAW_UPLOAD_ADD(g,RESIDENT_MISSES,1);
        else if(g->stream_enabled&&g->geometry.skip_store)NV_RAW_UPLOAD_ADD(g,RESIDENT_COOLDOWNS,1);
        else NV_RAW_UPLOAD_ADD(g,RESIDENT_BYPASS,1);
    }
    if(geometry_hit)first=layout?resident_offset:resident_offset/256;
    float stream_conversion[6][4];
    if(g->stream_enabled){memcpy(stream_conversion,conversion,sizeof(stream_conversion));stream_conversion[0][3]=layout?0:(float)first;conversion=stream_conversion;}
    int program_changed=!g->program_valid||g->uploaded_program.params[0]!=start||
        memcmp(g->uploaded_program.p,p,sizeof(g->uploaded_program.p));
    int layout_changed=g->specialize_layout&&layout&&(!g->last_layout_valid||memcmp(g->last_layout,layout->attribute,sizeof(g->last_layout)));
    unsigned outputs=g->material_outputs_enabled?(g->material_outputs&127):127;
    int compiled=nv_vertex_gpu_collect_compiles(g,ctx);
    if(g->specialized_enabled&&(compiled||program_changed||layout_changed||outputs!=g->last_material_outputs)){
        g->active_shader=nv_vertex_gpu_select_layout(g,ctx,p,start,layout);
        if(outputs!=127){unsigned saved=g->material_outputs;g->material_outputs=127;
            g->full_shader=nv_vertex_gpu_select_layout(g,ctx,p,start,layout);g->material_outputs=saved;
            /* Compact specialized and interpreter constant layouts differ. */
            if(g->compact_constants&&g->full_shader==g->shader)g->active_shader=g->shader;
        }else g->full_shader=g->active_shader;
        g->last_material_outputs=outputs;
        if(layout&&g->specialize_layout){memcpy(g->last_layout,layout->attribute,sizeof(g->last_layout));g->last_layout_valid=1;}
    }
    /* draw_main consumes SV_VertexID and the host draw count, not the compute
     * fixture's vertex_count. Keep this unused word canonical so size-only
     * changes do not upload all constants/program words again. */
    /* These private buffers have one writer. Compare every byte, including
     * draw parameters and non-finite float payloads; guest addresses are not
     * immutable cache keys. Shutdown clears validity with resource ownership. */
    unsigned compact=g->compact_constants&&g->active_shader!=g->shader;
    int revision_hit=revision&&g->program_valid&&revision==g->uploaded_constant_revision;
    if(revision_hit&&g->constant_revision_shadow&&memcmp(g->uploaded_program.c,constants,sizeof(g->uploaded_program.c))){
        fprintf(stderr,"[CONSTANT-REVISION] ownership mismatch\n");abort();
    }
    int constants_changed=!g->program_valid||(!revision_hit&&memcmp(g->uploaded_program.c,constants,sizeof(g->uploaded_program.c)));
    unsigned program_dirty=program_changed||constants_changed||compact!=g->last_compact;
    unsigned conversion_dirty=!g->conversion_valid||!nv_equal96(g->uploaded_conversion,conversion);
    if(program_dirty){
        /* Compare source fields directly; unchanged draws need no temporary
         * 5264-byte program copy. All source bits are still compared. This
         * private cache is updated only on the immediate-context owner thread. */
        memcpy(g->uploaded_program.c,constants,sizeof(g->uploaded_program.c));
        if(!g->selective_program_copy||program_changed)
            memcpy(g->uploaded_program.p,p,sizeof(g->uploaded_program.p));
        g->uploaded_program.params[0]=start;
        g->uploaded_program.params[1]=g->uploaded_program.params[2]=g->uploaded_program.params[3]=0;
        if(!g->arena.buffer&&(!compact||constants_changed||compact!=g->last_compact)){
            if(!g->resident_enabled||compact||!nv_vertex_gpu_resident_upload(g,ctx)){
                g->resident_selected=NULL;
                ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)(compact?g->compact_program:g->program),0,NULL,&g->uploaded_program,0,0);
                NV_RAW_UPLOAD_ADD(g,PROGRAM_UPDATES,1);NV_RAW_UPLOAD_ADD(g,PROGRAM_UPDATE_BYTES,compact?sizeof(g->uploaded_program.c):sizeof(g->uploaded_program));
                ++g->program_uploads;
            }
        }
        g->last_compact=compact;g->program_valid=1;
    }
    g->uploaded_constant_revision=revision;
    if(conversion_dirty){
        if(!g->arena.buffer){ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)g->conversion,0,NULL,conversion,0,0);
            NV_RAW_UPLOAD_ADD(g,CONVERSION_UPDATES,1);NV_RAW_UPLOAD_ADD(g,CONVERSION_UPDATE_BYTES,sizeof(g->uploaded_conversion));}
        memcpy(g->uploaded_conversion,conversion,sizeof(g->uploaded_conversion));g->conversion_valid=1;++g->conversion_uploads;
    }
    /* Fresh owned bytes are compared on every lookup. No address-only reuse. */
    if(geometry_hit){
        g->vertex_first=layout?0:first;
    }else if(g->stream_enabled){
        D3D11_MAP mode=first?D3D11_MAP_WRITE_NO_OVERWRITE:D3D11_MAP_WRITE_DISCARD;
        if(!first)nv_geometry_expire(&g->geometry);
        D3D11_MAPPED_SUBRESOURCE mapped={0};
        if(first)NV_RAW_UPLOAD_ADD(g,VERTEX_MAP_APPEND,1);else NV_RAW_UPLOAD_ADD(g,VERTEX_MAP_DISCARD,1);
        if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)g->vertices,0,mode,0,&mapped))){NV_RAW_UPLOAD_ADD(g,VERTEX_MAP_FAILURES,1);if(g->arena.buffer)g->program_valid=g->conversion_valid=0;return 0;}
        /* Never read write-combined memory. Only append to unused storage;
         * DISCARD gives previous queued draws ownership of the old allocation. */
        uint8_t *destination=(uint8_t*)mapped.pData+(layout?first:(size_t)first*256);
        if(compact_directory){
            memcpy(destination,compact_upload.directory,compact_upload.directory_bytes);
            memcpy(destination+compact_upload.directory_bytes,compact_upload.payload,compact_upload.payload_bytes);
        }else memcpy(destination,attrs,upload_bytes);
        NV_RAW_UPLOAD_ADD(g,VERTEX_COPY_BYTES,upload_bytes);
        ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)g->vertices,0);
        uint64_t geometry_uploads=measure_geometry?g->geometry.uploads:0;
        if(compact_directory)nv_geometry_store_parts(&g->geometry,compact_upload.directory,compact_upload.directory_bytes,
            compact_upload.payload,compact_upload.payload_bytes,layout?first:first*256);
        else nv_geometry_store(&g->geometry,attrs,(unsigned)upload_bytes,layout?first:first*256);
        if(measure_geometry&&g->geometry.uploads!=geometry_uploads)NV_RAW_UPLOAD_ADD(g,SHADOW_COPY_BYTES,upload_bytes);
        g->vertex_first=layout?0:first;g->vertex_cursor=first+units;
        if(first)++g->stream_appends;else ++g->stream_discards;
    }else{
        D3D11_BOX box={0,0,0,count*256,1,1};ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)g->vertices,0,&box,attrs,0,0);
        NV_RAW_UPLOAD_ADD(g,VERTEX_UPDATES,1);NV_RAW_UPLOAD_ADD(g,VERTEX_UPDATE_BYTES,(uint64_t)count*256);
        g->vertex_first=0;
    }
    if(layout){
        struct {uint32_t attributes[16][4],defaults[16][4],params[4];} metadata;
        memcpy(metadata.attributes,layout->attribute,sizeof(metadata.attributes));memcpy(metadata.defaults,layout->defaults,sizeof(metadata.defaults));
        metadata.params[0]=stride;metadata.params[1]=first;metadata.params[2]=count;metadata.params[3]=0;
        if(g->packed_enabled==2)metadata.params[0]=g->raw_packet_count;
        if(g->arena.buffer){
            NV_RAW_UPLOAD_ADD(g,ARENA_CALLS,1);NV_RAW_UPLOAD_ADD(g,ARENA_COVERAGE_GAPS,1);
            if(!nv_constant_arena_upload(&g->arena,ctx,&g->uploaded_program,g->uploaded_conversion,&metadata,program_dirty,conversion_dirty)){
                NV_RAW_UPLOAD_ADD(g,ARENA_FAILURES,1);
                g->program_valid=g->conversion_valid=0;return 0;
            }
            if(program_dirty)++g->program_uploads;
        }else if(g->split_metadata){
            if(!g->metadata_valid||memcmp(g->uploaded_metadata,&metadata,sizeof(g->uploaded_metadata))){
                ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)g->packed_metadata,0,NULL,&metadata,0,0);
                NV_RAW_UPLOAD_ADD(g,METADATA_UPDATES,1);NV_RAW_UPLOAD_ADD(g,METADATA_UPDATE_BYTES,sizeof(g->uploaded_metadata));
                memcpy(g->uploaded_metadata,&metadata,sizeof(g->uploaded_metadata));g->metadata_valid=1;
            }
            ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)g->packed_params,0,NULL,metadata.params,0,0);
            NV_RAW_UPLOAD_ADD(g,PARAM_UPDATES,1);NV_RAW_UPLOAD_ADD(g,PARAM_UPDATE_BYTES,sizeof(metadata.params));
        }else if(g->dynamic_packed_metadata){
            D3D11_MAPPED_SUBRESOURCE mapped={0};
            NV_RAW_UPLOAD_ADD(g,METADATA_MAPS,1);
            if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)g->packed_metadata,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))){NV_RAW_UPLOAD_ADD(g,METADATA_MAP_FAILURES,1);return 0;}
            /* Entire allocation replaced; never read write-combined memory.
             * Earlier submitted draws retain the driver's old allocation. */
            memcpy(mapped.pData,&metadata,sizeof(metadata));
            NV_RAW_UPLOAD_ADD(g,METADATA_COPY_BYTES,sizeof(metadata));
            ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)g->packed_metadata,0);
        }else {ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)g->packed_metadata,0,NULL,&metadata,0,0);
            NV_RAW_UPLOAD_ADD(g,METADATA_UPDATES,1);NV_RAW_UPLOAD_ADD(g,METADATA_UPDATE_BYTES,sizeof(metadata));}
    }
    g->uploaded_vertices=count;++g->vertex_uploads;NV_RAW_UPLOAD_ADD(g,SUCCESSES,1);return 1;
}
static int nv_vertex_gpu_upload(NvVertexGPU*g,ID3D11DeviceContext*ctx,const uint32_t p[136][4],unsigned start,const float constants[192][4],const float*attrs,unsigned count,const float conversion[6][4]){
    return nv_vertex_gpu_upload_core(g,ctx,p,start,constants,attrs,count,conversion,NULL);
}
static int nv_vertex_gpu_upload_indices(NvVertexGPU*g,ID3D11DeviceContext*ctx,const uint32_t *indices,unsigned count){
    if(!g->indices||!indices||!count||count>16384||!g->uploaded_vertices)return 0;
    for(unsigned i=0;i<count;++i)if(indices[i]>=g->uploaded_vertices)return 0;
    /* This conversion writer is not admitted by the native16 cache. */
    nv_geometry_expire(&g->index_geometry);
    unsigned first=g->index_cursor;if(first>16384-count)first=0;
    D3D11_MAPPED_SUBRESOURCE mapped={0};
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)g->indices,0,first?D3D11_MAP_WRITE_NO_OVERWRITE:D3D11_MAP_WRITE_DISCARD,0,&mapped)))return 0;
    if(g->narrow_indices){uint16_t*out=(uint16_t*)mapped.pData+first;for(unsigned i=0;i<count;++i)out[i]=(uint16_t)indices[i];}
    else memcpy((uint32_t*)mapped.pData+first,indices,(size_t)count*4);
    ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)g->indices,0);
    g->index_first=first;g->index_cursor=first+count;return 1;
}
static int nv_vertex_gpu_upload_indices16(NvVertexGPU*g,ID3D11DeviceContext*ctx,const uint16_t *indices,unsigned count){
    if(!g->indices||!g->narrow_indices||!indices||!count||count>16384||!g->uploaded_vertices)return 0;
    for(unsigned i=0;i<count;++i)if(indices[i]>=g->uploaded_vertices)return 0;
    unsigned resident=0;
    if(nv_geometry_find(&g->index_geometry,indices,count*2,&resident)){g->index_first=resident/2;return 1;}
    unsigned first=g->index_cursor;if(first>16384-count)first=0;
    if(!first)nv_geometry_expire(&g->index_geometry);
    D3D11_MAPPED_SUBRESOURCE mapped={0};
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)g->indices,0,first?D3D11_MAP_WRITE_NO_OVERWRITE:D3D11_MAP_WRITE_DISCARD,0,&mapped)))return 0;
    memcpy((uint16_t*)mapped.pData+first,indices,(size_t)count*2);
    ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)g->indices,0);
    nv_geometry_store(&g->index_geometry,indices,count*2,first*2);
    g->index_first=first;g->index_cursor=first+count;return 1;
}
#endif
