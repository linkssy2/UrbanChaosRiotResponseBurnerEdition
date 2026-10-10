#include "migration_services_observers_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00207D70(void),sub_00207F00(void),sub_001F9270(void),sub_002D5CCD(void);
static int observer_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void observer_lookup(void){g_eax=MEM32(g_esp+4);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f8fd0);sub_00207D70();g_esp+=4;}
static void observer_add(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+0x64);g_esi=g_edi+0x64;
 if(!g_eax){g_eax=MEM32(g_edi+0x54);g_ecx=MEM32(MEM32(0x51c398)+8);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x20a510);sub_001F9270();}
 for(g_eax=0,g_ecx=g_esi;g_eax<3;g_eax++,g_ecx+=4)if(!MEM32(g_ecx)){g_edx=MEM32(g_esp+12);MEM32(g_edi+g_eax*4+0x64)=g_edx;break;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;
}
int uc_migration_services_observers(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_OBSERVERS");enabled=p&&!strcmp(p,"1");}if(!enabled)return 0;
 uint32_t s=g_esp,p=g_ecx;if(!observer_span(s-32,48)||MEM8(0x51c178))return 0;
 switch(entry){
 case 0x1f8fc0:observer_lookup();return 1;
 case 0x1f9120:{
  /* Lookup remains the public instrumented boundary; admission precedes it. */
  if(!observer_span(MEM32(0x51c398),12))return 0;
  g_eax=MEM32(s+4);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_esi=g_ecx;PUSH32(g_esp,0x1f912d);observer_lookup();g_esp+=4;
  int missing=!g_eax;if(!missing){PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x1f913c);observer_add();}
  POP32(g_esp,g_esi);g_recomp_zf=missing;g_esp+=8;return 1;
 }
 case 0x20a4f0:if(!observer_span(p,0x70)||!observer_span(MEM32(0x51c398),12))return 0;observer_add();return 1;
 case 0x1f9150:{
  if(!observer_span(p,4))return 0;uint32_t owner=MEM32(p);if(!observer_span(owner,12))return 0;uint32_t sentinel=MEM32(owner+4);
  if(!observer_span(sentinel,12))return 0;
  /* ponytail: bounded validation; owner-time validation can replace this scan if it becomes hot. */
  uint32_t node=MEM32(sentinel),prev=sentinel;unsigned count=0;
  while(node!=sentinel){if(++count>4096||!observer_span(node,12)||MEM32(node+4)!=prev)return 0;prev=node;node=MEM32(node);}
  if(MEM32(sentinel+4)!=prev)return 0;
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);
  if(g_eax!=g_ecx){g_edx=MEM32(g_esp+8);while(MEM32(g_eax+8)!=g_edx){g_eax=MEM32(g_eax);if(g_eax==g_ecx){g_recomp_zf=1;break;}}
   if(g_eax!=MEM32(g_esi+4)){g_ecx=MEM32(g_eax+4);g_edx=MEM32(g_eax);MEM32(g_ecx)=g_edx;g_ecx=MEM32(g_eax);g_edx=MEM32(g_eax+4);PUSH32(g_esp,g_eax);MEM32(g_ecx+4)=g_edx;PUSH32(g_esp,0x1f9189);sub_002D5CCD();g_eax=MEM32(g_esi+8);g_esp+=4;MEM32(g_esi+8)=--g_eax;}
  }
  POP32(g_esp,g_esi);g_esp+=8;return 1;
 }
 case 0x20a140:
  if(!observer_span(p,0x60))return 0;g_eax=MEM32(s+4);g_edx=MEM32(p+0x5c)|g_eax;MEM32(p+0x5c)=g_edx;
  if(g_eax==1){g_eax=MEM32(p+0x54);g_ecx=MEM32(0x51c4dc);MEM32(s+4)=g_eax;sub_00207F00();}else{g_recomp_zf=0;g_esp+=8;}return 1;
 case 0x215ff0:if(!observer_span(p,0x7c))return 0;MEM32(p+0x78)=0;g_esp+=4;return 1;
 case 0x2219e0:{
  if(!observer_span(p,16))return 0;uint32_t free_node=MEM32(p+8),head=MEM32(p+12);
  if(free_node&&(!observer_span(free_node,12)||(head&&!observer_span(head,12))))return 0;
  g_edx=free_node;SET_LO8(g_eax,0);if(free_node){g_eax=MEM32(g_edx+4);MEM32(p+8)=g_eax;g_eax=MEM32(s+4);MEM32(g_edx)=g_eax;g_eax=MEM32(p+12);MEM32(g_edx+4)=g_eax;g_eax=MEM32(p+12);if(g_eax)MEM32(g_eax+8)=g_edx;MEM32(p+4)++;MEM32(p+12)=g_edx;SET_LO8(g_eax,1);}g_esp+=8;return 1;
 }
 default:return 0;
 }
}
