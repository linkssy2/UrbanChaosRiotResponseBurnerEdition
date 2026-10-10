#include "migration_particles_lightning_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_002D3780(void),sub_00174990(void),sub_002D4EE0(void);
static int enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_LIGHTNING_MATH");value=p&&!strcmp(p,"1");}return value;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define FP_PUSH(v) do{double v_=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=v_;}while(0)
#define FP_TOP g_fp_stack[g_fp_top]
#define FP_ST(i) g_fp_stack[(g_fp_top+(i))&7u]
#define FP_POP() (g_fp_top=(g_fp_top+1u)&7u)

/* Three draws from the original shared stream, including retired argument
 * slots and x87 residues. The independently proposed RNG admission extension
 * covers these PCs; the original RNG remains available when that gate is off. */
static void random_vector(void){
 g_esp-=8;
 for(unsigned axis=0;axis<3;axis++){
  PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0xbf800000);
  static const unsigned returns[]={0x1749a2,0x1749b5,0x1749c8};CALL(sub_002D3780,returns[axis]);
  if(axis<2){MEMF(g_esp+(axis?16:12))=(float)FP_TOP;FP_POP();}
 }
 FP_PUSH(MEMF(g_esp+24));g_eax=MEM32(g_esp+36);g_ecx=MEM32(g_esp+28);
 MEMF(g_eax)=(float)FP_TOP;FP_POP();MEM32(g_eax+4)=g_ecx;MEMF(g_eax+8)=(float)FP_TOP;FP_POP();
 g_esp+=32;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void reset_endpoint(void){
 g_esp-=24;PUSH32(g_esp,g_esi);g_eax=g_esp+16;PUSH32(g_esp,g_eax);g_esi=g_ecx;CALL(sub_00174990,0x1749f0);
 g_xmm1=XMM_SCALAR(MEMF(g_eax));g_xmm0=XMM_SCALAR(MEMF(0x44c538));g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+8)=g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(MEMF(g_eax+4));g_edx=MEM32(g_esp+8);g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(MEMF(g_eax+8));g_eax=MEM32(g_esp+12);g_ecx=g_esi;MEM32(g_ecx)=g_edx;MEM32(g_ecx+4)=g_eax;
 g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+16)=g_xmm1.f[0];g_edx=MEM32(g_esp+16);g_eax=g_esp+20;PUSH32(g_esp,g_eax);MEM32(g_ecx+8)=g_edx;CALL(sub_00174990,0x174a44);
 g_xmm1=XMM_SCALAR(MEMF(g_eax));g_xmm0=XMM_SCALAR(MEMF(0x44c1f8));g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(MEMF(g_eax+4));g_ecx=MEM32(g_esp+12);g_xmm1.f[0]*=g_xmm0.f[0];g_esp+=8;MEMF(g_esp+8)=g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(MEMF(g_eax+8));g_edx=MEM32(g_esp+8);g_esi+=12;MEM32(g_esi)=g_ecx;g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm1.f[0];
 g_eax=MEM32(g_esp+12);MEM32(g_esi+4)=g_edx;MEM32(g_esi+8)=g_eax;POP32(g_esp,g_esi);g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void integrate_endpoint(void){
 g_eax=MEM32(0x51c4dc);g_xmm0=XMM_SCALAR(MEMF(g_eax+20));
 g_xmm1=XMM_SCALAR(MEMF(g_ecx+12));g_xmm2=XMM_SCALAR(MEMF(g_ecx+16));g_xmm3=XMM_SCALAR(MEMF(g_ecx+20));
 g_xmm1.f[0]*=g_xmm0.f[0];g_xmm2.f[0]*=g_xmm0.f[0];g_xmm3.f[0]*=g_xmm0.f[0];
 for(unsigned axis=0;axis<3;axis++){
  g_xmm0=XMM_SCALAR(MEMF(g_ecx+4*axis));g_xmm0.f[0]+=axis==0?g_xmm1.f[0]:axis==1?g_xmm2.f[0]:g_xmm3.f[0];MEMF(g_ecx+4*axis)=g_xmm0.f[0];
 }
 g_esp+=4;
}
/* Cubic Bezier: p0*(1-t)^3 + 3*p1*(1-t)^2*t + 3*p2*(1-t)*t^2
 * + p3*t^3. Preserve the original scalar grouping and scratch/register ABI;
 * vector results may alias inputs because all inputs precede the stores. */
