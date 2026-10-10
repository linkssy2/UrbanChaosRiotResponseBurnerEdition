/* Whole original particle manager update/clear. No second particle graph and
 * no callback replay; every live source reload retains original ordering. */
#include "hybrid_particle_owner_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int manager_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_OWNER");flag=v&&!strcmp(v,"1");}return flag;}
static int manager_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int manager_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_particle_owner(unsigned clear){
 if(!manager_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if(clear>1||(s&3)||s<0x10400u||!manager_span(s-1024,1048)||!manager_span(p,0x1d)||!manager_apart(p,0x1d,s-1024,1048)||
    !manager_span(0x51c178,1)||MEM8(0x51c178)||!manager_apart(p,0x1d,0x51c178,1)||!manager_apart(s-1024,1048,0x51c178,1))return 0;
 uint32_t count=MEM32(p+8),capacity=MEM32(p+12),list=MEM32(p+4);
 if(count>capacity||capacity>4096||(count&&(!manager_span(list,count*4)||!manager_apart(list,count*4,p,0x1d)||!manager_apart(list,count*4,s-1024,1048))))return 0;
 /* Completing callbacks own lifetime and may replace lists/objects/counts.
  * Initial admission does not cache them or assert later immutability. After
  * this point there is no native rejection, rollback or callback replay. */
 if(clear){
  g_esp-=12;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+8);PUSH32(g_esp,g_edi);g_edi=0;
  if(g_eax)do{
   g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_edi*4);
   if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x238de0);RECOMP_ICALL_SAFE(target,saved);}
   g_eax=MEM32(g_esi+8);++g_edi;
  }while(g_edi<g_eax);
  g_xmm0=XMM_ZERO();g_eax=g_esi+16;MEMF(g_esp+8)=g_xmm0.f[0];g_ecx=MEM32(g_esp+8);MEM32(g_eax)=g_ecx;
  MEMF(g_esp+12)=g_xmm0.f[0];g_edx=MEM32(g_esp+12);MEMF(g_esp+16)=g_xmm0.f[0];g_ecx=MEM32(g_esp+16);MEM32(g_eax+4)=g_edx;
  POP32(g_esp,g_edi);MEM32(g_esi+8)=0;MEM8(g_esi+28)=0;MEM32(g_eax+8)=g_ecx;POP32(g_esp,g_esi);g_esp+=12;
  g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+8);PUSH32(g_esp,g_edi);g_edi=0;
 if(g_eax){
  PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+16);
  do{
   g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_edi*4);g_edx=MEM32(g_ecx);
   {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x238e4e);RECOMP_ICALL_SAFE(target,saved);}
   g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_edi*4);g_edx=MEM32(g_ecx);
   {uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x238e59);RECOMP_ICALL_SAFE(target,saved);}
   if(LO8(g_eax)){
    g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_edi*4);
    if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x238e6d);RECOMP_ICALL_SAFE(target,saved);}
    g_eax=MEM32(g_esi+8)-1;MEM32(g_esi+8)=g_eax;g_ecx=g_eax;
    if(g_eax){g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_ecx*4);MEM32(g_eax+g_edi*4)=g_ecx;}
   }else ++g_edi;
  }while(g_edi<MEM32(g_esi+8));
  POP32(g_esp,g_ebx);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;return 1;
}
