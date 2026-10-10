#include "hybrid_fx_fire_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_00208C50(void),sub_00219530(void),sub_002D57FB(void),sub_002D5CCD(void);
extern void sub_00207B30(void),sub_002084E0(void),sub_00210320(void),sub_00216BD0(void);
extern void sub_0018F250(void),sub_0018F6A0(void),sub_0018EE10(void),sub_0018EF10(void),sub_0018FD40(void);
extern void sub_00218C40(void),sub_002160C0(void),sub_00190440(void),sub_00190D10(void),sub_001F9140(void),sub_00218CE0(void);
extern void sub_00218A60(void),sub_001F8DC0(void),sub_000BBC20(void),sub_00216020(void);
extern void sub_0023EB60(void),sub_00219450(void),sub_00210CA0(void),sub_001F8FC0(void),sub_0020A140(void),sub_002D631D(void),sub_00218B30(void);
extern void sub_002D38F0(void),sub_0003BD10(void),sub_002D4360(void),sub_00055EE0(void),sub_000236D0(void),sub_001F9120(void),sub_001903C0(void),sub_0018FCA0(void);
extern void sub_00190270(void);
#define FIRE_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define FIRE_FRAME_CALL(fn,pc) do{g_ebp=frame;FIRE_CALL(fn,pc);}while(0)
#define FIRE_VCALL(target,pc,saved) do{uint32_t fire_target=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(fire_target,saved);}while(0)
static unsigned fire_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}

/* Each row is one original publication, with its exact ten retained call PCs.
 * The two entry points use different properties; only the first rate in the
 * first set is doubled. Finite lifetime remains the original custom emitter. */
static const uint32_t fire_recipe_pc[6][10]={
 {0x18f281,0x18f297,0x18f2f5,0x18f2fd,0x18f310,0x18f331,0x18f34e,0x18f3b1,0x18f3c8,0x18f3d6},
 {0x18f3e1,0x18f3f4,0x18f449,0x18f451,0x18f462,0x18f485,0x18f4a2,0x18f509,0x18f51d,0x18f52b},
 {0x18f536,0x18f54c,0x18f5aa,0x18f5b2,0x18f5c3,0x18f5e6,0x18f603,0x18f660,0x18f67b,0x18f689},
 {0x18f6d1,0x18f6e7,0x18f745,0x18f74d,0x18f75e,0x18f77f,0x18f79c,0x18f7ff,0x18f816,0x18f824},
 {0x18f82f,0x18f842,0x18f897,0x18f89f,0x18f8b0,0x18f8d3,0x18f8f0,0x18f957,0x18f96b,0x18f979},
 {0x18f984,0x18f99a,0x18f9f8,0x18fa00,0x18fa11,0x18fa34,0x18fa51,0x18faae,0x18fac9,0x18fad7}
};
static void fire_recipes(unsigned second){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x3965fa);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x448;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_edi);
 for(unsigned phase=0;phase<3;phase++){
  const uint32_t*pc=fire_recipe_pc[second*3+phase];unsigned desc=phase==0?0x190:phase==1?0x30:0x2f0;
  if(phase)g_ecx=MEM32(0x51c4dc);FIRE_FRAME_CALL(sub_00208C50,pc[0]);
  g_edx=MEM32(g_eax);g_ebx=g_esi+0xf4+12*second+4*phase;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);g_ecx=g_esp+desc+4;PUSH32(g_esp,g_ecx);g_ecx=g_eax;FIRE_VCALL(MEM32(g_edx+0x1c),pc[1],saved);}
  g_xmm0=XMM_SCALAR(MEMF(g_esi+0x168));g_xmm2=XMM_SCALAR(MEMF(g_esi+0x158));g_xmm1=XMM_SCALAR(MEMF(g_esi+0x148));g_xmm2.f[0]=g_xmm2.f[0]*MEMF(0x44c538);
  MEMF(g_esp+0x28)=g_xmm0.f[0];g_ecx=MEM32(g_esp+0x28);MEMF(g_esp+0x20)=g_xmm2.f[0];g_edx=MEM32(g_esp+0x20);MEMF(g_esp+0x24)=g_xmm1.f[0];g_eax=MEM32(g_esp+0x24);
  MEM32(g_esp+desc+12)=g_ecx;g_ecx=MEM32(0x51c4dc);MEM32(g_esp+desc+4)=g_edx;MEM32(g_esp+desc+8)=g_eax;FIRE_FRAME_CALL(sub_00208C50,pc[2]);
  g_edx=MEM32(g_eax);{uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);g_ecx=g_eax;FIRE_VCALL(MEM32(g_edx+0x34),pc[3],saved);}
  g_fp_stack[g_fp_top]=g_fp_stack[g_fp_top]*MEMF(g_esi+0x16c);PUSH32(g_esp,0x1c);
  if(!second&&!phase)g_fp_stack[g_fp_top]=g_fp_stack[g_fp_top]+g_fp_stack[g_fp_top];
  MEMF(g_esp+0x18)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;FIRE_FRAME_CALL(sub_002D57FB,pc[4]);
  g_edi=g_eax;g_esp+=4;MEM32(g_esp+0x2c)=g_edi;if(!phase)g_eax=0;MEM32(g_esp+0x45c)=phase;
  if(g_edi){
   g_ecx=MEM32(0x51c4dc);FIRE_FRAME_CALL(sub_00208C50,pc[5]);
   g_ecx=MEM32(g_esi+0x54);g_xmm0=XMM_SCALAR(MEMF(g_esi+0x170));g_edx=MEM32(g_eax);MEM32(g_esp+(phase?0x18:0x1c))=g_ecx;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);g_ecx=g_eax;MEMF(g_esp+(phase?0x20:0x1c))=g_xmm0.f[0];FIRE_VCALL(MEM32(g_edx+0x20),pc[6],saved);}
   g_xmm0=XMM_SCALAR(MEMF(g_esp+0x14));g_ecx=MEM32(g_esp+(phase?0x18:0x1c));MEMF(g_edi+12)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+(phase?0x1c:0x18)));
   unsigned flags=fire_compare(g_xmm0.f[0],MEMF(0x44bf80));MEM32(g_edi+8)=g_eax;SET_HI8(g_eax,flags);MEM32(g_edi)=0x474270;MEM32(g_edi+4)=g_ecx;MEMF(g_edi+16)=g_xmm0.f[0];MEM8(g_edi+20)=1;
   if(!RECOMP_PARITY8(HI8(g_eax)&0x44))MEM8(g_edi+20)=0;
   g_xmm0=XMM_ZERO();MEMF(g_edi+24)=g_xmm0.f[0];if(phase<2)MEM32(g_esp+0x14)=g_edi;
  }else if(phase<2)MEM32(g_esp+0x14)=phase?0:g_eax;else g_edi=0;
  g_ecx=MEM32(0x51c4dc);MEM32(g_esp+0x45c)=0xffffffffu;FIRE_FRAME_CALL(sub_00208C50,pc[7]);
  if(phase<2){
   g_edx=g_esp+desc;PUSH32(g_esp,g_edx);g_ebx=g_eax;g_eax=MEM32(g_esi+0x54);g_edi=MEM32(g_ebx);PUSH32(g_esp,0);PUSH32(g_esp,g_eax);FIRE_FRAME_CALL(sub_00219530,pc[8]);g_esp+=8;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x1c);PUSH32(g_esp,g_eax);g_ecx=g_ebx;FIRE_VCALL(MEM32(g_edi+0x14),pc[9],saved);}
  }else{
   g_esi=MEM32(g_esi+0x54);g_edx=g_esp+desc;PUSH32(g_esp,g_edx);g_ebx=g_eax;g_eax=MEM32(g_ebx);PUSH32(g_esp,0);PUSH32(g_esp,g_esi);MEM32(g_esp+0x24)=g_eax;FIRE_FRAME_CALL(sub_00219530,pc[8]);g_esp+=8;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x20);PUSH32(g_esp,g_edi);g_ecx=g_ebx;FIRE_VCALL(MEM32(g_eax+0x14),pc[9],saved);}
  }
 }
 g_ecx=MEM32(g_esp+0x454);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;
}

