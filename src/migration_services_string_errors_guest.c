#include "migration_services_string_errors_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00011EF0(void),sub_00011E00(void),sub_002D61D7(void),sub_002D619E(void);
static int se_span(unsigned p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void se_frame(void){unsigned f=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,g_eax);g_eax=MEM32(0);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+12);MEM32(0)=g_esp;MEM32(g_esp+12)=f;g_seh_ebp=g_esp+12;MEM32(g_esp-4)=g_eax;}
static void se_string(void){
 g_edx=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=g_edx;PUSH32(g_esp,g_edi);MEM32(g_esi+24)=15;MEM32(g_esi+20)=0;MEM8(g_esi+4)=0;g_edi=g_eax+1;
 do{SET_LO8(g_ecx,MEM8(g_eax));g_eax++;}while(LO8(g_ecx));
 g_eax-=g_edi;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,0x12042);sub_00011EF0();POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=!g_eax;g_esp+=8;
}
static void se_object(void){
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x389738);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;PUSH32(g_esp,0x12072);sub_002D61D7();
 g_eax=0;g_ecx=g_esi+12;MEM32(g_esi)=0x44e000;PUSH32(g_esp,0xffffffff);MEM32(g_ecx+20)=g_eax;MEM32(g_ecx+24)=15;PUSH32(g_esp,g_eax);MEM32(g_esp+24)=g_eax;MEM8(g_ecx+4)=0;g_eax=MEM32(g_esp+32);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1209b);sub_00011E00();
 g_ecx=MEM32(g_esp+8);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=8;
}
int uc_migration_services_string_errors(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_STRING_ERRORS");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!se_span(g_esp-512,528))return 0;
 if(entry==0x2d8704){se_frame();return 1;}
 if(entry==0x12010){unsigned p=MEM32(g_esp+4);if(!se_span(g_ecx,28)||!se_span(p,1)||!memchr((void*)XBOX_PTR(p),0,g_xbox_total_ram-p))return 0;se_string();return 1;}
 if(entry==0x12050){if(!se_span(g_ecx,40)||!se_span(MEM32(g_esp+4),28))return 0;se_object();return 1;}
 if(entry!=0x2d581f&&entry!=0x2d585f)return 0;
 unsigned delta=entry-0x2d581f;g_eax=0x3a328b;PUSH32(g_esp,0x2d5829+delta);se_frame();unsigned f=g_seh_ebp;g_esp-=68;
 PUSH32(g_esp,0x496eb4+(delta?24:0));g_ecx=f-40;PUSH32(g_esp,0x2d5839+delta);se_string();MEM32(f-4)=0;g_eax=f-40;PUSH32(g_esp,g_eax);g_ecx=f-80;PUSH32(g_esp,0x2d5849+delta);se_object();
 PUSH32(g_esp,delta?0x4c37ac:0x4c49b8);g_eax=f-80;PUSH32(g_esp,g_eax);MEM32(f-80)=0x44e000;PUSH32(g_esp,0x2d585e + delta);sub_002D619E();__debugbreak();return 1;
}

