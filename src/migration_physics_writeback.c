/* Original six motion writeback policies, sharing their swept-transform update.
 * Quaternion services remain explicit until their own native qualification. */
#include "migration_physics_writeback.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E2720(void);void sub_002E26A0(void);void sub_002E2930(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int bounded(uint32_t p,unsigned n){for(unsigned i=0;i<n;i++)if((MEM32(p+i*4)&0x7fffffff)>0x47800000)return 0;return 1;}
static void push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void pop(void){g_fp_top=(g_fp_top+1)&7;}
#define TOP g_fp_stack[g_fp_top]
#define PREV g_fp_stack[(g_fp_top+1)&7]
static unsigned status(void){unsigned s=((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);g_eax=(g_eax&0xffff0000)|(uint16_t)s;return s>>8;}
static void rotate(uint32_t matrix){
 g_xmm2=XMM_MEM(matrix+32);g_xmm3=XMM_MEM(matrix+16);
 g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);
 g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm3=XMM_MUL(g_xmm3,g_xmm1);
 g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=XMM_MEM(matrix);g_xmm0=XMM_MUL(g_xmm0,g_xmm1);
}
int uc_migration_physics_writeback(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_WRITEBACK");enabled=v&&!strcmp(v,"1");}
 if(!enabled)return 0;
 unsigned kind;switch(entry){case 0x2f4160:kind=0;break;case 0x2f57a0:kind=1;break;case 0x2f3400:kind=2;break;case 0x2f3710:kind=3;break;case 0x2fe9a0:kind=4;break;case 0x2fe7d0:kind=5;break;default:return 0;}
 if((g_esp&3)||g_esp<0x10200||!span(g_esp-256,276))return 0;
 if(kind==5){g_eax=MEM32(g_esp+12)+128;g_recomp_zf=g_eax==0;g_esp+=16;return 1;}
 uint32_t motion=g_ecx,step=MEM32(g_esp+4),acc=MEM32(g_esp+12),frame=g_ebp;
 if((unsigned)g_fp_top>7||(motion&15)||(acc&15)||!span(motion,256)||!span(step,16)||!span(acc,128)||!span(0x440000,0x60000))return 0;
 const uint32_t regions[][2]={{motion,256},{step,16},{acc,128},{g_esp-256,276},{0x440000,0x60000}};
 for(unsigned i=0;i<5;i++)for(unsigned j=0;j<i;j++)if(!apart(regions[i][0],regions[i][1],regions[j][0],regions[j][1]))return 0;
 /* Original C SIMD expressions permit compiler-dependent NaN payload choices.
  * Exceptional/overflow-prone states stay on the untouched original path;
  * checking integer representations cannot raise floating-point exceptions. */
 if(!bounded(motion+16,43)||!bounded(motion+0xd0,8)||!bounded(step,4)||!bounded(acc+16,20)||!bounded(g_esp+8,1))return 0;
 int preserve=kind<2,rotated=kind==1||kind==3,kinematic=kind==4;
 static const uint32_t returns[5][3]={{0x2f439e,0x2f43a7,0x2f43df},{0x2f5a0d,0x2f5a16,0x2f5a4e},{0x2f363c,0x2f3645,0x2f367d},{0x2f397b,0x2f3984,0x2f39bc},{0x2feb99,0x2feba2,0x2febd9}};
 static const uint32_t angles[]={0x49a9a8,0x49a9f8,0x49a8b0,0x49a8b4,0x49aee8};
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-52;
 if(!kinematic)g_eax=MEM32(frame+12);
 PUSH32(g_esp,g_ebx);
 if(preserve){g_ebx=acc;MEM32(g_esp+16)=g_eax;}
 PUSH32(g_esp,g_esi);
 if(!preserve&&!kinematic)MEM32(g_esp+20)=g_eax;
 PUSH32(g_esp,g_edi);uint32_t scratch=g_esp;
 if(!kinematic){
  if(preserve)g_edi=motion;else g_edi=acc;
  g_xmm0=XMM_SCALAR(MEMF(scratch+24));g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);
  g_xmm1=XMM_MEM(acc+0x40);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);XMM_STORE(motion+0xd0,g_xmm2);
  if(rotated){
   g_xmm1=g_xmm0;g_xmm0=XMM_MEM(acc+0x50);g_xmm3=XMM_MEM(motion+0x30);g_xmm4=XMM_MEM(motion+0x20);
   g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm4=XMM_MUL(g_xmm4,g_xmm2);
   g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=XMM_MEM(motion+16);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm0=XMM_ADD(g_xmm0,g_xmm4);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);
   g_xmm2=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm2=XMM_MUL(g_xmm2,g_xmm0);
  }else{g_xmm1=XMM_MEM(acc+0x50);g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);}
  XMM_STORE(motion+0xe0,g_xmm2);
  if(preserve)SET_LO16(g_ecx,MEM16(acc+4));else SET_LO16(g_edx,MEM16(acc+4));MEM16(motion+0xbe)=MEM16(acc+4);
 }
 g_xmm0=XMM_MEM(motion+0x60);g_esi=motion+16;XMM_STORE(motion+0x50,g_xmm0);
 if(preserve){g_ecx=step;g_edx=MEM32(step);MEM32(motion+0x5c)=g_edx;}else{g_edx=step;g_eax=MEM32(step);MEM32(motion+0x5c)=g_eax;}
 /* Clamp linear speed using the original extended intermediate and status. */
 push(MEMF(motion+0xb4));g_xmm1=XMM_MEM(motion+0xd0);push(TOP);TOP=TOP*PREV;
 g_xmm0=XMM_MUL(g_xmm1,g_xmm1);g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_eax=scratch+20;
 g_xmm2.f[0]=g_xmm2.f[0]+g_xmm0.f[0];g_xmm3.f[0]=g_xmm3.f[0]+g_xmm2.f[0];MEMF(g_eax)=g_xmm3.f[0];
 push(MEMF(scratch+20));g_fp_cmp=RECOMP_FCMP(TOP,PREV);pop();pop();unsigned linear=status();pop();
 if(!(linear&0x41)){
  push(MEMF(scratch+20));TOP=sqrt(TOP);TOP=MEMF(motion+0xb4)/TOP;MEMF(scratch+24)=(float)TOP;pop();
  g_xmm0=XMM_SCALAR(MEMF(scratch+24));g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);XMM_STORE(motion+0xd0,g_xmm2);
 }
 if(preserve)g_edx=MEM32(step+8);else g_eax=MEM32(step+8);MEM32(scratch+24)=MEM32(step+8);
 g_xmm1=XMM_MEM(motion+0xd0);g_xmm0=XMM_SCALAR(MEMF(scratch+24));g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=XMM_MEM(motion+0x60);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);XMM_STORE(motion+0x60,g_xmm0);
 g_eax=MEM32(step+12);MEM32(motion+0x6c)=g_eax;g_eax=motion+0x80;if(kinematic)g_edi=g_eax;
 for(unsigned i=0;i<3;i++){if(preserve)g_edx=MEM32(g_eax+i*4);else g_ebx=MEM32(g_eax+i*4);MEM32(scratch+32+i*4)=MEM32(g_eax+i*4);}
 g_eax=MEM32(g_eax+12);MEM32(scratch+44)=g_eax;g_xmm0=XMM_MEM(scratch+32);XMM_STORE(motion+0x70,g_xmm0);
 /* Integrate angular displacement with the game's original limit/polynomial. */
 push(MEMF(step+8));TOP=TOP*MEMF(0x44c1f8);g_xmm2=XMM_MEM(motion+0xe0);
 if(preserve)g_edx=scratch+24;else g_eax=scratch+24;
 MEMF(scratch+24)=(float)TOP;pop();g_xmm0=XMM_SCALAR(MEMF(scratch+24));g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm1=XMM_MUL(g_xmm1,g_xmm2);
 g_xmm0=XMM_MUL(g_xmm1,g_xmm1);g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm4=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);
 g_xmm3.f[0]=g_xmm3.f[0]+g_xmm0.f[0];g_xmm4.f[0]=g_xmm4.f[0]+g_xmm3.f[0];MEMF(scratch+24)=g_xmm4.f[0];
 push(MEMF(scratch+24));TOP=TOP*MEMF(0x498c14);XMM_STORE(scratch+48,g_xmm1);MEMF(scratch+20)=(float)TOP;pop();
 push(MEMF(motion+0xb8));TOP=TOP*MEMF(step+8);push(MEMF(0x44e548));g_fp_cmp=RECOMP_FCMP(TOP,PREV);pop();
 unsigned limit=status();if(!RECOMP_PARITY8(limit&5)){pop();push(MEMF(0x44e548));}
 push(TOP);TOP=TOP*PREV;MEMF(scratch+24)=(float)TOP;pop();push(MEMF(scratch+20));g_fp_cmp=RECOMP_FCMP(TOP,MEMF(scratch+24));pop();unsigned angular=status();
 if(RECOMP_PARITY8(angular&0x41)){
  push(MEMF(scratch+20));if(preserve){g_eax=MEM32(scratch+24);MEM32(scratch+20)=g_eax;}
  TOP=sqrt(TOP);PREV=PREV/TOP;pop();MEMF(scratch+28)=(float)TOP;pop();g_xmm0=XMM_SCALAR(MEMF(scratch+28));
  g_xmm3=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);XMM_STORE(motion+0xe0,g_xmm3);
  if(!preserve)g_ecx=MEM32(scratch+24);
  g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);XMM_STORE(scratch+48,g_xmm2);if(!preserve)MEM32(scratch+20)=g_ecx;
 }else pop();
 push(MEMF(scratch+20));if(preserve)g_ecx=scratch+32;else g_edx=scratch+32;TOP=TOP*MEMF(scratch+20);PUSH32(g_esp,scratch+32);
 push(MEMF(scratch+20));if(preserve)g_edx=scratch+48;else g_eax=scratch+48;TOP=TOP*MEMF(0x498c10);PUSH32(g_esp,scratch+48);g_ecx=scratch+32;TOP=MEMF(0x44bf94)-TOP;
 push(PREV);TOP=TOP*MEMF(0x498c0c);PREV=PREV-TOP;pop();{double v=TOP;TOP=PREV;PREV=v;}
 TOP=TOP*MEMF(scratch+20);TOP=TOP*MEMF(0x498c08);PREV=PREV-TOP;pop();MEMF(scratch+60)=(float)TOP;pop();
 g_ebp=frame;PUSH32(g_esp,returns[kind][0]);sub_002E2720();g_ecx=scratch+32;g_ebp=frame;PUSH32(g_esp,returns[kind][1]);sub_002E26A0();
 push(MEMF(scratch+20));TOP=sqrt(TOP);g_xmm0=XMM_MEM(scratch+48);g_xmm1=XMM_ADD(g_xmm0,g_xmm0);g_xmm0=XMM_MEM(scratch+32);
 if(preserve)g_eax=motion+0x80;else g_ecx=motion+0x80;XMM_STORE(motion+0xa0,g_xmm1);PUSH32(g_esp,motion+0x80);g_ecx=motion+16;
 TOP=TOP*MEMF(angles[kind]);MEMF(motion+0xac)=(float)TOP;pop();XMM_STORE(motion+0x80,g_xmm0);g_ebp=frame;PUSH32(g_esp,returns[kind][2]);sub_002E2930();
 g_xmm0=XMM_MEM(motion+0x90);rotate(motion+16);g_xmm1=XMM_MEM(motion+0x60);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);g_xmm1=XMM_SUB(g_xmm1,g_xmm0);XMM_STORE(motion+0x40,g_xmm1);
 if(preserve){
  g_xmm0=XMM_MEM(acc+16);XMM_STORE(motion+0xd0,g_xmm0);g_xmm0=XMM_MEM(acc+32);
  if(rotated){rotate(motion+16);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);}XMM_STORE(motion+0xe0,g_xmm0);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=acc+128;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=kinematic?g_eax==0:!(angular&0x41);g_esp+=16;return 1;
}