static void fire_activate(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x1b8);PUSH32(g_esp,g_edi);g_edi=0;
 if((int32_t)g_eax>0)do{
  g_eax=MEM32(g_esi+0x1bc);g_ecx=MEM32(g_eax+g_edi*4);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x51c4dc);FIRE_CALL(sub_00207B30,0x18fcc5);
  g_ecx=MEM32(0x51c4dc);g_eax&=0xfff0ffff;PUSH32(g_esp,g_eax);FIRE_CALL(sub_002084E0,0x18fcd6);g_ecx=g_eax;FIRE_CALL(sub_00210320,0x18fcdd);g_eax=MEM32(g_esi+0x1b8);g_edi++;
 }while((int32_t)g_edi<(int32_t)g_eax);
 g_xmm0=XMM_SCALAR(MEMF(g_esi+0x168));g_edx=MEM32(g_esi+0x178);g_xmm0.f[0]=g_xmm0.f[0]*MEMF(g_esi+0x148);g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c728);
 PUSH32(g_esp,0);PUSH32(g_esp,0x4742a8);PUSH32(g_esp,g_edx);g_ecx=g_esi;MEMF(g_esi+0x16c)=g_xmm0.f[0];FIRE_CALL(sub_00216BD0,0x18fd1d);SET_LO8(g_eax,MEM8(g_esi+0x175));
 if(LO8(g_eax)){g_ecx=g_esi;FIRE_CALL(sub_0018F250,0x18fd2e);POP32(g_esp,g_edi);g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_0018F6A0();}
 else{POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=4;}
}

/* The original damage/extinguishing and entity-event owners stay calls. */
static void fire_update(void){
 uint32_t frame=g_seh_ebp;g_xmm1=XMM_ZERO();PUSH32(g_esp,g_esi);g_esi=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_esi+0x170));unsigned flags=fire_compare(g_xmm0.f[0],g_xmm1.f[0]);
 if(!(flags&0x41)){
  g_eax=MEM32(0x51c4dc);g_xmm0.f[0]=g_xmm0.f[0]-MEMF(g_eax+0x14);flags=fire_compare(g_xmm1.f[0],g_xmm0.f[0]);MEMF(g_esi+0x170)=g_xmm0.f[0];
  if(!(flags&1)){FIRE_CALL(sub_0018EE10,0x18fedf);g_xmm0=XMM_ZERO();MEMF(g_esi+0x170)=g_xmm0.f[0];}
 }else{
  flags=fire_compare(g_xmm0.f[0],g_xmm1.f[0]);SET_HI8(g_eax,flags);
  if(!RECOMP_PARITY8(HI8(g_eax)&0x44)){
   g_ecx=MEM32(g_esi+0x178);PUSH32(g_esp,0x474268);PUSH32(g_esp,g_ecx);g_ecx=g_esi;FIRE_CALL(sub_002160C0,0x18ff26);
   if(!LO8(g_eax)){g_edx=MEM32(g_esi);g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;RECOMP_ITAIL(MEM32(g_edx+0x24));return;}
  }
 }
 SET_LO8(g_eax,MEM8(g_esi+0x174));if(LO8(g_eax)){g_ecx=g_esi;FIRE_CALL(sub_0018EF10,0x18fefb);g_ecx=g_esi;FIRE_CALL(sub_0018FD40,0x18ff02);}
 g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_00218C40();
}

/* Retirement publishes the final authored burst before releasing targets and
 * lists. All target lookups and gameplay callbacks continue to run in source. */
