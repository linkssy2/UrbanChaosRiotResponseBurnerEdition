#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "resource_revisions.h"
#include "ram_write_watch.h"
#include "xbox_memory_layout.h"
#include "certificate_reads.h"
#define UC_RESOURCE_SLOTS 256
#define UC_RESOURCE_ALIASES 32
#define UC_RESOURCE_LIGHT_PRIMARY 0x80000001u
#define UC_RESOURCE_LIGHT_SECONDARY 0x80000002u
static int resource_shared_lighting_enabled(void){
    static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_SHARED_LIGHT_REVISIONS");enabled=s&&!strcmp(s,"1");}return enabled;
}
typedef struct ResourceRegion {
    size_t offset,bytes;unsigned views;
    uint8_t*base[UC_RESOURCE_ALIASES];
    DECLSPEC_ALIGN(8) volatile LONG64 revision;
} ResourceRegion;
static struct {
    SRWLOCK gate;volatile LONG count,disabled,handlers,failed,native;
    DECLSPEC_ALIGN(8) volatile LONG64 native_epoch;
    void*handler;ResourceRegion region[UC_RESOURCE_SLOTS];
} resources={SRWLOCK_INIT,0,0,0,0,0,1};
static LONG resource_load(volatile LONG*p){return InterlockedCompareExchange(p,0,0);}
static int resource_overlap(size_t a,size_t n,size_t b,size_t m){
    return n&&m&&a<=SIZE_MAX-n&&b<=SIZE_MAX-m&&a<b+m&&b<a+n;
}
int uc_resource_overlaps(size_t offset,size_t bytes){
    /* Keep reservations even after disable: another tracker cannot re-protect
     * these pages after a guest protection operation changed their contract. */
    LONG count=resource_load(&resources.count);
    for(LONG i=0;i<count;i++)if(resource_overlap(offset,bytes,resources.region[i].offset,resources.region[i].bytes))return 1;
    return 0;
}
static LONG CALLBACK resource_fault(EXCEPTION_POINTERS*p){
    EXCEPTION_RECORD*e=p->ExceptionRecord;
    if(e->ExceptionCode!=EXCEPTION_ACCESS_VIOLATION||e->NumberParameters<2||e->ExceptionInformation[0]!=1)return EXCEPTION_CONTINUE_SEARCH;
    uintptr_t address=e->ExceptionInformation[1];LONG count=resource_load(&resources.count);
    for(LONG i=0;i<count;i++){
        ResourceRegion*r=&resources.region[i];
        for(unsigned a=0;a<r->views;a++){
            uintptr_t base=(uintptr_t)r->base[a];if(address<base||address-base>=r->bytes)continue;
            DWORD saved=GetLastError();InterlockedIncrement(&resources.handlers);BOOL retry=FALSE;
            LONG disabled=resource_load(&resources.disabled);
            if(!disabled){
                InterlockedIncrement64(&r->revision);DWORD old;
                retry=VirtualProtect((void*)(address&~(uintptr_t)4095),4096,PAGE_READWRITE,&old);
                if(!retry)InterlockedExchange(&resources.failed,1);
            }else if(disabled==1)retry=TRUE; /* disable is restoring pages */
            else {MEMORY_BASIC_INFORMATION info;
                retry=VirtualQuery((void*)address,&info,sizeof(info))&&info.State==MEM_COMMIT&&info.Protect==PAGE_READWRITE;
            }
            InterlockedDecrement(&resources.handlers);SetLastError(saved);
            return retry?EXCEPTION_CONTINUE_EXECUTION:EXCEPTION_CONTINUE_SEARCH;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static int resource_writable(uint8_t*base,size_t bytes){
    uintptr_t p=(uintptr_t)base,end=p+bytes;if(end<p)return 0;
    while(p<end){MEMORY_BASIC_INFORMATION info;
        if(!VirtualQuery((void*)p,&info,sizeof(info))||info.State!=MEM_COMMIT||info.Type!=MEM_MAPPED||info.Protect!=PAGE_READWRITE)return 0;
        uintptr_t next=(uintptr_t)info.BaseAddress+info.RegionSize;if(next<=p)return 0;p=next<end?next:end;
    }return 1;
}
static int resource_disable_locked(void){
    if(!resources.count&&!resources.handler)return 1;
    if(resource_load(&resources.disabled)==2)return !resource_load(&resources.failed);
    InterlockedExchange(&resources.disabled,1);
    while(resource_load(&resources.handlers))SwitchToThread();
    int ok=1;LONG count=resource_load(&resources.count);
    for(LONG i=0;i<count;i++){ResourceRegion*r=&resources.region[i];InterlockedIncrement64(&r->revision);
        for(unsigned a=0;a<r->views;a++){DWORD old;if(!VirtualProtect(r->base[a],r->bytes,PAGE_READWRITE,&old))ok=0;}
    }
    if(!ok)InterlockedExchange(&resources.failed,1);
    InterlockedExchange(&resources.disabled,2);return ok&&!resource_load(&resources.failed);
}
UcResourceRegistration uc_resource_register_result(size_t offset,size_t bytes){
    UcResourceRegistration result={0,UC_RESOURCE_UNSUPPORTED_MAPPING,0};
    DWORD saved=GetLastError();AcquireSRWLockExclusive(&resources.gate);
    result.native_epoch=(uint64_t)InterlockedCompareExchange64(&resources.native_epoch,0,0);
    if(resource_load(&resources.disabled)){result.status=UC_RESOURCE_DISABLED;goto done;}
    if(resource_load(&resources.failed)){result.status=UC_RESOURCE_FAILED;goto done;}
    if(resources.native){result.status=UC_RESOURCE_NATIVE_BUSY;goto done;}
    if(!bytes||offset>SIZE_MAX-bytes||offset+bytes>SIZE_MAX-4095){result.status=UC_RESOURCE_INVALID_RANGE;goto done;}
    size_t start=offset&~(size_t)4095,end=(offset+bytes+4095)&~(size_t)4095;
    if(end-start>1024u*1024u){result.status=UC_RESOURCE_TOO_LARGE;goto done;}
    LONG count=resource_load(&resources.count);
    for(LONG i=0;i<count;i++){
        ResourceRegion*r=&resources.region[i];
        if(start>=r->offset&&end<=r->offset+r->bytes){result.id=(unsigned)i+1;result.status=UC_RESOURCE_REGISTERED;goto done;}
        if(resource_overlap(start,end-start,r->offset,r->bytes)){result.status=UC_RESOURCE_PARTIAL_OVERLAP;goto done;}
    }
    if(count==UC_RESOURCE_SLOTS){result.status=UC_RESOURCE_CAPACITY;goto done;}
    if(uc_ram_watch_overlaps(start,end-start)||uc_ram_watch_second_overlaps(start,end-start)){
        result.status=UC_RESOURCE_LIGHTING_OVERLAP;
        /* Borrow the existing owner; never install competing protections.
         * Texture equality/token publication remains in texture_revision_equal. */
        if(resource_shared_lighting_enabled()){
            if(uc_ram_watch_contains(start,end-start))result.id=UC_RESOURCE_LIGHT_PRIMARY;
            else if(uc_ram_watch_second_contains(start,end-start))result.id=UC_RESOURCE_LIGHT_SECONDARY;
            if(result.id)result.status=UC_RESOURCE_REGISTERED;
        }
        goto done;
    }
    SYSTEM_INFO sys;GetSystemInfo(&sys);if(sys.dwPageSize!=4096)goto done;
    XboxMemoryAliasView aliases[UC_RESOURCE_ALIASES];size_t n=xbox_GetRamAliasViews(aliases,UC_RESOURCE_ALIASES);
    if(!n||n>UC_RESOURCE_ALIASES)goto done;
    ResourceRegion*r=&resources.region[count];r->offset=start;r->bytes=end-start;r->views=0;
    for(size_t a=0;a<n;a++){
        XboxMemoryAliasView*v=&aliases[a];
        if(!v->base||v->section_offset>SIZE_MAX-v->size)goto done;
        if(!resource_overlap(start,end-start,v->section_offset,v->size))continue;
        if(start<v->section_offset||end>v->section_offset+v->size)goto done;
        size_t delta=start-v->section_offset;
        if((uintptr_t)v->base>UINTPTR_MAX-delta)goto done;
        uint8_t*base=(uint8_t*)((uintptr_t)v->base+delta);
        if(((uintptr_t)base&4095)||!resource_writable(base,r->bytes))goto done;
        r->base[r->views++]=base;
    }
    if(!r->views)goto done;
    if(!resources.handler){resources.handler=AddVectoredExceptionHandler(1,resource_fault);if(!resources.handler){result.status=UC_RESOURCE_HANDLER_FAILED;goto done;}}
    InterlockedExchange64(&r->revision,1);InterlockedExchange(&resources.count,count+1);result.id=(unsigned)count+1;result.status=UC_RESOURCE_REGISTERED;
done:ReleaseSRWLockExclusive(&resources.gate);SetLastError(saved);return result;
}
unsigned uc_resource_register(size_t offset,size_t bytes){return uc_resource_register_result(offset,bytes).id;}
unsigned uc_resource_find_existing(size_t offset,size_t bytes){
    if(!bytes||offset>SIZE_MAX-bytes||!uc_resource_native_epoch())return 0;
    unsigned id=0;DWORD saved=GetLastError();AcquireSRWLockShared(&resources.gate);
    if(resources.disabled||resources.failed||resources.native)goto done;
    for(LONG i=0;i<resources.count;i++){
        ResourceRegion*r=&resources.region[i];
        if(offset>=r->offset&&offset-r->offset<=r->bytes&&bytes<=r->bytes-(offset-r->offset)){
            id=(unsigned)i+1;break;
        }
    }
    if(!id&&resource_shared_lighting_enabled()){
        if(uc_ram_watch_contains(offset,bytes))id=UC_RESOURCE_LIGHT_PRIMARY;
        else if(uc_ram_watch_second_contains(offset,bytes))id=UC_RESOURCE_LIGHT_SECONDARY;
    }
done:ReleaseSRWLockShared(&resources.gate);SetLastError(saved);return id;
}
uint64_t uc_resource_native_epoch(void){
#if UCRR_CERTIFICATE_READS
    uc_certificate_fence();
    uint64_t epoch=(uint64_t)uc_certificate_read64(&resources.native_epoch);
    if(uc_certificate_read32(&resources.disabled)||uc_certificate_read32(&resources.failed)||uc_certificate_read32(&resources.native))epoch=0;
    uc_certificate_fence();return epoch;
#else
    uint64_t epoch=(uint64_t)InterlockedCompareExchange64(&resources.native_epoch,0,0);
    if(resource_load(&resources.disabled)||resource_load(&resources.failed)||resource_load(&resources.native))return 0;
    return epoch;
#endif
}
int uc_resource_clean(unsigned id,uint64_t token){
    if(id==UC_RESOURCE_LIGHT_PRIMARY||id==UC_RESOURCE_LIGHT_SECONDARY){
        if(!token||!uc_resource_native_epoch())return 0;
        return !uc_ram_watch_failed()&&(id==UC_RESOURCE_LIGHT_PRIMARY?uc_ram_watch_clean(token):uc_ram_watch_second_clean(token));
    }
#if UCRR_CERTIFICATE_READS
    if(!token||!id)return 0; /* Original rejection performs no shared-memory read. */
    uc_certificate_fence();
    int clean=token&&id&&id<=(unsigned)uc_certificate_read32(&resources.count)&&
        !uc_certificate_read32(&resources.disabled)&&!uc_certificate_read32(&resources.failed)&&
        !uc_certificate_read32(&resources.handlers)&&!uc_certificate_read32(&resources.native)&&
        (uint64_t)uc_certificate_read64(&resources.region[id-1].revision)==token;
    uc_certificate_fence();return clean;
#else
    if(!token||!id||id>(unsigned)resource_load(&resources.count)||resource_load(&resources.disabled)||
        resource_load(&resources.failed)||resource_load(&resources.handlers)||resource_load(&resources.native))return 0;
    return (uint64_t)InterlockedCompareExchange64(&resources.region[id-1].revision,0,0)==token;
#endif
}
uint64_t uc_resource_arm(unsigned id){
    if(id==UC_RESOURCE_LIGHT_PRIMARY||id==UC_RESOURCE_LIGHT_SECONDARY){
        uint64_t epoch=uc_resource_native_epoch();if(!epoch||uc_ram_watch_failed())return 0;
        uint64_t token=id==UC_RESOURCE_LIGHT_PRIMARY?uc_ram_watch_arm():uc_ram_watch_second_arm();
        return epoch==uc_resource_native_epoch()&&uc_resource_clean(id,token)?token:0;
    }
    uint64_t token=0;DWORD saved=GetLastError();AcquireSRWLockExclusive(&resources.gate);
    if(!id||id>(unsigned)resources.count||resources.disabled||resources.failed||resources.native)goto done;
    ResourceRegion*r=&resources.region[id-1];token=(uint64_t)InterlockedCompareExchange64(&r->revision,0,0);
    if(resource_load(&resources.handlers)){token=0;goto done;}
    for(unsigned a=0;a<r->views;a++){DWORD old;
        if(!VirtualProtect(r->base[a],r->bytes,PAGE_READONLY,&old)){resource_disable_locked();token=0;goto done;}
    }
    if(!uc_resource_clean(id,token))token=0;
done:ReleaseSRWLockExclusive(&resources.gate);SetLastError(saved);return token;
}
int uc_resource_native_begin(void*buffer,size_t bytes){
    if(!bytes||resource_load(&resources.disabled))return 0;
    DWORD saved=GetLastError();int entered=0;AcquireSRWLockExclusive(&resources.gate);
    if(resources.disabled)goto done;
    /* Even a write started before the first resource was registered must
     * prevent registration/rearming until its native output has completed. */
    InterlockedIncrement(&resources.native);InterlockedIncrement64(&resources.native_epoch);entered=1;
    for(LONG i=0;i<resources.count;i++){ResourceRegion*r=&resources.region[i];int dirty=0;
        for(unsigned a=0;a<r->views;a++)if(resource_overlap((uintptr_t)buffer,bytes,(uintptr_t)r->base[a],r->bytes)){
            if(!dirty){InterlockedIncrement64(&r->revision);dirty=1;}
            DWORD old;if(!VirtualProtect(r->base[a],r->bytes,PAGE_READWRITE,&old)){resource_disable_locked();goto done;}
        }
    }
done:ReleaseSRWLockExclusive(&resources.gate);SetLastError(saved);return entered;
}
void uc_resource_native_end(int entered){
    if(entered){InterlockedIncrement64(&resources.native_epoch);InterlockedDecrement(&resources.native);}
}
int uc_resource_disable(void){
    DWORD saved=GetLastError();AcquireSRWLockExclusive(&resources.gate);
    int ok=resource_disable_locked();ReleaseSRWLockExclusive(&resources.gate);SetLastError(saved);return ok;
}
int uc_resource_shutdown(void){
    int ok=uc_resource_disable();DWORD saved=GetLastError();AcquireSRWLockExclusive(&resources.gate);
    if(resources.handler){if(!RemoveVectoredExceptionHandler(resources.handler))ok=0;resources.handler=NULL;}
    ReleaseSRWLockExclusive(&resources.gate);SetLastError(saved);return ok;
}
