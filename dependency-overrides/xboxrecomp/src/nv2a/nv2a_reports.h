#ifndef NV2A_REPORTS_H
#define NV2A_REPORTS_H
#include "nv2a_dma_resolve.h"
#ifndef NV_COMPLETED_RAM_WRITE
#define NV_COMPLETED_RAM_WRITE(ram,size,address,bytes) ((void)0)
#endif
#define NV_REPORT_QUEUE 1024
/* Ring storage is fixed; admission keeps the original 256-record budget unless
 * this isolated experiment is selected before any report work. */
static unsigned nv_report_limit;
static unsigned nv_report_capacity(void){
    if(!nv_report_limit){const char*s=getenv("UCRR_REPORT_QUEUE");nv_report_limit=s&&!strcmp(s,"1024")?1024:256;}
    return nv_report_limit;
}
#ifndef NV_QUERY_GET_DATA
#define NV_QUERY_GET_DATA ID3D11DeviceContext_GetData
#define NV_QUERY_FLUSH ID3D11DeviceContext_Flush
#endif
#define NV_REPORT_POOL 1024
static unsigned nv_report_pool_limit;
static unsigned nv_report_pool_capacity(void){
    if(!nv_report_pool_limit){const char*s=getenv("UCRR_REPORT_POOL");nv_report_pool_limit=s&&!strcmp(s,"1024")?1024:64;}
    return nv_report_pool_limit;
}
#ifndef NV_REPORT_SAMPLE_COUNT
#define NV_REPORT_SAMPLE_COUNT(n) (n)
#endif
#ifndef NV_QUERY_ADMISSION_DEFER
#define NV_QUERY_ADMISSION_DEFER() 0
#endif
#ifndef NV_REPORT_ADMISSION_POLL_DEFER
#define NV_REPORT_ADMISSION_POLL_DEFER(ctx,ram,size) 0
#endif
#ifndef NV_REPORT_FLUSH
#define NV_REPORT_FLUSH NV_QUERY_FLUSH
#endif
#ifndef NV_QUERY_SUBMIT_ALLOWED
#define NV_QUERY_SUBMIT_ALLOWED(ctx,origin) 1
#endif
typedef struct NvReportItem { ID3D11Query *query; unsigned kind; uint32_t address; unsigned submitted; } NvReportItem;
typedef struct NvReports {
    NvReportItem queue[NV_REPORT_QUEUE];
    unsigned head,count,enabled,failed;
    ID3D11Query *active;
    uint64_t samples;
    unsigned completed;
    /* Only queries whose GetData completed successfully enter this pool.
     * Active/pending queries retain exclusive ownership until completion. */
    ID3D11Query *available[NV_REPORT_POOL];
    unsigned available_count,created,reused;
    uint64_t pressure_entries,pressure_covered,pressure_submits,pressure_sleeps;
    uint64_t admission_poll_skips;
} NvReports;
static int nv_report_pressure_option=-1;
static int nv_report_pressure_enabled(void){
    if(nv_report_pressure_option<0){const char*s=getenv("UCRR_REPORT_PRESSURE");nv_report_pressure_option=s&&!strcmp(s,"1");}
    return nv_report_pressure_option;
}
/* Only ended queries live in this ring. Check every entry: the head can be
 * covered while a later End still requires submission. Active/future queries
 * are not dependencies of these already ordered report records. */
static int nv_report_has_unsubmitted(const NvReports*r){
    for(unsigned i=0;i<r->count;++i){const NvReportItem*q=&r->queue[(r->head+i)%NV_REPORT_QUEUE];
        if(q->kind==0&&!q->submitted)return 1;
    }
    return 0;
}
static int nv_report_push(NvReports *r, unsigned kind, ID3D11Query *q, uint32_t address)
{
    if(r->count==NV_REPORT_QUEUE) { fprintf(stderr,"[GPU-REPORT-FAIL] queue full completed=%u\n",r->completed);r->failed=1; return 0; }
    r->queue[(r->head+r->count++)%NV_REPORT_QUEUE]=(NvReportItem){q,kind,address};
    return 1;
}
static void nv_report_begin(NvReports *r, ID3D11Device *dev, ID3D11DeviceContext *ctx)
{
    if(r->active || r->failed || !dev || !ctx) return;
    if(r->available_count){
        r->active=r->available[--r->available_count];
        r->available[r->available_count]=NULL;++r->reused;
    }else{
        D3D11_QUERY_DESC desc={D3D11_QUERY_OCCLUSION,0};
        if(FAILED(ID3D11Device_CreateQuery(dev,&desc,&r->active))) {r->failed=1;return;}
        ++r->created;
    }
    ID3D11DeviceContext_Begin(ctx,(ID3D11Asynchronous *)r->active);
}
static void nv_report_end(NvReports *r, ID3D11DeviceContext *ctx)
{
    if(!r->active) return;
    ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous *)r->active);
    if(!nv_report_push(r,0,r->active,0)) ID3D11Query_Release(r->active);
    r->active=NULL;
}
/* All queued query End calls precede this immediate-context Flush. Mark only
 * already-enqueued queries, never active or subsequently issued queries.
 * Submission is not completion; GetData still gates every result/write. */
