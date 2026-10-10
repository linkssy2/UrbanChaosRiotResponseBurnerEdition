#ifndef NV2A_SEMAPHORES_H
#define NV2A_SEMAPHORES_H
#include "nv2a_dma_resolve.h"
#include <stdlib.h>
#include <string.h>
#ifndef NV_COMPLETED_RAM_WRITE
#define NV_COMPLETED_RAM_WRITE(ram,size,address,bytes) ((void)0)
#endif
#define NV_SEMAPHORE_QUEUE 256
#ifndef NV_QUERY_ADMISSION_DEFER
#define NV_QUERY_ADMISSION_DEFER() 0
#endif
#ifndef NV_QUERY_GET_DATA
#define NV_QUERY_GET_DATA ID3D11DeviceContext_GetData
#define NV_QUERY_FLUSH ID3D11DeviceContext_Flush
#endif
typedef struct NvSemaphoreItem {ID3D11Query *query;uint32_t address,value;unsigned submitted;} NvSemaphoreItem;
#ifndef NV_SEMAPHORE_FLUSH
#define NV_SEMAPHORE_FLUSH NV_QUERY_FLUSH
#endif
#ifndef NV_QUERY_SUBMIT_ALLOWED
#define NV_QUERY_SUBMIT_ALLOWED(ctx,origin) 1
#endif
typedef struct NvSemaphores {
    NvSemaphoreItem items[NV_SEMAPHORE_QUEUE];
    unsigned head,count,failed,completed;
    uint64_t prefix_probes,prefix_completed,prefix_saved_polls;
} NvSemaphores;
static int nv_semaphore_prefix_option=-1;
static int nv_semaphore_prefix_enabled(void){
    if(nv_semaphore_prefix_option<0){const char*v=getenv("UCRR_SEMAPHORE_PREFIX");nv_semaphore_prefix_option=v&&!strcmp(v,"1");}
    return nv_semaphore_prefix_option;
}
/* Graphics objects have RAMHT engine=1; DMA objects use engine=0. */
static int nv_graphics_class(const uint8_t *regs,size_t size,uint32_t handle,uint32_t *klass){
    if(!regs||size<0x800000)return 0;
    uint32_t cfg=nv2a_dma_u32(regs+0x2210),base=0x700000+((cfg&0x1F0)<<8);
    uint32_t bytes=4096u<<((cfg>>16)&3);int found=0;
    if((uint64_t)base+bytes>size)return 0;
    for(uint32_t i=0;i<bytes;i+=8){
        uint32_t key=nv2a_dma_u32(regs+base+i),ctx=nv2a_dma_u32(regs+base+i+4);
        if(key!=handle||!(ctx&0x80000000u)||(ctx&0x1F030000u)!=0x10000u)continue;
        uint32_t object=0x700000+((ctx&0xFFFF)<<4);
        if(found||(uint64_t)object+4>size)return 0;
        *klass=nv2a_dma_u32(regs+object)&0xFFu;found=1;
    }
    return found;
}
/* Owner-thread immediate context: a flush covers all End calls already queued,
 * but does not authorize completion or cover a later issue. */
static void nv_semaphore_submit_pending(NvSemaphores *s,ID3D11DeviceContext *ctx){
    NV_SEMAPHORE_FLUSH(ctx);
    for(unsigned i=0;i<s->count;++i)s->items[(s->head+i)%NV_SEMAPHORE_QUEUE].submitted=1;
}
static void nv_semaphore_poll_mode(NvSemaphores *s,ID3D11DeviceContext *ctx,uint8_t *ram,size_t size,int submit){
    unsigned covered=0,probed=0;
    while(s->count&&!s->failed&&ctx&&ram){
        NvSemaphoreItem *item=&s->items[s->head];BOOL done=FALSE;
        HRESULT hr;
        if(covered){--covered;++s->prefix_saved_polls;hr=S_OK;done=TRUE;}
        else hr=NV_QUERY_GET_DATA(ctx,(ID3D11Asynchronous*)item->query,&done,sizeof(done),D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(hr==S_FALSE||!done){
            if(FAILED(hr))s->failed=1;
            else if(submit&&!item->submitted&&NV_QUERY_SUBMIT_ALLOWED(ctx,1))nv_semaphore_submit_pending(s,ctx);
            return;
        }
        if(FAILED(hr)||(uint64_t)item->address+4>size){s->failed=1;return;}
        /* A real completed EVENT at the tail covers earlier Ends on this same
         * immediate context. Probe only after the head actually completes, once
         * per call, and never hold up ready head progress on a pending tail.
         * Covered length is local: newly issued or recycled events cannot enter. */
        if(!probed&&s->count>=4&&nv_semaphore_prefix_enabled()){
            probed=1;BOOL tail_done=FALSE;
            NvSemaphoreItem*tail=&s->items[(s->head+s->count-1)%NV_SEMAPHORE_QUEUE];
            ++s->prefix_probes;
            HRESULT tail_hr=NV_QUERY_GET_DATA(ctx,(ID3D11Asynchronous*)tail->query,&tail_done,sizeof(tail_done),D3D11_ASYNC_GETDATA_DONOTFLUSH);
            if(FAILED(tail_hr)){s->failed=1;return;}
            if(tail_hr==S_OK&&tail_done){covered=s->count-1;++s->prefix_completed;}
        }
        /* Completion is tied to the actual ordered host EVENT, never PUT. */
        InterlockedExchange((volatile LONG*)(ram+item->address),(LONG)item->value);
        NV_COMPLETED_RAM_WRITE(ram,size,item->address,4);
        ++s->completed;s->head=(s->head+1)%NV_SEMAPHORE_QUEUE;--s->count;
    }
}
static void nv_semaphore_poll(NvSemaphores*s,ID3D11DeviceContext*ctx,uint8_t*ram,size_t size){nv_semaphore_poll_mode(s,ctx,ram,size,1);}
static int nv_semaphore_issue(NvSemaphores *s,ID3D11Device *dev,ID3D11DeviceContext *ctx,
    uint8_t *ram,size_t size,uint32_t address,uint32_t value){
    if(!dev||!ctx||!ram||(address&3)||(uint64_t)address+4>size)return 0;
    nv_semaphore_poll_mode(s,ctx,ram,size,!NV_QUERY_ADMISSION_DEFER());
    if(s->count==NV_SEMAPHORE_QUEUE){
        ULONGLONG deadline=GetTickCount64()+2000;nv_semaphore_submit_pending(s,ctx);
        while(s->count==NV_SEMAPHORE_QUEUE&&!s->failed&&GetTickCount64()<deadline){nv_semaphore_poll(s,ctx,ram,size);if(s->count==NV_SEMAPHORE_QUEUE)Sleep(0);}
    }
    if(s->failed||s->count==NV_SEMAPHORE_QUEUE){s->failed=1;return 0;}
    NvSemaphoreItem *item=&s->items[(s->head+s->count)%NV_SEMAPHORE_QUEUE];
    if(!item->query){D3D11_QUERY_DESC d={D3D11_QUERY_EVENT,0};if(FAILED(ID3D11Device_CreateQuery(dev,&d,&item->query))){s->failed=1;return 0;}}
    item->address=address;item->value=value;item->submitted=0;
    ID3D11DeviceContext_End(ctx,(ID3D11Asynchronous*)item->query);++s->count;return 1;
}
static void nv_semaphore_shutdown(NvSemaphores *s){
    for(unsigned i=0;i<NV_SEMAPHORE_QUEUE;++i)if(s->items[i].query)ID3D11Query_Release(s->items[i].query);
    memset(s,0,sizeof(*s));
}
#endif
