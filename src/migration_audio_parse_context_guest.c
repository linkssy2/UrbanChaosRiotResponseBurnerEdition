#include "migration_audio_parse_context_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243EE0(void);
static int parse_context_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_PARSE_CONTEXT");f=v&&!strcmp(v,"1");}return f;}
static int parse_context_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int parse_context_safe(uint32_t p,uint64_t n){return parse_context_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+8<=p);}
static int parse_context_admit(uint32_t a){
    if((a!=0x24dcd0&&a!=0x24f1b0&&a!=0x24f220&&a!=0x24f2a0&&a!=0x24f300)||!parse_context_enabled()||!parse_context_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!parse_context_span(g_esp-4096,4104)||!parse_context_safe(g_ecx,a==0x24f2a0||a==0x24f300?44:16))return 0;
    if(a==0x24dcd0){uint32_t count=MEM32(g_ecx+4),p=MEM32(g_ecx);if(count>65536||!parse_context_safe(MEM32(g_esp+4),8)||(p&&!parse_context_safe(p,8ull*count)))return 0;}
    return 1;
}
static void parse_context_register(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+4)*8+8;
    if(!MEM32(g_esi)){PUSH32(g_esp,0x24dce8);sub_00243EE0();g_edx=MEM32(g_eax);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_edi);g_ecx=g_eax;uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x24dcf2);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi)=g_eax;}
    else{
        PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x24dcfc);sub_00243EE0();g_edx=MEM32(g_eax);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_edi);g_ecx=g_eax;uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x24dd06);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+4);g_ebx=g_eax;g_eax=0;
        if(g_ecx){g_ecx=g_ebx+8;do{g_edx=MEM32(g_esi);g_edi=MEM32(g_edx+g_eax*8);MEM32(g_ecx)=g_edi;g_edx=MEM32(g_edx+g_eax*8+4);MEM32(g_ecx+4)=g_edx;g_edx=MEM32(g_esi+4);g_eax++;g_ecx+=8;}while(g_eax<g_edx);}
        g_edi=MEM32(g_esi);PUSH32(g_esp,0x24dd34);sub_00243EE0();g_edx=MEM32(g_eax);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_edi);g_ecx=g_eax;target=MEM32(g_edx+8);PUSH32(g_esp,0x24dd3e);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi)=g_ebx;POP32(g_esp,g_ebx);
    }
    g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi);g_eax++;uint32_t count=g_eax;MEM32(g_esi+4)=g_eax;g_eax=MEM32(g_esp+12);g_edx=MEM32(g_eax);MEM32(g_ecx)=g_edx;g_eax=MEM32(g_eax+4);POP32(g_esp,g_edi);MEM32(g_ecx+4)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=count==0;g_esp+=8;
}
static void parse_context_list_init(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39ea9b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);frame=0;PUSH32(g_esp,g_edi);g_edi=g_ecx;g_esi=g_edi+8;MEM32(g_edi)=frame;MEM32(g_edi+4)=frame;MEM32(g_esp+12)=g_edi;MEM32(g_esi)=frame;MEM32(g_esi+4)=frame;g_eax=frame;MEM32(g_esp+24)=frame;
    PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x24f207);parse_context_register();g_ecx=MEM32(g_esp+16);g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void parse_context_list_free(void){
    PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39ea9b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+12)=g_esi;g_eax=MEM32(g_esi+12);g_edi=0;MEM32(g_esp+24)=g_edi;
    if(g_eax){g_ebx=MEM32(g_esi+8);PUSH32(g_esp,0x24f254);sub_00243EE0();g_edx=MEM32(g_eax);uint32_t saved=g_esp;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);g_ecx=g_eax;uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x24f25d);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+8)=g_edi;MEM32(g_esi+12)=g_edi;}
    MEM32(g_esp+24)=0xffffffff;
    if(MEM32(g_esi+12)!=g_edi){g_ebx=MEM32(g_esi+8);PUSH32(g_esp,0x24f278);sub_00243EE0();g_edx=MEM32(g_eax);uint32_t saved=g_esp;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);g_ecx=g_eax;uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x24f281);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+8)=g_edi;MEM32(g_esi+12)=g_edi;}
    g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void parse_context_init(void){
    PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39eabb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=g_esi+4;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x48436c;PUSH32(g_esp,0x24f2cb);parse_context_list_init();g_ecx=g_esi+20;MEM32(g_esp+16)=0;PUSH32(g_esp,0x24f2db);parse_context_list_init();g_eax=MEM32(g_esp+24);g_ecx=MEM32(g_esp+8);MEM32(g_esi+40)=g_eax;g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void parse_context_free(void){
    PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39eabb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x48436c;g_ecx=g_esi+20;MEM32(g_esp+16)=0;PUSH32(g_esp,0x24f333);parse_context_list_free();g_ecx=g_esi+4;MEM32(g_esp+16)=0xffffffff;PUSH32(g_esp,0x24f343);parse_context_list_free();g_ecx=MEM32(g_esp+8);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_audio_parse_context(uint32_t a){if(!parse_context_admit(a))return 0;switch(a){case 0x24dcd0:parse_context_register();break;case 0x24f1b0:parse_context_list_init();break;case 0x24f220:parse_context_list_free();break;case 0x24f2a0:parse_context_init();break;default:parse_context_free();break;}return 1;}