static void fire_retire(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_esp-=0x184;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x51c4dc);unsigned burst=(MEM32(g_ecx+0x10)&0x602)==0;PUSH32(g_esp,g_edi);
 if(burst){
  MEM32(g_esp+0x1c)=0x474284;FIRE_FRAME_CALL(sub_00208C50,0x18fb21);g_edx=MEM32(g_eax);g_ecx=g_esp+0x1c;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_esp+0x34;PUSH32(g_esp,g_ecx);g_ecx=g_eax;FIRE_VCALL(MEM32(g_edx+0x1c),0x18fb32,saved);}
  g_xmm0=XMM_SCALAR(MEMF(g_esi+0x158));g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c538);MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0x148));g_edx=MEM32(g_esp+0x20);MEMF(g_esp+0x24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0x168));g_eax=MEM32(g_esp+0x24);MEMF(g_esp+0x28)=g_xmm0.f[0];g_ecx=MEM32(g_esp+0x28);
  MEM32(g_esp+0x3c)=g_ecx;g_ecx=MEM32(0x51c4dc);MEM32(g_esp+0x34)=g_edx;MEM32(g_esp+0x38)=g_eax;FIRE_FRAME_CALL(sub_00208C50,0x18fb87);g_edx=MEM32(g_eax);g_ecx=g_esp+0x1c;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_eax;FIRE_VCALL(MEM32(g_edx+0x34),0x18fb93,saved);}
  g_fp_stack[g_fp_top]=g_fp_stack[g_fp_top]*MEMF(g_esi+0x16c);g_ecx=MEM32(0x51c4dc);MEMF(g_esp+0x18)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;FIRE_FRAME_CALL(sub_00208C50,0x18fba8);g_edx=MEM32(g_eax);g_ecx=g_esp+0x1c;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_eax;FIRE_VCALL(MEM32(g_edx+0x38),0x18fbb4,saved);}
  MEMF(g_esp+0x2c)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;g_edx=MEM32(g_esp+0x2c);g_eax=MEM32(g_esp+0x18);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);FIRE_FRAME_CALL(sub_0023EB60,0x18fbc7);
  g_ecx=MEM32(0x51c4dc);g_esp+=8;MEM32(g_esp+0x18)=g_eax;FIRE_FRAME_CALL(sub_00208C50,0x18fbd9);g_ecx=g_esp+0x30;PUSH32(g_esp,g_ecx);g_edx=g_esi+0x10;g_edi=g_eax;g_ebx=MEM32(g_edi);PUSH32(g_esp,g_edx);FIRE_FRAME_CALL(sub_00219450,0x18fbeb);g_esp+=4;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x20);PUSH32(g_esp,g_eax);g_ecx=g_edi;FIRE_VCALL(MEM32(g_ebx+0x14),0x18fbf9,saved);}
 }
 g_eax=MEM32(g_esi+0x1c4);g_edi=0;
 if((int32_t)g_eax>0)do{
  g_ecx=MEM32(g_esi+0x1c0);g_edx=MEM32(g_ecx+g_edi*4);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_edx);FIRE_FRAME_CALL(sub_002084E0,0x18fc25);g_ecx=g_eax;FIRE_FRAME_CALL(sub_00210CA0,0x18fc2c);PUSH32(g_esp,g_eax);FIRE_FRAME_CALL(sub_001F8FC0,0x18fc32);g_esp+=4;
  if(g_eax){PUSH32(g_esp,1);g_ecx=g_eax;FIRE_FRAME_CALL(sub_0020A140,0x18fc42);}g_eax=MEM32(g_esi+0x1c4);g_edi++;
 }while((int32_t)g_edi<(int32_t)g_eax);
 g_eax=MEM32(g_esi+0x1c0);PUSH32(g_esp,g_eax);FIRE_FRAME_CALL(sub_002D631D,0x18fc59);SET_LO8(g_eax,MEM8(g_esi+0x176));g_esp+=4;
 if(!LO8(g_eax)){g_ecx=g_esi;FIRE_FRAME_CALL(sub_0018EE10,0x18fc6d);}
 g_ecx=MEM32(g_esi+0x1b0);PUSH32(g_esp,g_ecx);FIRE_FRAME_CALL(sub_002D631D,0x18fc79);g_edx=MEM32(g_esi+0x1bc);PUSH32(g_esp,g_edx);FIRE_FRAME_CALL(sub_002D631D,0x18fc85);g_esp+=8;g_ecx=g_esi;FIRE_FRAME_CALL(sub_00218B30,0x18fc8f);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;
}

static void fire_destroy(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x396688);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;MEM32(g_esp+12)=g_edi;g_esi=g_edi+0xd0;g_ebx=0;MEM32(g_esi)=0x4742b4;g_eax=MEM32(g_esi+8);unsigned allocated=g_eax!=g_ebx;MEM32(g_esp+0x18)=g_ebx;
 if(allocated){PUSH32(g_esp,g_eax);FIRE_CALL(sub_002D5CCD,0x190d4e);g_esp+=4;}
 g_ecx=g_esi;MEM32(g_esi+8)=g_ebx;MEM32(g_esi+12)=g_ebx;MEM32(g_esi+16)=g_ebx;FIRE_CALL(sub_001F9140,0x190d61);g_ecx=g_edi;MEM32(g_esp+0x18)=0xffffffffu;FIRE_CALL(sub_00218CE0,0x190d70);
 g_ecx=MEM32(g_esp+0x10);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}

static void fire_query(void){
 g_edx=MEM32(g_esp+4);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=g_ecx;g_edi=0x465b00;g_esi=g_edx;g_ecx=12;g_ebx=0;unsigned equal=0;
 while(g_ecx){unsigned a=MEM8(g_esi),b=MEM8(g_edi);equal=a==b;g_esi++;g_edi++;g_ecx--;if(!equal)break;}
 if(equal){
  SET_LO8(g_ecx,MEM8(g_eax+0x1ac));if(LO8(g_ecx)){g_eax=MEM32(g_esp+0x14);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(g_eax)=1;MEM32(g_eax+4)=5;SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_recomp_zf=0;g_esp+=12;return;}
 }else{
  g_edi=0x474250;g_esi=g_edx;g_ecx=24;g_eax=0;
  while(g_ecx){unsigned a=MEM8(g_esi),b=MEM8(g_edi);equal=a==b;g_esi++;g_edi++;g_ecx--;if(!equal)break;}
  if(equal){g_eax=MEM32(g_esp+0x14);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(g_eax)=3;MEM8(g_eax+4)=1;SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=12;return;}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);SET_LO8(g_eax,0);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=12;
}

