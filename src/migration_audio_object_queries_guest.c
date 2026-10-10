#include "migration_audio_object_queries_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0024B6B0(void);
extern void sub_00244010(void);
static int object_queries_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_OBJECT_QUERIES");f=v&&!strcmp(v,"1");}return f;}
static int object_queries_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int object_queries_safe(uint32_t p,uint64_t n){return object_queries_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+8<=p);}
static int object_queries_admit(uint32_t a){
    if((a!=0x245d90&&a!=0x2488f0&&a!=0x248960&&a!=0x2471a0&&a!=0x247d30)||!object_queries_enabled()||!object_queries_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!object_queries_span(g_esp-4096,4104)||!object_queries_safe(g_ecx,96))return 0;
    if(a==0x245d90||a==0x2471a0)return object_queries_span(MEM32(g_ecx),a==0x245d90?52:8);
    if(a==0x247d30){uint32_t p=g_ecx;unsigned budget=128;while(p){if(!budget--||!object_queries_safe(p,56))return 0;p=MEM32(p+44);}return 1;}
    uint32_t bank=MEM32(g_ecx+60),def=MEM32(g_ecx+80);return (!bank||(object_queries_safe(bank,144)&&object_queries_span(MEM32(bank),36)))&&(!def||object_queries_safe(def,16))&&(a!=0x2488f0||object_queries_safe(MEM32(g_esp+4),1));
}
static void object_queries_event(void){
    g_esp-=8;uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_ecx=g_esp+8;PUSH32(g_esp,g_ecx);g_eax=0;g_edx=g_esp+16;PUSH32(g_esp,g_edx);MEM32(g_esp+16)=g_eax;MEM32(g_esp+20)=g_eax;g_eax=MEM32(g_esi);PUSH32(g_esp,0x4fec20);g_ecx=g_esi;uint32_t target=MEM32(g_eax+48);PUSH32(g_esp,0x245db7);RECOMP_ICALL_SAFE(target,saved);
    int found=0;if(LO8(g_eax)){g_edi=MEM32(g_esp+20);do{if(MEM32(g_esp+8)==g_edi){found=1;break;}g_eax=MEM32(g_esi);g_ecx=g_esp+8;saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=g_esp+16;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x4fec20);g_ecx=g_esi;target=MEM32(g_eax+48);PUSH32(g_esp,0x245ddc);RECOMP_ICALL_SAFE(target,saved);}while(LO8(g_eax));}
    POP32(g_esp,g_edi);SET_LO8(g_eax,found);POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void object_queries_name(int compare){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+60);int valid=g_eax!=0;
    if(valid){g_eax=MEM32(g_esi+80);valid=g_eax!=0;}
    if(valid){SET_LO16(g_eax,MEM16(g_eax+14));valid=LO16(g_eax)!=0xffff;}
    if(valid){g_ecx=MEM32(g_esi+60);g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+32);PUSH32(g_esp,compare?0x248913:0x248983);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fed94);valid=g_ecx==MEM32(g_eax);}
    if(!valid){if(compare)SET_LO8(g_eax,0);else g_eax=0;POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=compare?8:4;return;}
    g_ecx=MEM32(g_esi+60);g_esi=MEM32(g_esi+80);int has_def=g_esi!=0;if(has_def)SET_LO16(g_eax,MEM16(g_esi+14));else g_eax=0xffff;
    if(compare){g_edx=MEM32(g_esp+8);PUSH32(g_esp,g_edx);}g_eax&=0x3fff;PUSH32(g_esp,g_eax);PUSH32(g_esp,compare?0x248942:has_def?0x2489a6:0x2489b8);sub_0024B6B0();
    if(compare){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x248948);sub_00244010();g_esp+=8;g_eax=g_eax==0;}
    POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=compare?8:4;
}
static void object_queries_inherited(void){
    PUSH32(g_esp,g_esi);g_esi=(uint32_t)(int32_t)SMEM16(g_ecx+52);g_ecx=MEM32(g_ecx+44);if(g_ecx){PUSH32(g_esp,0x247d41);object_queries_inherited();g_eax=SX16(LO16(g_eax));g_esi+=g_eax;}
    g_eax=MEM32(0x4839b0);g_recomp_zf=g_eax==g_esi;if((int32_t)g_eax<=(int32_t)g_esi)SET_LO16(g_eax,LO16(g_esi));POP32(g_esp,g_esi);g_esp+=4;
}
int uc_migration_audio_object_queries(uint32_t a){
    if(!object_queries_admit(a))return 0;
    switch(a){case 0x245d90:object_queries_event();break;case 0x2488f0:object_queries_name(1);break;case 0x248960:object_queries_name(0);break;case 0x247d30:object_queries_inherited();break;
    default:g_eax=MEM32(g_ecx+32)-1;MEM32(g_ecx+32)=g_eax;if(!g_eax){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x2471b0);RECOMP_ICALL_SAFE(target,saved);g_eax=0;}g_esp+=4;break;}return 1;
}
