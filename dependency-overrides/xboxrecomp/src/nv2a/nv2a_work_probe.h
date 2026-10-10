#ifndef NV2A_WORK_PROBE_H
#define NV2A_WORK_PROBE_H
#include "nv2a_pgraph_d3d11.h"
/* Opt-in measurement only. Spans consecutive PGRAPH flush boundaries (including
 * GPU idle gaps), NOT busy GPU time or scanout. Never flush/wait for a sample. */
typedef struct NvWorkSlot {ID3D11Query *first,*last,*disjoint;unsigned pending;} NvWorkSlot;
typedef struct NvWorkProbe {
    NvWorkSlot slots[8];unsigned cursor,active,failed,completed,discarded,skipped,window;
    double sum_ms,max_ms;
    uint64_t polls,pending_polls,poll_samples,poll_ticks,flushes,flush_ticks;
    unsigned reason;
    struct {uint64_t polls,pending,flushes,flush_ticks;} reasons[NV_WAIT_REASON_COUNT];
} NvWorkProbe;
static int nv_work_enabled(void){
#if UCRR_PLAY_ONLY
    return 0;
#endif
static int e=-1;if(e<0){const char*v=getenv("UCRR_GPU_WORK_TRACE");e=v&&!strcmp(v,"1");}return e;}
static uint64_t nv_work_ticks(void){LARGE_INTEGER q;QueryPerformanceCounter(&q);return (uint64_t)q.QuadPart;}
static HRESULT nv_work_get_data(NvWorkProbe*t,ID3D11DeviceContext*c,ID3D11Asynchronous*q,void*out,UINT size,UINT flags){
    if(!nv_work_enabled())return ID3D11DeviceContext_GetData(c,q,out,size,flags);
    unsigned sample=(++t->polls%256)==0;uint64_t start=sample?nv_work_ticks():0;
    HRESULT hr=ID3D11DeviceContext_GetData(c,q,out,size,flags);
    ++t->reasons[t->reason].polls;
    if(hr==S_FALSE)++t->reasons[t->reason].pending;
    if(sample){t->poll_ticks+=nv_work_ticks()-start;++t->poll_samples;}
    if(hr==S_FALSE)++t->pending_polls;return hr;
}
static void nv_work_flush(NvWorkProbe*t,ID3D11DeviceContext*c){
    if(!nv_work_enabled()){ID3D11DeviceContext_Flush(c);return;}
    uint64_t start=nv_work_ticks();ID3D11DeviceContext_Flush(c);
    uint64_t elapsed=nv_work_ticks()-start;
    t->flush_ticks+=elapsed;++t->flushes;
    ++t->reasons[t->reason].flushes;t->reasons[t->reason].flush_ticks+=elapsed;
}
static void nv_work_reasons(NvWorkProbe*t,unsigned frame){
    if(!nv_work_enabled())return;
    static const char*names[]={"other","kick","mmio","ram","reuse_before","reuse_after","visibility","frame","callback","kick_idle"};
    LARGE_INTEGER f;QueryPerformanceFrequency(&f);
    for(unsigned i=0;i<NV_WAIT_REASON_COUNT;++i)
        fprintf(stderr,"[QUERY-CAUSE] frame=%u reason=%s polls=%llu pending=%llu flushes=%llu flush_ms=%.6f\n",frame,names[i],
          (unsigned long long)t->reasons[i].polls,(unsigned long long)t->reasons[i].pending,
          (unsigned long long)t->reasons[i].flushes,t->reasons[i].flush_ticks*1000.0/f.QuadPart);
}
static void nv_work_shutdown(NvWorkProbe*t,ID3D11DeviceContext*c){
    if(t->active&&c)ID3D11DeviceContext_End(c,(ID3D11Asynchronous*)t->slots[t->cursor].disjoint);
    for(unsigned i=0;i<8;++i){NvWorkSlot*s=&t->slots[i];
        if(s->first)ID3D11Query_Release(s->first);if(s->last)ID3D11Query_Release(s->last);if(s->disjoint)ID3D11Query_Release(s->disjoint);}
    memset(t,0,sizeof(*t));
}
static void nv_work_boundary(NvWorkProbe*t,ID3D11Device*d,ID3D11DeviceContext*c){
    if(!d||!c||!nv_work_enabled())return;
    if(t->active){NvWorkSlot*s=&t->slots[t->cursor];ID3D11DeviceContext_End(c,(ID3D11Asynchronous*)s->last);
        ID3D11DeviceContext_End(c,(ID3D11Asynchronous*)s->disjoint);s->pending=1;t->active=0;t->cursor=(t->cursor+1)%8;}
    for(unsigned i=0;i<8&&!t->failed;++i){NvWorkSlot*s=&t->slots[i];if(!s->pending)continue;
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT j={0};UINT64 a=0,b=0;
        HRESULT hr=ID3D11DeviceContext_GetData(c,(ID3D11Asynchronous*)s->disjoint,&j,sizeof(j),D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(hr==S_FALSE)continue;if(FAILED(hr)){t->failed=1;break;}
        hr=ID3D11DeviceContext_GetData(c,(ID3D11Asynchronous*)s->last,&b,sizeof(b),D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(hr==S_FALSE)continue;if(FAILED(hr)){t->failed=1;break;}
        hr=ID3D11DeviceContext_GetData(c,(ID3D11Asynchronous*)s->first,&a,sizeof(a),D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(hr==S_FALSE)continue;if(FAILED(hr)){t->failed=1;break;}
        s->pending=0;if(j.Disjoint||!j.Frequency||b<a){++t->discarded;continue;}
        double ms=(double)(b-a)*1000.0/(double)j.Frequency;t->sum_ms+=ms;if(ms>t->max_ms)t->max_ms=ms;++t->completed;
        if(++t->window==120){LARGE_INTEGER f;QueryPerformanceFrequency(&f);double scale=1000.0/(double)f.QuadPart;
            fprintf(stderr,"[GPU-WORK] spans=%u avg_span_ms=%.6f max_span_ms=%.6f skipped=%u discarded=%u query_polls=%llu not_ready=%llu poll_samples=%llu sampled_poll_ms=%.6f explicit_flushes=%llu flush_ms=%.6f\n",
                t->completed,t->sum_ms/120,t->max_ms,t->skipped,t->discarded,(unsigned long long)t->polls,(unsigned long long)t->pending_polls,
                (unsigned long long)t->poll_samples,t->poll_ticks*scale,(unsigned long long)t->flushes,t->flush_ticks*scale);
            t->window=0;t->sum_ms=t->max_ms=0;}
    }
    if(t->failed)return;NvWorkSlot*s=&t->slots[t->cursor];if(s->pending){++t->skipped;return;}
    if(!s->first){D3D11_QUERY_DESC q={D3D11_QUERY_TIMESTAMP,0};
        if(FAILED(ID3D11Device_CreateQuery(d,&q,&s->first))||FAILED(ID3D11Device_CreateQuery(d,&q,&s->last))){t->failed=1;return;}
        q.Query=D3D11_QUERY_TIMESTAMP_DISJOINT;if(FAILED(ID3D11Device_CreateQuery(d,&q,&s->disjoint))){t->failed=1;return;}}
    ID3D11DeviceContext_Begin(c,(ID3D11Asynchronous*)s->disjoint);ID3D11DeviceContext_End(c,(ID3D11Asynchronous*)s->first);t->active=1;
}
#endif