/* Property callbacks may replace the live owner, descriptor and destination
 * words. Keep stores between calls rather than collecting a detached recipe. */
static void fire_properties(uint32_t frame){
 g_edi=0;MEM32(g_esp+0xdc)=g_edi;MEM32(g_ebx)=0x474330;MEM32(g_ebx+0xd0)=0x4742b4;
 for(unsigned off=0xd8;off<=0xe4;off+=4)MEM32(g_ebx+off)=g_edi;
 g_esi=MEM32(frame+8);MEM32(g_ebx+0xf4)=g_edi;MEM32(g_ebx+0xf8)=g_edi;g_eax=g_esp+0x40;PUSH32(g_esp,g_eax);MEM32(g_ebx+0xfc)=g_edi;MEM32(g_ebx+0x100)=g_edi;PUSH32(g_esp,0x462428);MEM32(g_ebx+0x104)=g_edi;PUSH32(g_esp,4);g_ecx=g_esi;MEM8(g_esp+0xe8)=2;MEM32(g_ebx+0x108)=g_edi;FIRE_FRAME_CALL(sub_001F8DC0,0x1904e2);
 if(!LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(0x458800));g_xmm1=XMM_SCALAR(MEMF(0x44c5e8));MEMF(g_esp+0x20)=g_xmm0.f[0];MEMF(g_esp+0x24)=g_xmm1.f[0];}
 else{g_xmm0=XMM_SCALAR(MEMF(g_esp+0x40));MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x44));MEMF(g_esp+0x24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x48));}
 g_ecx=MEM32(g_esp+0x20);g_edx=MEM32(g_esp+0x24);g_eax=g_ebx+0xe8;MEM32(g_eax)=g_ecx;MEM32(g_eax+4)=g_edx;g_edx=g_esp+0x1c;PUSH32(g_esp,g_edx);MEMF(g_esp+0x2c)=g_xmm0.f[0];g_ecx=MEM32(g_esp+0x2c);PUSH32(g_esp,0x462418);MEM32(g_eax+8)=g_ecx;PUSH32(g_esp,9);g_ecx=g_esi;MEM32(g_esp+0x28)=g_edi;FIRE_FRAME_CALL(sub_001F8DC0,0x190559);
 static const uint32_t keys[5]={0x4623e4,0x4623b0,0x46237c,0x462348,0x4622f0},pcs[5]={0x19057a,0x19059b,0x1905bc,0x1905dd,0x1905fe};
 for(unsigned i=0;i<5;i++){
  unsigned k=i%3;
  if(k==0){g_eax=MEM32(g_esp+0x1c);g_ecx=g_esp+0x1c;PUSH32(g_esp,g_ecx);PUSH32(g_esp,keys[i]);PUSH32(g_esp,9);g_ecx=g_esi;MEM32(g_ebx+0xf4+i*4)=g_eax;}
  else if(k==1){g_edx=MEM32(g_esp+0x1c);g_eax=g_esp+0x1c;PUSH32(g_esp,g_eax);PUSH32(g_esp,keys[i]);PUSH32(g_esp,9);g_ecx=g_esi;MEM32(g_ebx+0xf4+i*4)=g_edx;}
  else{g_ecx=MEM32(g_esp+0x1c);g_edx=g_esp+0x1c;PUSH32(g_esp,g_edx);PUSH32(g_esp,keys[i]);MEM32(g_ebx+0xf4+i*4)=g_ecx;PUSH32(g_esp,9);g_ecx=g_esi;}
  MEM32(g_esp+0x28)=g_edi;FIRE_FRAME_CALL(sub_001F8DC0,pcs[i]);
 }
 g_ecx=MEM32(g_esp+0x1c);g_edx=g_esp+0x1c;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x4588f0);MEM32(g_ebx+0x108)=g_ecx;PUSH32(g_esp,2);g_ecx=g_esi;FIRE_FRAME_CALL(sub_001F8DC0,0x19061b);
 g_xmm0=XMM_SCALAR(MEMF(0x44bf80));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+0x1c));g_eax=g_esp+0x1c;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4622e4);PUSH32(g_esp,3);g_ecx=g_esi;MEMF(g_ebx+0x170)=g_xmm0.f[0];FIRE_FRAME_CALL(sub_001F8DC0,0x190648);
 if(!LO8(g_eax))SET_LO8(g_eax,1);else{g_eax=MEM32(g_esp+0x1c)&0xff;SET_LO8(g_eax,g_eax!=0);}
 g_ecx=g_esp+0x1c;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x44c1fc);PUSH32(g_esp,6);g_ecx=g_esi;MEM8(g_ebx+0x174)=LO8(g_eax);MEM32(g_esp+0x28)=g_edi;FIRE_FRAME_CALL(sub_001F8DC0,0x190679);
 g_edx=MEM32(g_esp+0x1c);g_eax=g_esp+0x1c;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4622d0);PUSH32(g_esp,3);g_ecx=g_esi;MEM32(g_esp+0x60)=g_edx;FIRE_FRAME_CALL(sub_001F8DC0,0x190694);
 if(!LO8(g_eax))SET_LO8(g_eax,1);else{g_eax=MEM32(g_esp+0x1c)&0xff;SET_LO8(g_eax,g_eax!=0);}
 g_ecx=g_esp+0x1c;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x4622b0);PUSH32(g_esp,3);g_ecx=g_esi;MEM8(g_ebx+0x175)=LO8(g_eax);FIRE_FRAME_CALL(sub_001F8DC0,0x1906c1);
 if(LO8(g_eax)){g_eax=MEM32(g_esp+0x1c)&0xff;SET_LO8(g_eax,g_eax!=0);}
 PUSH32(g_esp,g_edi);PUSH32(g_esp,0x458f10);g_edx=g_ebx+0x1b4;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x474314);g_ecx=g_esi;MEM8(g_ebx+0x1ac)=LO8(g_eax);FIRE_FRAME_CALL(sub_000BBC20,0x1906f0);
 PUSH32(g_esp,0x463ff4);MEM32(g_ebx+0x1b0)=g_eax;PUSH32(g_esp,0x4609f8);g_eax=g_ebx+0x1b8;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4742fc);g_ecx=g_esi;FIRE_FRAME_CALL(sub_000BBC20,0x190713);
 PUSH32(g_esp,g_edi);PUSH32(g_esp,0x4609f8);g_ecx=g_ebx+0x1c4;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x4742d8);g_ecx=g_esi;MEM32(g_ebx+0x1bc)=g_eax;FIRE_FRAME_CALL(sub_000BBC20,0x190732);
 g_edx=g_esp+0x1c;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x4742c0);PUSH32(g_esp,3);g_ecx=g_esi;MEM32(g_ebx+0x1c0)=g_eax;FIRE_FRAME_CALL(sub_001F8DC0,0x19074b);
 if(!LO8(g_eax))SET_LO8(g_eax,1);else{g_eax=MEM32(g_esp+0x1c)&0xff;SET_LO8(g_eax,g_eax!=0);}
 MEM8(g_ebx+0x1c8)=LO8(g_eax);PUSH32(g_esp,g_edi);g_eax=g_esp+0x58;PUSH32(g_esp,g_eax);g_ecx=g_ebx;FIRE_FRAME_CALL(sub_00216020,0x190772);MEM32(g_ebx+0x178)=g_eax;g_ecx=MEM32(0x51c4dc);g_ecx=MEM32(g_ecx+0x6c);
 if(g_ecx!=g_edi){g_edx=MEM32(g_ecx);g_eax=g_esp+0x54;{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);FIRE_VCALL(MEM32(g_edx+0x1c),0x19078f,saved);}if(g_eax!=g_edi){g_ecx=MEM32(g_ebx+0x178);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_ebx;FIRE_FRAME_CALL(sub_00216BD0,0x1907a3);}}
}

