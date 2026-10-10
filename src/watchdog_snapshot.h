#ifndef UCRR_WATCHDOG_SNAPSHOT_H
#define UCRR_WATCHDOG_SNAPSHOT_H
#include <windows.h>
#include <stdint.h>
typedef struct UCRR_STACK_SNAPSHOT {
    CONTEXT context;
    uintptr_t base;
    SIZE_T size;
    unsigned char bytes[65536];
} UCRR_STACK_SNAPSHOT;

/* No CRT, allocation, symbol lookup or logging while the target is suspended:
 * the target may own exactly those locks. Capture bounded bytes and resume
 * before formatting or unwinding. The caller allocates storage beforehand. */
static int ucrr_capture_stack(HANDLE process,HANDLE thread,UCRR_STACK_SNAPSHOT *s)
{
    MEMORY_BASIC_INFORMATION region;
    s->size=0;
    s->context.ContextFlags=CONTEXT_FULL;
    if(SuspendThread(thread)==(DWORD)-1)return 0;
    int ok=GetThreadContext(thread,&s->context)!=0;
    if(ok){
        s->base=(uintptr_t)s->context.Rsp;
        if(VirtualQueryEx(process,(void *)s->base,&region,sizeof(region)) &&
           region.State==MEM_COMMIT && !(region.Protect&(PAGE_GUARD|PAGE_NOACCESS))){
            SIZE_T available=(uintptr_t)region.BaseAddress+region.RegionSize-s->base;
            SIZE_T amount=available<sizeof(s->bytes)?available:sizeof(s->bytes);
            SIZE_T received=0;
            if(ReadProcessMemory(process,(void *)s->base,s->bytes,amount,&received))s->size=received;
        }
    }
    ResumeThread(thread);
    return ok;
}
#endif
