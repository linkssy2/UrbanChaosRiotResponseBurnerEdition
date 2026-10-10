#ifndef NV2A_DEPENDENCY_PROBE_H
#define NV2A_DEPENDENCY_PROBE_H
/* Bounded diagnostic events, never a replay or permission to remove barriers.
 * Included after g_pg. F10 starts capture at a drain boundary on its owner. */
typedef struct NvDependencyEvent {
    uint64_t qpc,id,value;
    uint32_t kind,frame,reason,query,thread,reserved;
} NvDependencyEvent;
#define NV_DEPENDENCY_CAPACITY 131072u
static volatile LONG nv_dependency_mark_requested;
void pgraph_d3d11_dependency_mark(void){InterlockedExchange(&nv_dependency_mark_requested,1);}
uint64_t pgraph_d3d11_dependency_event(unsigned kind,uint64_t id,uint64_t value){
    static int enabled=-1,finished;
    static unsigned count,owner;
    static volatile LONG foreign_thread;
    static uint64_t sequence;
    static NvDependencyEvent*events;
    if(enabled<0){const char*x=getenv("UCRR_DEPENDENCY_PROBE");enabled=x&&!strcmp(x,"1");}
    if(!enabled||finished)return 0;
    if(!events){
        if(kind!=1)return 0;
        if(!InterlockedExchange(&nv_dependency_mark_requested,0))return 0;
        events=(NvDependencyEvent*)calloc(NV_DEPENDENCY_CAPACITY,sizeof(*events));
        if(!events){finished=1;return 0;}owner=GetCurrentThreadId();
    }
    /* Unexpected concurrent ownership cannot produce a valid trace. */
    if(owner!=GetCurrentThreadId()){InterlockedExchange(&foreign_thread,1);return 0;}
    if(kind==1||kind==3)id=++sequence;
    LARGE_INTEGER now;QueryPerformanceCounter(&now);
    NvDependencyEvent*e=&events[count++];e->qpc=(uint64_t)now.QuadPart;e->id=id;e->value=value;
    e->kind=kind;e->frame=g_pg.stats.frames;e->reason=work_probe.reason;
    e->query=g_pg.reports.active!=NULL;e->thread=owner;
    if(count==NV_DEPENDENCY_CAPACITY){
        finished=1;
        char path[96];snprintf(path,sizeof(path),"dependency-probe-%lu.bin",GetCurrentProcessId());
        FILE*f=fopen(path,"wb");
        if(f){const char*mixed=getenv("UCRR_MIXED_COMMAND_PROBE");
            uint32_t header[6]={0x44504355u,(mixed&&!strcmp(mixed,"1"))?2u:1u,sizeof(*events),count,owner,(uint32_t)InterlockedCompareExchange(&foreign_thread,0,0)};LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
            uint64_t hz=(uint64_t)frequency.QuadPart;
            size_t h=fwrite(header,sizeof(header),1,f),q=fwrite(&hz,sizeof(hz),1,f),n=fwrite(events,sizeof(*events),count,f);
            int result=fclose(f);if(h!=1||q!=1||n!=count||result)fprintf(stderr,"[DEPENDENCY-PROBE] incomplete write\n");
        }
        free(events);events=NULL;
    }
    return id;
}
#endif