/* Build the authored emitter basis and bottom-origin transform. Intermediate
 * stores are retained because caller-provided owner/stack aliases are live. */
static void fire_construct_basis(void){
 g_xmm1 = XMM_SCALAR(MEMF(g_ebx + 0xF0));g_xmm4 = XMM_SCALAR(MEMF(g_ebx + 0x48));g_xmm2 = XMM_SCALAR(MEMF(g_ebx + 0xEC));
 g_xmm3 = XMM_SCALAR(MEMF(g_ebx + 0xE8));g_xmm0 = XMM_SCALAR(MEMF(0x44C1F8));g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];
 g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];g_xmm4.f[0] = g_xmm4.f[0] - g_xmm1.f[0];g_xmm1 = XMM_SCALAR(MEMF(g_ebx + 0x44));
 g_xmm1.f[0] = g_xmm1.f[0] - g_xmm2.f[0];g_xmm2 = XMM_SCALAR(MEMF(g_ebx + 0x40));g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0];
 g_xmm2.f[0] = g_xmm2.f[0] - g_xmm3.f[0];MEMF(g_esp + 0x20) = g_xmm2.f[0];g_eax = MEM32(g_esp + 0x20);
 g_xmm2 = XMM_SCALAR(MEMF(g_ebx + 0xEC));MEMF(g_esp + 0x24) = g_xmm1.f[0];g_ecx = MEM32(g_esp + 0x24);
 g_edx = g_ebx + 0x13C;MEM32(g_edx) = g_eax;MEM32(g_edx + 4) = g_ecx;
 g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];MEMF(g_esp + 0x28) = g_xmm4.f[0];g_eax = MEM32(g_esp + 0x28);
 MEM32(g_edx + 8) = g_eax;g_xmm1 = XMM_SCALAR(MEMF(g_ebx + 0x48));g_xmm0 = XMM_SCALAR(MEMF(g_ebx + 0x44));
 g_xmm0.f[0] = g_xmm0.f[0] - g_xmm2.f[0];g_xmm2 = XMM_SCALAR(MEMF(g_ebx + 0x40));MEMF(g_esp + 0x48) = g_xmm1.f[0];
 g_xmm1 = XMM_SCALAR(MEMF(0x44BF94));MEMF(g_esp + 0x40) = g_xmm2.f[0];MEMF(g_esp + 0x44) = g_xmm0.f[0];
 MEMF(g_esp + 0x4C) = g_xmm1.f[0];g_xmm0 = XMM_MEM(g_esp + 0x40);XMM_STORE(g_ebx + 0x40, g_xmm0);
 g_xmm0 = XMM_SCALAR(MEMF(g_ebx + 0xE8));g_xmm2 = XMM_SCALAR(MEMF(g_ebx + 0xEC));MEMF(g_esp + 0x20) = g_xmm0.f[0];
 g_xmm0 = XMM_ZERO();g_edx = MEM32(g_esp + 0x20);g_ecx = g_ebx + 0x148;
 MEM32(g_ecx) = g_edx;MEMF(g_esp + 0x24) = g_xmm0.f[0];g_eax = MEM32(g_esp + 0x24);
 MEM32(g_ecx + 4) = g_eax;MEMF(g_esp + 0x28) = g_xmm0.f[0];g_edx = MEM32(g_esp + 0x28);
 MEM32(g_ecx + 8) = g_edx;MEMF(g_esp + 0x20) = g_xmm0.f[0];g_ecx = MEM32(g_esp + 0x20);
 MEMF(g_esp + 0x24) = g_xmm2.f[0];g_edx = MEM32(g_esp + 0x24);g_xmm2 = XMM_SCALAR(MEMF(g_ebx + 0xF0));
 g_eax = g_ebx + 0x154;MEM32(g_eax) = g_ecx;MEM32(g_eax + 4) = g_edx;
 MEMF(g_esp + 0x28) = g_xmm0.f[0];g_ecx = MEM32(g_esp + 0x28);MEM32(g_eax + 8) = g_ecx;
 MEMF(g_esp + 0x20) = g_xmm0.f[0];g_eax = MEM32(g_esp + 0x20);g_edx = g_ebx + 0x160;
 MEMF(g_esp + 0x24) = g_xmm0.f[0];g_ecx = MEM32(g_esp + 0x24);MEM32(g_edx) = g_eax;
 MEMF(g_esp + 0x28) = g_xmm2.f[0];g_eax = MEM32(g_esp + 0x28);MEM32(g_edx + 4) = g_ecx;
 MEM32(g_edx + 8) = g_eax;MEMF(g_esp + 0x30) = g_xmm0.f[0];MEMF(g_esp + 0x34) = g_xmm0.f[0];
 MEMF(g_esp + 0x38) = g_xmm1.f[0];MEMF(g_esp + 0x3C) = g_xmm0.f[0];MEMF(g_esp + 0x20) = g_xmm1.f[0];
 MEMF(g_esp + 0x24) = g_xmm0.f[0];MEMF(g_esp + 0x28) = g_xmm0.f[0];MEMF(g_esp + 0x2C) = g_xmm0.f[0];
 MEMF(g_esp + 0x40) = g_xmm0.f[0];MEMF(g_esp + 0x44) = g_xmm1.f[0];MEMF(g_esp + 0x48) = g_xmm0.f[0];
 MEMF(g_esp + 0x4C) = g_xmm0.f[0];g_xmm0 = XMM_MEM(g_ebx + 0x40);g_xmm1 = XMM_MEM(g_esp + 0x40);
 XMM_STORE(g_ebx + 0x10, g_xmm1);g_xmm1 = XMM_MEM(g_esp + 0x20);XMM_STORE(g_ebx + 0x20, g_xmm1);
 g_xmm1 = XMM_MEM(g_esp + 0x30);XMM_STORE(g_ebx + 0x30, g_xmm1);XMM_STORE(g_ebx + 0x40, g_xmm0);
}

