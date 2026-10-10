#ifndef NV2A_CAUSAL_TRACE_H
#define NV2A_CAUSAL_TRACE_H
#include <stdint.h>
/* Diagnostic only. The default build has no clocks, allocations, or recorder.
 * IDs describe observed operations, not replayable packets or safe removals.
 * Define NV_CAUSAL_TRACE_IMPLEMENTATION in exactly one translation unit. */
#ifndef UCRR_CAUSAL_TRACE
#define UCRR_CAUSAL_TRACE 0
#endif
enum { NV_CAUSAL_DRAIN=1, NV_CAUSAL_DRAW, NV_CAUSAL_PREPARATION,
       NV_CAUSAL_TEXTURE, NV_CAUSAL_LIGHTING, NV_CAUSAL_HOST_DRAW,
       NV_CAUSAL_UPLOAD, NV_CAUSAL_CLEAR, NV_CAUSAL_PRESENT, NV_CAUSAL_OBSERVER };
enum { NV_CAUSAL_BEGIN=1, NV_CAUSAL_END, NV_CAUSAL_RESOURCE, NV_CAUSAL_FRAME };
enum { NV_CAUSAL_TRUNCATED=1, NV_CAUSAL_FOREIGN=2, NV_CAUSAL_BAD_END=4,
       NV_CAUSAL_STACK_FULL=8, NV_CAUSAL_EXPLICIT_FINISH=16, NV_CAUSAL_FRAME_LIMIT_REACHED=32 };
enum { NV_CAUSAL_RES_2D=1, NV_CAUSAL_RES_CUBE=2, NV_CAUSAL_RES_VOLUME=4,
       NV_CAUSAL_RES_SURFACE=8, NV_CAUSAL_RES_NULL=16,
       NV_CAUSAL_RES_ID_UNKNOWN=32, NV_CAUSAL_RES_VERSION_UNKNOWN=64,
       NV_CAUSAL_RES_PROCESS_POINTER=128, NV_CAUSAL_RES_FINAL_BINDING=256,
       NV_CAUSAL_RES_VALIDATION_EPOCH=512, NV_CAUSAL_RES_FAILED=1024 };
typedef struct NvCausalEvent {
    uint64_t qpc,sequence,scope,parent,value,host_identity,content_generation;
    uint32_t kind,category,frame,thread,site,origin,stage,guest_offset,format,flags;
} NvCausalEvent;
typedef struct NvCausalHeader {
    char magic[8];
    uint32_t version,header_size,event_size,capacity,count,owner,flags,open_scopes;
    uint64_t frequency,started_qpc,stopped_qpc,dropped,foreign_events,bad_ends,scope_overflow;
} NvCausalHeader;
#if UCRR_CAUSAL_TRACE
void nv_causal_mark(void);
void nv_causal_set_frame(uint32_t frame);
uint64_t nv_causal_begin(uint32_t category,uint32_t site,uint32_t origin,uint64_t value);
void nv_causal_end(uint64_t token,uint64_t result);
void nv_causal_resource(uint32_t stage,uint32_t guest_offset,uint32_t format,
    uint64_t host_identity,uint64_t content_generation,uint32_t flags);
/* finish and flush are safe-boundary operations. Both refuse active scopes.
 * flush also refuses an active recording. Saturation finishes automatically.
 * Call finish+flush at orderly shutdown; periodic flush never stops a capture. */
void nv_causal_finish(void);
void nv_causal_flush(void);
#else
static __inline void nv_causal_mark(void){}
static __inline void nv_causal_set_frame(uint32_t frame){(void)frame;}
static __inline uint64_t nv_causal_begin(uint32_t category,uint32_t site,uint32_t origin,uint64_t value){(void)category;(void)site;(void)origin;(void)value;return 0;}
static __inline void nv_causal_end(uint64_t token,uint64_t result){(void)token;(void)result;}
static __inline void nv_causal_resource(uint32_t stage,uint32_t offset,uint32_t format,uint64_t identity,uint64_t generation,uint32_t flags){(void)stage;(void)offset;(void)format;(void)identity;(void)generation;(void)flags;}
static __inline void nv_causal_finish(void){}
static __inline void nv_causal_flush(void){}
#endif

