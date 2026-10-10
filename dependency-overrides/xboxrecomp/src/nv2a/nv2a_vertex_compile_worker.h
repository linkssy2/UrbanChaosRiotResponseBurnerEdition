#ifndef NV_VERTEX_COMPILE_WORKER_H
#define NV_VERTEX_COMPILE_WORKER_H
/* Render-thread producer/consumer; one low-priority compiler per vertex route.
 * Jobs own source, options and bytecode. No guest pointers, D3D device/context,
 * or renderer state are touched by the worker. Slots are never recycled. */
typedef struct NvVertexCompileJob {
    volatile LONG state; /* 0 empty, 1 queued, 2 compiling, 3 ready, 4 consumed */
    char *source;
    char cache_dir[1024];
    uint32_t contract[6];
    unsigned packed,stream,split,compact,raw_packet_vertex_plan;
    void *code;size_t bytes;
    DWORD compiler_thread;
} NvVertexCompileJob;
typedef struct NvVertexCompiler {
    HANDLE thread,wake;volatile LONG stop,ready;
    NvVertexCompileJob jobs[64];
} NvVertexCompiler;
static DWORD WINAPI nv_vertex_compile_worker(void*arg){
    NvVertexCompiler*w=arg;
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
    for(;;){
        WaitForSingleObject(w->wake,INFINITE);
        if(InterlockedCompareExchange(&w->stop,0,0))break;
        for(unsigned i=0;i<64;i++){
            if(InterlockedCompareExchange(&w->stop,0,0))break;
            NvVertexCompileJob*j=&w->jobs[i];
            if(InterlockedCompareExchange(&j->state,2,1)!=1)continue;
            j->compiler_thread=GetCurrentThreadId();
            j->code=nv_shader_disk_load(j->cache_dir,j->source,j->contract,&j->bytes);
            if(!j->code){
                D3D_SHADER_MACRO macros[]={{"DRAW_VERTEX","1"},{"PACKED_PARTICLE_STREAM",j->packed==3?"1":"0"},{"RAW_SOURCE_STREAM",j->packed==2?"1":"0"},
                    {"RAW_PACKET_STREAM",j->packed==2?"1":"0"},{"SPLIT_PACKED_METADATA",j->split?"1":"0"},
                    {"RAW_PACKET_VERTEX_PLAN",j->raw_packet_vertex_plan?"1":"0"},
                    {"COMPACT_SPECIALIZED",j->compact?"1":"0"},{j->stream?"TYPED_VERTEX_STREAM":NULL,"1"},
                    {j->packed?"PACKED_VERTEX_STREAM":NULL,"1"},{NULL,NULL}};
                ID3DBlob*code=NULL,*errors=NULL;
                HRESULT hr=D3DCompile(j->source,strlen(j->source),"async specialized NV2A",macros,NULL,
                    "draw_main","vs_5_0",j->contract[0],0,&code,&errors);
                if(errors)ID3D10Blob_Release(errors);
                if(SUCCEEDED(hr)&&code){
                    j->bytes=ID3D10Blob_GetBufferSize(code);j->code=malloc(j->bytes);
                    if(j->code){memcpy(j->code,ID3D10Blob_GetBufferPointer(code),j->bytes);
                        nv_shader_disk_save(j->cache_dir,j->source,j->contract,j->code,j->bytes);}
                }
                if(code)ID3D10Blob_Release(code);
            }
            free(j->source);j->source=NULL;
            InterlockedExchange(&j->state,3);InterlockedIncrement(&w->ready);
        }
    }
    return 0;
}
static int nv_vertex_compile_queue_options(NvVertexCompiler*w,unsigned slot,char*source,const uint32_t contract[6],
    const char*cache,unsigned packed,unsigned stream,unsigned split,unsigned compact,unsigned raw_packet_vertex_plan){
    if(slot>=64||!source||InterlockedCompareExchange(&w->jobs[slot].state,0,0))return 0;
    if(!w->thread){
        w->wake=CreateEventW(NULL,FALSE,FALSE,NULL);if(!w->wake)return 0;
        w->thread=CreateThread(NULL,0,nv_vertex_compile_worker,w,0,NULL);
        if(!w->thread){CloseHandle(w->wake);w->wake=NULL;return 0;}
    }
    NvVertexCompileJob*j=&w->jobs[slot];j->source=source;memcpy(j->contract,contract,24);
    if(cache&&strlen(cache)<sizeof(j->cache_dir))strcpy(j->cache_dir,cache);
    j->packed=packed;j->stream=stream;j->split=split;j->compact=compact;
    j->raw_packet_vertex_plan=raw_packet_vertex_plan;
    InterlockedExchange(&j->state,1);SetEvent(w->wake);return 1;
}
static int nv_vertex_compile_queue(NvVertexCompiler*w,unsigned slot,char*source,const uint32_t contract[6],
    const char*cache,unsigned packed,unsigned stream,unsigned split,unsigned compact){
    return nv_vertex_compile_queue_options(w,slot,source,contract,cache,packed,stream,split,compact,0);
}
static void nv_vertex_compile_stop(NvVertexCompiler*w){
    if(w->thread){InterlockedExchange(&w->stop,1);SetEvent(w->wake);
        /* Shutdown only, never a draw-path wait. Join before freeing ownership. */
        WaitForSingleObject(w->thread,INFINITE);CloseHandle(w->thread);CloseHandle(w->wake);}
    for(unsigned i=0;i<64;i++){free(w->jobs[i].source);free(w->jobs[i].code);}
    memset(w,0,sizeof(*w));
}
#endif