/* Construct both AABBs through the original bounds/matrix services. The ABI
 * projection retains all eight SIMD lanes and the original retired scratch. */
static void fire_construct_bounds(uint32_t frame){
 g_xmm0 = XMM_SCALAR(MEMF(0x44C1F8));g_xmm2 = XMM_SCALAR(MEMF(g_ebx + 0xF0));g_xmm1 = XMM_SCALAR(MEMF(g_ebx + 0xEC));
 g_xmm3 = XMM_SCALAR(MEMF(g_ebx + 0xE8));g_xmm4 = XMM_SCALAR(MEMF(g_ebx + 0x48));g_xmm5 = XMM_SCALAR(MEMF(g_ebx + 0x40));
 g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0];g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
 g_xmm0 = XMM_SCALAR(MEMF(g_ebx + 0x44));g_xmm0.f[0] = g_xmm0.f[0] + g_xmm1.f[0];g_xmm7 = g_xmm0;
 g_xmm7.f[0] = g_xmm7.f[0] + g_xmm1.f[0];MEMF(g_esp + 0x1C) = g_xmm7.f[0];g_xmm7 = g_xmm5;
 g_xmm6 = g_xmm4;g_xmm7.f[0] = g_xmm7.f[0] + g_xmm3.f[0];MEMF(g_esp + 0x30) = g_xmm7.f[0];
 g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x1C));g_xmm6.f[0] = g_xmm6.f[0] + g_xmm2.f[0];MEMF(g_esp + 0x38) = g_xmm6.f[0];
 g_xmm6 = XMM_ZERO();MEMF(g_esp + 0x34) = g_xmm7.f[0];MEMF(g_esp + 0x3C) = g_xmm6.f[0];
 g_xmm7 = XMM_MEM(g_esp + 0x30);g_eax = g_esp + 0x30;g_xmm0.f[0] = g_xmm0.f[0] - g_xmm1.f[0];
 PUSH32(g_esp, g_eax);g_ecx = g_esp + 0x44;g_xmm4.f[0] = g_xmm4.f[0] - g_xmm2.f[0];
 g_xmm5.f[0] = g_xmm5.f[0] - g_xmm3.f[0];MEMF(g_esp + 0x34) = g_xmm5.f[0];MEMF(g_esp + 0x38) = g_xmm0.f[0];
 MEMF(g_esp + 0x3C) = g_xmm4.f[0];MEMF(g_esp + 0x40) = g_xmm6.f[0];g_xmm0 = XMM_MEM(g_esp + 0x34);
 PUSH32(g_esp, g_ecx);g_ecx = g_esp + 0xA8;MEMF(g_esp + 0x58) = g_xmm2.f[0];
 MEMF(g_esp + 0x60) = g_xmm1.f[0];MEMF(g_esp + 0x28) = g_xmm3.f[0];XMM_STORE(g_esp + 0x48, g_xmm7);
 XMM_STORE(g_esp + 0x38, g_xmm0);FIRE_FRAME_CALL(sub_002D38F0,0x00190A81);g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x20));
 g_xmm1 = XMM_SCALAR(MEMF(0x44C5E8));g_xmm0.f[0] = g_xmm0.f[0] + g_xmm1.f[0];MEMF(g_esp + 0x20) = g_xmm0.f[0];
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x58));g_esi = g_eax;MEMF(g_esp + 0x24) = g_xmm0.f[0];
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x50));g_edi = g_ebx + 0x10C;g_ecx = 0xC;
 g_edx = g_esp + 0x60;memcpy((void*)XBOX_PTR(g_edi), (void*)XBOX_PTR(g_esi), g_ecx * 4);g_esi += g_ecx * 4; g_edi += g_ecx * 4; g_ecx = 0;
 g_xmm0.f[0] = g_xmm0.f[0] + g_xmm1.f[0];PUSH32(g_esp, g_edx);g_ecx = g_ebx;
 MEMF(g_esp + 0x2C) = g_xmm0.f[0];FIRE_FRAME_CALL(sub_0003BD10,0x00190AD0);g_ecx = g_esp + 0x60;
 FIRE_FRAME_CALL(sub_002D4360,0x00190AD9);g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x28));g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x24));
 g_xmm0.f[0] = g_xmm0.f[0] * MEMF(0x44C530);g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x20));g_xmm1 = g_xmm2;
 MEMF(g_esp + 0x34) = g_xmm0.f[0];g_xmm0 = XMM_ZERO();MEMF(g_esp + 0x38) = g_xmm1.f[0];
 MEMF(g_esp + 0x30) = g_xmm3.f[0];MEMF(g_esp + 0x3C) = g_xmm0.f[0];g_xmm1 = XMM_MEM(g_esp + 0x30);
 XMM_STORE(g_esp + 0x40, g_xmm1);g_xmm1 = g_xmm0;g_xmm1.f[0] = g_xmm1.f[0] - g_xmm2.f[0];
 g_xmm2 = g_xmm0;g_xmm2.f[0] = g_xmm2.f[0] - g_xmm3.f[0];MEMF(g_esp + 0x30) = g_xmm2.f[0];
 MEMF(g_esp + 0x34) = g_xmm0.f[0];g_xmm3 = XMM_MEM(g_esp + 0x60);g_xmm4 = XMM_MEM(g_esp + 0x70);
 g_xmm7 = XMM_MEM(g_esp + 0x80);MEMF(g_esp + 0x38) = g_xmm1.f[0];g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x44));
 g_xmm1.f[0] = g_xmm1.f[0] + MEMF(0x44C5E4);MEMF(g_esp + 0x44) = g_xmm1.f[0];MEMF(g_esp + 0x3C) = g_xmm0.f[0];
 g_xmm0 = XMM_MEM(g_esp + 0x30);g_xmm2 = g_xmm0;g_xmm2 = XMM_SHUFFLE(g_xmm2, g_xmm0, 0);
 g_xmm1 = g_xmm3;g_xmm1 = XMM_MUL(g_xmm1, g_xmm2);g_xmm2 = g_xmm0;
 g_xmm2 = XMM_SHUFFLE(g_xmm2, g_xmm0, 0x55);g_xmm6 = g_xmm0;g_xmm6 = XMM_SHUFFLE(g_xmm6, g_xmm0, 0xAA);
 g_xmm5 = g_xmm4;g_xmm5 = XMM_MUL(g_xmm5, g_xmm2);g_xmm1 = XMM_ADD(g_xmm1, g_xmm5);
 g_xmm2 = g_xmm7;g_xmm2 = XMM_MUL(g_xmm2, g_xmm6);g_xmm6 = XMM_MEM(g_esp + 0x90);
 g_xmm0 = XMM_SHUFFLE(g_xmm0, g_xmm0, 0xFF);g_xmm6 = XMM_MUL(g_xmm6, g_xmm0);g_xmm0 = XMM_MEM(g_esp + 0x40);
 g_xmm2 = XMM_ADD(g_xmm2, g_xmm6);g_xmm1 = XMM_ADD(g_xmm1, g_xmm2);XMM_STORE(g_esp + 0x30, g_xmm1);
 g_xmm1 = g_xmm0;g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0);g_xmm3 = XMM_MUL(g_xmm3, g_xmm1);
 g_xmm1 = g_xmm0;g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0x55);g_xmm4 = XMM_MUL(g_xmm4, g_xmm1);
 g_xmm1 = g_xmm0;g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0xAA);g_xmm7 = XMM_MUL(g_xmm7, g_xmm1);
 g_xmm1 = XMM_MEM(g_esp + 0x90);g_eax = g_esp + 0x30;g_xmm0 = XMM_SHUFFLE(g_xmm0, g_xmm0, 0xFF);
 PUSH32(g_esp, g_eax);g_ecx = g_esp + 0x44;g_xmm1 = XMM_MUL(g_xmm1, g_xmm0);
 g_xmm7 = XMM_ADD(g_xmm7, g_xmm1);g_xmm3 = XMM_ADD(g_xmm3, g_xmm4);PUSH32(g_esp, g_ecx);
 g_xmm3 = XMM_ADD(g_xmm3, g_xmm7);g_ecx = g_esp + 0xA8;XMM_STORE(g_esp + 0x48, g_xmm3);
 FIRE_FRAME_CALL(sub_002D38F0,0x00190C06);
}

