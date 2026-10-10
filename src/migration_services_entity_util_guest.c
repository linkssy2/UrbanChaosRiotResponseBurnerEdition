#include "migration_services_entity_util_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D73A4(void);
static int mapped(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_services_entity_util(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_ENTITY_UTIL");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!mapped(g_esp-64,80))return 0;
 if(entry==0x22b770){g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x22b77a);sub_002D73A4();POP32(g_esp,g_ecx);g_esp+=12;return 1;}
 if(entry==0x8a050||entry==0xbbca0){
  if(!mapped(g_ecx,80)||!mapped(MEM32(g_esp+4),entry==0x8a050?64:12))return 0;
  if(entry==0x8a050){g_eax=MEM32(g_esp+4);for(unsigned i=0;i<4;i++){g_xmm0=XMM_MEM(g_eax+16*i);XMM_STORE(g_ecx+16+16*i,g_xmm0);}g_esp+=8;}
  else{PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;g_esp=(g_esp&~15u)-16;g_eax=MEM32(f+8);for(unsigned i=0;i<3;i++){g_xmm0=XMM_SCALAR(MEMF(g_eax+4*i));MEMF(g_esp+4*i)=g_xmm0.f[0];}g_xmm0=XMM_SCALAR(MEMF(0x44bf94));MEMF(g_esp+12)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp);XMM_STORE(g_ecx+64,g_xmm0);g_recomp_zf=f+4==0;g_esp=f+12;}
  return 1;
 }
 if(entry!=0x1f19f0||!mapped(g_ecx,44))return 0;
 uint32_t node=MEM32(g_ecx+40);for(unsigned n=0;node;n++){if(n>=4096||!mapped(node,16)||!mapped(MEM32(node),52))return 0;uint32_t owner=MEM32(node+8);if(owner&&!(MEM8(node+12)&16)&&(!mapped(owner,4)||!mapped(MEM32(owner),8)))return 0;node=MEM32(node+4);}
 PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+40);
 if(g_eax){
  PUSH32(g_esp,g_esi);
  do{
   g_esi=MEM32(g_edi+40);g_eax=MEM32(g_esi+4);MEM32(g_edi+40)=g_eax;g_edx=MEM32(g_esi);g_ecx=g_esi;
   uint32_t saved=g_esp,target=MEM32(g_edx+48);PUSH32(g_esp,0x1f1a10);RECOMP_ICALL_SAFE(target,saved);
   g_eax=MEM32(g_esi+8);
   if(g_eax&&!(MEM8(g_esi+12)&16)){g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,g_esi);target=MEM32(g_eax+4);PUSH32(g_esp,0x1f1a26);RECOMP_ICALL_SAFE(target,saved);}
   else if(MEM8(g_esi+12)&4){g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;target=MEM32(g_edx);PUSH32(g_esp,0x1f1a36);RECOMP_ICALL_SAFE(target,saved);}
   g_eax=MEM32(g_edi+40);
  }while(g_eax);
  POP32(g_esp,g_esi);
 }
 POP32(g_esp,g_edi);g_recomp_zf=1;g_esp+=4;return 1;
}
