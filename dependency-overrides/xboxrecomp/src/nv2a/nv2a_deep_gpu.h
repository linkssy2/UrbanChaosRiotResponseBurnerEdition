#ifndef NV2A_DEEP_GPU_H
#define NV2A_DEEP_GPU_H
/* One outstanding measurement; never Flush or wait. Measures GPU command-stream
 * elapsed time from first captured submission through scanout blit, not total
 * GPU utilization or CPU time. CPU submission starvation can be included. */
typedef struct NvDeepGpu {ID3D11Query *a,*b,*disjoint;unsigned open,pending,failed,issued,completed,skipped,burst,frame;} NvDeepGpu;
static void nv_deep_gpu_shutdown(NvDeepGpu*t){
 if(t->issued)fprintf(stderr,"[DEEP-GPU-STATUS] issued=%u completed=%u pending=%u open=%u failed=%u\n",t->issued,t->completed,t->pending,t->open,t->failed);
 if(t->a)ID3D11Query_Release(t->a);if(t->b)ID3D11Query_Release(t->b);if(t->disjoint)ID3D11Query_Release(t->disjoint);memset(t,0,sizeof(*t));
}
static void nv_deep_gpu_poll(NvDeepGpu*t,ID3D11DeviceContext*ctx){
 if(!t->pending||t->failed||!ctx)return;
 D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d={0};UINT64 a=0,b=0;
 HRESULT hr=ID3D11DeviceContext_GetData(ctx,(ID3D11Asynchronous*)t->disjoint,&d,sizeof(d),D3D11_ASYNC_GETDATA_DONOTFLUSH);
 if(hr==S_FALSE)return;if(FAILED(hr))goto fail;
 hr=ID3D11DeviceContext_GetData(ctx,(ID3D11Asynchronous*)t->b,&b,sizeof(b),D3D11_ASYNC_GETDATA_DONOTFLUSH);
 if(hr==S_FALSE)return;if(FAILED(hr))goto fail;
 hr=ID3D11DeviceContext_GetData(ctx,(ID3D11Asynchronous*)t->a,&a,sizeof(a),D3D11_ASYNC_GETDATA_DONOTFLUSH);
 if(hr==S_FALSE)return;if(FAILED(hr))goto fail;
 t->pending=0;++t->completed;
 fprintf(stderr,"[DEEP-GPU] burst=%u frame=%u valid=%u ms=%.6f issued=%u completed=%u skipped=%u\n",t->burst,t->frame,!d.Disjoint&&d.Frequency&&b>=a,(!d.Disjoint&&d.Frequency&&b>=a)?(b-a)*1000.0/d.Frequency:0,t->issued,t->completed,t->skipped);return;
fail:t->failed=1;fprintf(stderr,"[DEEP-GPU-ERROR] hr=%08lX burst=%u frame=%u\n",hr,t->burst,t->frame);
}
static void nv_deep_gpu_begin(NvDeepGpu*t,ID3D11Device*dev,ID3D11DeviceContext*ctx,unsigned burst,unsigned frame){
 if(t->open||t->failed||!dev||!ctx||t->issued>=128)return;
 if(t->pending){++t->skipped;return;}
 if(!t->a){D3D11_QUERY_DESC q={D3D11_QUERY_TIMESTAMP,0};
  if(FAILED(ID3D11Device_CreateQuery(dev,&q,&t->a))||FAILED(ID3D11Device_CreateQuery(dev,&q,&t->b)))goto fail;
  q.Query=D3D11_QUERY_TIMESTAMP_DISJOINT;if(FAILED(ID3D11Device_CreateQuery(dev,&q,&t->disjoint)))goto fail;}
 t->burst=burst;t->frame=frame;t->open=1;++t->issued;
 ID3D11DeviceContext_Begin(ctx,(ID3D11Asynchronous*)t->disjoint);ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)t->a);return;
fail:t->failed=1;fprintf(stderr,"[DEEP-GPU-ERROR] query_creation_failed=1\n");
}
static void nv_deep_gpu_end(NvDeepGpu*t,ID3D11DeviceContext*ctx){
 if(!t->open||!ctx)return;ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)t->b);ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)t->disjoint);t->open=0;t->pending=1;
}
#endif
