#ifndef UCRR_SCENERY_SUBMISSION_PROBE_H
#define UCRR_SCENERY_SUBMISSION_PROBE_H
/* Read-only RunPushBuffer census, not an executed-command replay. Nested kicks
 * and fixups may have executed before this post-original snapshot. Include once,
 * in recomp_manual.c after recomp_types.h. All file I/O is at capture end. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef UC_SCENERY_RECORD_LIMIT
#define UC_SCENERY_RECORD_LIMIT 8192u
#endif
#ifndef UC_SCENERY_BYTE_LIMIT
#define UC_SCENERY_BYTE_LIMIT (16u*1024u*1024u)
#endif
#ifndef UC_SCENERY_BUFFER_LIMIT
#define UC_SCENERY_BUFFER_LIMIT 65536u
#endif
enum { UC_SP_PENDING=1, UC_SP_RESOURCE_END=2, UC_SP_SIZE=4,
       UC_SP_COMMAND_RANGE=8, UC_SP_BYTE_CAP=16, UC_SP_COPIED=32 };
typedef struct UCSceneryRecord {
    uint64_t sequence;
    uint32_t caller, entry_esp, resource, fixup, device, device_flags;
    uint32_t before[6], after[6];
    uint32_t status, payload_offset, payload_bytes, final_word;
} UCSceneryRecord;
typedef struct UCSceneryHeader {
    uint32_t magic, version, header_bytes, record_bytes;
    uint32_t records, payload_bytes, pid, owner_tid;
    uint64_t start_qpc, end_qpc, calls, stack_reject, resource_reject;
    uint64_t record_overflow, nesting_overflow, end_mismatch;
    uint64_t range_reject, oversized, byte_overflow, copied;
    uint32_t owner_errors, allocation_failed, write_failed, reserved;
} UCSceneryHeader;
static struct {
    int initialized, enabled, active, finished;
    DWORD owner;
    volatile LONG owner_errors;
    unsigned depth, pending[16];
    UCSceneryHeader header;
    UCSceneryRecord *records;
    unsigned char *bytes;
} uc_scenery_probe;

static void uc_scenery_probe_init(void) {
#if UCRR_PLAY_ONLY
    uc_scenery_probe.initialized=1;
    uc_scenery_probe.enabled=0;
    return;
#endif

    if(uc_scenery_probe.initialized)return;
    uc_scenery_probe.initialized=1;
    const char *s=getenv("UCRR_SCENERY_SUBMISSION_PROBE");
    uc_scenery_probe.enabled=s&&!strcmp(s,"1");
}
static int uc_scenery_probe_owner(void) {
    if(GetCurrentThreadId()==uc_scenery_probe.owner)return 1;
    InterlockedIncrement(&uc_scenery_probe.owner_errors);return 0;
}
/* Exact project alias normalization, with whole-span checks. Never cross a WC
 * mirror boundary or enter the FD000000 device aperture. Unaligned objects are
 * legitimate: the title passes its callback object +9 as the resource. */
static const void *uc_scenery_probe_ram(uint32_t va,uint32_t bytes) {
    if(!bytes||va<0x10000u||(uint64_t)va+bytes>UINT64_C(0x100000000))return NULL;
    uint32_t first=recomp_normalize_xbox_va(va);
    uint32_t last=recomp_normalize_xbox_va(va+bytes-1u);
    if(first<0x10000u||(uint64_t)first+bytes>(uint64_t)g_xbox_total_ram||
       (uint64_t)first+bytes-1u!=last)return NULL;
    return (const void *)(g_xbox_mem_offset+(uintptr_t)first);
}

/* Called at the existing F8 recorder boundaries. One bounded window per process;
 * no allocation, clock, guest write, or file operation occurs in begin/end. */
