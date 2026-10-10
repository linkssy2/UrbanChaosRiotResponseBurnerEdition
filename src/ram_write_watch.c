/* Experimental physical-resource tracker. The second instance uses this same
 * implementation in a separate translation unit; overlapping ranges reject. */
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "ram_write_watch.h"
#include "xbox_memory_layout.h"
#include "certificate_reads.h"
#ifdef UCRR_RESOURCE_REVISIONS
#include "resource_revisions.h"
#endif
#define UC_WATCH_VIEWS 32
enum { UC_WATCH_COLD=-1,UC_WATCH_DISABLED=0,UC_WATCH_ACTIVE=1,UC_WATCH_DISABLING=2 };
static struct {
    SRWLOCK gate;
    volatile LONG state,handlers,failed;
    DECLSPEC_ALIGN(8) volatile LONG64 sequence;
    unsigned count,native_inflight;
    size_t offset,bytes,page_size;
    uint8_t*views[UC_WATCH_VIEWS];
    void*handler;
} uc_watch={SRWLOCK_INIT,UC_WATCH_COLD};
static LONG uc_watch_state(void){return InterlockedCompareExchange(&uc_watch.state,0,0);}
static uint64_t uc_watch_sequence(void){return (uint64_t)InterlockedCompareExchange64(&uc_watch.sequence,0,0);}
static LONG CALLBACK uc_watch_fault(EXCEPTION_POINTERS*p){
    EXCEPTION_RECORD*r=p->ExceptionRecord;
    if(r->ExceptionCode!=EXCEPTION_ACCESS_VIOLATION||r->NumberParameters<2||r->ExceptionInformation[0]!=1)return EXCEPTION_CONTINUE_SEARCH;
    uintptr_t address=r->ExceptionInformation[1];
    for(unsigned i=0;i<uc_watch.count;++i){uintptr_t base=(uintptr_t)uc_watch.views[i];
        if(address<base||address-base>=uc_watch.bytes)continue;
        DWORD saved=GetLastError();InterlockedIncrement(&uc_watch.handlers);
        LONG state=uc_watch_state();BOOL retry=FALSE;
        if(state==UC_WATCH_ACTIVE){
            InterlockedIncrement64(&uc_watch.sequence);DWORD old;
            retry=VirtualProtect((void*)(address&~(uintptr_t)(uc_watch.page_size-1)),uc_watch.page_size,PAGE_READWRITE,&old);
            if(!retry)InterlockedExchange(&uc_watch.failed,1);
        }else if(state==UC_WATCH_DISABLING){retry=TRUE;}
        else{
            MEMORY_BASIC_INFORMATION info;
            retry=VirtualQuery((void*)address,&info,sizeof(info))&&info.State==MEM_COMMIT&&info.Protect==PAGE_READWRITE;
        }
        InterlockedDecrement(&uc_watch.handlers);SetLastError(saved);
        return retry?EXCEPTION_CONTINUE_EXECUTION:EXCEPTION_CONTINUE_SEARCH;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
/* Caller owns gate. No handler ever acquires it. Metadata remains immutable. */
static int uc_watch_disable_locked(void){
    if(uc_watch_state()==UC_WATCH_COLD)return 1;
    if(uc_watch_state()==UC_WATCH_DISABLED)return !uc_watch.failed;
    InterlockedExchange(&uc_watch.state,UC_WATCH_DISABLING);InterlockedIncrement64(&uc_watch.sequence);
    while(InterlockedCompareExchange(&uc_watch.handlers,0,0))SwitchToThread();
    int ok=1;
    for(unsigned i=0;i<uc_watch.count;++i){DWORD old;if(!VirtualProtect(uc_watch.views[i],uc_watch.bytes,PAGE_READWRITE,&old))ok=0;}
    if(!ok)InterlockedExchange(&uc_watch.failed,1);
    InterlockedExchange(&uc_watch.state,UC_WATCH_DISABLED);return ok&&!uc_watch.failed;
}
static int uc_watch_writable_mapping(uint8_t*base,size_t bytes){
    uintptr_t p=(uintptr_t)base,end=p+bytes;if(end<p)return 0;
    while(p<end){MEMORY_BASIC_INFORMATION info;
        if(!VirtualQuery((void*)p,&info,sizeof(info))||info.State!=MEM_COMMIT||info.Type!=MEM_MAPPED||info.Protect!=PAGE_READWRITE)return 0;
        uintptr_t next=(uintptr_t)info.BaseAddress+info.RegionSize;if(next<=p)return 0;
        p=next<end?next:end;
    }return 1;
}
int uc_ram_watch_start(size_t offset,size_t bytes){
    int result=0;DWORD saved=GetLastError();AcquireSRWLockExclusive(&uc_watch.gate);
    LONG state=uc_watch_state();
    if(state==UC_WATCH_ACTIVE){result=uc_watch.offset==offset&&uc_watch.bytes==bytes;goto done;}
    if(state!=UC_WATCH_COLD)goto done;
    SYSTEM_INFO sys;GetSystemInfo(&sys);
    if(sys.dwPageSize!=4096||!bytes||bytes>1024u*1024u||(offset&4095u)||(bytes&4095u)||offset>SIZE_MAX-bytes)goto done;
#ifdef UCRR_RESOURCE_REVISIONS
    if(uc_resource_overlaps(offset,bytes))goto done;
#endif
#ifdef UC_WATCH_SECONDARY
    if(uc_ram_watch_primary_overlaps(offset,bytes))goto done;
#elif defined(UCRR_RAM_WATCH_SHADOW)
    if(uc_ram_watch_second_overlaps(offset,bytes))goto done;
#endif
    XboxMemoryAliasView aliases[UC_WATCH_VIEWS];size_t count=xbox_GetRamAliasViews(aliases,UC_WATCH_VIEWS);
    if(!count||count>UC_WATCH_VIEWS)goto done;
    uint8_t*selected[UC_WATCH_VIEWS];unsigned used=0;
    for(size_t i=0;i<count;++i){XboxMemoryAliasView*a=aliases+i;
        if(!a->base||a->section_offset>SIZE_MAX-a->size)goto done;
        if(offset+bytes<=a->section_offset||offset>=a->section_offset+a->size)continue;
        /* Partial aliases cannot be silently dropped. */
        if(offset<a->section_offset||bytes>a->size-(offset-a->section_offset))goto done;
        size_t delta=offset-a->section_offset;
        if((uintptr_t)a->base>UINTPTR_MAX-delta)goto done;
        uint8_t*address=(uint8_t*)((uintptr_t)a->base+delta);
        if(((uintptr_t)address&4095u)||!uc_watch_writable_mapping(address,bytes))goto done;
        selected[used++]=address;
    }
    if(!used)goto done;
    uc_watch.offset=offset;uc_watch.bytes=bytes;uc_watch.page_size=sys.dwPageSize;
    memcpy(uc_watch.views,selected,used*sizeof(selected[0]));uc_watch.count=used;
    uc_watch.handler=AddVectoredExceptionHandler(1,uc_watch_fault);
    if(!uc_watch.handler){uc_watch.count=0;goto done;}
    InterlockedExchange64(&uc_watch.sequence,1);InterlockedExchange(&uc_watch.state,UC_WATCH_ACTIVE);result=1;
done:ReleaseSRWLockExclusive(&uc_watch.gate);SetLastError(saved);return result;
}
int uc_ram_watch_matches(size_t offset,size_t bytes){return uc_watch_state()==UC_WATCH_ACTIVE&&uc_watch.offset==offset&&uc_watch.bytes==bytes;}
int uc_ram_watch_contains(size_t offset,size_t bytes){
    return uc_watch_state()==UC_WATCH_ACTIVE&&!InterlockedCompareExchange(&uc_watch.failed,0,0)&&
        bytes&&offset>=uc_watch.offset&&offset-uc_watch.offset<=uc_watch.bytes&&
        bytes<=uc_watch.bytes-(offset-uc_watch.offset);
}
int uc_ram_watch_overlaps(size_t offset,size_t bytes){return uc_watch_state()==UC_WATCH_ACTIVE&&bytes&&offset<=SIZE_MAX-bytes&&offset<uc_watch.offset+uc_watch.bytes&&uc_watch.offset<offset+bytes;}
int uc_ram_watch_clean(uint64_t token){
#if UCRR_CERTIFICATE_READS
    if(!token)return 0;
    uc_certificate_fence();
    int clean=token&&uc_certificate_read32(&uc_watch.state)==UC_WATCH_ACTIVE&&
        !uc_certificate_read32(&uc_watch.handlers)&&(uint64_t)uc_certificate_read64(&uc_watch.sequence)==token;
    uc_certificate_fence();return clean;
#else
    return token&&uc_watch_state()==UC_WATCH_ACTIVE&&!InterlockedCompareExchange(&uc_watch.handlers,0,0)&&uc_watch_sequence()==token;
#endif
}
uint64_t uc_ram_watch_arm(void){
    uint64_t token=0;DWORD saved=GetLastError();AcquireSRWLockExclusive(&uc_watch.gate);
    if(uc_watch_state()!=UC_WATCH_ACTIVE||uc_watch.native_inflight)goto done;
    token=uc_watch_sequence();
    /* Sequence must precede active-handler check: otherwise a handler can
     * publish a new sequence before Arm samples it, then unlock after rearm. */
    if(InterlockedCompareExchange(&uc_watch.handlers,0,0)){token=0;goto done;}
    for(unsigned i=0;i<uc_watch.count;++i){DWORD old;
        if(!VirtualProtect(uc_watch.views[i],uc_watch.bytes,PAGE_READONLY,&old)){
            uc_watch_disable_locked();token=0;goto done;
        }
    }
    if(!uc_ram_watch_clean(token))token=0;
done:ReleaseSRWLockExclusive(&uc_watch.gate);SetLastError(saved);return token;
}
static int uc_watch_overlap(const void*buffer,size_t bytes,unsigned i){
    uintptr_t p=(uintptr_t)buffer,v=(uintptr_t)uc_watch.views[i];
    return bytes&&p<=UINTPTR_MAX-bytes&&v<=UINTPTR_MAX-uc_watch.bytes&&p<v+uc_watch.bytes&&v<p+bytes;
}
static int uc_watch_native_begin_single(void*buffer,size_t bytes){
    LONG state=uc_watch_state();if(state==UC_WATCH_COLD||state==UC_WATCH_DISABLED||!bytes)return 0;
    int overlap=0;for(unsigned i=0;i<uc_watch.count;++i)overlap|=uc_watch_overlap(buffer,bytes,i);
    if(!overlap)return 0;
    DWORD saved=GetLastError();int entered=0;AcquireSRWLockExclusive(&uc_watch.gate);
    if(uc_watch_state()==UC_WATCH_ACTIVE){
        ++uc_watch.native_inflight;InterlockedIncrement64(&uc_watch.sequence);entered=1;
        for(unsigned i=0;i<uc_watch.count;++i)if(uc_watch_overlap(buffer,bytes,i)){
            DWORD old;if(!VirtualProtect(uc_watch.views[i],uc_watch.bytes,PAGE_READWRITE,&old)){uc_watch_disable_locked();break;}
        }
    }
    ReleaseSRWLockExclusive(&uc_watch.gate);SetLastError(saved);return entered;
}
static void uc_watch_native_end_single(int entered){
    if(!entered)return;DWORD saved=GetLastError();AcquireSRWLockExclusive(&uc_watch.gate);
    if(uc_watch.native_inflight)--uc_watch.native_inflight;else InterlockedExchange(&uc_watch.failed,1);
    ReleaseSRWLockExclusive(&uc_watch.gate);SetLastError(saved);
}
static int uc_watch_disable_single(void){
    DWORD saved=GetLastError();AcquireSRWLockExclusive(&uc_watch.gate);int ok=uc_watch_disable_locked();
    ReleaseSRWLockExclusive(&uc_watch.gate);SetLastError(saved);return ok;
}
static int uc_watch_shutdown_single(void){
    int ok=uc_watch_disable_single();DWORD saved=GetLastError();AcquireSRWLockExclusive(&uc_watch.gate);
    if(uc_watch.handler){if(!RemoveVectoredExceptionHandler(uc_watch.handler))ok=0;uc_watch.handler=NULL;}
    ReleaseSRWLockExclusive(&uc_watch.gate);SetLastError(saved);return ok;
}
int uc_ram_watch_native_begin(void*buffer,size_t bytes){
    int entered=uc_watch_native_begin_single(buffer,bytes);
#if defined(UCRR_RAM_WATCH_SHADOW) && !defined(UC_WATCH_SECONDARY)
    entered|=uc_ram_watch_second_native_begin(buffer,bytes)<<1;
#ifdef UCRR_RESOURCE_REVISIONS
    entered|=uc_resource_native_begin(buffer,bytes)<<2;
#endif
#endif
    return entered;
}
void uc_ram_watch_native_end(int entered){
    uc_watch_native_end_single(entered&1);
#if defined(UCRR_RAM_WATCH_SHADOW) && !defined(UC_WATCH_SECONDARY)
    uc_ram_watch_second_native_end((entered>>1)&1);
#ifdef UCRR_RESOURCE_REVISIONS
    uc_resource_native_end((entered>>2)&1);
#endif
#endif
}
int uc_ram_watch_disable(void){
    int ok=uc_watch_disable_single();
#if defined(UCRR_RAM_WATCH_SHADOW) && !defined(UC_WATCH_SECONDARY)
    ok=uc_ram_watch_second_disable()&&ok;
#ifdef UCRR_RESOURCE_REVISIONS
    ok=uc_resource_disable()&&ok;
#endif
#endif
    return ok;
}
int uc_ram_watch_shutdown(void){
    int ok=uc_watch_shutdown_single();
#if defined(UCRR_RAM_WATCH_SHADOW) && !defined(UC_WATCH_SECONDARY)
    ok=uc_ram_watch_second_shutdown()&&ok;
#ifdef UCRR_RESOURCE_REVISIONS
    ok=uc_resource_shutdown()&&ok;
#endif
#endif
    return ok;
}
int uc_ram_watch_failed(void){
    int failed=!!InterlockedCompareExchange(&uc_watch.failed,0,0);
#if defined(UCRR_RAM_WATCH_SHADOW) && !defined(UC_WATCH_SECONDARY)
    failed|=uc_ram_watch_second_failed();
#endif
    return failed;
}
