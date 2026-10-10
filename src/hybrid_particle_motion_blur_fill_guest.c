#include "hybrid_particle_motion_blur_fill_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_0023A010(void);
static int motion_blur_fill_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_MOTION_BLUR_FILL");enabled=v&&!strcmp(v,"1");}return enabled;}
static int motion_blur_fill_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define MB_PUSH(v) do{double value=(v);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;}while(0)
#define MB_POP() (g_fp_top=(g_fp_top+1)&7)
#define MB_TOP() g_fp_stack[g_fp_top]
#define MB_ST1() g_fp_stack[(g_fp_top+1)&7]

/* Two source-owned loops. The arithmetic schedule preserves retired scratch,
 * live output aliases, exceptional values and the virtual x87 inactive slots. */
int uc_hybrid_particle_motion_blur_fill(void){
 if(!motion_blur_fill_enabled())return 0;
 uint32_t start=g_esp;
 if((start&3)||start<0x10180u||!motion_blur_fill_span(start-384,412)||!motion_blur_fill_span(g_ecx,0x114)||
    !motion_blur_fill_span(0x51c178,1)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7)return 0;
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-0xd4;
 g_edx=MEM32(g_ecx+0x20);SET_LO8(g_eax,MEM8(g_edx+0x9c));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edi=MEM32(frame+16);g_xmm0=XMM_SCALAR(MEMF(g_edi+32));g_edx+=0x30;MEMF(g_esp+0x2c)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_edi+0x24));MEMF(g_esp+0x30)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_edi+0x28));
 int camera=LO8(g_eax)!=0;g_eax=MEM32(frame+12);MEM32(g_esp+0x70)=g_ecx;MEMF(g_esp+0x34)=g_xmm0.f[0];MEM32(g_esp+0x5c)=g_edx;
 if(camera){
  if((int32_t)g_eax>0){
   g_esi=MEM32(frame+0x18);g_ebx=MEM32(frame+8);MEM32(g_esp+0x18)=g_esi;g_esi=MEM32(frame+0x14);g_ebx<<=5;g_esi+=0x24;MEM32(g_esp+0x1c)=g_eax;
   do{
    g_xmm0 = XMM_SCALAR(MEMF(g_edx + 0x9C)); g_eax = MEM32(g_ecx + 0x10); g_xmm1 = XMM_SCALAR(MEMF(g_eax + g_ebx + 0x14));
    g_xmm4 = XMM_SCALAR(MEMF(g_eax + g_ebx + 0x10)); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0]; g_xmm4.f[0] = g_xmm4.f[0] * g_xmm0.f[0];
    g_xmm2 = XMM_MEM(g_edi + 0x10); g_eax = g_eax + g_ebx; g_xmm3 = XMM_MEM(g_edi);
    MEMF(g_esp + 0x78) = g_xmm1.f[0]; g_xmm1 = XMM_SCALAR(MEMF(g_eax + 0x18)); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_eax)); MEMF(g_esp + 0xA0) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4));
    MEMF(g_esp + 0xA4) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8)); MEMF(g_esp + 0xA8) = g_xmm0.f[0];
    g_xmm0 = XMM_ZERO(); MEMF(g_esp + 0xAC) = g_xmm0.f[0]; g_xmm0 = XMM_MEM(g_esp + 0xA0);
    g_xmm5 = g_xmm0; g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0xAA); MEMF(g_esp + 0x7C) = g_xmm1.f[0];
    g_xmm1 = XMM_MEM(g_edi + 0x20); g_xmm6 = g_xmm1; g_xmm6 = XMM_MUL(g_xmm6, g_xmm5);
    g_xmm5 = g_xmm0; g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0x55); g_xmm7 = g_xmm2;
    g_xmm7 = XMM_MUL(g_xmm7, g_xmm5); g_xmm5 = g_xmm0; g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0);
    g_xmm0 = g_xmm3; g_xmm0 = XMM_MUL(g_xmm0, g_xmm5); g_xmm5 = XMM_SCALAR(MEMF(g_eax + 8));
    g_xmm5.f[0] = g_xmm5.f[0] + MEMF(g_esp + 0x7C); g_xmm7 = XMM_ADD(g_xmm7, g_xmm6); g_xmm6 = XMM_MEM(g_edi + 0x30);
    g_xmm0 = XMM_ADD(g_xmm0, g_xmm6); g_xmm0 = XMM_ADD(g_xmm0, g_xmm7); XMM_STORE(g_esp + 0xC0, g_xmm0);
    g_xmm0 = XMM_SCALAR(MEMF(g_eax)); g_xmm0.f[0] = g_xmm0.f[0] + g_xmm4.f[0]; g_xmm4 = XMM_SCALAR(MEMF(g_eax + 4));
    g_xmm4.f[0] = g_xmm4.f[0] + MEMF(g_esp + 0x78); MEMF(g_esp + 0xB0) = g_xmm0.f[0]; g_xmm0 = XMM_ZERO();
    MEMF(g_esp + 0xB4) = g_xmm4.f[0]; MEMF(g_esp + 0xBC) = g_xmm0.f[0]; MEMF(g_esp + 0xB8) = g_xmm5.f[0];
    g_xmm0 = XMM_MEM(g_esp + 0xB0); g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0xC8)); g_xmm4 = g_xmm0;
    g_xmm4 = XMM_SHUFFLE(g_xmm4, g_xmm0, 0xAA); g_xmm1 = XMM_MUL(g_xmm1, g_xmm4); g_xmm4 = g_xmm0;
    g_xmm4 = XMM_SHUFFLE(g_xmm4, g_xmm0, 0x55); g_xmm2 = XMM_MUL(g_xmm2, g_xmm4); g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0xC4));
    g_xmm2 = XMM_ADD(g_xmm2, g_xmm1); g_xmm1 = g_xmm0; g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0);
    g_xmm0 = XMM_SCALAR(MEMF(g_eax + 0xC)); g_xmm3 = XMM_MUL(g_xmm3, g_xmm1); g_xmm3 = XMM_ADD(g_xmm3, g_xmm6);
    g_xmm3 = XMM_ADD(g_xmm3, g_xmm2); XMM_STORE(g_esp + 0x80, g_xmm3); g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0xC0));
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x84)); g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x88)); MEMF(g_esp + 0x6C) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x80)); g_xmm0.f[0] = g_xmm0.f[0] - g_xmm3.f[0]; MEMF(g_esp + 0x20) = g_xmm0.f[0];
    MB_PUSH(MEMF(g_esp + 0x20)); MB_TOP() = MB_TOP() * MEMF(g_esp + 0x20); g_xmm1.f[0] = g_xmm1.f[0] - g_xmm4.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] - g_xmm5.f[0]; MEMF(g_esp + 0x24) = g_xmm1.f[0]; MEMF(g_esp + 0x28) = g_xmm2.f[0];
    MB_PUSH(MEMF(g_esp + 0x28)); MB_TOP() = MB_TOP() * MEMF(g_esp + 0x28); g_edx = g_esi + -36;
    MB_ST1() = MB_ST1() + MB_TOP(); MB_POP(); MB_PUSH(MEMF(g_esp + 0x24)); MB_TOP() = MB_TOP() * MEMF(g_esp + 0x24);
    MB_ST1() = MB_ST1() + MB_TOP(); MB_POP(); MB_TOP() = sqrt(MB_TOP()); MB_TOP() = MEMF(0x44BF94) / MB_TOP();
    MEMF(g_esp + 0x9C) = (float)MB_TOP(); MB_POP(); g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x9C)); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm7.f[0];
    g_xmm6 = g_xmm1; g_xmm0.f[0] = g_xmm0.f[0] * g_xmm7.f[0]; MEMF(g_esp + 0x90) = g_xmm0.f[0];
    g_xmm0 = g_xmm6; g_xmm6.f[0] = g_xmm6.f[0] * MEMF(g_esp + 0x2C); g_xmm0.f[0] = g_xmm0.f[0] * MEMF(g_esp + 0x34);
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm7.f[0]; g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x90)); g_xmm1 = g_xmm2;
    g_xmm1.f[0] = g_xmm1.f[0] * MEMF(g_esp + 0x30); g_xmm2.f[0] = g_xmm2.f[0] * MEMF(g_esp + 0x2C); g_xmm0.f[0] = g_xmm0.f[0] - g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x34)); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm7.f[0]; g_xmm2.f[0] = g_xmm2.f[0] - g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x30)); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm7.f[0]; g_xmm1.f[0] = g_xmm1.f[0] - g_xmm6.f[0];
    g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0x6C)); g_xmm0.f[0] = g_xmm0.f[0] * g_xmm6.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm6.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm6.f[0]; g_xmm6 = XMM_SCALAR(MEMF(0x44C1F8)); g_xmm0.f[0] = g_xmm0.f[0] * g_xmm6.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm6.f[0]; g_xmm1.f[0] = g_xmm1.f[0] * g_xmm6.f[0]; g_xmm6 = g_xmm0;
    g_xmm6.f[0] = g_xmm6.f[0] + g_xmm3.f[0]; MEMF(g_esp + 0x50) = g_xmm6.f[0]; g_eax = MEM32(g_esp + 0x50);
    MEM32(g_edx) = g_eax; g_xmm6 = g_xmm2; g_xmm6.f[0] = g_xmm6.f[0] + g_xmm4.f[0];
    MEMF(g_esp + 0x54) = g_xmm6.f[0]; g_eax = MEM32(g_esp + 0x54); MEM32(g_edx + 4) = g_eax;
    g_xmm6 = g_xmm1; g_xmm6.f[0] = g_xmm6.f[0] + g_xmm5.f[0]; MEMF(g_esp + 0x58) = g_xmm6.f[0];
    g_eax = MEM32(g_esp + 0x58); MEM32(g_edx + 8) = g_eax; g_xmm4.f[0] = g_xmm4.f[0] - g_xmm2.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] - g_xmm0.f[0]; MEMF(g_esp + 0x3C) = g_xmm4.f[0]; MEMF(g_esp + 0x38) = g_xmm3.f[0];
    g_eax = MEM32(g_esp + 0x38); g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x80)); g_xmm5.f[0] = g_xmm5.f[0] - g_xmm1.f[0];
    g_edx = g_esi + -24; MEM32(g_edx) = g_eax; g_eax = MEM32(g_esp + 0x3C);
    MEMF(g_esp + 0x40) = g_xmm5.f[0]; g_xmm4 = g_xmm3; g_xmm4.f[0] = g_xmm4.f[0] - g_xmm0.f[0];
    MEMF(g_esp + 0x60) = g_xmm4.f[0]; g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x84)); MEM32(g_edx + 4) = g_eax;
    g_eax = MEM32(g_esp + 0x40); g_xmm5 = g_xmm4; g_xmm5.f[0] = g_xmm5.f[0] - g_xmm2.f[0];
    MEMF(g_esp + 0x64) = g_xmm5.f[0]; g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x88)); MEM32(g_edx + 8) = g_eax;
    g_xmm6 = g_xmm5; g_eax = MEM32(g_esp + 0x60); g_edx = g_esi;
    MEM32(g_edx) = g_eax; g_eax = MEM32(g_esp + 0x64); MEM32(g_edx + 4) = g_eax;
    g_xmm6.f[0] = g_xmm6.f[0] - g_xmm1.f[0]; MEMF(g_esp + 0x68) = g_xmm6.f[0]; g_eax = MEM32(g_esp + 0x68);
    MEM32(g_edx + 8) = g_eax; g_xmm0.f[0] = g_xmm0.f[0] + g_xmm3.f[0]; MEMF(g_esp + 0x44) = g_xmm0.f[0];
    g_eax = MEM32(g_esp + 0x44); g_edx = g_esi + -12; MEM32(g_edx) = g_eax;
    g_xmm2.f[0] = g_xmm2.f[0] + g_xmm4.f[0]; MEMF(g_esp + 0x48) = g_xmm2.f[0]; g_eax = MEM32(g_esp + 0x48);
    MEM32(g_edx + 4) = g_eax; g_xmm1.f[0] = g_xmm1.f[0] + g_xmm5.f[0]; MEMF(g_esp + 0x4C) = g_xmm1.f[0];
    g_eax = MEM32(g_esp + 0x4C); MEM32(g_edx + 8) = g_eax;
    g_edx=MEM32(g_esp+0x5c);SET_LO8(g_eax,MEM8(g_edx+0x76));int thermal=LO8(g_eax)!=0;
    if(thermal){g_eax=MEM32(g_ecx+0xac);thermal=MEM8(g_eax+12)!=0;}
    if(thermal)g_xmm0=XMM_MEM(g_edx+0x80);
    else{g_edx=MEM32(g_ecx+16);g_eax=MEM16(g_edx+g_ebx+0x1c);PUSH32(g_esp,g_ecx);g_xmm0.f[0]=(float)(int32_t)g_eax;g_xmm0.f[0]*=MEMF(0x482fbc);g_edx=g_esp+0xd4;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_edx);g_ecx+=0xbc;g_ebp=frame;PUSH32(g_esp,0x23a82c);sub_0023A010();g_xmm0=XMM_MEM(g_eax);g_ecx=MEM32(g_esp+0x70);g_edx=MEM32(g_esp+0x5c);}
    g_eax=MEM32(g_esp+0x18);XMM_STORE(g_eax,g_xmm0);g_eax+=16;MEM32(g_esp+0x18)=g_eax;g_eax=MEM32(g_esp+0x1c);g_ebx+=32;g_esi+=48;g_eax--;MEM32(g_esp+0x1c)=g_eax;
   }while(g_eax);
   POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=24;return 1;
  }
 }else if((int32_t)g_eax>0){
  g_esi=MEM32(frame+0x18);g_edi=MEM32(frame+8);MEM32(g_esp+0x18)=g_esi;g_esi=MEM32(frame+0x14);g_edi<<=5;g_esi+=0x24;MEM32(g_esp+0x1c)=g_eax;
  do{
    g_eax = MEM32(g_ecx + 0x10); g_xmm0 = XMM_SCALAR(MEMF(g_edx + 0x9C)); g_xmm3 = XMM_SCALAR(MEMF(g_eax + g_edi));
    g_xmm1 = XMM_SCALAR(MEMF(g_eax + g_edi + 0x10)); g_xmm4 = XMM_SCALAR(MEMF(g_eax + g_edi + 4)); g_xmm2 = XMM_SCALAR(MEMF(g_eax + g_edi + 0x14));
    g_xmm5 = XMM_SCALAR(MEMF(g_eax + g_edi + 8)); g_xmm7 = XMM_SCALAR(MEMF(g_eax + g_edi + 0x18)); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0]; g_xmm7.f[0] = g_xmm7.f[0] * g_xmm0.f[0]; g_eax = g_eax + g_edi;
    g_xmm6 = g_xmm3; g_xmm6.f[0] = g_xmm6.f[0] + g_xmm1.f[0]; g_xmm1 = g_xmm4;
    g_xmm1.f[0] = g_xmm1.f[0] + g_xmm2.f[0]; MEMF(g_esp + 0x94) = g_xmm1.f[0]; g_xmm1.f[0] = g_xmm1.f[0] - g_xmm4.f[0];
    MEMF(g_esp + 0x24) = g_xmm1.f[0]; g_xmm2 = g_xmm5; g_xmm2.f[0] = g_xmm2.f[0] + g_xmm7.f[0];
    MEMF(g_esp + 0x98) = g_xmm2.f[0]; g_xmm2.f[0] = g_xmm2.f[0] - g_xmm5.f[0]; MEMF(g_esp + 0x28) = g_xmm2.f[0];
    MB_PUSH(MEMF(g_esp + 0x28)); MB_TOP() = MB_TOP() * MEMF(g_esp + 0x28); g_xmm0 = g_xmm6;
    MB_PUSH(MEMF(g_esp + 0x24)); g_xmm0.f[0] = g_xmm0.f[0] - g_xmm3.f[0]; MB_TOP() = MB_TOP() * MEMF(g_esp + 0x24);
    MEMF(g_esp + 0x20) = g_xmm0.f[0]; MB_ST1() = MB_ST1() + MB_TOP(); MB_POP(); MB_PUSH(MEMF(g_esp + 0x20));
    MB_TOP() = MB_TOP() * MEMF(g_esp + 0x20); MB_ST1() = MB_ST1() + MB_TOP(); MB_POP(); MB_TOP() = sqrt(MB_TOP());
    MB_TOP() = MEMF(0x44BF94) / MB_TOP(); MEMF(g_esp + 0x6C) = (float)MB_TOP(); MB_POP(); g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x6C));
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm7.f[0]; g_xmm0.f[0] = g_xmm0.f[0] * g_xmm7.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm7.f[0];
    MEMF(g_esp + 0x74) = g_xmm0.f[0]; g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x74)); g_xmm0 = g_xmm1;
    g_xmm1 = g_xmm2; g_xmm1.f[0] = g_xmm1.f[0] * MEMF(g_esp + 0x30); g_xmm2.f[0] = g_xmm2.f[0] * MEMF(g_esp + 0x2C);
    MEMF(g_esp + 0x78) = g_xmm0.f[0]; g_xmm0.f[0] = g_xmm0.f[0] * MEMF(g_esp + 0x34); g_xmm0.f[0] = g_xmm0.f[0] - g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x34)); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm7.f[0]; g_xmm2.f[0] = g_xmm2.f[0] - g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x30)); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm7.f[0]; g_xmm7 = XMM_SCALAR(MEMF(g_eax + 0xC));
    MEMF(g_esp + 0x84) = g_xmm2.f[0]; g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x78)); g_xmm2.f[0] = g_xmm2.f[0] * MEMF(g_esp + 0x2C);
    g_xmm1.f[0] = g_xmm1.f[0] - g_xmm2.f[0]; g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x84)); g_xmm0.f[0] = g_xmm0.f[0] * g_xmm7.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm7.f[0]; g_xmm1.f[0] = g_xmm1.f[0] * g_xmm7.f[0]; g_xmm7 = XMM_SCALAR(MEMF(0x44C1F8));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm7.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm7.f[0]; g_xmm1.f[0] = g_xmm1.f[0] * g_xmm7.f[0];
    g_xmm7 = g_xmm0; g_xmm7.f[0] = g_xmm7.f[0] + g_xmm3.f[0]; MEMF(g_esp + 0x44) = g_xmm7.f[0];
    g_xmm7 = g_xmm2; g_xmm7.f[0] = g_xmm7.f[0] + g_xmm4.f[0]; MEMF(g_esp + 0x48) = g_xmm7.f[0];
    g_xmm7 = g_xmm1; g_ebx = MEM32(g_esp + 0x44); g_eax = g_esi + -36;
    MEM32(g_eax) = g_ebx; g_ebx = MEM32(g_esp + 0x48); MEM32(g_eax + 4) = g_ebx;
    g_xmm7.f[0] = g_xmm7.f[0] + g_xmm5.f[0]; g_xmm3.f[0] = g_xmm3.f[0] - g_xmm0.f[0]; MEMF(g_esp + 0x4C) = g_xmm7.f[0];
    g_ebx = MEM32(g_esp + 0x4C); MEM32(g_eax + 8) = g_ebx; MEMF(g_esp + 0x60) = g_xmm3.f[0];
    g_ebx = MEM32(g_esp + 0x60); g_xmm4.f[0] = g_xmm4.f[0] - g_xmm2.f[0]; MEMF(g_esp + 0x64) = g_xmm4.f[0];
    g_eax = g_esi + -24; MEM32(g_eax) = g_ebx; g_ebx = MEM32(g_esp + 0x64);
    MEM32(g_eax + 4) = g_ebx; g_xmm5.f[0] = g_xmm5.f[0] - g_xmm1.f[0]; MEMF(g_esp + 0x68) = g_xmm5.f[0];
    g_ebx = MEM32(g_esp + 0x68); MEM32(g_eax + 8) = g_ebx; g_xmm3 = g_xmm6;
    g_xmm3.f[0] = g_xmm3.f[0] - g_xmm0.f[0]; MEMF(g_esp + 0x38) = g_xmm3.f[0]; g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x94));
    g_ebx = MEM32(g_esp + 0x38); g_xmm4 = g_xmm3; g_xmm4.f[0] = g_xmm4.f[0] - g_xmm2.f[0];
    MEMF(g_esp + 0x3C) = g_xmm4.f[0]; g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x98)); g_eax = g_esi;
    MEM32(g_eax) = g_ebx; g_ebx = MEM32(g_esp + 0x3C); MEM32(g_eax + 4) = g_ebx;
    g_xmm5 = g_xmm4; g_xmm5.f[0] = g_xmm5.f[0] - g_xmm1.f[0]; MEMF(g_esp + 0x40) = g_xmm5.f[0];
    g_ebx = MEM32(g_esp + 0x40); MEM32(g_eax + 8) = g_ebx; g_xmm0.f[0] = g_xmm0.f[0] + g_xmm6.f[0];
    MEMF(g_esp + 0x50) = g_xmm0.f[0]; g_ebx = MEM32(g_esp + 0x50); g_eax = g_esi + -12;
    MEM32(g_eax) = g_ebx; g_xmm2.f[0] = g_xmm2.f[0] + g_xmm3.f[0]; MEMF(g_esp + 0x54) = g_xmm2.f[0];
    g_ebx = MEM32(g_esp + 0x54); MEM32(g_eax + 4) = g_ebx; g_xmm1.f[0] = g_xmm1.f[0] + g_xmm4.f[0];
    MEMF(g_esp + 0x58) = g_xmm1.f[0]; g_ebx = MEM32(g_esp + 0x58); MEM32(g_eax + 8) = g_ebx;
    SET_LO8(g_eax,MEM8(g_edx+0x76));int thermal=LO8(g_eax)!=0;
    if(thermal){g_eax=MEM32(g_ecx+0xac);SET_LO8(g_ebx,MEM8(g_eax+12));thermal=LO8(g_ebx)!=0;}
    if(thermal)g_xmm0=XMM_MEM(g_edx+0x80);
    else{g_edx=MEM32(g_ecx+16);g_eax=MEM16(g_edx+g_edi+0x1c);PUSH32(g_esp,g_ecx);g_xmm0.f[0]=(float)(int32_t)g_eax;g_xmm0.f[0]*=MEMF(0x482fbc);g_edx=g_esp+0xd4;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_edx);g_ecx+=0xbc;g_ebp=frame;PUSH32(g_esp,0x23ab1c);sub_0023A010();g_xmm0=XMM_MEM(g_eax);g_ecx=MEM32(g_esp+0x70);g_edx=MEM32(g_esp+0x5c);}
    g_eax=MEM32(g_esp+0x18);g_ebx=g_eax;XMM_STORE(g_eax,g_xmm0);g_eax=MEM32(g_esp+0x1c);g_ebx+=16;g_edi+=32;g_esi+=48;g_eax--;MEM32(g_esp+0x18)=g_ebx;MEM32(g_esp+0x1c)=g_eax;
  }while(g_eax);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=24;return 1;
}
#undef MB_PUSH
#undef MB_POP
#undef MB_TOP
#undef MB_ST1