/* The secondary listener searches the live callback vector and forwards only
 * the matching event. Gameplay event handlers remain the original targets. */
static void fire_event(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+12);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_ecx+8);g_esi=0;g_ebx=0;
 while(g_edi){
  g_edx=MEM32(g_ecx+12);g_edx-=g_edi;g_eax=0x2aaaaaab;
  {int64_t product=(int64_t)(int32_t)g_eax*(int64_t)(int32_t)g_edx;g_eax=(uint32_t)product;g_edx=(uint32_t)(product>>32);}
  g_edx=(uint32_t)((int32_t)g_edx>>1);g_eax=g_edx;g_eax>>=31;g_eax+=g_edx;
  if(g_esi>=g_eax)break;
  g_eax=MEM32(g_ecx+8);g_edx=MEM32(frame);g_eax+=g_ebx;g_eax=MEM32(g_eax);
  if(g_eax==g_edx){
   g_eax=MEM32(g_ecx+8);g_ecx=MEM32(g_ecx+0x14);g_edx=g_esi+g_esi*2;g_eax+=g_edx*4;g_edx=MEM32(g_esp+0x20);
   {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+0x20);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+0x20);PUSH32(g_esp,g_edx);FIRE_VCALL(MEM32(g_eax+8),0x19030f,saved);}
   POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=20;return;
  }
  g_esi++;g_ebx+=12;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,0);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=20;
}

