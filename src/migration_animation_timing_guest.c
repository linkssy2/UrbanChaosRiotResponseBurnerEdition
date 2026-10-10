#include "migration_animation_timing_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static unsigned compare_phase(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
static int timing_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Preserve synchronous virtual feedback between phase-rate reads, active-state
 * queries, interval collection and deactivation. No batched interval plan can
 * replace these calls: each callback may change the next field or target. */
int uc_migration_animation_timing(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_TIMING");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;
 if(!enabled||(s&3)||s<0x10400u||!timing_span(s-1024,1048)||!timing_span(p,0x38)||
 !((uint64_t)p+0x38<=s-1024||(uint64_t)s+24<=p)||!timing_span(0x51c178,1)||MEM8(0x51c178)||
 !timing_span(0x44bf94,4)||!timing_span(0x44e554,4)||g_fp_top<0||g_fp_top>7)return 0;
 unsigned flags;
 #define TIMING_CALL(target,pc,saved) do{uint32_t timing_target=(target);PUSH32(g_esp,(pc));RECOMP_ICALL_SAFE(timing_target,(saved));}while(0)
 #define RATE_TOP g_fp_stack[g_fp_top]
 #define STORE_RATE(at) do{MEMF(at)=(float)RATE_TOP;g_fp_top=(g_fp_top+1u)&7u;}while(0)
 g_esp-=16;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);TIMING_CALL(MEM32(g_eax+60),0x1f1fcb,saved);}
 RATE_TOP*=MEMF(g_esi+32);g_edx=MEM32(g_esi);g_ecx=g_esi;RATE_TOP*=MEMF(g_esp+28);STORE_RATE(g_esp+8);
 {uint32_t saved=g_esp;TIMING_CALL(MEM32(g_edx+60),0x1f1fdd,saved);}
 RATE_TOP*=MEMF(g_esi+36);g_eax=MEM32(g_esi);g_ecx=g_esi;STORE_RATE(g_esp+16);
 {uint32_t saved=g_esp;TIMING_CALL(MEM32(g_eax+60),0x1f1feb,saved);}
 RATE_TOP*=MEMF(g_esi+40);g_edx=MEM32(g_esi);g_ecx=g_esi;STORE_RATE(g_esp+4);
 {uint32_t saved=g_esp;TIMING_CALL(MEM32(g_edx+40),0x1f1ff9,saved);}
 if(!LO8(g_eax)){
  SET_LO8(g_eax,MEM8(g_esi+12));SET_LO8(g_eax,~LO8(g_eax));g_eax=SX8(LO8(g_eax));g_eax=(g_eax&8)>>3;
  goto timing_return;
 }
 g_xmm0=XMM_SCALAR(MEMF(g_esi+16));g_xmm0.f[0]+=MEMF(g_esp+4);g_xmm1.f[0]=(float)(int32_t)MEM32(g_esi+28);
 unsigned wait_for_weight=MEM8(g_esi+12)&0x20;g_ecx=MEM32(g_esi+16);g_xmm0.f[0]-=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm0.f[0]-=g_xmm1.f[0];
 MEMF(g_esp+12)=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEMF(g_esp+28)=g_xmm0.f[0];MEM32(g_esi+20)=g_ecx;
 unsigned advance=1;
 if(wait_for_weight){g_xmm0=XMM_SCALAR(MEMF(g_esi+24));flags=compare_phase(g_xmm0.f[0],g_xmm1.f[0]);SET_HI8(g_eax,flags);advance=!RECOMP_PARITY8(HI8(g_eax)&0x44);}
 if(advance){g_xmm0=XMM_SCALAR(MEMF(g_esi+16));g_xmm0.f[0]+=MEMF(g_esp+8);MEMF(g_esi+16)=g_xmm0.f[0];}
 g_xmm0=XMM_SCALAR(MEMF(g_esi+16));flags=compare_phase(g_xmm0.f[0],g_xmm1.f[0]);g_edx=MEM32(g_esi);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);
 if(!(flags&1)){
  g_eax=MEM32(g_esi+20);g_ebx=MEM32(g_esp+32);
  {uint32_t saved=g_esp;PUSH32(g_esp,0x3f800347);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);g_ecx=g_esi;TIMING_CALL(MEM32(g_edx+32),0x1f2090,saved);}
  g_edi=(int32_t)MEMF(g_esi+16);g_xmm0=XMM_SCALAR(MEMF(g_esi+16));g_eax=MEM32(g_esi+28);g_xmm1.f[0]=(float)(int32_t)g_edi;g_xmm0.f[0]-=g_xmm1.f[0];MEMF(g_esi+16)=g_xmm0.f[0];
  if((int32_t)g_eax>=0){
   g_xmm1=XMM_SCALAR(MEMF(g_esp+12));g_eax-=g_edi;g_xmm0.f[0]+=g_xmm1.f[0];g_xmm2.f[0]=(float)(int32_t)g_eax;g_xmm0.f[0]-=g_xmm2.f[0];g_xmm0.f[0]-=MEMF(0x44bf94);
   MEM32(g_esi+28)=g_eax;MEMF(g_esp+36)=g_xmm0.f[0];
   if((int32_t)g_eax<0){
    g_xmm0=XMM_SCALAR(MEMF(0x44e554));MEMF(g_esi+16)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_edi+=g_eax;
    flags=compare_phase(g_xmm0.f[0],g_xmm1.f[0]);MEM32(g_esi+28)=0;
    if(!(flags&0x41)){g_eax=MEM32(g_esi+12)&0xbfffffffu;MEMF(g_esp+36)=g_xmm0.f[0];MEM32(g_esi+12)=g_eax;}
    else{g_edx=MEM32(g_esi);g_ecx=g_esi;uint32_t saved=g_esp;TIMING_CALL(MEM32(g_edx+48),0x1f2111,saved);}
   }
   --g_edi;
   if((int32_t)g_edi>0)do{
    g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,0x3f800347);PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);g_ecx=g_esi;TIMING_CALL(MEM32(g_eax+32),0x1f2125,saved);
   }while(--g_edi);
   g_eax=MEM32(g_esi+28);
   if((int32_t)g_eax>0){g_eax=MEM32(g_esi+16);g_edx=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);g_ecx=g_esi;TIMING_CALL(MEM32(g_edx+32),0x1f213d,saved);}
  }else{
   --g_edi;
   if((int32_t)g_edi>0)do{
    g_edx=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,0x3f800347);PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);g_ecx=g_esi;TIMING_CALL(MEM32(g_edx+32),0x1f2153,saved);
   }while(--g_edi);
   g_ecx=MEM32(g_esi+16);g_eax=MEM32(g_esi);
   {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);g_ecx=g_esi;TIMING_CALL(MEM32(g_eax+32),0x1f2164,saved);}
   g_eax=MEM32(g_esi+28);if((int32_t)g_eax<-1){++g_eax;MEM32(g_esi+28)=g_eax;}
  }
 }else{
  g_eax=MEM32(g_esi+16);g_ecx=MEM32(g_esi+20);
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+36);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_ecx=g_esi;TIMING_CALL(MEM32(g_edx+32),0x1f2184,saved);}
  g_eax=MEM32(g_esi+28);
  if((int32_t)g_eax>=0){g_xmm0=XMM_SCALAR(MEMF(g_esp+20));g_xmm0.f[0]+=MEMF(g_esp+16);MEMF(g_esp+36)=g_xmm0.f[0];}
 }
 g_xmm0=XMM_SCALAR(MEMF(g_esp+24));g_xmm3=XMM_ZERO();flags=compare_phase(g_xmm0.f[0],g_xmm3.f[0]);POP32(g_esp,g_edi);POP32(g_esp,g_ebx);
 if(!(flags&1)){
  g_xmm2=XMM_SCALAR(MEMF(g_esp+28));flags=compare_phase(g_xmm2.f[0],g_xmm3.f[0]);
  if(!(flags&0x41)){
   g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm2.f[0]/=MEMF(g_esp+4);g_xmm0=g_xmm1;g_xmm0.f[0]-=g_xmm2.f[0];MEMF(g_esi+24)=g_xmm0.f[0];
  }else{
   flags=compare_phase(g_xmm0.f[0],g_xmm3.f[0]);SET_HI8(g_eax,flags);
   if(RECOMP_PARITY8(HI8(g_eax)&0x44)){
    g_xmm1=XMM_SCALAR(MEMF(g_esp+8));g_xmm1.f[0]/=g_xmm0.f[0];g_xmm1.f[0]+=MEMF(g_esi+24);g_xmm0=g_xmm1;g_xmm1=XMM_SCALAR(MEMF(0x44bf94));
    flags=compare_phase(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_esi+24)=g_xmm0.f[0];if(!(flags&0x41))MEMF(g_esi+24)=g_xmm1.f[0];
   }else{g_xmm1=XMM_SCALAR(MEMF(0x44bf94));MEMF(g_esi+24)=g_xmm1.f[0];}
  }
  flags=compare_phase(g_xmm3.f[0],MEMF(g_esi+24));
  if(!(flags&1)){SET_LO8(g_ecx,MEM8(g_esi+12));SET_LO8(g_ecx,~LO8(g_ecx));g_eax=SX8(LO8(g_ecx));g_eax=(g_eax&8)>>3;goto timing_return;}
  g_xmm0=XMM_SCALAR(MEMF(g_esi+24));flags=compare_phase(g_xmm0.f[0],g_xmm1.f[0]);
  if(!(flags&1)){SET_LO8(g_ecx,MEM8(g_esi+12));g_eax=2;if(!(LO8(g_eax)&LO8(g_ecx)))goto timing_return;}
 }
 g_eax=0;
timing_return:
 POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
 #undef STORE_RATE
 #undef RATE_TOP
 #undef TIMING_CALL
}