#if UCRR_CAUSAL_TRACE && defined(NV_CAUSAL_TRACE_IMPLEMENTATION)
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef NV_CAUSAL_CAPACITY
#define NV_CAUSAL_CAPACITY 1048576u
#endif
#ifndef NV_CAUSAL_FRAME_LIMIT
#define NV_CAUSAL_FRAME_LIMIT 8u
#endif
#ifndef NV_CAUSAL_CLOCK
static uint64_t nv_causal_clock(void){LARGE_INTEGER t;QueryPerformanceCounter(&t);return (uint64_t)t.QuadPart;}
#define NV_CAUSAL_CLOCK() nv_causal_clock()
#endif
#ifndef NV_CAUSAL_FREQUENCY
static uint64_t nv_causal_frequency(void){LARGE_INTEGER t;QueryPerformanceFrequency(&t);return (uint64_t)t.QuadPart;}
#define NV_CAUSAL_FREQUENCY() nv_causal_frequency()
#endif
#ifndef NV_CAUSAL_THREAD
#define NV_CAUSAL_THREAD() ((uint32_t)GetCurrentThreadId())
#endif
typedef struct NvCausalScope {uint64_t id,parent;uint32_t category,site,origin;} NvCausalScope;
static struct {
    int initialized,enabled,active,finished,written;
    volatile LONG mark;
    uint32_t frame,depth,start_frame;
    uint64_t sequence,scope_sequence;
    NvCausalHeader header;
    NvCausalScope stack[32];
    NvCausalEvent *events;
} nv_causal_state;
static INIT_ONCE nv_causal_init_once=INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK nv_causal_initialize(PINIT_ONCE once,PVOID parameter,PVOID*context){
    const char*s=getenv("UCRR_CAUSAL_TRACE");(void)once;(void)parameter;(void)context;
    nv_causal_state.enabled=s&&!strcmp(s,"1");nv_causal_state.initialized=1;return TRUE;
}
static int nv_causal_enabled(void){
    InitOnceExecuteOnce(&nv_causal_init_once,nv_causal_initialize,NULL,NULL);
    return nv_causal_state.enabled;
}
static int nv_causal_owner(void){
    if(!nv_causal_state.events)return 1;
    if(NV_CAUSAL_THREAD()==nv_causal_state.header.owner)return 1;
    /* An unexpected caller invalidates the capture; it never edits owner stack. */
    InterlockedIncrement64((volatile LONG64*)&nv_causal_state.header.foreign_events);
    InterlockedOr((volatile LONG*)&nv_causal_state.header.flags,NV_CAUSAL_FOREIGN);
    return 0;
}
void nv_causal_mark(void){if(nv_causal_enabled())InterlockedExchange(&nv_causal_state.mark,1);}
static void nv_causal_emit(NvCausalEvent e);
void nv_causal_set_frame(uint32_t frame){
    if(!nv_causal_enabled()||!nv_causal_owner())return;
    uint32_t previous=nv_causal_state.frame;nv_causal_state.frame=frame;
    if(nv_causal_state.active&&!nv_causal_state.depth&&frame!=previous){
        NvCausalEvent e={0};e.kind=NV_CAUSAL_FRAME;e.value=previous;nv_causal_emit(e);
        if(nv_causal_state.active&&(uint32_t)(frame-nv_causal_state.start_frame)>=NV_CAUSAL_FRAME_LIMIT){
            nv_causal_state.header.flags|=NV_CAUSAL_FRAME_LIMIT_REACHED;
            nv_causal_state.active=0;nv_causal_state.finished=1;
        }
    }
}
static int nv_causal_start(void){
    NvCausalHeader*h=&nv_causal_state.header;
    nv_causal_state.events=(NvCausalEvent*)calloc(NV_CAUSAL_CAPACITY,sizeof(NvCausalEvent));
    if(!nv_causal_state.events){nv_causal_state.finished=1;return 0;}
    memcpy(h->magic,"UCCAU001",8);h->version=1;h->header_size=sizeof(*h);
    h->event_size=sizeof(NvCausalEvent);h->capacity=NV_CAUSAL_CAPACITY;
    h->owner=NV_CAUSAL_THREAD();h->frequency=NV_CAUSAL_FREQUENCY();
    nv_causal_state.start_frame=nv_causal_state.frame;nv_causal_state.active=1;return 1;
}
static void nv_causal_emit(NvCausalEvent e){
    NvCausalHeader*h=&nv_causal_state.header;
    if(!nv_causal_state.active){if(nv_causal_state.events)++h->dropped;return;}
    e.qpc=NV_CAUSAL_CLOCK();e.sequence=++nv_causal_state.sequence;
    e.frame=nv_causal_state.frame;e.thread=h->owner;
    if(!h->count)h->started_qpc=e.qpc;
    nv_causal_state.events[h->count++]=e;h->stopped_qpc=e.qpc;
    if(e.kind==NV_CAUSAL_BEGIN)++h->open_scopes;
    if(e.kind==NV_CAUSAL_END)--h->open_scopes;
    if(h->count==h->capacity){h->flags|=NV_CAUSAL_TRUNCATED;nv_causal_state.active=0;nv_causal_state.finished=1;}
}
uint64_t nv_causal_begin(uint32_t category,uint32_t site,uint32_t origin,uint64_t value){
    if(!nv_causal_enabled()||!nv_causal_owner())return 0;
    if(!nv_causal_state.events){
        if(nv_causal_state.finished||category!=NV_CAUSAL_DRAIN||
           !InterlockedExchange(&nv_causal_state.mark,0)||!nv_causal_start())return 0;
    }
    if(!nv_causal_state.active){if(nv_causal_state.depth)++nv_causal_state.header.dropped;return 0;}
    if(nv_causal_state.depth==32){++nv_causal_state.header.scope_overflow;
        ++nv_causal_state.header.dropped;nv_causal_state.header.flags|=NV_CAUSAL_STACK_FULL;return 0;}
    NvCausalScope s={++nv_causal_state.scope_sequence,0,category,site,origin};
    if(nv_causal_state.depth)s.parent=nv_causal_state.stack[nv_causal_state.depth-1].id;
    nv_causal_state.stack[nv_causal_state.depth++]=s;
    NvCausalEvent e={0};e.kind=NV_CAUSAL_BEGIN;e.category=category;e.scope=s.id;e.parent=s.parent;
    e.site=site;e.origin=origin;e.value=value;nv_causal_emit(e);return s.id;
}
void nv_causal_end(uint64_t token,uint64_t result){
    if(!token||!nv_causal_state.events||!nv_causal_owner())return;
    if(!nv_causal_state.depth||nv_causal_state.stack[nv_causal_state.depth-1].id!=token){
        ++nv_causal_state.header.bad_ends;nv_causal_state.header.flags|=NV_CAUSAL_BAD_END;return;}
    NvCausalScope s=nv_causal_state.stack[--nv_causal_state.depth];
    NvCausalEvent e={0};e.kind=NV_CAUSAL_END;e.scope=s.id;e.parent=s.parent;e.category=s.category;
    e.site=s.site;e.origin=s.origin;e.value=result;nv_causal_emit(e);
}
void nv_causal_resource(uint32_t stage,uint32_t offset,uint32_t format,uint64_t identity,uint64_t generation,uint32_t flags){
    if(!nv_causal_enabled()||!nv_causal_state.events||nv_causal_state.written||!nv_causal_owner())return;
    if(!nv_causal_state.active){if(nv_causal_state.depth)++nv_causal_state.header.dropped;return;}
    if(!nv_causal_state.depth){++nv_causal_state.header.dropped;return;}
    NvCausalScope s=nv_causal_state.stack[nv_causal_state.depth-1];
    NvCausalEvent e={0};e.kind=NV_CAUSAL_RESOURCE;e.scope=s.id;e.parent=s.parent;e.category=s.category;
    e.site=s.site;e.origin=s.origin;e.stage=stage;e.guest_offset=offset;e.format=format;
    e.host_identity=identity;e.content_generation=generation;
    e.flags=flags|(!identity?NV_CAUSAL_RES_ID_UNKNOWN:0)|(!generation?NV_CAUSAL_RES_VERSION_UNKNOWN:0);
    nv_causal_emit(e);
}
void nv_causal_finish(void){
    if(!nv_causal_state.events||!nv_causal_owner()||nv_causal_state.depth||!nv_causal_state.active)return;
    nv_causal_state.header.flags|=NV_CAUSAL_EXPLICIT_FINISH;
    nv_causal_state.active=0;nv_causal_state.finished=1;
}
void nv_causal_flush(void){
    if(!nv_causal_state.events||!nv_causal_owner()||nv_causal_state.active||nv_causal_state.depth||nv_causal_state.written)return;
    char path[96];snprintf(path,sizeof(path),"causal-trace-%lu.bin",GetCurrentProcessId());
    /* No fopen, fwrite, allocation, or synchronous GPU readback while recording. */
    FILE*f=fopen(path,"wbx");
    if(!f){fprintf(stderr,"[CAUSAL-TRACE] cannot create %s; capture retained\n",path);return;}
    NvCausalHeader*h=&nv_causal_state.header;
    size_t a=fwrite(h,sizeof(*h),1,f),b=fwrite(nv_causal_state.events,sizeof(NvCausalEvent),h->count,f);
    int close_result=fclose(f);
    if(a!=1||b!=h->count||close_result){fprintf(stderr,"[CAUSAL-TRACE] incomplete write %s; capture retained in memory\n",path);return;}
    nv_causal_state.written=1;free(nv_causal_state.events);nv_causal_state.events=NULL;
}
#endif
#endif