static void nv_report_submit_pending(NvReports *r, ID3D11DeviceContext *ctx)
{
    NV_REPORT_FLUSH(ctx);
    for(unsigned i=0;i<r->count;++i){
        NvReportItem *item=&r->queue[(r->head+i)%NV_REPORT_QUEUE];
        if(item->kind==0)item->submitted=1;
    }
}
static void nv_report_poll_mode(NvReports *r, ID3D11DeviceContext *ctx, uint8_t *ram, size_t size, int submit)
{
    while(r->count && !r->failed && ctx && ram) {
        NvReportItem *item=&r->queue[r->head];
        if(item->kind==0) {
            UINT64 samples=0;
            HRESULT hr=NV_QUERY_GET_DATA(ctx,(ID3D11Asynchronous *)item->query,&samples,sizeof(samples),D3D11_ASYNC_GETDATA_DONOTFLUSH);
            /* Submit an unfinished ended query once, then observe it without
             * repeatedly flushing from the guest's tight progress loop. */
            if(hr==S_FALSE) {
                if(submit&&!item->submitted&&NV_QUERY_SUBMIT_ALLOWED(ctx,0))nv_report_submit_pending(r,ctx);
                return;
            }
            if(FAILED(hr)) {r->failed=1;return;}
            r->samples+=samples;
            if(r->available_count<nv_report_pool_capacity())r->available[r->available_count++]=item->query;
            else ID3D11Query_Release(item->query);
            item->query=NULL;
        } else if(item->kind==1) r->samples=0;
        else {
            if((uint64_t)item->address+16>size) {r->failed=1;return;}
            /* Host monotonic ns is provisional timing, not Xbox timer fidelity. */
            LARGE_INTEGER now,freq; QueryPerformanceCounter(&now); QueryPerformanceFrequency(&freq);
            uint64_t ns=(uint64_t)(now.QuadPart/freq.QuadPart)*1000000000ull+
                (uint64_t)(now.QuadPart%freq.QuadPart)*1000000000ull/freq.QuadPart;
            uint32_t count=(uint32_t)NV_REPORT_SAMPLE_COUNT(r->samples), done=0;
            memcpy(ram+item->address,&ns,8); memcpy(ram+item->address+8,&count,4);
            memcpy(ram+item->address+12,&done,4);
            NV_COMPLETED_RAM_WRITE(ram,size,item->address,16);
            if(++r->completed<=8) fprintf(stderr,"[GPU-REPORT] address=%08X samples=%u completed=%u\n",item->address,count,r->completed);
        }
        r->head=(r->head+1)%NV_REPORT_QUEUE; --r->count;
    }
}
static void nv_report_poll(NvReports*r,ID3D11DeviceContext*ctx,uint8_t*ram,size_t size){nv_report_poll_mode(r,ctx,ram,size,1);}
static void nv_report_shutdown(NvReports *r, ID3D11DeviceContext *ctx)
{
    const char*record=getenv("UCRR_REPORT_PRESSURE_RECORD");
    if(record&&!strcmp(record,"1")&&r->pressure_entries){
        char path[96];snprintf(path,sizeof(path),"report-pressure-%lu.json",GetCurrentProcessId());
        FILE*f=fopen(path,"w");if(f){fprintf(f,"{\"enabled\":%d,\"entries\":%llu,\"covered\":%llu,\"submits\":%llu,\"sleeps\":%llu,\"failed\":%u,\"completed\":%u}\n",nv_report_pressure_enabled(),(unsigned long long)r->pressure_entries,(unsigned long long)r->pressure_covered,(unsigned long long)r->pressure_submits,(unsigned long long)r->pressure_sleeps,r->failed,r->completed);fclose(f);}
    }
    if(r->active) {if(ctx) ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous *)r->active); ID3D11Query_Release(r->active);}
    for(unsigned i=0;i<r->count;++i) {ID3D11Query *q=r->queue[(r->head+i)%NV_REPORT_QUEUE].query;if(q) ID3D11Query_Release(q);}
    for(unsigned i=0;i<r->available_count;++i)ID3D11Query_Release(r->available[i]);
    memset(r,0,sizeof(*r));
}
/* Bound resources without dropping submitted work. Called before a command
 * can enqueue up to three ordered records. Only actual host query results
 * free space; a device/timeout failure is explicit, never a fabricated result. */
static int nv_report_reserve(NvReports *r,ID3D11DeviceContext *ctx,uint8_t *ram,size_t size)
{
    unsigned limit=nv_report_capacity();
    /* Only decoder-owned admission can defer. Public progress/callback polls
     * remain unconditional, and capacity exhaustion falls through to actual
     * GetData/Flush/wait. No result is fabricated or published here. */
    if(!r->failed&&ctx&&ram&&r->count<=limit-3&&
       NV_REPORT_ADMISSION_POLL_DEFER(ctx,ram,size)){
        ++r->admission_poll_skips;return 1;
    }
    /* Admission can reap real completions without prematurely splitting the
     * decoder batch. Public progress polls and capacity waits still submit. */
    nv_report_poll_mode(r,ctx,ram,size,!NV_QUERY_ADMISSION_DEFER());
    if(r->count<=limit-3)return !r->failed;
    ++r->pressure_entries;
    int covered=!nv_report_has_unsubmitted(r);r->pressure_covered+=covered;
    ULONGLONG deadline=GetTickCount64()+2000;
    if(ctx&&(!nv_report_pressure_enabled()||!covered)){nv_report_submit_pending(r,ctx);++r->pressure_submits;}
    while(!r->failed && r->count>limit-3 && GetTickCount64()<deadline){
        nv_report_poll(r,ctx,ram,size);
        if(r->count>limit-3){++r->pressure_sleeps;Sleep(0);}
    }
    if(r->count>limit-3){fprintf(stderr,"[GPU-REPORT-FAIL] backpressure timeout count=%u completed=%u\n",r->count,r->completed);r->failed=1;}
    return !r->failed;
}
#endif
