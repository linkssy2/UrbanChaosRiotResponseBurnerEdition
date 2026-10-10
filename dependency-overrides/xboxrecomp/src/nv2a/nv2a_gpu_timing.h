#ifndef NV2A_GPU_TIMING_H
#define NV2A_GPU_TIMING_H
/* Bounded diagnostic: at most one sampled draw per PGRAPH frame, one pending
 * triple, nonblocking reads. Not whole-frame FPS or all-draw GPU utilization. */
typedef struct NvGpuTiming {
 ID3D11Query *first,*last,*disjoint;
 uint32_t frame,vertices;unsigned seen,pending,failed,completed,discarded,skipped;
 double last_ms;
} NvGpuTiming;
static void nv_gpu_timing_shutdown(NvGpuTiming*t){
 if(t->first)ID3D11Query_Release(t->first);if(t->last)ID3D11Query_Release(t->last);
 if(t->disjoint)ID3D11Query_Release(t->disjoint);memset(t,0,sizeof(*t));
}
static int nv_gpu_timing_poll(NvGpuTiming*t,ID3D11DeviceContext*ctx){
 if(!t->pending)return !t->failed;
 D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d={0};UINT64 a=0,b=0;
 HRESULT hr=ID3D11DeviceContext_GetData(ctx,(ID3D11Asynchronous*)t->disjoint,&d,sizeof(d),D3D11_ASYNC_GETDATA_DONOTFLUSH);
 if(hr==S_FALSE)return 0;if(FAILED(hr)){t->failed=1;return 0;}
 hr=ID3D11DeviceContext_GetData(ctx,(ID3D11Asynchronous*)t->last,&b,sizeof(b),D3D11_ASYNC_GETDATA_DONOTFLUSH);
 if(hr==S_FALSE)return 0;if(FAILED(hr)){t->failed=1;return 0;}
 hr=ID3D11DeviceContext_GetData(ctx,(ID3D11Asynchronous*)t->first,&a,sizeof(a),D3D11_ASYNC_GETDATA_DONOTFLUSH);
 if(hr==S_FALSE)return 0;if(FAILED(hr)){t->failed=1;return 0;}
 t->pending=0;
 if(d.Disjoint||!d.Frequency||b<a){++t->discarded;return 1;}
 t->last_ms=(double)(b-a)*1000.0/(double)d.Frequency;++t->completed;
 if(t->completed<=16 || !(t->completed%128))fprintf(stderr,"[GPU-DRAW-TIME] sample=%u vertices=%u ms=%.6f skipped=%u disjoint=%u\n",t->completed,t->vertices,t->last_ms,t->skipped,t->discarded);
 return 1;
}
static int nv_gpu_timing_begin(NvGpuTiming*t,ID3D11Device*dev,ID3D11DeviceContext*ctx,uint32_t frame,uint32_t vertices){
 if(t->failed||vertices<512||(t->seen&&frame==t->frame))return 0;
 t->seen=1;t->frame=frame;
 if(!nv_gpu_timing_poll(t,ctx)){++t->skipped;return 0;}
 if(!t->first){
  D3D11_QUERY_DESC q={D3D11_QUERY_TIMESTAMP,0};
  if(FAILED(ID3D11Device_CreateQuery(dev,&q,&t->first))||FAILED(ID3D11Device_CreateQuery(dev,&q,&t->last)))goto fail;
  q.Query=D3D11_QUERY_TIMESTAMP_DISJOINT;if(FAILED(ID3D11Device_CreateQuery(dev,&q,&t->disjoint)))goto fail;
 }
 t->vertices=vertices;ID3D11DeviceContext_Begin(ctx,(ID3D11Asynchronous*)t->disjoint);
 ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)t->first);return 1;
fail:t->failed=1;return 0;
}
static void nv_gpu_timing_end(NvGpuTiming*t,ID3D11DeviceContext*ctx){
 ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)t->last);
 ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)t->disjoint);t->pending=1;
}
#endif
