#include "migration_services_seh_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int seh_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void cleanup_debug(void){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);g_ebx=0x504820;g_ecx=MEM32(g_seh_ebp+8);MEM32(g_ebx+8)=g_ecx;MEM32(g_ebx+4)=g_eax;MEM32(g_ebx+12)=g_seh_ebp;POP32(g_esp,g_ecx);POP32(g_esp,g_ebx);g_esp+=8;}
int uc_migration_services_seh(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_SEH");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!seh_span(g_esp-512,536))return 0;
 if(entry==0x2d6186){if(!seh_span(g_seh_ebp,12))return 0;cleanup_debug();return 1;}
 if(entry==0x2d7c78){
  unsigned locals=MEM32(g_esp+8);if(locals>256)return 0;
  uint32_t frame=g_seh_ebp;PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+16);MEM32(g_esp+16)=frame;frame=g_esp+16;g_esp-=g_eax;
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=MEM32(frame-8);MEM32(frame-24)=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(frame-4);MEM32(frame-4)=0xffffffff;MEM32(frame-8)=g_eax;g_eax=frame-16;MEM32(0)=g_eax;g_recomp_zf=!g_esp;g_seh_ebp=frame;g_esp+=4;return 1;
 }
 if(entry==0x2d7cb3){
  uint32_t f=g_seh_ebp;if(!seh_span(f-16,20))return 0;g_ecx=MEM32(f-16);MEM32(0)=g_ecx;POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);PUSH32(g_esp,g_ecx);g_esp+=4;return 1;
 }
 if(entry!=0x2d60f2)return 0;
 uint32_t rec=MEM32(g_esp+4),stop=MEM32(g_esp+8);if(!seh_span(rec,16)||!seh_span(g_seh_ebp,12))return 0;uint32_t table=MEM32(rec+8),idx=MEM32(rec+12);
 for(unsigned n=0;idx!=0xffffffff&&idx!=stop;n++){uint64_t row=(uint64_t)table+12ull*idx;if(n>=4096||row>0xffffffff||!seh_span((uint32_t)row,12))return 0;idx=MEM32((uint32_t)row);}
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=MEM32(g_esp+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,0xfffffffe);PUSH32(g_esp,0x2d60d0);PUSH32(g_esp,MEM32(0));MEM32(0)=g_esp;
 for(;;){
  g_eax=MEM32(g_esp+32);g_ebx=MEM32(g_eax+8);g_esi=MEM32(g_eax+12);if(g_esi==0xffffffff||g_esi==MEM32(g_esp+36))break;
  g_esi*=3;g_ecx=MEM32(g_ebx+4*g_esi);MEM32(g_esp+8)=g_ecx;MEM32(g_eax+12)=g_ecx;
  if(!MEM32(g_ebx+4*g_esi+4)){PUSH32(g_esp,0x101);g_eax=MEM32(g_ebx+4*g_esi+8);PUSH32(g_esp,0x2d6146);cleanup_debug();uint32_t saved=g_esp,target=MEM32(g_ebx+4*g_esi+8);PUSH32(g_esp,0x2d614a);RECOMP_ICALL_SAFE(target,saved);}
 }
 uint32_t old;POP32(g_esp,old);MEM32(0)=old;g_esp+=12;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
