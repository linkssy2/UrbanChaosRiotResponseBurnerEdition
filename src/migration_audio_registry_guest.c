#include "migration_audio_registry_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243F40(void);
extern void sub_00243F60(void);
extern void sub_0024B950(void);
extern void sub_002D57FB(void);
extern void sub_002530F0(void);
static int registry_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_REGISTRY");f=v&&!strcmp(v,"1");}return f;}
static int registry_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int registry_safe(uint32_t p,uint32_t n){return registry_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int registry_admit(uint32_t a){
    if((a!=0x24b720&&a!=0x24b9a0&&a!=0x24c250)||!registry_enabled()||!registry_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!registry_span(g_esp-4096,4108))return 0;
    uint32_t arg=MEM32(g_esp+4);
    if(a==0x24c250)return registry_safe(arg,40)&&registry_span(MEM32(arg),12)&&registry_safe(MEM32(arg+32),8)&&registry_safe(MEM32(arg+36),8)&&MEM32(MEM32(arg+32)+4)==arg+32&&MEM32(MEM32(arg+36))==arg+32;
    if(!registry_safe(g_ecx,112))return 0;
    if(a==0x24b720)return !arg||(registry_safe(arg,4)&&registry_span(MEM32(arg),4));
    if(arg&&!registry_safe(arg,16))return 0;
    uint32_t head=g_ecx+104,node=MEM32(head),previous=head;
    /* ponytail: O(n) admission, replace with trusted list generations if measured. */
    for(unsigned n=0;node!=head;n++){if(n>=4096||node<4||!registry_safe(node-4,44)||MEM32(node+4)!=previous||!registry_span(MEM32(node-4),12))return 0;previous=node;node=MEM32(node);}return MEM32(head+4)==previous;
}
static void registry_attach(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esp+8);
    if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fee18);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x24b736);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_eax+12);g_eax+=12;
        if(g_ecx==g_eax){PUSH32(g_esp,g_eax);g_esi+=92;PUSH32(g_esp,g_esi);PUSH32(g_esp,0x24b74a);sub_00243F40();g_esp+=8;}
    }POP32(g_esp,g_esi);g_esp+=8;
}
static void registry_change(void){
    g_eax=MEM32(0);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e87b);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+24);PUSH32(g_esp,g_edi);g_ebx=g_ecx;int add=0;
    if(g_esi){g_eax=MEM32(g_esi+8);g_ecx=MEM32(g_esi);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_ebx;PUSH32(g_esp,0x24b9d0);sub_0024B950();g_edi=g_eax;SET_LO8(g_eax,MEM8(g_esp+32));
        if(LO8(g_eax)){if(g_edi){g_edx=g_edi+4;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24b9e7);sub_00243F60();g_eax=MEM32(g_edi);g_esp+=4;uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_edi;uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24b9f3);RECOMP_ICALL_SAFE(target,saved);}}
        else if(!g_edi){
            PUSH32(g_esp,60);PUSH32(g_esp,0x24ba14);sub_002D57FB();g_esp+=4;MEM32(g_esp+32)=g_eax;MEM32(g_esp+20)=g_edi;if(g_eax){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x24ba2c);sub_002530F0();}else g_eax=0;
            g_eax+=4;PUSH32(g_esp,g_eax);g_ebx+=104;PUSH32(g_esp,g_ebx);MEM32(g_esp+28)=0xffffffffu;PUSH32(g_esp,0x24ba45);sub_00243F40();g_ecx=MEM32(g_esp+20);g_esp+=8;add=1;
        }
    }
    if(add){POP32(g_esp,g_edi);POP32(g_esp,g_esi);SET_LO8(g_eax,1);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);}
    else{SET_LO8(g_eax,0);g_ecx=MEM32(g_esp+12);MEM32(0)=g_ecx;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);}
    g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void registry_detach(void){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=g_esi+32;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24c25e);sub_00243F60();g_edx=MEM32(g_esi);g_esp+=4;g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x24c268);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_audio_registry(uint32_t a){if(!registry_admit(a))return 0;if(a==0x24b720)registry_attach();else if(a==0x24b9a0)registry_change();else registry_detach();return 1;}