static void fire_construct(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x396667);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0xc8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+0x5c)=g_ebx;FIRE_FRAME_CALL(sub_00218A60,0x19046f);
 fire_properties(frame);fire_construct_basis();
 g_eax=MEM32(g_ebx+0x54);MEM8(g_ebx+0x176)=0;g_ecx=MEM32(0x51c398);g_ecx=MEM32(g_ecx+12);g_edx=MEM32(g_ecx);
 {uint32_t saved=g_esp;PUSH32(g_esp,0x47211c);PUSH32(g_esp,g_eax);FIRE_VCALL(MEM32(g_edx+4),0x190986,saved);}
 SET_LO8(g_eax,MEM8(g_ebx+0x1ac));if(LO8(g_eax)){
  g_eax=MEM32(0x51c398);g_ecx=MEM32(g_eax+12);g_eax=MEM32(g_ebx+0x54);g_edx=MEM32(g_ecx);
  {uint32_t saved=g_esp;PUSH32(g_esp,0x465ae0);PUSH32(g_esp,g_eax);FIRE_VCALL(MEM32(g_edx+4),0x1909a6,saved);}
 }
 fire_construct_bounds(frame);
 g_esi=g_eax;SET_LO8(g_eax,MEM8(g_ebx+0x1c8));unsigned collision=LO8(g_eax)!=0;g_edi=g_ebx+0x17c;g_ecx=12;
 memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
 g_xmm0=XMM_SCALAR(MEMF(g_ebx+0x44));g_xmm1=XMM_SCALAR(MEMF(g_ebx+0x40));g_xmm2=XMM_SCALAR(MEMF(g_ebx+0x48));g_xmm0.f[0]=g_xmm0.f[0]+MEMF(g_esp+0x24);
 if(collision){
  g_edx=MEM32(g_ebx+0xe8);PUSH32(g_esp,0);g_esp-=12;g_eax=g_esp;MEM32(g_eax)=g_edx;g_ecx=MEM32(g_ebx+0xec);MEM32(g_eax+4)=g_ecx;g_edx=MEM32(g_ebx+0xf0);MEM32(g_eax+8)=g_edx;MEM32(g_esp+0x60)=g_esp;g_esp-=12;g_eax=g_esp;
  MEMF(g_eax)=g_xmm1.f[0];MEMF(g_eax+4)=g_xmm0.f[0];MEMF(g_eax+8)=g_xmm2.f[0];g_eax=MEM32(g_ebx+0x54);g_ecx=MEM32(0x51c4dc);MEM32(g_esp+0x6c)=g_esp;PUSH32(g_esp,g_eax);FIRE_FRAME_CALL(sub_00055EE0,0x190c7f);g_ecx=g_eax;FIRE_FRAME_CALL(sub_000236D0,0x190c86);
 }
 g_ecx=g_ebx+0xd0;MEM32(g_ecx+0x14)=g_ebx;g_eax=MEM32(g_ebx+0x54);PUSH32(g_esp,g_eax);FIRE_FRAME_CALL(sub_001F9120,0x190c98);
 g_ecx=MEM32(0x4f9dd8);g_edx=g_esp+0x40;MEM32(g_esp+0x40)=g_ecx;PUSH32(g_esp,g_edx);g_ecx=g_ebx+0xd4;MEM32(g_esp+0x48)=0xd8520;MEM32(g_esp+0x4c)=0;FIRE_FRAME_CALL(sub_001903C0,0x190cc2);
 PUSH32(g_esp,0x2000);g_ecx=g_ebx;FIRE_FRAME_CALL(sub_0020A140,0x190cce);g_ecx=g_ebx;FIRE_FRAME_CALL(sub_0018FCA0,0x190cd5);
 g_ecx=MEM32(g_esp+0xd4);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=g_ebx;MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=8;
}

static int fire_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_FX_FIRE");enabled=v&&!strcmp(v,"1");}return enabled;}
static int fire_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int fire_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_fx_fire(uint32_t entry){
 if(!fire_enabled())return 0;
 switch(entry){case 0x18ed80:case 0x18edf0:case 0x18f250:case 0x18f6a0:case 0x18faf0:case 0x18fca0:case 0x18feb0:case 0x190270:case 0x1902b0:case 0x190330:case 0x190440:case 0x190cf0:case 0x190d10:case 0x190d90:break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x12000u||!fire_span(s-8192,8224)||!fire_span(0x51c178,1)||MEM8(0x51c178)||!fire_apart(s-8192,8224,0x51c178,1)||(unsigned)g_fp_top>7)return 0;
 unsigned bytes=(entry==0x190270||entry==0x1902b0||entry==0x190330)?0x18:0x1d0;
 if(entry!=0x190d90&&(!fire_span(p,bytes)||!fire_apart(p,bytes,s-8192,8224)||!fire_apart(p,bytes,0x51c178,1)))return 0;
 switch(entry){
 case 0x18ed80:fire_query();break;
 case 0x18edf0:
  PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_esi=g_ecx+0x17c;g_ecx=12;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;break;
 case 0x18f250:case 0x18f6a0:fire_recipes(entry==0x18f6a0);break;
 case 0x18faf0:fire_retire();break;
 case 0x18fca0:fire_activate();break;
 case 0x18feb0:fire_update();break;
 case 0x1902b0:fire_event();break;
 case 0x190270:{
  uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi)=0x4742b4;g_eax=MEM32(g_esi+8);if(g_eax){PUSH32(g_esp,g_eax);FIRE_CALL(sub_002D5CCD,0x190286);g_esp+=4;}MEM32(g_esi+8)=0;MEM32(g_esi+12)=0;MEM32(g_esi+16)=0;g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_001F9140();break;}
 case 0x190330:
  PUSH32(g_esp,g_esi);g_esi=g_ecx;FIRE_CALL(sub_00190270,0x190338);if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);FIRE_CALL(sub_002D5CCD,0x190345);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;break;
 case 0x190440:fire_construct();break;
 case 0x190d10:fire_destroy();break;
 case 0x190cf0:
  PUSH32(g_esp,g_esi);g_esi=g_ecx;FIRE_CALL(sub_00190D10,0x190cf8);if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);FIRE_CALL(sub_002D5CCD,0x190d05);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;break;
 case 0x190d90:
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x3966ab);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1d0);FIRE_CALL(sub_002D57FB,0x190db0);g_esp+=4;MEM32(g_esp)=g_eax;{unsigned allocated=g_eax!=0;MEM32(g_esp+12)=0;if(allocated){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;FIRE_CALL(sub_00190440,0x190dce);}else g_eax=0;g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;}break;
 }
 return 1;
}
#undef FIRE_CALL
#undef FIRE_FRAME_CALL
#undef FIRE_VCALL
