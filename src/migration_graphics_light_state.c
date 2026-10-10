#include "migration_graphics_light_state.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void),sub_003EDEC0(void),sub_003ECEF0(void),sub_003EDD00(void),sub_003EDC10(void),sub_003E8760(void),sub_003E89F0(void),sub_003E8990(void);
static int light_state_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_LIGHT_STATE");flag=v&&!strcmp(v,"1");}return flag;}
static int state_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void state_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void state_store(uint32_t p){MEMF(p)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
#define LIGHT_FP(n) g_fp_stack[(g_fp_top+(n))&7]
#define LIGHT_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static void light_vector(void){
 g_eax=MEM32(g_esp+8);g_ecx=MEM32(g_esp+16);
 for(unsigned row=0;row<4;row++){
  if(row==3)g_eax=MEM32(g_esp+4);
  RecompXmm value=XMM_SCALAR(row==3?MEMF(g_esp+12):MEMF(g_eax+4*row));value=XMM_SHUFFLE(value,value,0);value=XMM_MUL(value,XMM_MEM(g_ecx+16*row));
  if(!row)g_xmm2=value;else{g_xmm1=value;g_xmm2=XMM_ADD(g_xmm2,g_xmm1);}
 }
 XMM_STORE_LOW(g_eax,g_xmm2);g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm2,2);MEMF(g_eax+8)=g_xmm2.f[0];g_esp+=20;
}
static void light_state(void){
 uint32_t frame=g_seh_ebp;g_esp-=44;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+56);g_eax=MEM32(frame);int grow=g_eax>=MEM32(frame+4);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);if(grow)LIGHT_CALL(sub_003E9810,0x3ee018);
 g_ecx=MEM32(g_esp+68);g_esi=g_eax;
 if(g_ecx&0x1000){
  g_ecx=MEM32(0x3f413c);g_ebx=1;g_ecx=(g_ecx||(MEM8(frame+8)&64))?1:0;g_edx=MEM32(frame+0x794);int enabled=!(MEM8(g_edx+4)&0x12);
  if(enabled){g_edx=MEM32(0x3f4138);enabled=g_edx!=0;}
  if(enabled){g_edx=MEM32(frame+0x1928)&~16u;enabled=g_edx!=g_ebx;}
  if(enabled){
   g_edi=g_ebx;if(g_ecx){g_ecx=MEM32(0x3f4140);if(g_ecx){g_ecx=MEM32(frame+0x7cc);if(g_ecx)g_edi=0x10001;}LIGHT_CALL(sub_003EDEC0,0x3ee089);g_esi=g_eax;}
   MEM32(g_esi)=0x40294;MEM32(g_esi+4)=g_edi;MEM32(g_esi+8)=0x40314;MEM32(g_esi+12)=g_ebx;MEM32(g_esi+16)=0x403b8;MEM32(g_esi+20)=g_ebx;LIGHT_CALL(sub_003ECEF0,0x3ee0ad);
   MEM32(frame+0x7d0)=g_eax;MEM32(g_esi+24)=0x40298;MEM32(g_esi+28)=g_eax;PUSH32(g_esp,g_eax);g_eax=g_esi+32;LIGHT_CALL(sub_003EDD00,0x3ee0c6);g_esi=g_eax;
  }else{MEM32(g_eax)=0x40314;MEM32(g_eax+4)=0;MEM32(g_eax+8)=0x403b8;MEM32(g_eax+12)=g_ecx;MEM32(g_eax+16)=0x40294;MEM32(g_eax+20)=0x20001;g_esi=g_eax+24;}
 }
 g_edx=MEM32(g_esp+68);
 if(g_edx&0xff8000){
  g_eax=MEM32(frame+0x7d0);MEM32(g_esp+20)=g_eax;g_eax=MEM32(frame+0x7cc);MEM32(g_esi)=0x403bc;MEM32(g_esi+4)=g_eax;g_esi+=8;g_ecx=0;g_eax=frame+0x7ac;MEM32(g_esp+64)=g_ecx;g_ebx=0x41024;MEM32(g_esp+16)=g_eax;
  for(;;){
   g_eax=MEM32(g_esp+16);g_edi=MEM32(g_eax);
   if(g_edi){
    g_eax=0x10000u<<(g_ecx&31);int transform=(g_edx&g_eax)!=0;
    if(transform||(g_edx&0x8000)){
     MEM32(frame)=g_esi;g_eax=g_esi;if(g_esi>=MEM32(frame+4)){if(transform)LIGHT_CALL(sub_003E9810,0x3ee196);else LIGHT_CALL(sub_003E9810,0x3ee170);}
     if(!transform){g_ecx=MEM32(g_esp+64);g_edx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_edi;LIGHT_CALL(sub_003EDC10,0x3ee180);g_esi=g_eax;}
     else{
      g_edx=MEM32(g_esp+64);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+24);g_ecx=g_edi;LIGHT_CALL(sub_003EDC10,0x3ee1a6);g_esi=g_eax;g_eax=frame+0xc60;PUSH32(g_esp,g_eax);
      if(MEM32(g_edi)==3){
       PUSH32(g_esp,0);g_edi+=0x6c;PUSH32(g_esp,g_edi);g_ecx=g_esp+36;PUSH32(g_esp,g_ecx);LIGHT_CALL(sub_003E8760,0x3ee1c8);g_edx=g_esp+24;PUSH32(g_esp,g_edx);LIGHT_CALL(sub_003E89F0,0x3ee1d2);
       state_push(MEMF(g_esp+32));LIGHT_FP(0)-=MEMF(0x44bf94);g_eax=MEM32(g_esp+24);g_ecx=MEM32(g_esp+28);g_edx=g_esp+36;PUSH32(g_esp,g_edx);state_store(g_esp+48);MEM32(g_esp+40)=g_eax;MEM32(g_esp+44)=g_ecx;LIGHT_CALL(sub_003E89F0,0x3ee1fa);
       MEM32(g_esi)=g_ebx;MEM32(g_esi+4)=0x7149f2ca;g_eax=g_ebx+0x140004;MEM32(g_esi+8)=g_eax;
       g_ecx=MEM32(g_esp+36);MEM32(g_esi+12)=g_ecx;g_edx=MEM32(g_esp+40);MEM32(g_esi+16)=g_edx;g_eax=MEM32(g_esp+44);MEM32(g_esi+20)=g_eax;g_ecx=MEM32(g_esp+24);MEM32(g_esi+24)=g_ecx;g_edx=MEM32(g_esp+28);MEM32(g_esi+28)=g_edx;g_eax=MEM32(g_esp+32);MEM32(g_esi+32)=g_eax;g_esi+=36;
      }else{
       state_push(MEMF(g_edi+0x4c));PUSH32(g_esp,0x3f800000);g_ecx=g_edi+0x34;state_store(g_esi+4);PUSH32(g_esp,g_ecx);g_edx=g_esp+60;MEM32(g_esi)=g_ebx;PUSH32(g_esp,g_edx);LIGHT_CALL(sub_003E8760,0x3ee259);
       g_eax=g_ebx+0x140038;MEM32(g_esi+8)=g_eax;g_ecx=MEM32(g_esp+48);MEM32(g_esi+12)=g_ecx;g_edx=MEM32(g_esp+52);MEM32(g_esi+16)=g_edx;g_eax=MEM32(g_esp+56);MEM32(g_esi+20)=g_eax;g_ecx=MEM32(g_edi+0x54);MEM32(g_esi+24)=g_ecx;g_edx=MEM32(g_edi+0x58);MEM32(g_esi+28)=g_edx;g_eax=MEM32(g_edi+0x5c);MEM32(g_esi+32)=g_eax;g_eax=MEM32(g_edi);g_esi+=36;
       if(g_eax==2){
        g_eax=frame+0xc60;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_ecx=g_edi+0x6c;PUSH32(g_esp,g_ecx);g_edx=g_esp+36;PUSH32(g_esp,g_edx);LIGHT_CALL(sub_003E8760,0x3ee2ae);
        state_push(MEMF(g_esp+32));LIGHT_FP(0)*=MEMF(g_esp+32);PUSH32(g_esp,g_ecx);state_push(MEMF(g_esp+32));LIGHT_FP(0)*=MEMF(g_esp+32);LIGHT_FP(1)+=LIGHT_FP(0);g_fp_top=(g_fp_top+1)&7;state_push(MEMF(g_esp+28));LIGHT_FP(0)*=MEMF(g_esp+28);LIGHT_FP(1)+=LIGHT_FP(0);g_fp_top=(g_fp_top+1)&7;state_store(g_esp);LIGHT_CALL(sub_003E8990,0x3ee2d3);
        LIGHT_FP(0)*=MEMF(g_edi+0x84);g_eax=g_ebx+0x18001c;MEM32(g_esi)=g_eax;g_ecx=MEM32(g_edi+0x78);MEM32(g_esi+4)=g_ecx;g_edx=MEM32(g_edi+0x7c);MEM32(g_esi+8)=g_edx;g_eax=MEM32(g_edi+0x80);MEM32(g_esi+12)=g_eax;state_push(MEMF(g_esp+24));LIGHT_FP(0)*=LIGHT_FP(1);g_esi+=32;state_store(g_esi-16);state_push(MEMF(g_esp+28));LIGHT_FP(0)*=LIGHT_FP(1);state_store(g_esi-12);LIGHT_FP(0)*=MEMF(g_esp+32);state_store(g_esi-8);g_ecx=MEM32(g_edi+0x88);MEM32(g_esi-4)=g_ecx;
       }
      }
     }
    }
   }
   g_ecx=MEM32(g_esp+64);g_edi=MEM32(g_esp+16);g_ecx++;g_edi+=4;g_ebx+=128;MEM32(g_esp+64)=g_ecx;MEM32(g_esp+16)=g_edi;if(g_ebx>=0x41424)break;g_edx=MEM32(g_esp+68);
  }
 }
 POP32(g_esp,g_edi);MEM32(frame)=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=44;g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_graphics_light_state(uint32_t entry){
 if(!light_state_enabled())return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!state_span(s-128,152)||!state_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7)return 0;
 if(entry==0x3e8760){
  uint32_t v=MEM32(s+8),m=MEM32(s+16);if((_mm_getcsr()&0x1f80)!=0x1f80||!state_span(MEM32(s+4),12)||!state_span(v,12)||!state_span(m,64))return 0;
  for(unsigned i=0;i<3;i++)if((MEM32(v+4*i)&0x7fffffff)>0x5d800000)return 0;for(unsigned i=0;i<16;i++)if((MEM32(m+4*i)&0x7fffffff)>0x5d800000)return 0;if((MEM32(s+12)&0x7fffffff)>0x5d800000)return 0;
  light_vector();return 1;
 }
 if(entry==0x3ee000){if(!state_span(MEM32(s+4),0x192c))return 0;light_state();return 1;}return 0;
}
