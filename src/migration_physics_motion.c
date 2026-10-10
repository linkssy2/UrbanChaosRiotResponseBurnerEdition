/* Original motion classes produce the actual 128-byte solver accumulators.
 * No alternate integration, damping, mass, or ragdoll policy is introduced. */
#include "migration_physics_motion.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static void push_fp(double value){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;}
static void pop_fp(void){g_fp_top=(g_fp_top+1)&7;}
#define TOP g_fp_stack[g_fp_top]
/* Retain the guest x87 stack/status as well as the scalar result. The original
 * clamps with its status-word test, whose NaN behavior differs from fmaxf. */
static unsigned damping(uint32_t motion,uint32_t coefficient,uint32_t velocity,uint32_t scratch){
 TOP=TOP*MEMF(motion+coefficient);TOP=MEMF(0x44bf94)-TOP;
 push_fp(MEMF(0x44c174));g_fp_cmp=RECOMP_FCMP(TOP,g_fp_stack[(g_fp_top+1)&7]);pop_fp();
 unsigned status=((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);
 g_eax=(g_eax&0xffff0000)|(uint16_t)status;
 if(!((status>>8)&0x41)){pop_fp();push_fp(MEMF(0x44c174));}
 g_xmm1=XMM_MEM(motion+velocity);MEMF(scratch)=(float)TOP;pop_fp();
 g_xmm0=XMM_SCALAR(MEMF(scratch));g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);
 XMM_STORE(motion+velocity,g_xmm2);return !((status>>8)&0x41);
}
static void position(uint32_t motion,uint32_t step,uint32_t scratch,uint32_t output){
 push_fp(MEMF(step));TOP=TOP-MEMF(motion+0x5c);
 g_xmm1=XMM_MEM(motion+0x60);g_xmm3=XMM_MEM(0x48ddf0);TOP=TOP*MEMF(motion+0x6c);
 MEMF(scratch)=(float)TOP;pop_fp();g_xmm0=XMM_SCALAR(MEMF(scratch));g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);
 g_xmm2=g_xmm0;g_xmm3=XMM_SUB(g_xmm3,g_xmm0);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);
 g_xmm1=XMM_MEM(motion+0x50);g_xmm3=XMM_MUL(g_xmm3,g_xmm1);g_xmm3=XMM_ADD(g_xmm3,g_xmm2);XMM_STORE(output,g_xmm3);
}
static void identity(uint32_t output){
 for(unsigned i=0;i<3;i++)XMM_STORE(output+i*16,g_xmm0);
 for(unsigned i=0;i<3;i++)MEM32(output+i*20)=0x3f800000;
}
static void transpose_basis(uint32_t source,uint32_t output){
 /* Sources and destinations are preflighted disjoint. Padding is explicitly
  * zeroed, while the last guest source loads remain visible in EAX/EDX. */
 for(unsigned row=0;row<3;row++){
  for(unsigned col=0;col<3;col++)MEM32(output+row*16+col*4)=MEM32(source+col*16+row*4);
  MEM32(output+row*16+12)=0;
 }
 g_ecx=output;g_edx=MEM32(source+0x24);g_eax=MEM32(source+0x18);g_recomp_zf=g_edx==0;g_esp+=8;
}
int uc_migration_physics_motion(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_MOTION");enabled=v&&!strcmp(v,"1");}
 if(!enabled||(entry!=0x2f3cd0&&entry!=0x2f5630&&entry!=0x2fe830&&entry!=0x2fe940))return 0;
 if((g_esp&3)||g_esp<0x10100||!span(g_esp-128,144))return 0;
 uint32_t motion=g_ecx,step=MEM32(g_esp+4),output=MEM32(g_esp+8),frame=g_ebp;
 int dynamic=entry==0x2f3cd0||entry==0x2f5630,rotated=entry==0x2f5630;
 if((output&15)||!span(output,128)||!apart(output,128,g_esp-128,144))return 0;
 if(entry!=0x2fe830&&((motion&15)||!span(motion,256)||!apart(motion,256,output,128)||!apart(motion,256,g_esp-128,144)))return 0;
 if(dynamic&&(!span(step,12)||!span(0x44bf94,4)||!span(0x44c174,4)||!span(0x48ddf0,16)||!apart(step,12,output,128)||!apart(step,12,g_esp-128,144)))return 0;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;
 if(!dynamic){
  g_eax=output;MEM8(output)=1;g_xmm0=XMM_ZERO();XMM_STORE(output+0x30,g_xmm0);
  if(entry==0x2fe940){
   const unsigned from[]={0x60,0xe0,0xd0},to[]={0x40,0x20,0x10};
   for(unsigned i=0;i<3;i++){g_xmm1=XMM_MEM(motion+from[i]);XMM_STORE(output+to[i],g_xmm1);}
   identity(output+0x50);g_ecx=0x3f800000;MEM8(output+12)=1;
  }else{for(unsigned i=1;i<8;i++)if(i!=3)XMM_STORE(output+i*16,g_xmm0);}
  g_eax+=128;g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_eax==0;g_esp+=12;return 1;
 }
 g_esp&=~15u;
 if(rotated){PUSH32(g_esp,g_ecx);g_eax=step;push_fp(MEMF(step+8));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);push_fp(TOP);PUSH32(g_esp,g_edi);g_edi=motion;}
 else{g_esp-=12;g_edx=step;push_fp(MEMF(step+8));PUSH32(g_esp,g_esi);push_fp(TOP);}
 damping(motion,0xc8,0xd0,g_esp+12);
 unsigned last_zero=damping(motion,0xcc,0xe0,g_esp+12);
 if(rotated){
  g_esi=output;g_ebx=motion+16;g_ecx=output+0x50;PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x2f56dd);transpose_basis(g_ebx,g_ecx);
  g_ecx=step;MEM8(output)=0;g_xmm0=XMM_MEM(motion+0xf0);XMM_STORE(output+0x30,g_xmm0);g_xmm0=XMM_MEM(motion+0xd0);XMM_STORE(output+0x10,g_xmm0);
  position(motion,step,g_esp+12,output+0x40);
  g_xmm0=XMM_MEM(motion+0xe0);g_xmm2=XMM_MEM(output+0x70);g_xmm3=XMM_MEM(output+0x60);
  g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);
  g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm3=XMM_MUL(g_xmm3,g_xmm1);
  g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=XMM_MEM(output+0x50);g_xmm0=XMM_MUL(g_xmm0,g_xmm1);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);XMM_STORE(output+0x20,g_xmm0);
  MEM8(output+12)=0;g_edx=MEM16(motion+0xbc);MEM32(output+8)=g_edx;g_eax=MEM16(motion+0xbe);MEM32(output+4)=g_eax;
  POP32(g_esp,g_edi);g_eax=output+128;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=last_zero;
 }else{
  g_eax=output;MEM8(output)=0;g_esi=MEM32(motion+0xc4);MEM32(g_esp+12)=g_esi;g_xmm0=XMM_SCALAR(MEMF(g_esp+12));g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);XMM_STORE(output+0x30,g_xmm0);g_esi=MEM32(motion+0xc0);MEM32(output+0x3c)=g_esi;
  POP32(g_esp,g_esi);position(motion,step,g_esp+8,output+0x40);g_edx=0x3f800000;g_eax=output+128;
  g_xmm0=XMM_MEM(motion+0xe0);XMM_STORE(output+0x20,g_xmm0);g_xmm0=XMM_MEM(motion+0xd0);XMM_STORE(output+0x10,g_xmm0);g_xmm0=XMM_ZERO();identity(output+0x50);
  MEM8(output+12)=1;g_edx=MEM16(motion+0xbc);MEM32(output+8)=g_edx;g_ecx=MEM16(motion+0xbe);MEM32(output+4)=g_ecx;g_recomp_zf=g_eax==0;
 }
 g_esp=frame;POP32(g_esp,frame);g_esp+=12;return 1;
}
