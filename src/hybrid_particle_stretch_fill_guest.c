#include "hybrid_particle_stretch_fill_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023A010(void),sub_0023CC80(void),sub_0023CD80(void),sub_002D6EB0(void);
static int stretch_fill_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_STRETCH_FILL");flag=v&&!strcmp(v,"1");}return flag;}
static int stretch_fill_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define ST_PUSH(v) do{double value=(v);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;}while(0)
#define ST_POP() (g_fp_top=(g_fp_top+1)&7)
#define ST_TOP() g_fp_stack[g_fp_top]
#define ST_ST1() g_fp_stack[(g_fp_top+1)&7]
/* Preserve all four endpoint calls and their stacked arguments. Source loads and
 * stores remain ordered for caller-owned aliases and exceptional values. */
static void stretch_filler(void){
 uint32_t frame=g_ebp,_sse_flags=0x02u;
   PUSH32(g_esp, frame); frame = g_esp; g_ebp = frame;
   g_esp = g_esp & 0xFFFFFFF0u; g_esp = g_esp - 0x144; PUSH32(g_esp, g_ebx);
   PUSH32(g_esp, g_esi); g_esi = g_ecx; g_eax = MEM32(g_esi + 0x20);
   g_xmm0 = XMM_SCALAR(MEMF(g_eax + 0xFC)); SET_LO8(g_ecx, MEM8(g_eax + 0x9C)); g_eax = g_eax + 0x30;
   MEMF(g_esp + 0xBC) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 0xB0)); MEMF(g_esp + 0xC0) = g_xmm0.f[0];
   g_xmm0 = XMM_SCALAR(MEMF(g_eax + 0xB4)); MEM32(g_esp + 0xB8) = g_eax; g_eax = MEM32(frame + 0xC);
   PUSH32(g_esp, g_edi);
   MEMF(g_esp + 0xC8) = g_xmm0.f[0];
 if(LO8(g_ecx)) {
  if((int32_t)g_eax>0){
   g_ecx = MEM32(frame + 0x18); g_ebx = MEM32(frame + 0x14); g_edi = MEM32(frame + 0x10);
   MEM32(g_esp + 0x10) = g_ecx; g_ecx = MEM32(frame + 8); g_ecx = g_ecx << 5;
   MEM32(g_esp + 0x14) = g_ecx; g_ebx = g_ebx + 0x24; MEM32(g_esp + 0x5C) = g_eax;
   do{
   g_edx = MEM32(g_esi + 0x10); g_eax = g_ecx + g_edx; g_edx = ZX16(MEM16(g_eax + 0x1C));
   g_xmm1.f[0] = (float)(int32_t)g_edx; g_edx = ZX16(MEM16(g_eax + 0x1E)); g_edx = g_edx & 0x8000000Fu;
   g_xmm1.f[0] = g_xmm1.f[0] * MEMF(0x482FBC); MEMF(g_esp + 0x58) = g_xmm1.f[0];
   /* The packed seed is zero-extended, so signed remainder repair is unreachable. */
   g_xmm0 = XMM_SCALAR(MEMF(g_edx * 4 + 0x51CA98)); ST_PUSH(MEMF(g_eax + 0xC)); g_xmm3 = XMM_SCALAR(MEMF(g_esi + 0xC4));
   g_xmm2 = g_xmm0; g_xmm0.f[0] = g_xmm0.f[0] * MEMF(g_esi + 0xC0); g_xmm2.f[0] = g_xmm2.f[0] * MEMF(g_esi + 0xB8);
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esi + 0xBC); g_xmm2.f[0] = g_xmm2.f[0] + MEMF(g_esi + 0xB4); g_xmm0.f[0] = g_xmm0.f[0] * MEMF(0x44D95C);
   g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0]; g_xmm2.f[0] = g_xmm2.f[0] + g_xmm0.f[0];
   MEMF(g_esp + 0x54) = g_xmm2.f[0]; ST_PUSH(MEMF(g_esp + 0x54)); g_xmm1 = XMM_SCALAR(MEMF(g_edi));
   ST_TOP() = sin(ST_TOP()); g_xmm2 = XMM_SCALAR(MEMF(g_edi + 4)); g_xmm3 = XMM_SCALAR(MEMF(g_edi + 8));
   g_xmm5 = XMM_SCALAR(MEMF(g_edi + 0x10)); g_xmm6 = XMM_SCALAR(MEMF(g_edi + 0x14)); g_xmm7 = XMM_SCALAR(MEMF(g_edi + 0x18));
   g_eax = MEM32(g_esi + 0x10); MEMF(g_esp + 0x3C) = g_xmm1.f[0]; MEMF(g_esp + 0x40) = g_xmm2.f[0];
   MEMF(g_esp + 0x44) = g_xmm3.f[0]; MEMF(g_esp + 0x48) = g_xmm5.f[0]; MEMF(g_esp + 0x4C) = g_xmm6.f[0];
   MEMF(g_esp + 0x50) = g_xmm7.f[0]; g_eax = g_eax + g_ecx; ST_TOP() = ST_TOP() * ST_ST1();
   MEMF(g_esp + 0xC) = (float)ST_TOP(); ST_POP(); ST_PUSH(MEMF(g_esp + 0x54)); g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0xC));
   ST_TOP() = cos(ST_TOP()); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];
   g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0]; ST_TOP() = ST_TOP() * ST_ST1(); MEMF(g_esp + 0x84) = (float)ST_TOP(); ST_POP();
   g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x84)); g_xmm5.f[0] = g_xmm5.f[0] * g_xmm4.f[0]; ST_POP();
   g_xmm1.f[0] = g_xmm1.f[0] + g_xmm5.f[0]; g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x48)); g_xmm5.f[0] = g_xmm5.f[0] * g_xmm0.f[0];
   g_xmm6.f[0] = g_xmm6.f[0] * g_xmm4.f[0]; g_xmm2.f[0] = g_xmm2.f[0] + g_xmm6.f[0]; g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0x4C));
   g_xmm6.f[0] = g_xmm6.f[0] * g_xmm0.f[0]; g_xmm7.f[0] = g_xmm7.f[0] * g_xmm4.f[0]; g_xmm3.f[0] = g_xmm3.f[0] + g_xmm7.f[0];
   g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x50)); g_xmm7.f[0] = g_xmm7.f[0] * g_xmm0.f[0]; g_xmm0 = XMM_ZERO();
   g_xmm0.f[0] = g_xmm0.f[0] - g_xmm4.f[0]; MEMF(g_esp + 0x30) = g_xmm1.f[0]; g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x3C));
   MEMF(g_esp + 0x34) = g_xmm2.f[0]; g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x40)); MEMF(g_esp + 0x38) = g_xmm3.f[0];
   g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x44)); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];
   g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0]; g_xmm1.f[0] = g_xmm1.f[0] + g_xmm5.f[0]; MEMF(g_esp + 0x24) = g_xmm1.f[0];
   g_xmm2.f[0] = g_xmm2.f[0] + g_xmm6.f[0]; MEMF(g_esp + 0x28) = g_xmm2.f[0]; g_xmm3.f[0] = g_xmm3.f[0] + g_xmm7.f[0];
   MEMF(g_esp + 0x2C) = g_xmm3.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax)); MEMF(g_esp + 0x100) = g_xmm0.f[0];
   g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4)); MEMF(g_esp + 0x104) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8));
   MEMF(g_esp + 0x108) = g_xmm0.f[0]; g_xmm0 = XMM_ZERO(); MEMF(g_esp + 0x10C) = g_xmm0.f[0];
   g_xmm0 = XMM_MEM(g_esp + 0x100); g_xmm2 = XMM_MEM(g_edi + 0x20); g_xmm3 = XMM_MEM(g_edi + 0x10);
   g_eax = MEM32(g_esp + 0x58); g_xmm1 = g_xmm0; g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0xAA);
   g_xmm2 = XMM_MUL(g_xmm2, g_xmm1); g_xmm1 = g_xmm0; g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0x55);
   g_xmm3 = XMM_MUL(g_xmm3, g_xmm1); g_xmm1 = XMM_MEM(g_edi + 0x30); g_xmm3 = XMM_ADD(g_xmm3, g_xmm2);
   g_xmm2 = g_xmm0; g_xmm2 = XMM_SHUFFLE(g_xmm2, g_xmm0, 0); g_xmm0 = XMM_MEM(g_edi);
   g_xmm0 = XMM_MUL(g_xmm0, g_xmm2); g_xmm0 = XMM_ADD(g_xmm0, g_xmm1); g_xmm0 = XMM_ADD(g_xmm0, g_xmm3);
   PUSH32(g_esp, g_eax); g_ecx = g_esi + 0x12C; XMM_STORE(g_esp + 0xA4, g_xmm0);
   g_ebp = frame; PUSH32(g_esp, 0x0023C1F2u); sub_0023CD80(); ST_PUSH(MEMF(g_esp + 0xC0));
   g_ebp = frame; PUSH32(g_esp, 0x0023C1FEu); sub_002D6EB0(); g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0xC4));
   g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x3C)); g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0xC8)); g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x50));
   g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0]; g_xmm4 = g_xmm0; g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x40));
   g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0]; g_xmm5 = g_xmm0; g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x44));
   g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0]; g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x4C)); g_xmm6 = g_xmm0;
   g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x48)); g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0]; g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
   g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0]; MEMF(g_esp + 0xC) = (float)ST_TOP(); ST_POP(); g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0xC));
   g_xmm0.f[0] = g_xmm0.f[0] + g_xmm4.f[0]; g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0]; MEMF(g_esp + 0x18) = g_xmm0.f[0];
   ST_PUSH(MEMF(g_esp + 0x18)); ST_TOP() = ST_TOP() * MEMF(g_esp + 0x18); g_xmm2.f[0] = g_xmm2.f[0] + g_xmm6.f[0];
   g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0]; MEMF(g_esp + 0x20) = g_xmm2.f[0]; ST_PUSH(MEMF(g_esp + 0x20));
   ST_TOP() = ST_TOP() * MEMF(g_esp + 0x20); g_xmm1.f[0] = g_xmm1.f[0] + g_xmm5.f[0]; g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
   MEMF(g_esp + 0x1C) = g_xmm1.f[0]; ST_ST1() = ST_ST1() + ST_TOP(); ST_POP(); ST_PUSH(MEMF(g_esp + 0x1C));
   g_ecx = g_esp + 0x18; ST_TOP() = ST_TOP() * MEMF(g_esp + 0x1C); PUSH32(g_esp, g_ecx);
   g_edx = g_esp + 0x64; PUSH32(g_esp, g_edx); ST_ST1() = ST_ST1() + ST_TOP(); ST_POP();
   g_eax = g_esp + 0x2C; ST_TOP() = sqrt(ST_TOP()); ST_TOP() = MEMF(0x44BF94) / ST_TOP();
   MEMF(g_esp + 0x14) = (float)ST_TOP(); ST_POP(); g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x14)); g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
   g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0]; MEMF(g_esp + 0x68) = g_xmm0.f[0];
   MEMF(g_esp + 0x6C) = g_xmm1.f[0]; MEMF(g_esp + 0x70) = g_xmm2.f[0]; PUSH32(g_esp, g_eax);
   g_ecx = g_esp + 0x3C; PUSH32(g_esp, g_ecx); g_edx = g_esp + 0x104;
   PUSH32(g_esp, g_edx); g_ebp = frame; PUSH32(g_esp, 0x0023C2F6u); sub_0023CC80();
   g_xmm0 = XMM_SCALAR(MEMF(g_eax)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xB4); MEMF(g_esp + 0x8C) = g_xmm0.f[0];
   g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xB8); g_ecx = MEM32(g_esp + 0x8C);
   MEMF(g_esp + 0x90) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xBC);
   g_edx = MEM32(g_esp + 0x90); g_eax = g_ebx + -36; MEM32(g_eax) = g_ecx;
   MEM32(g_eax + 4) = g_edx; MEMF(g_esp + 0x94) = g_xmm0.f[0]; g_ecx = MEM32(g_esp + 0x94);
   g_xmm0 = XMM_ZERO(); MEM32(g_eax + 8) = g_ecx; g_edx = g_esp + 0x2C;
   PUSH32(g_esp, g_edx); g_eax = g_esp + 0x78; PUSH32(g_esp, g_eax);
   g_ecx = g_esp + 0xCC; g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x40);
   PUSH32(g_esp, g_ecx); g_edx = g_esp + 0x50; MEMF(g_esp + 0xD0) = g_xmm1.f[0];
   g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x48); g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esp + 0x4C);
   PUSH32(g_esp, g_edx); g_eax = g_esp + 0x158; PUSH32(g_esp, g_eax);
   MEMF(g_esp + 0xDC) = g_xmm1.f[0]; MEMF(g_esp + 0xE0) = g_xmm0.f[0]; g_ebp = frame;
   PUSH32(g_esp, 0x0023C3B4u); sub_0023CC80(); g_xmm0 = XMM_SCALAR(MEMF(g_eax)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xC8);
   MEMF(g_esp + 0xBC) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xCC);
   g_edx = MEM32(g_esp + 0xBC); MEMF(g_esp + 0xC0) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8));
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xD0); g_eax = MEM32(g_esp + 0xC0); MEMF(g_esp + 0xC4) = g_xmm0.f[0];
   g_xmm0 = XMM_ZERO(); g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x4C);
   MEMF(g_esp + 0x10C) = g_xmm1.f[0]; g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x50);
   MEMF(g_esp + 0x110) = g_xmm1.f[0]; g_ecx = g_ebx + -24; MEM32(g_ecx) = g_edx;
   g_edx = MEM32(g_esp + 0xC4); g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x54);
   MEMF(g_esp + 0x114) = g_xmm1.f[0]; MEM32(g_ecx + 4) = g_eax; g_xmm1 = g_xmm0;
   g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x58); MEM32(g_ecx + 8) = g_edx; MEMF(g_esp + 0x100) = g_xmm1.f[0];
   g_eax = g_esp + 0x40; g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x5C);
   g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esp + 0x60); PUSH32(g_esp, g_eax); g_ecx = g_esp + 0x8C;
   MEMF(g_esp + 0x108) = g_xmm1.f[0]; MEMF(g_esp + 0x10C) = g_xmm0.f[0]; PUSH32(g_esp, g_ecx);
   g_edx = g_esp + 0x114; PUSH32(g_esp, g_edx); g_eax = g_esp + 0x10C;
   PUSH32(g_esp, g_eax); g_ecx = g_esp + 0x154; PUSH32(g_esp, g_ecx);
   g_ebp = frame; PUSH32(g_esp, 0x0023C4AEu); sub_0023CC80(); g_xmm0 = XMM_SCALAR(MEMF(g_eax));
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xDC); MEMF(g_esp + 0xA8) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4));
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xE0); MEMF(g_esp + 0xAC) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8));
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xE4); g_eax = MEM32(g_esp + 0xA8); g_ecx = MEM32(g_esp + 0xAC);
   MEMF(g_esp + 0xB0) = g_xmm0.f[0]; g_xmm0 = XMM_ZERO(); g_edx = g_ebx;
   MEM32(g_edx) = g_eax; g_eax = MEM32(g_esp + 0xB0); MEM32(g_edx + 4) = g_ecx;
   MEM32(g_edx + 8) = g_eax; g_ecx = g_esp + 0x54; PUSH32(g_esp, g_ecx);
   g_edx = g_esp + 0xA0; PUSH32(g_esp, g_edx); g_eax = g_esp + 0x68;
   g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x74); PUSH32(g_esp, g_eax);
   g_ecx = g_esp + 0x114; MEMF(g_esp + 0x114) = g_xmm1.f[0]; g_xmm1 = g_xmm0;
   g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x7C); g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esp + 0x80); PUSH32(g_esp, g_ecx);
   g_edx = g_esp + 0x174; PUSH32(g_esp, g_edx); MEMF(g_esp + 0x120) = g_xmm1.f[0];
   MEMF(g_esp + 0x124) = g_xmm0.f[0]; g_ebp = frame; PUSH32(g_esp, 0x0023C571u); sub_0023CC80();
   g_xmm0 = XMM_SCALAR(MEMF(g_eax)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xF0); MEMF(g_esp + 0xD8) = g_xmm0.f[0];
   g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xF4); g_ecx = MEM32(g_esp + 0xD8);
   MEMF(g_esp + 0xDC) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0xF8);
   g_edx = MEM32(g_esp + 0xDC); g_eax = g_ebx + -12; MEM32(g_eax) = g_ecx;
   MEM32(g_eax + 4) = g_edx; MEMF(g_esp + 0xE0) = g_xmm0.f[0]; g_ecx = MEM32(g_esp + 0xE0);
   MEM32(g_eax + 8) = g_ecx; g_eax = MEM32(g_esp + 0x10C); SET_LO8(g_ecx, MEM8(g_eax + 0x76));
   g_esp = g_esp + 0x50;
   int thermal=LO8(g_ecx)!=0;if(thermal){g_edx=MEM32(g_esi+0xac);SET_LO8(g_ecx,MEM8(g_edx+12));thermal=LO8(g_ecx)!=0;}
   if(thermal)g_xmm0=XMM_MEM(g_eax+0x80);else{
   g_ecx = MEM32(g_esp + 0x58); PUSH32(g_esp, g_ecx); g_edx = g_esp + 0x144;
   PUSH32(g_esp, g_edx); g_ecx = g_esi + 0xCC; g_ebp = frame;
   PUSH32(g_esp, 0x0023C614u); sub_0023A010(); g_xmm0 = XMM_MEM(g_eax);
   }
   g_eax = MEM32(g_esp + 0x10); g_ecx = MEM32(g_esp + 0x10); XMM_STORE(g_eax, g_xmm0);
   MEM32(g_esp + 0x14) = MEM32(g_esp + 0x14) + 0x20; g_eax = MEM32(g_esp + 0x5C); g_ecx = g_ecx + 0x10;
   g_ebx = g_ebx + 0x30;
   g_eax--;
   MEM32(g_esp + 0x10) = g_ecx; MEM32(g_esp + 0x5C) = g_eax;
   if(!g_eax)break;g_ecx=MEM32(g_esp+0x14);
   }while(1);
   POP32(g_esp, g_edi); POP32(g_esp, g_esi); POP32(g_esp, g_ebx);
   g_esp = frame; POP32(g_esp, frame);
   g_recomp_zf=1;g_esp+=24;return;
  }
 }else if((int32_t)g_eax>0){
   g_ecx = MEM32(frame + 0x18); g_ebx = MEM32(frame + 0x14); MEM32(g_esp + 0x10) = g_ecx;
   g_ecx = MEM32(frame + 8); g_ecx = g_ecx << 5; MEM32(g_esp + 0x14) = g_ecx;
   g_ebx = g_ebx + 0x24; MEM32(g_esp + 0x54) = g_eax;
  do{
   g_edx = MEM32(g_esi + 0x10); g_eax = g_ecx + g_edx; g_edx = ZX16(MEM16(g_eax + 0x1C));
   g_xmm0.f[0] = (float)(int32_t)g_edx; g_edx = ZX16(MEM16(g_eax + 0x1E)); g_edx = g_edx & 0x8000000Fu;
   g_xmm0.f[0] = g_xmm0.f[0] * MEMF(0x482FBC); MEMF(g_esp + 0x58) = g_xmm0.f[0];
   g_xmm0.f[0] = g_xmm0.f[0] * MEMF(g_esi + 0xC4); ST_PUSH(MEMF(g_eax + 0xC)); g_xmm1 = XMM_SCALAR(MEMF(g_edx * 4 + 0x51CA98));
   g_xmm2 = XMM_SCALAR(MEMF(g_esi + 0xB8)); g_xmm2.f[0] = g_xmm2.f[0] * g_xmm1.f[0]; g_xmm2.f[0] = g_xmm2.f[0] + MEMF(g_esi + 0xB4);
   g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_esi + 0xC0)); g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0];
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esi + 0xBC); g_xmm0.f[0] = g_xmm0.f[0] * MEMF(0x44D95C); g_xmm2.f[0] = g_xmm2.f[0] + g_xmm0.f[0];
   MEMF(g_esp + 0x5C) = g_xmm2.f[0]; ST_PUSH(MEMF(g_esp + 0x5C)); g_eax = MEM32(frame + 0x10);
   ST_TOP() = sin(ST_TOP()); g_xmm1 = XMM_SCALAR(MEMF(g_eax)); g_xmm2 = XMM_SCALAR(MEMF(g_eax + 4));
   g_xmm3 = XMM_SCALAR(MEMF(g_eax + 8)); g_xmm5 = XMM_SCALAR(MEMF(g_eax + 0x10)); g_xmm6 = XMM_SCALAR(MEMF(g_eax + 0x14));
   g_xmm7 = XMM_SCALAR(MEMF(g_eax + 0x18)); g_edi = MEM32(g_esi + 0x10); g_eax = MEM32(g_esp + 0x58);
   MEMF(g_esp + 0x48) = g_xmm1.f[0]; MEMF(g_esp + 0x4C) = g_xmm2.f[0]; MEMF(g_esp + 0x50) = g_xmm3.f[0];
   MEMF(g_esp + 0x60) = g_xmm5.f[0]; MEMF(g_esp + 0x64) = g_xmm6.f[0]; MEMF(g_esp + 0x68) = g_xmm7.f[0];
   g_edi = g_edi + g_ecx; PUSH32(g_esp, g_eax); g_ecx = g_esi + 0x12C;
   ST_TOP() = ST_TOP() * ST_ST1(); MEMF(g_esp + 0x88) = (float)ST_TOP(); ST_POP(); ST_PUSH(MEMF(g_esp + 0x60));
   g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x88)); ST_TOP() = cos(ST_TOP()); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
   g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0]; g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0]; ST_TOP() = ST_TOP() * ST_ST1();
   MEMF(g_esp + 0x10) = (float)ST_TOP(); ST_POP(); g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x10)); g_xmm5.f[0] = g_xmm5.f[0] * g_xmm4.f[0];
   ST_POP(); g_xmm1.f[0] = g_xmm1.f[0] + g_xmm5.f[0]; g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x64));
   g_xmm6.f[0] = g_xmm6.f[0] * g_xmm4.f[0]; g_xmm2.f[0] = g_xmm2.f[0] + g_xmm6.f[0]; g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0x68));
   g_xmm7.f[0] = g_xmm7.f[0] * g_xmm4.f[0]; g_xmm3.f[0] = g_xmm3.f[0] + g_xmm7.f[0]; g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0x6C));
   g_xmm5.f[0] = g_xmm5.f[0] * g_xmm0.f[0]; g_xmm6.f[0] = g_xmm6.f[0] * g_xmm0.f[0]; g_xmm7.f[0] = g_xmm7.f[0] * g_xmm0.f[0];
   g_xmm0 = XMM_ZERO(); MEMF(g_esp + 0x28) = g_xmm1.f[0]; g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x4C));
   MEMF(g_esp + 0x2C) = g_xmm2.f[0]; g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x50)); MEMF(g_esp + 0x30) = g_xmm3.f[0];
   g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x54)); g_xmm0.f[0] = g_xmm0.f[0] - g_xmm4.f[0]; g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
   g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0]; g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0]; g_xmm1.f[0] = g_xmm1.f[0] + g_xmm5.f[0];
   g_xmm2.f[0] = g_xmm2.f[0] + g_xmm6.f[0]; g_xmm3.f[0] = g_xmm3.f[0] + g_xmm7.f[0]; MEMF(g_esp + 0x34) = g_xmm1.f[0];
   MEMF(g_esp + 0x38) = g_xmm2.f[0]; MEMF(g_esp + 0x3C) = g_xmm3.f[0]; g_ebp = frame;
   PUSH32(g_esp, 0x0023C80Du); sub_0023CD80(); ST_PUSH(MEMF(g_esp + 0xC0)); g_ebp = frame;
   PUSH32(g_esp, 0x0023C819u); sub_002D6EB0(); g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0xC4)); MEMF(g_esp + 0xC) = (float)ST_TOP(); ST_POP();
   g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x48)); g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0xC8)); g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x68));
   g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0]; g_xmm4 = g_xmm0; g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x4C));
   g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0]; g_xmm5 = g_xmm0; g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x50));
   g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0]; g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x64)); g_xmm6 = g_xmm0;
   g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x60)); g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
   g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0]; g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0xC)); g_xmm0.f[0] = g_xmm0.f[0] + g_xmm4.f[0];
   g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0]; g_xmm2.f[0] = g_xmm2.f[0] + g_xmm6.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
   g_xmm1.f[0] = g_xmm1.f[0] + g_xmm5.f[0]; g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0]; g_xmm3 = XMM_ZERO();
   MEMF(g_esp + 0x3C) = g_xmm3.f[0]; MEMF(g_esp + 0x40) = g_xmm3.f[0]; MEMF(g_esp + 0x44) = g_xmm3.f[0];
   g_xmm3 = g_xmm2; MEMF(g_esp + 0x18) = g_xmm0.f[0]; g_xmm3.f[0] = g_xmm3.f[0] * g_xmm2.f[0];
   MEMF(g_esp + 0x20) = g_xmm2.f[0]; g_xmm0.f[0] = g_xmm0.f[0] * g_xmm0.f[0]; g_xmm2 = g_xmm1;
   g_xmm0.f[0] = g_xmm0.f[0] + g_xmm3.f[0]; g_xmm2.f[0] = g_xmm2.f[0] * g_xmm1.f[0]; g_xmm0.f[0] = g_xmm0.f[0] + g_xmm2.f[0];
   { double _sse_a = g_xmm0.f[0]; double _sse_b = MEMF(0x44C728); _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
   _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u; } MEMF(g_esp + 0x1C) = g_xmm1.f[0];
   MEMF(g_esp + 0xF0) = g_xmm0.f[0];
   if((_sse_flags&0x41u)==0){
   ST_PUSH(MEMF(g_esp + 0xF0)); ST_TOP() = sqrt(ST_TOP()); ST_TOP() = MEMF(0x44BF94) / ST_TOP();
   ST_PUSH(MEMF(g_esp + 0x18)); ST_TOP() = ST_TOP() * ST_ST1(); MEMF(g_esp + 0x88) = (float)ST_TOP(); ST_POP();
   g_ecx = MEM32(g_esp + 0x88); ST_PUSH(MEMF(g_esp + 0x1C)); MEM32(g_esp + 0x3C) = g_ecx;
   ST_TOP() = ST_TOP() * ST_ST1(); MEMF(g_esp + 0x8C) = (float)ST_TOP(); ST_POP(); g_edx = MEM32(g_esp + 0x8C);
   ST_PUSH(MEMF(g_esp + 0x20)); MEM32(g_esp + 0x40) = g_edx; ST_TOP() = ST_TOP() * ST_ST1();
   MEMF(g_esp + 0x90) = (float)ST_TOP(); ST_POP(); g_eax = MEM32(g_esp + 0x90); MEM32(g_esp + 0x44) = g_eax;
   ST_POP();
   }
   g_ecx = g_esp + 0x18; PUSH32(g_esp, g_ecx); g_edx = g_esp + 0x40;
   PUSH32(g_esp, g_edx); g_eax = g_esp + 0x38; PUSH32(g_esp, g_eax);
   g_ecx = g_esp + 0x30; PUSH32(g_esp, g_ecx); g_edx = g_esp + 0x138;
   PUSH32(g_esp, g_edx); g_ebp = frame; PUSH32(g_esp, 0x0023C95Fu); sub_0023CC80();
   g_xmm0 = XMM_SCALAR(MEMF(g_eax)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi); MEMF(g_esp + 0x80) = g_xmm0.f[0];
   g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi + 4); g_ecx = MEM32(g_esp + 0x80);
   MEMF(g_esp + 0x84) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi + 8);
   g_edx = MEM32(g_esp + 0x84); g_eax = g_ebx + -36; MEM32(g_eax) = g_ecx;
   MEM32(g_eax + 4) = g_edx; MEMF(g_esp + 0x88) = g_xmm0.f[0]; g_ecx = MEM32(g_esp + 0x88);
   g_xmm0 = XMM_ZERO(); MEM32(g_eax + 8) = g_ecx; g_edx = g_esp + 0x2C;
   PUSH32(g_esp, g_edx); g_eax = g_esp + 0x54; PUSH32(g_esp, g_eax);
   g_ecx = g_esp + 0xE8; g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x4C);
   PUSH32(g_esp, g_ecx); g_edx = g_esp + 0x44; MEMF(g_esp + 0xEC) = g_xmm1.f[0];
   g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x54); g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esp + 0x58);
   PUSH32(g_esp, g_edx); g_eax = g_esp + 0x140; PUSH32(g_esp, g_eax);
   MEMF(g_esp + 0xF8) = g_xmm1.f[0]; MEMF(g_esp + 0xFC) = g_xmm0.f[0]; g_ebp = frame;
   PUSH32(g_esp, 0x0023CA10u); sub_0023CC80(); g_xmm0 = XMM_SCALAR(MEMF(g_eax)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi);
   MEMF(g_esp + 0xBC) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi + 4);
   g_edx = MEM32(g_esp + 0xBC); MEMF(g_esp + 0xC0) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8));
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi + 8); g_eax = MEM32(g_esp + 0xC0); MEMF(g_esp + 0xC4) = g_xmm0.f[0];
   g_xmm0 = XMM_ZERO(); g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x58);
   MEMF(g_esp + 0x100) = g_xmm1.f[0]; g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x5C);
   MEMF(g_esp + 0x104) = g_xmm1.f[0]; g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x60);
   MEMF(g_esp + 0x108) = g_xmm1.f[0]; g_ecx = g_ebx + -24; MEM32(g_ecx) = g_edx;
   g_edx = MEM32(g_esp + 0xC4); g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x4C);
   MEMF(g_esp + 0x10C) = g_xmm1.f[0]; g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x50);
   g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esp + 0x54); MEM32(g_ecx + 4) = g_eax; MEM32(g_ecx + 8) = g_edx;
   MEMF(g_esp + 0x110) = g_xmm1.f[0]; MEMF(g_esp + 0x114) = g_xmm0.f[0]; g_eax = g_esp + 0x40;
   PUSH32(g_esp, g_eax); g_ecx = g_esp + 0x68; PUSH32(g_esp, g_ecx);
   g_edx = g_esp + 0x108; PUSH32(g_esp, g_edx); g_eax = g_esp + 0x118;
   PUSH32(g_esp, g_eax); g_ecx = g_esp + 0x16C; PUSH32(g_esp, g_ecx);
   g_ebp = frame; PUSH32(g_esp, 0x0023CAFAu); sub_0023CC80(); g_xmm0 = XMM_SCALAR(MEMF(g_eax));
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi); MEMF(g_esp + 0xB4) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4));
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi + 4); MEMF(g_esp + 0xB8) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8));
   g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi + 8); g_eax = MEM32(g_esp + 0xB4); g_ecx = MEM32(g_esp + 0xB8);
   MEMF(g_esp + 0xBC) = g_xmm0.f[0]; g_xmm0 = XMM_ZERO(); g_edx = g_ebx;
   MEM32(g_edx) = g_eax; g_eax = MEM32(g_esp + 0xBC); MEM32(g_edx + 4) = g_ecx;
   MEM32(g_edx + 8) = g_eax; g_ecx = g_esp + 0x54; PUSH32(g_esp, g_ecx);
   g_edx = g_esp + 0x7C; PUSH32(g_esp, g_edx); g_eax = g_esp + 0x74;
   g_xmm1 = g_xmm0; g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x68); PUSH32(g_esp, g_eax);
   g_ecx = g_esp + 0x13C; MEMF(g_esp + 0x13C) = g_xmm1.f[0]; g_xmm1 = g_xmm0;
   g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_esp + 0x70); g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_esp + 0x74); PUSH32(g_esp, g_ecx);
   g_edx = g_esp + 0xEC; PUSH32(g_esp, g_edx); MEMF(g_esp + 0x148) = g_xmm1.f[0];
   MEMF(g_esp + 0x14C) = g_xmm0.f[0]; g_ebp = frame; PUSH32(g_esp, 0x0023CBAAu); sub_0023CC80();
   g_xmm0 = XMM_SCALAR(MEMF(g_eax)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi); MEMF(g_esp + 0x100) = g_xmm0.f[0];
   g_xmm0 = XMM_SCALAR(MEMF(g_eax + 4)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi + 4); g_ecx = MEM32(g_esp + 0x100);
   MEMF(g_esp + 0x104) = g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8)); g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_edi + 8);
   g_edx = MEM32(g_esp + 0x104); g_eax = g_ebx + -12; MEM32(g_eax) = g_ecx;
   MEM32(g_eax + 4) = g_edx; MEMF(g_esp + 0x108) = g_xmm0.f[0]; g_ecx = MEM32(g_esp + 0x108);
   MEM32(g_eax + 8) = g_ecx; g_eax = MEM32(g_esp + 0x10C); SET_LO8(g_ecx, MEM8(g_eax + 0x76));
   g_esp = g_esp + 0x50;
   int thermal=LO8(g_ecx)!=0;if(thermal){g_edx=MEM32(g_esi+0xac);SET_LO8(g_ecx,MEM8(g_edx+12));thermal=LO8(g_ecx)!=0;}
   if(thermal)g_xmm0=XMM_MEM(g_eax+0x80);else{
   g_ecx = MEM32(g_esp + 0x58); PUSH32(g_esp, g_ecx); g_edx = g_esp + 0x144;
   PUSH32(g_esp, g_edx); g_ecx = g_esi + 0xCC; g_ebp = frame;
   PUSH32(g_esp, 0x0023CC40u); sub_0023A010(); g_xmm0 = XMM_MEM(g_eax);
   }
   g_eax = MEM32(g_esp + 0x10); g_edi = MEM32(g_esp + 0x14); g_ecx = g_eax;
   XMM_STORE(g_eax, g_xmm0); g_eax = MEM32(g_esp + 0x54); g_edi = g_edi + 0x20;
   g_ecx = g_ecx + 0x10; g_ebx = g_ebx + 0x30;
   g_eax--;
   MEM32(g_esp + 0x14) = g_edi; MEM32(g_esp + 0x10) = g_ecx; MEM32(g_esp + 0x54) = g_eax;
   if(!g_eax)break;g_ecx=MEM32(g_esp+0x14);
  }while(1);
 }
   POP32(g_esp, g_edi); POP32(g_esp, g_esi); POP32(g_esp, g_ebx);
   g_esp = frame; POP32(g_esp, frame);
 g_esp+=24;
}
static void stretch_project(void){
 uint32_t _sse_flags=0x02u;
   g_esp = g_esp - 0xC; g_eax = MEM32(g_esp + 0x14); g_ecx = MEM32(g_esp + 0x18);
   g_xmm6 = XMM_SCALAR(MEMF(g_eax + 8)); g_xmm6.f[0] = g_xmm6.f[0] + MEMF(g_ecx + 8); g_xmm4 = XMM_SCALAR(MEMF(g_eax));
   g_xmm4.f[0] = g_xmm4.f[0] + MEMF(g_ecx); g_xmm5 = XMM_SCALAR(MEMF(g_eax + 4)); g_xmm5.f[0] = g_xmm5.f[0] + MEMF(g_ecx + 4);
   MEMF(g_esp + 8) = g_xmm6.f[0]; ST_PUSH(MEMF(g_esp + 8)); ST_TOP() = ST_TOP() * MEMF(g_esp + 8);
   MEMF(g_esp) = g_xmm4.f[0]; ST_PUSH(MEMF(g_esp)); MEMF(g_esp + 4) = g_xmm5.f[0];
   ST_TOP() = ST_TOP() * MEMF(g_esp); g_eax = MEM32(g_esp + 0x1C); g_xmm1 = g_xmm4;
   ST_ST1() = ST_ST1() + ST_TOP(); ST_POP(); ST_PUSH(MEMF(g_esp + 4)); g_xmm2 = g_xmm5;
   ST_TOP() = ST_TOP() * MEMF(g_esp + 4); g_xmm3 = g_xmm6; ST_ST1() = ST_ST1() + ST_TOP(); ST_POP();
   ST_TOP() = sqrt(ST_TOP()); ST_TOP() = MEMF(0x44BF94) / ST_TOP(); MEMF(g_esp + 0x14) = (float)ST_TOP(); ST_POP();
   g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x14)); g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0]; g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
   g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0]; g_xmm0 = XMM_SCALAR(MEMF(g_eax + 8)); g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
   g_xmm3 = XMM_SCALAR(MEMF(g_eax + 4)); g_xmm3.f[0] = g_xmm3.f[0] * g_xmm2.f[0]; g_xmm2 = XMM_SCALAR(MEMF(g_eax));
   g_xmm2.f[0] = g_xmm2.f[0] * g_xmm1.f[0]; g_xmm1 = XMM_ZERO(); g_xmm0.f[0] = g_xmm0.f[0] + g_xmm3.f[0];
   g_xmm0.f[0] = g_xmm0.f[0] + g_xmm2.f[0]; { double _sse_a = g_xmm1.f[0]; double _sse_b = g_xmm0.f[0];
   _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u : _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u; }
 if((_sse_flags&0x41u)==0)g_xmm0=g_xmm1;else{
   g_xmm1 = XMM_SCALAR(MEMF(0x44BF94)); { double _sse_a = g_xmm0.f[0]; double _sse_b = g_xmm1.f[0];
   _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u : _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u; }
  if((_sse_flags&0x41u)==0)g_xmm0=g_xmm1;
 }
   g_eax = MEM32(g_esp + 0x20); g_xmm1 = XMM_SCALAR(MEMF(g_eax)); g_xmm2 = XMM_SCALAR(MEMF(g_eax + 4));
   g_xmm3 = XMM_SCALAR(MEMF(g_eax + 8)); g_eax = MEM32(g_esp + 0x10); g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
   g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0]; g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0]; g_xmm1.f[0] = g_xmm1.f[0] + g_xmm4.f[0];
   g_xmm2.f[0] = g_xmm2.f[0] + g_xmm5.f[0]; g_xmm3.f[0] = g_xmm3.f[0] + g_xmm6.f[0]; MEMF(g_eax) = g_xmm1.f[0];
   MEMF(g_eax + 4) = g_xmm2.f[0]; MEMF(g_eax + 8) = g_xmm3.f[0]; g_esp = g_esp + 0xC;
   g_recomp_zf = !!((g_esp == 0));
 g_esp+=4;
}
static void stretch_curve(void){
 uint32_t _sse_flags=0x02u;
   g_xmm1 = XMM_ZERO(); g_xmm0 = XMM_SCALAR(MEMF(g_esp + 4)); { double _sse_a = g_xmm1.f[0];
   double _sse_b = g_xmm0.f[0]; _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u : _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u;
   }
 if((_sse_flags&1u)==0)ST_PUSH(MEMF(g_ecx));else{
   { double _sse_a = g_xmm0.f[0]; double _sse_b = MEMF(0x44BF94); _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
   _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u; }
  if((_sse_flags&1u)==0)ST_PUSH(MEMF(g_ecx+8));else{
   g_xmm1 = XMM_SCALAR(MEMF(g_ecx + 0x14)); { double _sse_a = g_xmm1.f[0]; double _sse_b = g_xmm0.f[0];
   _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u : _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u; }
   if((_sse_flags&0x41u)==0){ST_PUSH(MEMF(g_esp+4));ST_TOP()=ST_TOP()*MEMF(g_ecx+12);ST_TOP()=ST_TOP()+MEMF(g_ecx);}
   else{
   { double _sse_a = g_xmm0.f[0]; double _sse_b = MEMF(g_ecx + 0x18); _sse_flags = (_sse_a != _sse_a || _sse_b != _sse_b) ? 0x47u :
   _sse_a < _sse_b ? 0x03u : _sse_a > _sse_b ? 0x02u : 0x42u; }
    if((_sse_flags&0x41u)==0){ST_PUSH(MEMF(g_esp+4));ST_TOP()=ST_TOP()-MEMF(g_ecx+24);ST_TOP()=ST_TOP()*MEMF(g_ecx+16);ST_TOP()=ST_TOP()+MEMF(g_ecx+4);}
    else ST_PUSH(MEMF(g_ecx+4));
   }
  }
 }
 g_recomp_zf=(_sse_flags&0x40u)!=0;g_esp+=8;
}
int uc_hybrid_particle_stretch_fill(unsigned operation){
 if(!stretch_fill_enabled()||operation>2)return 0;
 uint32_t start=g_esp;
 if((start&3)||start<0x10200u||!stretch_fill_span(start-512,540)||!stretch_fill_span(0x51c178,1)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7)return 0;
 if(operation!=1&&!stretch_fill_span(g_ecx,operation==2?28:0x148))return 0;
 if(operation==0)stretch_filler();else if(operation==1)stretch_project();else stretch_curve();return 1;
}
#undef ST_PUSH
#undef ST_POP
#undef ST_TOP
#undef ST_ST1