void uc_scenery_probe_boundary(int start,uint64_t qpc) {
    uc_scenery_probe_init();
    if(!uc_scenery_probe.enabled||uc_scenery_probe.finished)return;
    UCSceneryHeader *h=&uc_scenery_probe.header;
    if(start) {
        if(uc_scenery_probe.active)return;
        uc_scenery_probe.owner=GetCurrentThreadId();
        h->magic=0x50534355u;h->version=1;h->header_bytes=sizeof(*h);
        h->record_bytes=sizeof(UCSceneryRecord);h->pid=GetCurrentProcessId();
        h->owner_tid=uc_scenery_probe.owner;h->start_qpc=qpc;
        uc_scenery_probe.records=(UCSceneryRecord*)calloc(UC_SCENERY_RECORD_LIMIT,sizeof(UCSceneryRecord));
        uc_scenery_probe.bytes=(unsigned char*)malloc(UC_SCENERY_BYTE_LIMIT);
        h->allocation_failed=!uc_scenery_probe.records||!uc_scenery_probe.bytes;
        uc_scenery_probe.active=1;return;
    }
    if(!uc_scenery_probe.active||!uc_scenery_probe_owner())return;
    uc_scenery_probe.active=0;uc_scenery_probe.finished=1;
    h->end_qpc=qpc;h->owner_errors=(uint32_t)uc_scenery_probe.owner_errors;
    char path[128];snprintf(path,sizeof(path),"scenery-submission-%lu-%llu.bin",
        (unsigned long)h->pid,(unsigned long long)h->start_qpc);
    FILE *f=fopen(path,"wbx");
    if(!f)h->write_failed=1;
    else {
        int ok=fwrite(h,sizeof(*h),1,f)==1;
        if(ok&&h->records)ok=fwrite(uc_scenery_probe.records,sizeof(UCSceneryRecord),h->records,f)==h->records;
        if(ok&&h->payload_bytes)ok=fwrite(uc_scenery_probe.bytes,1,h->payload_bytes,f)==h->payload_bytes;
        if(fflush(f))ok=0;
        if(!ok){h->write_failed=1;if(!fseek(f,0,SEEK_SET))fwrite(h,sizeof(*h),1,f);}
        if(fclose(f))h->write_failed=1;
    }
    if(h->write_failed)fprintf(stderr,"[SCENERY-SUBMISSION] incomplete output: %s\n",path);
    free(uc_scenery_probe.records);free(uc_scenery_probe.bytes);
    uc_scenery_probe.records=NULL;uc_scenery_probe.bytes=NULL;
}
static uint32_t uc_scenery_probe_begin(void) {
    if(!uc_scenery_probe.active||!uc_scenery_probe_owner())return 0;
    UCSceneryHeader *h=&uc_scenery_probe.header;
    ++h->calls;
    if(h->allocation_failed)return 0;
    if(h->records==UC_SCENERY_RECORD_LIMIT){++h->record_overflow;return 0;}
    if(uc_scenery_probe.depth==16){++h->nesting_overflow;return 0;}
    uint32_t args[3];const void *p=uc_scenery_probe_ram(g_esp,sizeof(args));
    if(!p){++h->stack_reject;return 0;}memcpy(args,p,sizeof(args));
    p=uc_scenery_probe_ram(args[1],24);
    if(!p){++h->resource_reject;return 0;}
    unsigned slot=h->records++;
    UCSceneryRecord *r=&uc_scenery_probe.records[slot];
    r->sequence=h->calls;r->caller=args[0];r->entry_esp=g_esp;
    r->resource=args[1];r->fixup=args[2];memcpy(r->before,p,24);
    p=uc_scenery_probe_ram(0x3F4238u,4);
    if(p){memcpy(&r->device,p,4);p=uc_scenery_probe_ram(r->device,12);
        if(p)memcpy(&r->device_flags,(const unsigned char*)p+8,4);}
    r->status=UC_SP_PENDING;
    uc_scenery_probe.pending[uc_scenery_probe.depth++]=slot;
    return args[1];
}
static void uc_scenery_probe_end(uint32_t resource) {
    if(!resource||!uc_scenery_probe.active||!uc_scenery_probe_owner())return;
    UCSceneryHeader *h=&uc_scenery_probe.header;
    if(!uc_scenery_probe.depth){++h->end_mismatch;return;}
    UCSceneryRecord *r=&uc_scenery_probe.records[uc_scenery_probe.pending[uc_scenery_probe.depth-1]];
    if(r->resource!=resource){++h->end_mismatch;return;}
    --uc_scenery_probe.depth;
    const void *p=uc_scenery_probe_ram(resource,24);
    if(!p){r->status=UC_SP_RESOURCE_END;++h->resource_reject;return;}
    memcpy(r->after,p,24);
    uint32_t count=r->after[3];
    if(count<4||(count&3)||count>UC_SCENERY_BUFFER_LIMIT){
        r->status=UC_SP_SIZE;++h->oversized;return;}
    p=uc_scenery_probe_ram(r->after[1],count);
    if(!p){r->status=UC_SP_COMMAND_RANGE;++h->range_reject;return;}
    if(count>UC_SCENERY_BYTE_LIMIT-h->payload_bytes){
        r->status=UC_SP_BYTE_CAP;++h->byte_overflow;return;}
    r->payload_offset=h->payload_bytes;r->payload_bytes=count;
    memcpy(uc_scenery_probe.bytes+h->payload_bytes,p,count);
    memcpy(&r->final_word,(const unsigned char*)p+count-4,4);
    h->payload_bytes+=count;++h->copied;r->status=UC_SP_COPIED;
}
#endif
