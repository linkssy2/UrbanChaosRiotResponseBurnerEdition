#include "migration_particles_fire_cleanup_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002084E0(void),sub_00210770(void),sub_00207B30(void),sub_002102F0(void),sub_00055EE0(void),sub_00022A10(void),sub_00216BD0(void);
static int cleanup_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_FIRE_CLEANUP");f=p&&!strcmp(p,"1");}return f;}
static int cleanup_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int cleanup_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define CLEAN_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static void retire_fire_resources(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x1b4);PUSH32(g_esp,g_edi);g_edi=0;
 if((int32_t)g_eax>0)do{
  g_eax=MEM32(g_esi+0x1b0);g_ecx=MEM32(g_eax+g_edi*4);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x51c4dc);CLEAN_CALL(sub_002084E0,0x18ee35);
  g_ecx=g_eax;CLEAN_CALL(sub_00210770,0x18ee3c);g_eax=MEM32(g_esi+0x1b4);g_edi++;
 }while((int32_t)g_edi<(int32_t)g_eax);
 g_eax=MEM32(g_esi+0x1b8);g_edi=0;MEM32(g_esi+0x1b4)=0;
 if((int32_t)g_eax>0)do{
  g_edx=MEM32(g_esi+0x1bc);g_eax=MEM32(g_edx+g_edi*4);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);CLEAN_CALL(sub_00207B30,0x18ee75);
  g_ecx=MEM32(0x51c4dc);g_eax&=0xfff0ffffu;PUSH32(g_esp,g_eax);CLEAN_CALL(sub_002084E0,0x18ee86);g_ecx=g_eax;CLEAN_CALL(sub_002102F0,0x18ee8d);
  g_eax=MEM32(g_esi+0x1b8);g_edi++;
 }while((int32_t)g_edi<(int32_t)g_eax);
 g_eax=MEM32(g_esi+0x54);MEM32(g_esi+0x1b8)=0;g_ecx=MEM32(0x51c398);g_ecx=MEM32(g_ecx+12);g_edx=MEM32(g_ecx);
 for(unsigned event=0;event<2;event++){
  if(event){g_ecx=MEM32(0x51c398);g_ecx=MEM32(g_ecx+12);g_eax=MEM32(g_esi+0x54);g_edx=MEM32(g_ecx);}
  uint32_t saved=g_esp;PUSH32(g_esp,event?0x465ae0:0x47211c);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,event?0x18eed0:0x18eeb9);RECOMP_ICALL_SAFE(target,saved);
 }
 SET_LO8(g_eax,MEM8(g_esi+0x1c8));int absent=LO8(g_eax)==0;
 if(!absent){g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);CLEAN_CALL(sub_00055EE0,0x18eee9);g_ecx=g_eax;CLEAN_CALL(sub_00022A10,0x18eef0);}
 g_ecx=MEM32(g_esi+0x178);PUSH32(g_esp,0);PUSH32(g_esp,0x474268);PUSH32(g_esp,g_ecx);g_ecx=g_esi;CLEAN_CALL(sub_00216BD0,0x18ef05);
 POP32(g_esp,g_edi);MEM8(g_esi+0x176)=1;POP32(g_esp,g_esi);g_recomp_zf=absent;g_esp+=4;
}
int uc_migration_particles_fire_cleanup(unsigned entry){
 if(!cleanup_enabled()||entry!=0x18ee10)return 0;uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10100u||!cleanup_span(s-256,272)||!cleanup_span(p,0x1d0)||!cleanup_apart(p,0x1d0,s-256,272)||!cleanup_span(0x51c4dc,4)||!cleanup_span(0x51c398,4)||!cleanup_span(0x51c178,1)||MEM8(0x51c178))return 0;
 for(unsigned j=0;j<2;j++){
  int32_t count=(int32_t)MEM32(p+(j?0x1b8:0x1b4));uint32_t list=MEM32(p+(j?0x1bc:0x1b0));
  if(count>4096||(count>0&&(!cleanup_span(list,4u*count)||!cleanup_apart(list,4u*count,s-256,272))))return 0;
 }
 /* Completing callbacks retain the original valid graph/lifetime contract.
  * Counts, list pointers and callee-visible registers are reloaded live. */
 retire_fire_resources();return 1;
}
#undef CLEAN_CALL
