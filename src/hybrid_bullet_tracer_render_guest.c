#include "hybrid_bullet_tracer_render_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0006A370(void);
static int tracer_render_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_BULLET_TRACER_RENDER");enabled=v&&!strcmp(v,"1");}return enabled;}
static int tracer_render_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define TR_PUSH(v) do{double tracer_value=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=tracer_value;}while(0)
#define TR_POP() (g_fp_top=(g_fp_top+1u)&7u)
#define TR_TOP() g_fp_stack[g_fp_top]
#define TR_NEXT() g_fp_stack[(g_fp_top+1)&7u]

static void tracer_prepare(void){
    g_eax = MEM32(g_ecx + 0xC);
    g_edx = MEM32(g_esp + 4);
    MEM32(g_edx) = g_eax;
    g_eax = MEM32(g_esp + 8);
    g_edx = MEM32(g_esp + 0xC);
    MEM32(g_eax) = 2;
    g_ecx = MEM32(g_ecx + 0x41C);
    MEM32(g_edx) = g_ecx;
    g_esp += 16; return;
}

/* Preserve the source read/write interleaving and skipped padding. */
static void tracer_copy(void){
    g_eax = g_ecx;
    g_ecx = MEM32(g_esp + 4);
    g_edx = MEM32(g_ecx);
    MEM32(g_eax) = g_edx;
    g_edx = MEM32(g_ecx + 4);
    MEM32(g_eax + 4) = g_edx;
    g_edx = MEM32(g_ecx + 8);
    MEM32(g_eax + 8) = g_edx;
    g_edx = MEM32(g_ecx + 0xC);
    MEM32(g_eax + 0xC) = g_edx;
    g_edx = MEM32(g_ecx + 0x10);
    MEM32(g_eax + 0x10) = g_edx;
    g_edx = MEM32(g_ecx + 0x14);
    MEM32(g_eax + 0x14) = g_edx;
    g_xmm0 = XMM_MEM(g_ecx + 0x20);
    XMM_STORE(g_eax + 0x20, g_xmm0);
    g_edx = MEM32(g_ecx + 0x30);
    MEM32(g_eax + 0x30) = g_edx;
    g_edx = MEM32(g_ecx + 0x34);
    MEM32(g_eax + 0x34) = g_edx;
    g_ecx = MEM32(g_ecx + 0x38);
    MEM32(g_eax + 0x38) = g_ecx;
    g_esp += 8; return;
}

