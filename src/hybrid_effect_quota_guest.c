#include "hybrid_effect_quota_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void);
static int quota_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_EFFECT_QUOTA");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_effect_quota(unsigned operation){
 if(!quota_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if(operation>3||(s&3)||s<0x10100u||!span(s-256,276))return 0;
 if(operation&&(!span(p,0x508)||!apart(p,0x508,s-256,276)||MEM32(p+0x504)>64))return 0;
 if((operation==1||operation==2)&&(!span(MEM32(s+4),12)||!apart(MEM32(s+4),12,s-256,276)))return 0;
 /* This is the world's existing quota object; never a parallel native copy.
  * Preserve scratch registers/stack because callers still cross the guest ABI. */
 if(!operation){
  PUSH32(g_esp,0x508);PUSH32(g_esp,0x493ca);sub_002D57FB();g_esp+=4;
  if(g_eax){MEM32(g_eax)=0x4503a0;MEM32(g_eax+0x504)=0;}
  g_recomp_zf=g_eax==0;g_esp+=4;return 1;
 }
 if(operation==1){
  g_eax=MEM32(g_ecx+0x504);
  if(g_eax==64){
   /* Retail11030 is exactly RET. Keep its stack writes and skip dispatch. */
   PUSH32(g_esp,0x4503ac);PUSH32(g_esp,0x492b3);g_esp+=4;POP32(g_esp,g_ecx);
   g_recomp_zf=1;g_esp+=16;return 1;
  }
  g_edx=MEM32(g_esp+4);g_xmm0=XMM_SCALAR(MEMF(g_esp+8));PUSH32(g_esp,g_esi);
  g_eax=g_ecx+g_eax*20+4;PUSH32(g_esp,g_edi);g_edi=MEM32(g_edx);g_esi=g_eax;MEM32(g_esi)=g_edi;
  g_edi=MEM32(g_edx+4);MEM32(g_esi+4)=g_edi;g_edx=MEM32(g_edx+8);
  g_xmm1=g_xmm0;g_xmm1.f[0]*=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+20));
  MEM32(g_esi+8)=g_edx;MEMF(g_eax+12)=g_xmm1.f[0];MEMF(g_eax+16)=g_xmm0.f[0];
  g_eax=MEM32(g_ecx+0x504)+1;POP32(g_esp,g_edi);MEM32(g_ecx+0x504)=g_eax;POP32(g_esp,g_esi);
  g_recomp_zf=g_eax==0;g_esp+=16;return 1;
 }
 if(operation==2){
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+0x504);g_edx=0;PUSH32(g_esp,g_edi);
  if(g_esi){
   g_edi=MEM32(g_esp+12);g_eax=g_ecx+12;
   do{
    g_xmm0=XMM_SCALAR(MEMF(g_edi));g_xmm0.f[0]-=MEMF(g_eax-8);
    g_xmm2=XMM_SCALAR(MEMF(g_edi+8));g_xmm2.f[0]-=MEMF(g_eax);
    g_xmm1=XMM_SCALAR(MEMF(g_edi+4));g_xmm1.f[0]-=MEMF(g_eax-4);
    g_xmm3=g_xmm0;g_xmm3.f[0]*=g_xmm0.f[0];g_xmm0=g_xmm2;g_xmm0.f[0]*=g_xmm2.f[0];g_xmm3.f[0]+=g_xmm0.f[0];
    g_xmm0=g_xmm1;g_xmm0.f[0]*=g_xmm1.f[0];g_xmm3.f[0]+=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+4));
    double radius=g_xmm0.f[0],distance=g_xmm3.f[0];
    if(radius==radius&&distance==distance&&radius>distance){POP32(g_esp,g_edi);SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_recomp_zf=0;g_esp+=8;return 1;}
    ++g_edx;g_eax+=20;
   }while(g_edx<g_esi);
  }
  POP32(g_esp,g_edi);SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;return 1;
 }
 g_eax=MEM32(g_ecx+0x504);PUSH32(g_esp,g_esi);g_esi=0;
 if(g_eax){
  g_xmm1=XMM_SCALAR(MEMF(g_esp+8));g_xmm2=XMM_ZERO();PUSH32(g_esp,g_ebx);g_edx=g_ecx+20;PUSH32(g_esp,g_edi);
  do{
   g_xmm0=XMM_SCALAR(MEMF(g_edx));g_xmm0.f[0]-=g_xmm1.f[0];double remaining=g_xmm0.f[0];MEMF(g_edx)=g_xmm0.f[0];
   /* Original unordered comparison retains NaN, exactly like a positive age. */
   if(remaining!=remaining||remaining>0){++g_esi;g_edx+=20;}
   else{
    g_eax=MEM32(g_ecx+0x504);
    if(g_eax>1){
     g_eax=g_ecx+g_eax*20-16;g_ebx=MEM32(g_eax);g_edi=g_edx-16;MEM32(g_edi)=g_ebx;
     for(unsigned offset=4;offset<16;offset+=4){g_ebx=MEM32(g_eax+offset);MEM32(g_edi+offset)=g_ebx;}
     g_eax=MEM32(g_eax+16);MEM32(g_edi+16)=g_eax;
    }
    --MEM32(g_ecx+0x504);
   }
  }while(g_esi<MEM32(g_ecx+0x504));
  POP32(g_esp,g_edi);POP32(g_esp,g_ebx);
 }
 POP32(g_esp,g_esi);g_esp+=8;return 1;
}
