#include "migration_services_exit_registry_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002DA25D(void),sub_002DA170(void),sub_002D9814(void),sub_002D9762(void);
static int exit_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void exit_lock(int acquire){PUSH32(g_esp,8);PUSH32(g_esp,acquire?0x2d8411:0x2d841a);if(acquire)sub_002D9814();else sub_002D9762();POP32(g_esp,g_ecx);g_esp+=4;}
static void exit_guard(void){PUSH32(g_esp,0x2d6583);exit_lock(0);g_esp+=4;}
static void exit_append(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(0x520a54));PUSH32(g_esp,0x2d64b0);sub_002DA25D();POP32(g_esp,g_ecx);g_ecx=MEM32(0x520a50);g_esi=g_eax;g_eax=MEM32(0x520a54);g_edx=g_ecx-g_eax+4;
 if(g_esi<g_edx){
  g_ecx=g_esi<2048?g_esi:2048;g_ecx+=g_esi;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2d64dd);sub_002DA170();POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
  if(!g_eax){g_esi+=16;PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(0x520a54));PUSH32(g_esp,0x2d64f2);sub_002DA170();POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);if(!g_eax){POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=4;return;}}
  g_ecx=MEM32(0x520a50)-MEM32(0x520a54);MEM32(0x520a54)=g_eax;g_ecx=(unsigned)((int32_t)g_ecx>>2);g_ecx=g_eax+4*g_ecx;MEM32(0x520a50)=g_ecx;
 }
 MEM32(g_ecx)=g_edi;MEM32(0x520a50)+=4;g_eax=g_edi;POP32(g_esp,g_esi);g_esp+=4;
}
static void exit_register(void){
 unsigned f=g_esp-4,old=g_seh_ebp;PUSH32(g_esp,12);PUSH32(g_esp,0x496f48);PUSH32(g_esp,0x2d6558);PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(f)=old;g_esp-=12;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(f-24)=g_esp;MEM32(g_esp-4)=0x2d6558;MEM32(f-4)=0xffffffff;MEM32(f-8)=0x496f48;g_eax=f-16;MEM32(0)=g_eax;g_seh_ebp=f;g_recomp_zf=0;
 PUSH32(g_esp,0x2d655d);exit_lock(1);MEM32(f-4)=0;g_edi=MEM32(f+8);PUSH32(g_esp,0x2d6569);exit_append();MEM32(f-28)=g_eax;MEM32(f-4)=0xffffffff;PUSH32(g_esp,0x2d6575);exit_guard();g_eax=MEM32(f-28);
 g_seh_ebp=f;PUSH32(g_esp,0x2d657d);g_ecx=MEM32(f-16);MEM32(0)=g_ecx;POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);PUSH32(g_esp,g_ecx);g_esp+=8;
}
int uc_migration_services_exit_registry(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_EXIT_REGISTRY");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!exit_span(g_esp-256,272))return 0;
 switch(entry){
 case 0x2d64a4:exit_append();break;
 case 0x2d654c:exit_register();break;
 case 0x2d657e:exit_guard();break;
 case 0x2d840a:exit_lock(1);break;
 case 0x2d8413:exit_lock(0);break;
 case 0x2d6584:PUSH32(g_esp,MEM32(g_esp+4));PUSH32(g_esp,0x2d658d);exit_register();g_eax=g_eax?0:0xffffffff;POP32(g_esp,g_ecx);g_recomp_zf=!g_eax;g_esp+=4;break;
 default:return 0;
 }
 return 1;
}