static void tracer_fill(void){
 uint32_t frame=g_ebp;
    PUSH32(g_esp, frame);
    frame = g_esp;
    g_ebp = frame;
    g_esp = g_esp & 0xFFFFFFF0u;
    g_esp = g_esp - 0xD4;
    g_xmm0 = XMM_SCALAR(MEMF(0x44C1F8));
    g_eax = MEM32(frame + 0xC);
    g_xmm1 = XMM_SCALAR(MEMF(0x44BF94));
    PUSH32(g_esp, g_ebx);
    PUSH32(g_esp, g_esi);
    MEMF(g_esp + 0x3C) = g_xmm0.f[0];
    g_xmm0 = XMM_ZERO();
    PUSH32(g_esp, g_edi);
    MEMF(g_esp + 0x3C) = g_xmm1.f[0];
    MEMF(g_esp + 0x44) = g_xmm0.f[0];
    MEMF(g_esp + 0x48) = g_xmm0.f[0];
    MEMF(g_esp + 0x4C) = g_xmm0.f[0];
    MEMF(g_esp + 0x50) = g_xmm1.f[0];
 if((int32_t)g_eax>0){
    g_edi = MEM32(frame + 0x14);
    g_esi = MEM32(frame + 0x18);
    g_ebx = MEM32(frame + 0x1C);
    g_edx = MEM32(frame + 8);
    g_edi = g_edi + 0x18;
    g_esi = g_esi + 0x10;
    g_ebx = g_ebx + 0x20;
    g_edx = g_edx << 6;
    g_ecx = g_edx + g_ecx + 0x1C;
    MEM32(g_esp + 0x18) = g_ecx;
    MEM32(g_esp + 0x1C) = g_eax;
  do{
    g_edx = MEM32(g_esp + 0x18);
    PUSH32(g_esp, g_edx);
    g_ecx = g_esp + 0x94;
    g_ebp = frame;
    PUSH32(g_esp, 0x0006A091u); sub_0006A370();
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0xC4));
    g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x9C));
    g_xmm0 = XMM_SCALAR(MEMF(0x44BF94));
    g_xmm6 = XMM_SCALAR(MEMF(g_esp + 0xA0));
    g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0xA4));
    g_xmm7.f[0] = g_xmm7.f[0] - MEMF(g_esp + 0x98);
    g_eax = MEM32(frame + 0x10);
    g_xmm2 = g_xmm1;
    g_xmm2.f[0] = g_xmm2.f[0] / MEMF(g_esp + 0xC8);
    g_xmm0.f[0] = g_xmm0.f[0] - g_xmm2.f[0];
    g_xmm5 = g_xmm4;
    g_xmm5.f[0] = g_xmm5.f[0] - MEMF(g_esp + 0x90);
    g_xmm2 = g_xmm5;
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];
    MEMF(g_esp + 0x78) = g_xmm2.f[0];
    g_xmm3 = g_xmm6;
    g_xmm3.f[0] = g_xmm3.f[0] - MEMF(g_esp + 0x94);
    g_xmm2 = g_xmm3;
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];
    MEMF(g_esp + 0x7C) = g_xmm2.f[0];
    g_xmm2 = g_xmm7;
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];
    g_xmm0 = g_xmm2;
    g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0x98);
    g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x78));
    g_xmm2.f[0] = g_xmm2.f[0] + MEMF(g_esp + 0x90);
    MEMF(g_esp + 0x68) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_eax + 0x28));
    MEMF(g_esp + 0x60) = g_xmm2.f[0];
    g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x7C));
    g_xmm2.f[0] = g_xmm2.f[0] + MEMF(g_esp + 0x94);
    MEMF(g_esp + 0x6C) = g_xmm5.f[0];
    MEMF(g_esp + 0xDC) = g_xmm0.f[0];
    MEMF(g_esp + 0x84) = g_xmm5.f[0];
    g_xmm0 = g_xmm3;
    MEMF(g_esp + 0x70) = g_xmm3.f[0];
    g_xmm3 = g_xmm7;
    MEMF(g_esp + 0x74) = g_xmm7.f[0];
    g_xmm7 = XMM_SCALAR(MEMF(g_esp + 0xDC));
    MEMF(g_esp + 0x64) = g_xmm2.f[0];
    g_xmm2 = XMM_SCALAR(MEMF(g_eax + 0x24));
    MEMF(g_esp + 0x88) = g_xmm0.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm7.f[0];
    g_xmm5 = g_xmm3;
    g_xmm5.f[0] = g_xmm5.f[0] * g_xmm2.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] - g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_eax + 0x20));
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x84));
    g_xmm7.f[0] = g_xmm7.f[0] * g_xmm5.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x88));
    g_xmm3.f[0] = g_xmm3.f[0] - g_xmm7.f[0];
    g_xmm7 = XMM_SCALAR(MEMF(g_eax + 0x20));
    g_xmm5.f[0] = g_xmm5.f[0] * g_xmm7.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] - g_xmm5.f[0];
    MEMF(g_esp + 0x28) = g_xmm2.f[0];
    TR_PUSH(MEMF(g_esp + 0x28));
    TR_TOP() = TR_TOP() * MEMF(g_esp + 0x28);
    MEMF(g_esp + 0x24) = g_xmm3.f[0];
    TR_PUSH(MEMF(g_esp + 0x24));
    MEMF(g_esp + 0x20) = g_xmm0.f[0];
    TR_TOP() = TR_TOP() * MEMF(g_esp + 0x24);
    TR_NEXT() = TR_NEXT() + TR_TOP(); TR_POP();
    TR_PUSH(MEMF(g_esp + 0x20));
    TR_TOP() = TR_TOP() * MEMF(g_esp + 0x20);
    TR_NEXT() = TR_NEXT() + TR_TOP(); TR_POP();
    TR_TOP() = sqrt(TR_TOP());
    TR_TOP() = MEMF(0x44BF94) / TR_TOP();
    MEMF(g_esp + 0x38) = (float)TR_TOP(); TR_POP();
    g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0x38));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm5.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * MEMF(0x452834);
    g_xmm1.f[0] = g_xmm1.f[0] * MEMF(0x46C320);
    g_xmm1.f[0] = g_xmm1.f[0] + MEMF(0x44BF94);
    g_ecx = MEM32(g_esp + 0x60);
    g_edx = MEM32(g_esp + 0x64);
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm5.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(g_esp + 0xC0));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm5.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm5.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm5.f[0];
    g_xmm5 = XMM_SCALAR(MEMF(0x44C1F8));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm5.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0];
    g_eax = g_edi + -24;
    MEM32(g_eax) = g_ecx;
    g_ecx = MEM32(g_esp + 0x68);
    MEM32(g_eax + 4) = g_edx;
    MEM32(g_eax + 8) = g_ecx;
    g_xmm7 = g_xmm0;
    g_xmm0.f[0] = g_xmm0.f[0] + g_xmm4.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm5.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm5.f[0];
    MEMF(g_esp + 0x2C) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0xA4));
    g_eax = MEM32(g_esp + 0x2C);
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm1.f[0];
    g_edx = g_edi + -12;
    MEM32(g_edx) = g_eax;
    g_xmm5 = g_xmm3;
    g_xmm5.f[0] = g_xmm5.f[0] + g_xmm6.f[0];
    g_xmm1 = g_xmm2;
    g_xmm1.f[0] = g_xmm1.f[0] + g_xmm0.f[0];
    MEMF(g_esp + 0x30) = g_xmm5.f[0];
    g_ecx = MEM32(g_esp + 0x30);
    MEM32(g_edx + 4) = g_ecx;
    MEMF(g_esp + 0x34) = g_xmm1.f[0];
    g_eax = MEM32(g_esp + 0x34);
    MEM32(g_edx + 8) = g_eax;
    g_xmm4.f[0] = g_xmm4.f[0] - g_xmm7.f[0];
    MEMF(g_esp + 0x54) = g_xmm4.f[0];
    g_edx = MEM32(g_esp + 0x54);
    g_ecx = g_edi;
    MEM32(g_ecx) = g_edx;
    g_xmm6.f[0] = g_xmm6.f[0] - g_xmm3.f[0];
    MEMF(g_esp + 0x58) = g_xmm6.f[0];
    g_eax = MEM32(g_esp + 0x58);
    MEM32(g_ecx + 4) = g_eax;
    g_eax = MEM32(g_esp + 0x3C);
    g_xmm0.f[0] = g_xmm0.f[0] - g_xmm2.f[0];
    MEMF(g_esp + 0x5C) = g_xmm0.f[0];
    g_edx = MEM32(g_esp + 0x5C);
    g_xmm0 = XMM_MEM(g_esp + 0xB0);
    MEM32(g_ecx + 8) = g_edx;
    g_ecx = MEM32(g_esp + 0x40);
    g_edx = MEM32(g_esp + 0x44);
    XMM_STORE(g_ebx + -32, g_xmm0);
    MEM32(g_esi + -16) = g_eax;
    g_eax = MEM32(g_esp + 0x48);
    MEM32(g_esi + -12) = g_ecx;
    g_ecx = MEM32(g_esp + 0x4C);
    XMM_STORE(g_ebx + -16, g_xmm0);
    MEM32(g_esi + -4) = g_eax;
    g_eax = MEM32(g_esp + 0x1C);
    MEM32(g_esi + -8) = g_edx;
    g_edx = MEM32(g_esp + 0x50);
    XMM_STORE(g_ebx, g_xmm0);
    MEM32(g_esi) = g_ecx;
    g_ecx = MEM32(g_esp + 0x18);
    MEM32(g_esi + 4) = g_edx;
    g_ecx = g_ecx + 0x40;
    g_edi = g_edi + 0x24;
    g_ebx = g_ebx + 0x30;
    g_esi = g_esi + 0x18;
    g_eax--;
    MEM32(g_esp + 0x18) = g_ecx;
    MEM32(g_esp + 0x1C) = g_eax;
  }while(g_eax);
 }
    POP32(g_esp, g_edi);
    POP32(g_esp, g_esi);
    POP32(g_esp, g_ebx);
    g_esp = frame;
    POP32(g_esp, frame);
    g_esp += 28; return;
}
int uc_hybrid_bullet_tracer_render(unsigned operation){
 if(!tracer_render_enabled())return 0;
 uint32_t s=g_esp;
 if(operation>2||(s&3)||s<0x10100u||!tracer_render_span(s-256,288)||
    !tracer_render_span(g_ecx,operation==2?60:0x420)||
    !tracer_render_span(0x51c178,1)||MEM8(0x51c178)||
    (operation==1&&(g_fp_top<0||g_fp_top>7)))return 0;
 if(operation==2)tracer_copy();else if(operation)tracer_fill();else tracer_prepare();return 1;
}
#undef TR_PUSH
#undef TR_POP
#undef TR_TOP
#undef TR_NEXT
