#ifndef UCRR_MATERIAL_OPCODE_PROBE_H
#define UCRR_MATERIAL_OPCODE_PROBE_H
/* Diagnostic-only, single-owner bounded opcode observations. Included only in
 * the derived interpreter chunk. It never changes guest state or skips work. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef UC_OPCODE_CAPACITY
#define UC_OPCODE_CAPACITY 65536u
#endif
typedef struct UcOpcodeRecord {
    uint32_t invocation,opcode,target,stream,remaining,operand,mode;
} UcOpcodeRecord;
static volatile LONG uc_opcode_requested,uc_opcode_owner,uc_opcode_foreign;
static unsigned uc_opcode_count,uc_opcode_invocations,uc_opcode_finished;
static uint64_t uc_opcode_start;
static UcOpcodeRecord *uc_opcode_records;
void uc_material_probe_arm(void){InterlockedCompareExchange(&uc_opcode_requested,1,0);}
static unsigned uc_material_probe_begin(void){
    static __declspec(thread) int enabled=-1;
    if(enabled<0){const char*s=getenv("UCRR_MATERIAL_OPCODE_PROBE");enabled=s&&!strcmp(s,"1");}
    if(!enabled)return 0;
    LONG thread=(LONG)GetCurrentThreadId();
    LONG owner=InterlockedCompareExchange(&uc_opcode_owner,0,0);
    if(!owner){
        if(!InterlockedCompareExchange(&uc_opcode_requested,0,0))return 0;
        owner=InterlockedCompareExchange(&uc_opcode_owner,thread,0);
        if(!owner){
            LARGE_INTEGER start;QueryPerformanceCounter(&start);uc_opcode_start=(uint64_t)start.QuadPart;
            uc_opcode_records=(UcOpcodeRecord*)calloc(UC_OPCODE_CAPACITY,sizeof(*uc_opcode_records));
            if(!uc_opcode_records)uc_opcode_finished=1;
            owner=thread;
        }
    }
    if(owner!=thread){InterlockedIncrement(&uc_opcode_foreign);return 0;}
    if(uc_opcode_finished)return 0;
    return ++uc_opcode_invocations;
}
static void uc_material_probe_record(unsigned invocation,unsigned opcode,unsigned target,
                                      unsigned stream,unsigned remaining,unsigned operand,unsigned mode){
    if(!invocation||uc_opcode_finished)return;
    UcOpcodeRecord*r=&uc_opcode_records[uc_opcode_count++];
    *r=(UcOpcodeRecord){invocation,opcode,target,stream,remaining,operand,mode};
    if(uc_opcode_count==UC_OPCODE_CAPACITY){
        LARGE_INTEGER end;QueryPerformanceCounter(&end);
        uint32_t header[8]={0x4f504355u,1,sizeof(*r),uc_opcode_count,GetCurrentProcessId(),
            (uint32_t)uc_opcode_owner,(uint32_t)InterlockedCompareExchange(&uc_opcode_foreign,0,0),uc_opcode_invocations};
        uint64_t ticks[2]={uc_opcode_start,(uint64_t)end.QuadPart};
        char path[160];snprintf(path,sizeof(path),"material-opcode-probe-%lu-%llu.bin",GetCurrentProcessId(),uc_opcode_start);
        FILE*f=fopen(path,"wbx");
        if(f){int ok=fwrite(header,sizeof(header),1,f)==1;
            ok=(fwrite(ticks,sizeof(ticks),1,f)==1)&&ok;
            ok=(fwrite(uc_opcode_records,sizeof(*r),uc_opcode_count,f)==uc_opcode_count)&&ok;
            if(fclose(f))ok=0;
            if(!ok)fprintf(stderr,"[MATERIAL-OPCODE-PROBE] incomplete write\n");
        }else fprintf(stderr,"[MATERIAL-OPCODE-PROBE] output unavailable\n");
        free(uc_opcode_records);uc_opcode_records=NULL;uc_opcode_finished=1;
    }
}
#endif