static void cubic_bezier(void){
 g_esp-=24;g_xmm3=XMM_SCALAR(MEMF(g_esp+48));g_eax=MEM32(g_esp+44);
 g_xmm4=XMM_SCALAR(MEMF(g_eax));g_xmm5=XMM_SCALAR(MEMF(g_eax+4));g_xmm6=XMM_SCALAR(MEMF(g_eax+8));
 g_eax=MEM32(g_esp+40);g_xmm7=XMM_SCALAR(MEMF(0x46c31c));g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm1.f[0]-=g_xmm3.f[0];
 g_xmm0=g_xmm3;g_xmm0.f[0]*=g_xmm3.f[0];g_xmm2=g_xmm0;g_xmm2.f[0]*=g_xmm3.f[0];
 g_xmm4.f[0]*=g_xmm2.f[0];g_xmm6.f[0]*=g_xmm2.f[0];g_xmm5.f[0]*=g_xmm2.f[0];
 g_xmm0.f[0]*=g_xmm7.f[0];g_xmm0.f[0]*=g_xmm1.f[0];
 for(unsigned axis=0;axis<3;axis++){g_xmm2=XMM_SCALAR(MEMF(g_eax+4*axis));g_xmm2.f[0]*=g_xmm0.f[0];MEMF(g_esp+4*axis)=g_xmm2.f[0];}
 g_eax=MEM32(g_esp+36);g_xmm0=g_xmm1;g_xmm0.f[0]*=g_xmm1.f[0];g_xmm2=g_xmm0;g_xmm2.f[0]*=g_xmm7.f[0];
 g_xmm7=XMM_SCALAR(MEMF(g_eax));g_xmm2.f[0]*=g_xmm3.f[0];g_xmm3=XMM_SCALAR(MEMF(g_eax+4));g_xmm3.f[0]*=g_xmm2.f[0];MEMF(g_esp+16)=g_xmm3.f[0];
 g_xmm3=XMM_SCALAR(MEMF(g_eax+8));g_eax=MEM32(g_esp+32);g_xmm3.f[0]*=g_xmm2.f[0];g_xmm0.f[0]*=g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(MEMF(g_eax));g_xmm7.f[0]*=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_eax+4));MEMF(g_esp+20)=g_xmm3.f[0];g_xmm3=XMM_SCALAR(MEMF(g_eax+8));g_eax=MEM32(g_esp+28);
 g_xmm1.f[0]*=g_xmm0.f[0];g_xmm2.f[0]*=g_xmm0.f[0];g_xmm2.f[0]+=MEMF(g_esp+16);g_xmm2.f[0]+=MEMF(g_esp+4);
 g_xmm3.f[0]*=g_xmm0.f[0];g_xmm3.f[0]+=MEMF(g_esp+20);g_xmm3.f[0]+=MEMF(g_esp+8);
 g_xmm1.f[0]+=g_xmm7.f[0];g_xmm1.f[0]+=MEMF(g_esp);g_xmm1.f[0]+=g_xmm4.f[0];g_xmm2.f[0]+=g_xmm5.f[0];g_xmm3.f[0]+=g_xmm6.f[0];
 MEMF(g_eax)=g_xmm1.f[0];MEMF(g_eax+4)=g_xmm2.f[0];MEMF(g_eax+8)=g_xmm3.f[0];g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void build_beam(void){
 g_esp-=112;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);
 /* A zero-length authored beam gets the original one-unit Y displacement.
  * NaNs are unordered and therefore do not enter the equal-endpoint branch. */
 uint32_t p=g_esi;
 if(MEMF(p+0x2b4)==MEMF(p+0x2c0)&&MEMF(p+0x2b8)==MEMF(p+0x2c4)&&MEMF(p+0x2bc)==MEMF(p+0x2c8))MEMF(p+0x2c4)=MEMF(p+0x2c4)+MEMF(0x44bf94);
 /* Retain the original double-backed x87 stack, including dead slots. */
 FP_PUSH(MEMF(p+0x2c0));FP_TOP-=MEMF(p+0x2b4);
 FP_PUSH(MEMF(p+0x2c4));FP_TOP-=MEMF(p+0x2b8);
 FP_PUSH(MEMF(p+0x2c8));FP_TOP-=MEMF(p+0x2bc);
 {double v=FP_TOP;FP_PUSH(v);}FP_TOP*=FP_ST(1);
 {double v=FP_ST(2);FP_PUSH(v);}FP_TOP*=FP_ST(3);FP_ST(1)+=FP_TOP;FP_POP();
 {double v=FP_ST(3);FP_PUSH(v);}FP_TOP*=FP_ST(4);FP_ST(1)+=FP_TOP;FP_POP();
 FP_TOP=sqrt(FP_TOP);FP_ST(3)=FP_TOP;FP_POP();FP_POP();FP_POP();MEMF(g_esp+12)=(float)FP_TOP;FP_POP();
 float length=MEMF(g_esp+12);
 /* Form the two displaced interior control points. Keep each multiplication
  * and addition separate, in the original source-operand order. */
 for(unsigned axis=0;axis<3;axis++){
  float start=MEMF(p+0x2b4+4*axis),end=MEMF(p+0x2c0+4*axis),delta=end-start;
  float one=delta*MEMF(0x470000);one=start+one;
  float offset=MEMF(p+0x88+4*axis)*length;one=offset+one;
  float two=delta*MEMF(0x46675c);two=two+start;
  if(axis>0)two=start+(delta*MEMF(0x46675c));
  offset=MEMF(p+0xa0+4*axis)*length;two=offset+two;
  MEMF(g_esp+0x4c+4*axis)=start;MEMF(g_esp+0x58+4*axis)=one;MEMF(g_esp+0x64+4*axis)=two;MEMF(g_esp+0x70+4*axis)=end;
  if(axis==1)MEMF(g_esp+0x38)=one;
  if(axis==0)MEMF(g_esp+0x40)=(delta*MEMF(0x46675c))+start;
 }
 /* Twenty-one samples and symmetric finite-difference tangents. Guest
  * temporaries/arguments stay in the original slots for callers and aliases. */
 g_edi=0;g_ebx=g_esi+0x1b4;
 do{
  MEM32(g_esp+12)=g_edi;FP_PUSH((double)SMEM32(g_esp+12));FP_TOP*=MEMF(0x44c50c);
  g_ecx=g_esp+0x70;g_edx=g_esp+0x64;MEMF(g_esp+12)=(float)FP_TOP;FP_POP();g_eax=MEM32(g_esp+12);
  PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_eax=g_esp+0x64;PUSH32(g_esp,g_eax);g_ecx=g_esp+0x5c;PUSH32(g_esp,g_ecx);g_edx=g_esp+0x3c;PUSH32(g_esp,g_edx);CALL(sub_002D4EE0,0x174d84);
  FP_PUSH(MEMF(g_esp+0x24));g_ecx=MEM32(g_esp+0x40);FP_TOP-=MEMF(0x44d024);g_edx=MEM32(g_esp+0x44);g_eax=g_ebx-252;MEM32(g_eax)=g_ecx;
  MEMF(g_esp+0x14)=(float)FP_TOP;FP_POP();g_ecx=MEM32(g_esp+0x48);MEM32(g_eax+4)=g_edx;g_esp+=20;MEM32(g_eax+8)=g_ecx;
  g_edx=g_esp+0x74;PUSH32(g_esp,g_edx);g_eax=g_esp+0x6c;PUSH32(g_esp,g_eax);g_ecx=g_esp+0x64;PUSH32(g_esp,g_ecx);g_edx=g_esp+0x5c;PUSH32(g_esp,g_edx);g_eax=g_esp+0x54;PUSH32(g_esp,g_eax);CALL(sub_002D4EE0,0x174dcd);
  FP_PUSH(MEMF(g_esp+0x24));FP_TOP+=MEMF(0x44d024);g_esp+=20;g_ecx=g_esp+0x74;g_edx=g_esp+0x68;MEMF(g_esp)=(float)FP_TOP;FP_POP();
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_eax=g_esp+0x64;PUSH32(g_esp,g_eax);g_ecx=g_esp+0x5c;PUSH32(g_esp,g_ecx);g_edx=g_esp+0x48;PUSH32(g_esp,g_edx);CALL(sub_002D4EE0,0x174dfb);
  g_xmm2=XMM_SCALAR(MEMF(g_esp+0x54));g_xmm2.f[0]-=MEMF(g_esp+0x60);g_xmm1=XMM_SCALAR(MEMF(g_esp+0x50));g_xmm1.f[0]-=MEMF(g_esp+0x5c);g_xmm0=XMM_SCALAR(MEMF(g_esp+0x4c));g_xmm0.f[0]-=MEMF(g_esp+0x58);
  MEMF(g_esp+0x30)=g_xmm2.f[0];FP_PUSH(MEMF(g_esp+0x30));FP_TOP*=MEMF(g_esp+0x30);
  MEMF(g_esp+0x2c)=g_xmm1.f[0];FP_PUSH(MEMF(g_esp+0x2c));MEMF(g_esp+0x28)=g_xmm0.f[0];FP_TOP*=MEMF(g_esp+0x2c);g_esp+=24;FP_ST(1)+=FP_TOP;FP_POP();
  FP_PUSH(MEMF(g_esp+0x10));FP_TOP*=MEMF(g_esp+0x10);FP_ST(1)+=FP_TOP;FP_POP();FP_TOP=sqrt(FP_TOP);FP_TOP=MEMF(0x44bf94)/FP_TOP;MEMF(g_esp+12)=(float)FP_TOP;FP_POP();
  g_xmm3=XMM_SCALAR(MEMF(g_esp+12));g_xmm0.f[0]*=g_xmm3.f[0];MEMF(g_esp+0x1c)=g_xmm0.f[0];g_ecx=MEM32(g_esp+0x1c);g_eax=g_ebx;
  g_xmm1.f[0]*=g_xmm3.f[0];MEM32(g_eax)=g_ecx;MEMF(g_esp+0x20)=g_xmm1.f[0];g_edx=MEM32(g_esp+0x20);g_xmm2.f[0]*=g_xmm3.f[0];MEMF(g_esp+0x24)=g_xmm2.f[0];g_ecx=MEM32(g_esp+0x24);
  ++g_edi;MEM32(g_eax+4)=g_edx;g_ebx+=12;MEM32(g_eax+8)=g_ecx;
 }while(g_edi<21);
 PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);CALL(sub_002D3780,0x174eaf);MEMF(g_esi+0x2f0)=(float)FP_TOP;FP_POP();
 g_esp+=8;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=112;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_particles_lightning(unsigned entry){
 if(!enabled())return 0;uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10100u||!span(s-256,288)||!span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80u)!=0x1f80u)return 0;
 if(entry==0x174990){if(!span(MEM32(s+4),12)||!apart(MEM32(s+4),12,s-48,56))return 0;random_vector();return 1;}
 if(entry==0x1749e0){if(!span(p,24)||!apart(p,24,s-80,88)||!span(0x44c1f8,4)||!span(0x44c538,4))return 0;reset_endpoint();return 1;}
 if(entry==0x175740){if(!span(p,24)||!span(0x51c4dc,4)||!span(MEM32(0x51c4dc),24))return 0;integrate_endpoint();return 1;}
 if(entry==0x174ae0){if(!span(p,0x300)||!apart(p,0x300,s-256,288)||!span(0x44bf94,0x470004-0x44bf94))return 0;build_beam();return 1;}
 if(entry==0x2d4ee0){
  if(!span(0x44bf94,4)||!span(0x46c31c,4))return 0;
  for(unsigned i=1;i<=5;i++){uint32_t v=MEM32(s+4*i);if(!span(v,12)||!apart(v,12,s-24,52))return 0;}
  cubic_bezier();return 1;
 }
 return 0;
}
#undef CALL
#undef FP_PUSH
#undef FP_TOP
#undef FP_ST
#undef FP_POP
