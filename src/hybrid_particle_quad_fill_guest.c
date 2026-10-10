#include "hybrid_particle_quad_fill_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023A010(void);
extern void sub_0023B210(void);
/* Match original scalar destination/source NaN priority under precise lowering. */
static float quad_fill_add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
static float quad_fill_mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
static int quad_fill_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_QUAD_FILL");enabled=v&&!strcmp(v,"1");}return enabled;}
static int quad_fill_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define RQ_PUSH(v) do{double value=(v);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;}while(0)
#define RQ_POP() (g_fp_top=(g_fp_top+1)&7)
#define RQ_TOP() g_fp_stack[g_fp_top]
#define RQ_ST1() g_fp_stack[(g_fp_top+1)&7]
/* Source arithmetic keeps the original interpolation and inactive x87 slots. */
int uc_hybrid_particle_quad_fill(unsigned operation){
 if(!quad_fill_enabled()||operation>1)return 0;
 uint32_t start=g_esp;
 if((start&3)||start<0x10180u||!quad_fill_span(start-384,412)||!quad_fill_span(g_ecx,operation?20:0x124)||!quad_fill_span(0x51c178,1)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7)return 0;
 if(operation){
  g_eax=MEM32(g_esp+4)&0x8000000fu;if((int32_t)g_eax<0)g_eax=((g_eax-1)|0xfffffff0u)+1;
  RQ_PUSH(MEMF(g_eax*4+0x51ca98));{double saved_top=RQ_TOP();RQ_PUSH(saved_top);}RQ_TOP()*=MEMF(g_ecx+4);RQ_TOP()+=MEMF(g_ecx);
  RQ_PUSH(MEMF(g_esp+8));RQ_TOP()*=MEMF(g_ecx+16);RQ_ST1()*=RQ_TOP();RQ_POP();{double value=RQ_TOP();RQ_TOP()=RQ_ST1();RQ_ST1()=value;}
  RQ_TOP()*=MEMF(g_ecx+12);RQ_TOP()+=MEMF(g_ecx+8);RQ_TOP()*=MEMF(0x44d95c);RQ_ST1()+=RQ_TOP();RQ_POP();g_esp+=12;return 1;
 }
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-0xc4;
 PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_eax=MEM32(g_ebx+32);SET_LO8(g_ecx,MEM8(g_eax+0x9c));g_eax+=0x30;int camera=LO8(g_ecx)!=0;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(g_esp+0x2c)=g_eax;
 if(camera){g_eax=MEM32(frame+16);MEM32(g_esp+32)=g_eax;}else MEM32(g_esp+32)=0x51e110;
 g_eax=MEM32(frame+12);
 if((int32_t)g_eax>0){
  g_esi=MEM32(frame+8);g_ecx=MEM32(frame+24);g_edi=MEM32(frame+20);g_esi<<=5;MEM32(g_esp+24)=g_ecx;g_edi+=36;MEM32(g_esp+40)=g_eax;
  do{
   g_edx=MEM32(g_ebx+16);g_eax=MEM16(g_esi+g_edx+30);g_eax&=0x8000000fu;/* Zero extension makes the original signed-remainder repair unreachable. */
   g_eax<<=2;g_xmm0=XMM_SCALAR(MEMF(g_eax+0x51cad8));MEMF(g_esp+0x68)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+0x51cb18));g_eax=MEM32(g_ebx+16);g_ecx=MEM16(g_eax+g_esi+28);g_edx=MEM16(g_eax+g_esi+30);g_eax+=g_esi;MEMF(g_esp+36)=g_xmm0.f[0];
   PUSH32(g_esp,g_ecx);g_xmm0.f[0]=(float)(int32_t)g_ecx;g_xmm0.f[0] = quad_fill_mul(g_xmm0.f[0], MEMF(0x482fbc));MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_edx);g_ecx=g_ebx+0xb4;g_ebp=frame;PUSH32(g_esp,0x23af10);sub_0023B210();
   { double _t = RQ_TOP(); RQ_PUSH(_t); } RQ_TOP() = sin(RQ_TOP()); g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0x68));
   g_xmm0 = XMM_SCALAR(MEMF(0x44BF94)); g_eax = MEM32(g_ebx + 0x10); g_xmm3 = XMM_SCALAR(MEMF(g_eax + g_esi + 0xC));
   g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x24)); g_eax = g_eax + g_esi; MEMF(g_esp + 0x4C) = (float)RQ_TOP(); RQ_POP();
   g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x4C)); RQ_TOP() = cos(RQ_TOP()); g_xmm7 = g_xmm4;
   g_xmm4.f[0] = quad_fill_mul(g_xmm4.f[0], g_xmm3.f[0]); g_xmm7.f[0] = quad_fill_mul(g_xmm7.f[0], g_xmm5.f[0]); g_xmm7.f[0] = quad_fill_mul(g_xmm7.f[0], g_xmm3.f[0]);
   g_xmm4.f[0] = quad_fill_mul(g_xmm4.f[0], g_xmm6.f[0]); MEMF(g_esp + 0xB0) = g_xmm4.f[0]; MEMF(g_esp + 0x1C) = (float)RQ_TOP(); RQ_POP();
   g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x1C)); g_xmm0.f[0] = g_xmm0.f[0] - g_xmm2.f[0]; g_xmm1 = g_xmm0;
   g_xmm1.f[0] = quad_fill_mul(g_xmm1.f[0], g_xmm6.f[0]); g_xmm1.f[0] = quad_fill_mul(g_xmm1.f[0], g_xmm6.f[0]); g_xmm1.f[0] = quad_fill_add(g_xmm1.f[0], g_xmm2.f[0]);
   g_xmm2 = g_xmm0; g_xmm2.f[0] = quad_fill_mul(g_xmm2.f[0], g_xmm3.f[0]); g_xmm2.f[0] = quad_fill_mul(g_xmm2.f[0], g_xmm5.f[0]);
   g_xmm5 = XMM_ZERO(); g_xmm1.f[0] = quad_fill_mul(g_xmm1.f[0], g_xmm3.f[0]); g_xmm5.f[0] = g_xmm5.f[0] - g_xmm7.f[0];
   g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x24)); g_xmm0.f[0] = quad_fill_mul(g_xmm0.f[0], g_xmm7.f[0]); g_xmm0.f[0] = quad_fill_mul(g_xmm0.f[0], g_xmm7.f[0]);
   g_xmm0.f[0] = quad_fill_add(g_xmm0.f[0], MEMF(g_esp + 0x1C)); g_xmm0.f[0] = quad_fill_mul(g_xmm0.f[0], g_xmm3.f[0]); g_xmm3 = XMM_SCALAR(MEMF(g_eax));
   MEMF(g_esp + 0x80) = g_xmm3.f[0]; g_xmm3 = XMM_SCALAR(MEMF(g_eax + 4)); MEMF(g_esp + 0x84) = g_xmm3.f[0];
   g_xmm3 = XMM_SCALAR(MEMF(g_eax + 8)); g_eax = MEM32(g_esp + 0x20); g_xmm7 = XMM_MEM(g_eax + 0x10);
   MEMF(g_esp + 0x88) = g_xmm3.f[0]; g_xmm3 = XMM_ZERO(); MEMF(g_esp + 0x8C) = g_xmm3.f[0];
   g_xmm3 = XMM_MEM(g_esp + 0x80); g_xmm2.f[0] = quad_fill_mul(g_xmm2.f[0], g_xmm6.f[0]); g_xmm6 = XMM_MEM(g_eax + 0x20);
   g_xmm4 = g_xmm3; g_xmm4 = XMM_SHUFFLE(g_xmm4, g_xmm3, 0xAA); g_xmm6 = XMM_MUL(g_xmm6, g_xmm4);
   g_xmm4 = g_xmm3; g_xmm4 = XMM_SHUFFLE(g_xmm4, g_xmm3, 0x55); g_xmm7 = XMM_MUL(g_xmm7, g_xmm4);
   g_xmm4 = XMM_MEM(g_eax + 0x30); g_xmm7 = XMM_ADD(g_xmm7, g_xmm6); g_xmm6 = g_xmm3;
   g_xmm6 = XMM_SHUFFLE(g_xmm6, g_xmm3, 0); g_xmm3 = XMM_MEM(g_eax); g_xmm3 = XMM_MUL(g_xmm3, g_xmm6);
   g_xmm3 = XMM_ADD(g_xmm3, g_xmm4); MEMF(g_esp + 0xA0) = g_xmm2.f[0]; g_xmm3 = XMM_ADD(g_xmm3, g_xmm7);
   XMM_STORE(g_esp + 0x70, g_xmm3); g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x70)); g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x74));
   g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0x78)); g_xmm3.f[0] = quad_fill_add(g_xmm3.f[0], g_xmm1.f[0]); g_xmm7 = g_xmm3;
   g_xmm4.f[0] = quad_fill_add(g_xmm4.f[0], g_xmm2.f[0]); g_xmm7.f[0] = quad_fill_add(g_xmm7.f[0], g_xmm2.f[0]); MEMF(g_esp + 0x50) = g_xmm7.f[0];
   g_edx = MEM32(g_esp + 0x50); g_xmm7 = g_xmm4; g_xmm6.f[0] = quad_fill_add(g_xmm6.f[0], g_xmm5.f[0]);
   MEMF(g_esp + 0x48) = g_xmm6.f[0]; g_xmm7.f[0] = quad_fill_add(g_xmm7.f[0], g_xmm0.f[0]); g_xmm3.f[0] = g_xmm3.f[0] - g_xmm2.f[0];
   MEMF(g_esp + 0x54) = g_xmm7.f[0]; g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0xB0)); g_eax = MEM32(g_esp + 0x54);
   g_xmm6.f[0] = quad_fill_add(g_xmm6.f[0], g_xmm7.f[0]); g_ecx = g_edi + -36; MEM32(g_ecx) = g_edx;
   MEM32(g_ecx + 4) = g_eax; MEMF(g_esp + 0x5C) = g_xmm3.f[0]; g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x70));
   MEMF(g_esp + 0x58) = g_xmm6.f[0]; g_edx = MEM32(g_esp + 0x58); g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0x48));
   MEM32(g_ecx + 8) = g_edx; g_ecx = MEM32(g_esp + 0x5C); g_xmm3.f[0] = g_xmm3.f[0] - g_xmm1.f[0];
   MEMF(g_esp + 0xB8) = g_xmm4.f[0]; g_xmm4.f[0] = g_xmm4.f[0] - g_xmm0.f[0]; MEMF(g_esp + 0x60) = g_xmm4.f[0];
   g_edx = MEM32(g_esp + 0x60); g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x78)); g_eax = g_edi + -24;
   MEM32(g_eax) = g_ecx; MEM32(g_eax + 4) = g_edx; g_xmm6.f[0] = g_xmm6.f[0] - g_xmm7.f[0];
   MEMF(g_esp + 0x64) = g_xmm6.f[0]; g_ecx = MEM32(g_esp + 0x64); g_xmm6 = g_xmm3;
   g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x74)); MEM32(g_eax + 8) = g_ecx; g_xmm3.f[0] = g_xmm3.f[0] - g_xmm2.f[0];
   g_xmm1 = g_xmm6; g_xmm1.f[0] = g_xmm1.f[0] - g_xmm2.f[0]; MEMF(g_esp + 0x6C) = g_xmm3.f[0];
   g_xmm4.f[0] = g_xmm4.f[0] - g_xmm5.f[0]; MEMF(g_esp + 0x30) = g_xmm1.f[0]; g_eax = MEM32(g_esp + 0x30);
   g_xmm3.f[0] = g_xmm3.f[0] - g_xmm0.f[0]; g_xmm5 = g_xmm4; MEMF(g_esp + 0x34) = g_xmm3.f[0];
   g_ecx = MEM32(g_esp + 0x34); g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x6C)); g_xmm4.f[0] = g_xmm4.f[0] - g_xmm7.f[0];
   g_edx = g_edi; MEM32(g_edx) = g_eax; MEM32(g_edx + 4) = g_ecx;
   MEMF(g_esp + 0x38) = g_xmm4.f[0]; g_eax = MEM32(g_esp + 0x38); MEM32(g_edx + 8) = g_eax;
   g_xmm1 = g_xmm6; g_xmm1.f[0] = quad_fill_add(g_xmm1.f[0], g_xmm2.f[0]); MEMF(g_esp + 0x3C) = g_xmm1.f[0];
   g_edx = MEM32(g_esp + 0x3C); g_ecx = g_edi + -12; MEM32(g_ecx) = g_edx;
   g_xmm3.f[0] = quad_fill_add(g_xmm3.f[0], g_xmm0.f[0]); g_xmm4 = g_xmm5; MEMF(g_esp + 0x40) = g_xmm3.f[0];
   g_eax = MEM32(g_esp + 0x40); g_xmm4.f[0] = quad_fill_add(g_xmm4.f[0], g_xmm7.f[0]); MEM32(g_ecx + 4) = g_eax;
   g_eax = MEM32(g_esp + 0x2C); MEMF(g_esp + 0x44) = g_xmm4.f[0]; g_edx = MEM32(g_esp + 0x44);
   MEM32(g_ecx + 8) = g_edx;
   SET_LO8(g_ecx,MEM8(g_eax+0x76));int thermal=LO8(g_ecx)!=0;
   if(thermal){g_ecx=MEM32(g_ebx+0xac);SET_LO8(g_edx,MEM8(g_ecx+12));thermal=LO8(g_edx)!=0;}
   if(thermal){g_edx=MEM32(g_esp+24);g_xmm0=XMM_MEM(g_eax+0x80);XMM_STORE(g_edx,g_xmm0);}
   else{g_eax=MEM32(g_ebx+16);g_ecx=MEM16(g_esi+g_eax+28);PUSH32(g_esp,g_ecx);g_xmm0.f[0]=(float)(int32_t)g_ecx;g_xmm0.f[0] = quad_fill_mul(g_xmm0.f[0], MEMF(0x482fbc));g_edx=g_esp+0xc4;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_edx);g_ecx=g_ebx+0xcc;g_ebp=frame;PUSH32(g_esp,0x23b1d2);sub_0023A010();g_xmm0=XMM_MEM(g_eax);g_eax=MEM32(g_esp+24);XMM_STORE(g_eax,g_xmm0);}
   g_ecx=MEM32(g_esp+24);g_eax=MEM32(g_esp+40);g_ecx+=16;g_esi+=32;g_edi+=48;g_eax--;MEM32(g_esp+24)=g_ecx;MEM32(g_esp+40)=g_eax;
  }while(g_eax);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=24;return 1;
}
#undef RQ_PUSH
#undef RQ_POP
#undef RQ_TOP
#undef RQ_ST1
