#include "hybrid_effect_support_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_EFFECT_SUPPORT");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}

/* Preserve the CRT's observable count/data registers and stack, including its
 * aligned word reads. Neither strncpy nor a byte loop has the same ABI. */
static void name_copy(void){
 g_ecx=MEM32(g_esp+12);PUSH32(g_esp,g_edi);
 if(g_ecx){
  PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_esi=MEM32(g_esp+20);g_edi=MEM32(g_esp+16);
  while(g_esi&3){
   SET_LO8(g_eax,MEM8(g_esi++));MEM8(g_edi++)=LO8(g_eax);
   if(!--g_ecx)goto done;
   if(!LO8(g_eax)){
    while(g_edi&3){MEM8(g_edi++)=0;if(!--g_ecx)goto done;}
    g_ebx=g_ecx;g_ecx>>=2;
    if(g_ecx)goto pad_words;
    goto pad_tail;
   }
  }
  g_ebx=g_ecx;g_ecx>>=2;
  while(g_ecx){
   g_edx=MEM32(g_esi);g_eax=(~g_edx)^(0x7efefeffu+g_edx);g_esi+=4;
   if(!(g_edx&0xff))g_edx=0;
   else if(!(g_edx&0xff00))g_edx&=0xff;
   else if(!(g_edx&0xff0000))g_edx&=0xffff;
   else if(g_edx&0xff000000){MEM32(g_edi)=g_edx;g_edi+=4;--g_ecx;continue;}
   MEM32(g_edi)=g_edx;g_edi+=4;g_eax=0;--g_ecx;
pad_words:
   g_eax=0;while(g_ecx){MEM32(g_edi)=0;g_edi+=4;--g_ecx;}
   g_ebx&=3;if(g_ebx)goto pad_tail;goto done;
  }
  g_ebx&=3;
  while(g_ebx){SET_LO8(g_eax,MEM8(g_esi++));MEM8(g_edi++)=LO8(g_eax);--g_ebx;if(!LO8(g_eax))goto pad_tail;}
  goto done;
pad_tail:
  while(g_ebx){MEM8(g_edi++)=0;--g_ebx;}
done:
  g_eax=MEM32(g_esp+16);POP32(g_esp,g_ebx);POP32(g_esp,g_esi);
 }else g_eax=MEM32(g_esp+8);
 POP32(g_esp,g_edi);g_esp+=4;
}
static void emitter_defaults(void){
 g_esp-=12;g_xmm0=XMM_ZERO();g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_eax=g_ecx;
 MEM32(g_esp)=MEM32(g_esp+4)=MEM32(g_esp+8)=0;
 for(unsigned i=0;i<8;i++)MEM32(g_eax+i*4)=0;
 MEMF(g_eax+0x20)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44c178));MEMF(g_eax+0x24)=g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(MEMF(0x44c5e4));MEMF(g_eax+0x28)=g_xmm1.f[0];MEM32(g_eax+0x2c)=0;
 g_edx=0;g_ecx=g_eax+16;g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void parameter_defaults(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-28;
 g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_eax=g_ecx;g_xmm0=XMM_SCALAR(MEMF(0x4600d8));
 g_xmm3=XMM_SCALAR(MEMF(0x44c178));g_xmm2=XMM_SCALAR(MEMF(0x44bf80));MEMF(g_eax+0x9c)=g_xmm0.f[0];
 g_xmm0=XMM_ZERO();MEM32(g_esp+12)=0;MEM32(g_eax+0xb0)=0;MEMF(g_esp+16)=g_xmm1.f[0];MEMF(g_eax+0xb4)=g_xmm1.f[0];PUSH32(g_esp,g_esi);
 for(unsigned i=0;i<7;i++)MEM32(g_eax+i*4)=0;
 MEMF(g_eax+0xc0)=g_xmm3.f[0];g_ecx=0;
 unsigned zero[]={0x64,0x90,0xa8,0xb8,0xbc,0xc4,0xc8,0x94,0x70};
 for(unsigned i=0;i<sizeof zero/sizeof *zero;i++)MEM32(g_eax+zero[i])=0;
 MEMF(g_eax+0xa0)=g_xmm2.f[0];MEMF(g_eax+0x50)=g_xmm2.f[0];MEMF(g_eax+0x54)=g_xmm2.f[0];
 unsigned one[]={0xa4,0xac,0xcc,0x60,0xd0};for(unsigned i=0;i<sizeof one/sizeof *one;i++)MEMF(g_eax+one[i])=g_xmm1.f[0];
 g_xmm3=g_xmm1;g_xmm4=XMM_SHUFFLE(g_xmm3,g_xmm3,0);XMM_STORE(g_eax+0x20,g_xmm4);XMM_STORE(g_eax+0x30,g_xmm4);
 MEMF(g_esp+16)=g_xmm1.f[0];MEMF(g_esp+20)=g_xmm1.f[0];MEMF(g_esp+24)=g_xmm1.f[0];MEM32(g_esp+28)=0;
 g_xmm2=XMM_MEM(g_esp+16);XMM_STORE(g_eax+0x40,g_xmm2);g_xmm2=XMM_SCALAR(MEMF(0x44bf18));
 MEMF(g_eax+0x58)=g_xmm2.f[0];MEMF(g_eax+0x5c)=g_xmm2.f[0];MEM32(g_eax+0x68)=4;
 MEM8(g_eax+0x6c)=0;MEM8(g_eax+0x74)=1;MEM8(g_eax+0x75)=1;MEM8(g_eax+0x76)=0;MEM8(g_eax+0x98)=0;
 g_edx=g_esp+16;MEM32(g_edx)=0;g_xmm0=XMM_ZERO();g_xmm1=XMM_ZERO();XMM_STORE(g_eax+0x80,g_xmm1);
 POP32(g_esp,g_esi);g_esp=frame;uint32_t ignored;POP32(g_esp,ignored);(void)ignored;g_recomp_zf=1;g_esp+=4;
}
int uc_hybrid_effect_support(unsigned entry){
 if(!enabled())return 0;
 uint32_t s=g_esp,d=g_ecx;
 if((s&3)||s<0x10080u||!span(s-128,144)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x238ef0||entry==0x238f10){
  unsigned offset=entry==0x238ef0?0:8;uint32_t node=MEM32(s+4),next=0;unsigned count=0;
  /* ponytail: finite preflight bounds malformed/cyclic trees; valid deeper
   * trees use the original path until a measured need justifies cycle tracking. */
  do{if(++count>4096||!span(node,12))return 0;next=MEM32(node+offset);if(!span(next,0x192))return 0;if(MEM8(next+0x191))break;node=next;}while(1);
  g_eax=node;g_ecx=next;SET_LO8(g_edx,MEM8(next+0x191));g_recomp_zf=0;g_esp+=4;return 1;
 }
 if(entry==0x238e90){
  uint32_t a=MEM32(s+4);if(!span(a,12)||!span(d,28))return 0;
  g_eax=a;g_edx=MEM32(g_eax);g_ecx+=16;MEM32(g_ecx)=g_edx;g_edx=MEM32(g_eax+4);MEM32(g_ecx+4)=g_edx;g_eax=MEM32(g_eax+8);MEM32(g_ecx+8)=g_eax;g_recomp_zf=g_ecx==0;g_esp+=8;return 1;
 }
 if(entry==0x2d6380){
  uint32_t a=MEM32(s+8),n=MEM32(s+12);d=MEM32(s+4);
  if(n&&(!span(a,n)||!span(d,n)||!apart(a,n,d,n)||!apart(a,n,s-128,144)||!apart(d,n,s-128,144)))return 0;
  name_copy();return 1;
 }
 unsigned n=entry==0x23d700?0x160:0;
 if(!n||!span(d,n)||!apart(d,n,s-128,144)||!apart(d,n,0x440000,0x22000)||!span(0x440000,0x22000))return 0;
 g_xmm0=XMM_SCALAR(MEMF(0x44bf18));PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEMF(g_esi+4)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_edi=g_esi+16;g_ecx=g_edi;MEM32(g_esi)=0;MEM32(g_esi+8)=0;MEM32(g_esi+12)=20;
 PUSH32(g_esp,0x23d730);emitter_defaults();g_ecx=g_edi+0x30;PUSH32(g_esp,0x23d738);parameter_defaults();
 PUSH32(g_esp,63);MEM8(g_edi+0x110)=0;PUSH32(g_esp,0x4600dc);g_edi+=0x111;PUSH32(g_esp,g_edi);PUSH32(g_esp,0x23d752);name_copy();
 g_esp+=12;POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
