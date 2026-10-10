#include "migration_services_runtime_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D695D(void);

static int runtime_enabled(void) {
    static int flag=-1;
    if(flag<0){const char*p=getenv("UCRR_NATIVE_SERVICES_RUNTIME");flag=p&&!strcmp(p,"1");}
    return flag;
}
static int runtime_span(uint32_t p,unsigned n) {
    return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;
}
static uint32_t runtime_frame(unsigned locals,unsigned table,unsigned pc) {
    uint32_t frame=g_esp-4,old=g_seh_ebp;
    PUSH32(g_esp,locals);PUSH32(g_esp,table);PUSH32(g_esp,pc);PUSH32(g_esp,0x2d7ccc);
    g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(frame)=old;g_esp-=locals;
    PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
    MEM32(frame-24)=g_esp;MEM32(g_esp-4)=pc;MEM32(frame-4)=0xffffffffu;MEM32(frame-8)=table;
    g_eax=frame-16;MEM32(0)=g_eax;g_seh_ebp=frame;g_recomp_zf=0;return frame;
}
static void runtime_return(uint32_t frame,unsigned pc,unsigned args) {
    PUSH32(g_esp,pc);MEM32(0)=MEM32(frame-16);
    POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
    g_esp=frame;POP32(g_esp,g_seh_ebp);MEM32(g_esp-4)=g_ecx;g_esp+=args+4;
}
static void array_lifetime(int construct) {
    uint32_t f=runtime_frame(construct?16:12,construct?0x496f58:0x496f78,construct?0x2d6907:0x2d69c7);
    if(construct){g_eax=0;MEM32(f-32)=0;MEM32(f-4)=0;MEM32(f-28)=0;}
    else{MEM32(f-28)=0;g_esi=MEM32(f+12);g_eax=g_esi*MEM32(f+16);MEM32(f+8)+=g_eax;MEM32(f-4)=0;}
    for(;;){
        if(construct){g_eax=MEM32(f-28);if((int32_t)g_eax>=(int32_t)MEM32(f+16))break;g_esi=MEM32(f+8);g_ecx=g_esi;}
        else{uint32_t remaining=MEM32(f+16)-1;MEM32(f+16)=remaining;if((int32_t)remaining<0)break;MEM32(f+8)-=g_esi;g_ecx=MEM32(f+8);}
        uint32_t saved=g_esp,target=MEM32(f+20);PUSH32(g_esp,construct?0x2d6922:0x2d69e9);RECOMP_ICALL_SAFE(target,saved);
        if(construct){g_esi+=MEM32(f+12);MEM32(f+8)=g_esi;MEM32(f-28)++;}
    }
    MEM32(f-(construct?32:28))=1;MEM32(f-4)=0xffffffffu;
    PUSH32(g_esp,construct?0x2d693d:0x2d69fb);
    /* Keep the original exception cleanup route, including a callback that
     * changes the active SEH frame. Normal array traversal itself is native. */
    uint32_t cleanup=g_seh_ebp;int incomplete=MEM32(cleanup-(construct?32:28))==0;
    if(incomplete){
        PUSH32(g_esp,MEM32(cleanup+(construct?24:20)));PUSH32(g_esp,MEM32(cleanup+(construct?-28:16)));
        PUSH32(g_esp,MEM32(cleanup+12));PUSH32(g_esp,MEM32(cleanup+8));
        PUSH32(g_esp,construct?0x2d695c:0x2d6a1a);sub_002D695D();
    }
    g_recomp_zf=incomplete;g_esp+=4;g_seh_ebp=f;
    runtime_return(f,construct?0x2d6942:0x2d6a00,construct?20:16);
}
int uc_migration_services_runtime(unsigned entry) {
    if(!runtime_enabled())return 0;
    uint32_t s=g_esp;
    if((s&3)||s<0x10080||!runtime_span(s-128,160)||!runtime_span(0x51c178,1)||MEM8(0x51c178))return 0;
    if(entry==0x2d7290){
        unsigned size=g_eax;
        if(!size){g_esp+=4;return 1;}
        if(size>s-0x10000||!runtime_span(s-size,size+4))return 0;
        uint32_t bottom=s+4-size,ret=MEM32(s);
        if(size>=4096){MEM32(s-4)=g_ecx;g_recomp_zf=(MEM32(bottom)&(s-4))==0;}
        MEM32(bottom-4)=ret;g_eax=ret;g_esp=bottom;return 1;
    }
    if(entry!=0x2d68fb&&entry!=0x2d69bb)return 0;
    uint32_t count=MEM32(s+12),stride=MEM32(s+8),base=MEM32(s+4);
    /* ponytail: huge arrays retain the original path; no new count or pointer
     * snapshot is imposed on callbacks once the native traversal starts. */
    if(count>4096||((uint64_t)count*stride>0xffffffffu))return 0;
    if(count&&(!runtime_span(base,count*stride)||base<(uint64_t)s+32&&(uint64_t)base+count*stride>s-128))return 0;
    array_lifetime(entry==0x2d68fb);return 1;
}
