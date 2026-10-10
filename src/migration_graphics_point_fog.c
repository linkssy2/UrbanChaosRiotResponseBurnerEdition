#include "migration_graphics_point_fog.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void),sub_003EAA50(void),sub_003E6DF0(void);
#define PF_PUSH(v) do{double value=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value;}while(0)
#define PF_POP() (g_fp_top=(g_fp_top+1u)&7u)
#define PF_ST(i) g_fp_stack[(g_fp_top+(i))&7u]
static void pf_status(void){g_eax=(g_eax&0xffff0000u)|(uint16_t)(((g_fp_top&7u)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700u):g_fp_cmp==2?0x4500u:g_fp_cmp<0?0x100u:g_fp_cmp>0?0u:0x4000u));}
static int pf_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_POINT_FOG");flag=v&&!strcmp(v,"1");}return flag;}
static int pf_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void point_state(void){
 g_esp-=20;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+32);g_eax=MEM32(g_edi);
 if(g_eax>=MEM32(g_edi+4)){PUSH32(g_esp,0x3ecfa5);sub_003E9810();}
 g_ecx=MEM32(0x3f4174);g_edx=MEM32(0x3f418c);MEM32(g_esp+8)=g_ecx;g_ecx=MEM32(0x3f4170);MEM32(g_esp+12)=g_ecx;g_ecx=MEM32(0x3f417c);g_esi=g_eax;MEM32(g_esp+32)=g_edx;
 if(!g_ecx){
  PF_PUSH(MEMF(g_esp+12));PF_ST(0)*=MEMF(g_edi+0x964);g_fp_cmp=RECOMP_FCMP(PF_ST(0),MEMF(g_esp+8));pf_status();
  if(!RECOMP_PARITY8(HI8(g_eax)&5)){PF_POP();PF_PUSH(MEMF(g_esp+8));}
  g_fp_cmp=RECOMP_FCMP(PF_ST(0),MEMF(g_esp+32));pf_status();if(!(HI8(g_eax)&0x41)){PF_POP();PF_PUSH(MEMF(g_esp+32));}
  g_fp_cmp=RECOMP_FCMP(PF_ST(0),MEMF(0x3f2fbc));pf_status();if(!(HI8(g_eax)&0x41)){PF_POP();PF_PUSH(MEMF(0x3f2fbc));}
 }else{
  g_edx=MEM32(0x3f4180);PF_PUSH(MEMF(g_esp+32));PF_ST(0)-=MEMF(g_esp+8);MEM32(g_esp+16)=g_edx;g_edx=MEM32(0x3f4184);MEM32(g_esp+20)=g_edx;g_edx=MEM32(0x3f4188);MEMF(g_esp+32)=(float)PF_ST(0);PF_POP();PF_PUSH(MEMF(g_esp+12));MEM32(g_esp+24)=g_edx;g_edx=MEM32(g_edi+0xeec);MEM32(g_esp+12)=g_edx;PF_PUSH((double)(int32_t)g_edx);if((int32_t)g_edx<0)PF_ST(0)+=MEMF(0x44c510);
  PF_ST(0)*=PF_ST(1);MEM32(g_eax)=0x200a30;g_edx=MEM32(g_esp+32);PF_ST(0)=MEMF(g_esp+32)/PF_ST(0);MEM32(g_eax+16)=g_edx;MEM32(g_eax+20)=g_edx;MEM32(g_eax+24)=g_edx;g_edx=MEM32(g_esp+8);MEM32(g_eax+32)=g_edx;g_esi=g_eax+36;
  PF_PUSH(PF_ST(0));PF_ST(1)*=PF_ST(0);PF_POP();PF_PUSH(MEMF(g_esp+16));PF_ST(0)*=PF_ST(1);MEMF(g_eax+4)=(float)PF_ST(0);PF_POP();PF_PUSH(MEMF(g_esp+20));PF_ST(0)*=PF_ST(1);MEMF(g_eax+8)=(float)PF_ST(0);PF_POP();PF_ST(0)*=MEMF(g_esp+24);MEMF(g_eax+12)=(float)PF_ST(0);PF_POP();
  PF_PUSH(MEMF(g_esp+8));PF_ST(0)/=MEMF(g_esp+32);PF_ST(0)=-PF_ST(0);MEMF(g_eax+28)=(float)PF_ST(0);PF_POP();
 }
 PF_ST(0)*=MEMF(0x44eef8);g_eax=MEM32(0x3f4178);MEM32(g_esi)=0x80318;PUSH32(g_esp,g_ecx);PF_ST(0)+=MEMF(0x44c1f8);MEM32(g_esi+4)=g_ecx;MEM32(g_esi+8)=g_eax;MEMF(g_esp)=(float)PF_ST(0);PF_POP();PUSH32(g_esp,0x3ed0d2);sub_003EAA50();
 if(g_eax>0x1ff)g_eax=0x1ff;MEM32(g_esi+12)=0x4043c;MEM32(g_esi+16)=g_eax;g_esi+=20;MEM32(g_edi)=g_esi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void fog_state(void){
 g_esp-=8;PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+16);g_ecx=g_esi;PUSH32(g_esp,0x3ed67f);sub_003E6DF0();g_eax=MEM32(0x3f4110);
 if(g_eax){
  g_ecx=MEM32(0x3f4118);g_eax=MEM32(0x3f4114);g_edx=MEM32(0x3f411c);PUSH32(g_esp,g_ebx);g_ebx=0;PUSH32(g_esp,g_edi);g_edi=MEM32(0x3f4124);g_ebx=g_edi==0;MEM32(g_esp+12)=g_ecx;g_ecx=MEM32(0x3f4120);MEM32(g_esp+24)=g_edx;MEM32(g_esp+16)=g_ecx;g_ebx++;
  if(!g_eax){MEM32(g_esp+24)=0x3f800000;MEM32(g_esp+12)=0x3f800000;g_edi=0x2601;g_ebx=0;}
  else{
   if(g_eax==3){
    PF_PUSH(MEMF(g_esp+12));PF_PUSH(MEMF(g_esp+24));g_fp_cmp=RECOMP_FCMP(PF_ST(0),PF_ST(1));PF_POP();PF_POP();pf_status();
    if(RECOMP_PARITY8(HI8(g_eax)&0x44)){PF_PUSH(MEMF(g_esp+24));PF_ST(0)-=MEMF(g_esp+12);PF_ST(0)=MEMF(0x44bf94)/PF_ST(0);}else PF_PUSH(MEMF(0x3f2fc0));
    PF_PUSH(MEMF(g_esp+24));g_edi=0x2601;PF_ST(0)*=PF_ST(1);PF_ST(0)+=MEMF(0x44bf94);MEMF(g_esp+24)=(float)PF_ST(0);PF_POP();PF_ST(0)=-PF_ST(0);
   }else{int exponential=g_eax==1;PF_PUSH(MEMF(g_esp+16));MEM32(g_esp+24)=0x3fc00000;PF_ST(0)*=MEMF(exponential?0x455a6c:0x455a68);g_edi=exponential?0x800:0x801;}
   MEMF(g_esp+12)=(float)PF_ST(0);PF_POP();
  }
  g_eax=MEM32(g_esi);if(g_eax>=MEM32(g_esi+4)){PUSH32(g_esp,0x3ed75a);sub_003E9810();}
  g_ecx=MEM32(g_esp+12);g_edx=MEM32(g_esp+24);MEM32(g_eax)=0x802a0;MEM32(g_eax+4)=g_ebx;MEM32(g_eax+8)=1;MEM32(g_eax+12)=0x4029c;MEM32(g_eax+16)=g_edi;MEM32(g_eax+20)=0xc09c0;MEM32(g_eax+24)=g_edx;MEM32(g_eax+28)=g_ecx;MEM32(g_eax+32)=0;
  g_ecx=MEM32(g_esi+0x784);int shader=g_ecx!=0;POP32(g_esp,g_edi);POP32(g_esp,g_ebx);if(shader){g_ecx=MEM32(g_esi+0x788);shader=g_ecx!=0;}
  if(shader)g_eax+=36;
  else{g_ecx=(MEM32(0x3f413c)?0x20000:0)+0x130c0300;MEM32(g_eax+36)=0x80288;MEM32(g_eax+40)=g_ecx;MEM32(g_eax+44)=0x1c80;g_eax+=48;}
 }else{
  g_eax=MEM32(g_esi);if(g_eax>=MEM32(g_esi+4)){PUSH32(g_esp,0x3ed7f1);sub_003E9810();}
  MEM32(g_eax)=0x402a4;MEM32(g_eax+4)=0;g_ecx=MEM32(g_esi+0x784);int shader=g_ecx!=0;if(shader){g_ecx=MEM32(g_esi+0x788);shader=g_ecx!=0;}
  if(shader)g_eax+=8;
  else{g_edx=MEM32(0x3f413c);g_ecx=g_edx!=0;MEM32(g_eax+8)=0x80288;g_eax+=20;g_ecx=g_ecx*2+12;MEM32(g_eax-8)=g_ecx;MEM32(g_eax-4)=0x1c80;}
 }
 MEM32(g_esi)=g_eax;POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_graphics_point_fog(uint32_t entry){
 if(!pf_enabled()||(entry!=0x3ecf90&&entry!=0x3ed670))return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10400||!pf_span(s-1024,1040)||(unsigned)g_fp_top>7||!pf_span(0x51c178,1)||MEM8(0x51c178)||!pf_span(0x3f4110,0x80)||!pf_span(MEM32(s+4),0xef0))return 0;
 if(entry==0x3ecf90)point_state();else fog_state();return 1;
}
