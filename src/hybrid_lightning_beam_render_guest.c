#include "hybrid_lightning_beam_render_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int beam_render_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_LIGHTNING_BEAM_RENDER");enabled=v&&!strcmp(v,"1");}return enabled;}
static int beam_render_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define BEAM_PUSH(v) do{double beam_value=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=beam_value;}while(0)
#define BEAM_POP() (g_fp_top=(g_fp_top+1u)&7u)
#define BEAM_TOP() g_fp_stack[g_fp_top]
#define BEAM_NEXT() g_fp_stack[(g_fp_top+1)&7u]

/* These whole methods retain source-visible scratch and virtual x87 slots.
 * Source-valid caller buffers/strip indices remain the inherited contract. */
static void beam_prepare(void){
    BEAM_PUSH(MEMF(g_ecx + 0x234));
    BEAM_TOP() = BEAM_TOP() - MEMF(g_ecx + 0x240);
    BEAM_PUSH(MEMF(g_ecx + 0x238));
    BEAM_TOP() = BEAM_TOP() - MEMF(g_ecx + 0x244);
    BEAM_PUSH(MEMF(g_ecx + 0x23C));
    BEAM_TOP() = BEAM_TOP() - MEMF(g_ecx + 0x248);
    { double _t = BEAM_TOP(); BEAM_PUSH(_t); }
    BEAM_TOP() = BEAM_TOP() * BEAM_NEXT();
    { double _t = g_fp_stack[(g_fp_top + 2) & 7]; BEAM_PUSH(_t); }
    BEAM_TOP() = BEAM_TOP() * g_fp_stack[(g_fp_top + 3) & 7];
    BEAM_NEXT() = BEAM_NEXT() + BEAM_TOP(); BEAM_POP();
    { double _t = g_fp_stack[(g_fp_top + 3) & 7]; BEAM_PUSH(_t); }
    BEAM_TOP() = BEAM_TOP() * g_fp_stack[(g_fp_top + 4) & 7];
    BEAM_NEXT() = BEAM_NEXT() + BEAM_TOP(); BEAM_POP();
    BEAM_TOP() = sqrt(BEAM_TOP());
    g_fp_stack[(g_fp_top + 3) & 7] = BEAM_TOP(); BEAM_POP();
    BEAM_POP();
    BEAM_POP();
    BEAM_PUSH(MEMF(0x44BF94));
    g_fp_cmp = RECOMP_FCMP(BEAM_TOP(), BEAM_NEXT()); BEAM_POP();
    BEAM_POP();
 if(g_fp_cmp>0){g_eax=MEM32(g_esp+4);MEM32(g_eax)=0;g_recomp_zf=g_fp_cmp==0;g_esp+=16;return;}
 g_eax=MEM32(g_ecx-36);int visible=(HI8(g_eax)&1)==0;
 if(!visible){g_ecx=MEM32(g_esp+4);MEM32(g_ecx)=0;}
 else{g_edx=MEM32(g_esp+4);g_eax=MEM32(g_esp+8);MEM32(g_edx)=40;g_edx=MEM32(g_esp+12);MEM32(g_eax)=1;g_ecx=MEM32(g_ecx+0x230);MEM32(g_edx)=g_ecx;}
 g_recomp_zf=visible;g_esp+=16;
}

static void beam_fill(void){
 uint32_t frame=g_ebp;
    PUSH32(g_esp, frame);
    frame = g_esp;
    g_ebp = frame;
    g_esp = g_esp & 0xFFFFFFF0u;
    g_esp = g_esp - 0x94;
    BEAM_PUSH(MEMF(g_ecx + 0x234));
    g_eax = MEM32(frame + 0xC);
    BEAM_TOP() = BEAM_TOP() - MEMF(g_ecx + 0x240);
    BEAM_PUSH(MEMF(g_ecx + 0x238));
    PUSH32(g_esp, g_ebx);
    BEAM_TOP() = BEAM_TOP() - MEMF(g_ecx + 0x244);
    PUSH32(g_esp, g_esi);
    BEAM_PUSH(MEMF(g_ecx + 0x23C));
    PUSH32(g_esp, g_edi);
    BEAM_TOP() = BEAM_TOP() - MEMF(g_ecx + 0x248);
    MEM32(g_esp + 0x1C) = g_ecx;
    { double _t = BEAM_TOP(); BEAM_PUSH(_t); }
    BEAM_TOP() = BEAM_TOP() * BEAM_NEXT();
    { double _t = g_fp_stack[(g_fp_top + 2) & 7]; BEAM_PUSH(_t); }
    BEAM_TOP() = BEAM_TOP() * g_fp_stack[(g_fp_top + 3) & 7];
    BEAM_NEXT() = BEAM_NEXT() + BEAM_TOP(); BEAM_POP();
    { double _t = g_fp_stack[(g_fp_top + 3) & 7]; BEAM_PUSH(_t); }
    BEAM_TOP() = BEAM_TOP() * g_fp_stack[(g_fp_top + 4) & 7];
    BEAM_NEXT() = BEAM_NEXT() + BEAM_TOP(); BEAM_POP();
    BEAM_TOP() = sqrt(BEAM_TOP());
    g_fp_stack[(g_fp_top + 3) & 7] = BEAM_TOP(); BEAM_POP();
    BEAM_POP();
    BEAM_POP();
    MEMF(g_esp + 0x44) = (float)BEAM_TOP(); BEAM_POP();
 if((int32_t)g_eax>0){
    g_edx = MEM32(frame + 0x14);
    g_xmm0 = XMM_ZERO();
    g_xmm1 = XMM_SCALAR(MEMF(0x44BF94));
    g_ebx = MEM32(frame + 8);
    g_esi = MEM32(frame + 0x18);
    MEM32(g_esp + 0x10) = g_edx;
    g_edx = MEM32(frame + 0x1C);
    MEMF(g_esp + 0x84) = g_xmm0.f[0];
    MEMF(g_esp + 0x8C) = g_xmm1.f[0];
    MEMF(g_esp + 0x94) = g_xmm0.f[0];
    MEMF(g_esp + 0x9C) = g_xmm1.f[0];
    MEM32(g_esp + 0x40) = g_ebx;
    MEM32(g_esp + 0x14) = g_edx;
    MEM32(g_esp + 0x30) = g_eax;
  for(;;){
    g_xmm3 = XMM_SCALAR(MEMF(0x4700B8));
    g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x44));
    g_xmm4 = XMM_SCALAR(MEMF(0x453424));
    g_edi = MEM32(frame + 0x10);
    g_eax = g_ebx;
    g_edx = ((int32_t)g_eax < 0) ? 0xFFFFFFFF : 0;
    g_eax = g_eax - g_edx;
    g_eax = (uint32_t)((int32_t)g_eax >> ((1) & 31u));
    g_xmm0.f[0] = (float)(int32_t)g_eax;
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm2.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm4.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_ecx + 0x270);
    MEMF(g_esp + 0x80) = g_xmm0.f[0];
    MEMF(g_esp + 0x88) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_edi + 0x30));
    g_edx = g_ebx;
    g_edx = g_edx & 1;
    MEMF(g_esp + 0x64) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_edi + 0x34));
    MEMF(g_esp + 0x68) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_edi + 0x38));
    g_ebx = g_eax + 1;
    MEM32(g_esp + 0x18) = g_edx;
    g_edi = g_esp + g_edx * 8 + 0x80;
    g_edx = MEM32(g_esp + 0x14);
    g_xmm1.f[0] = (float)(int32_t)g_ebx;
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm2.f[0];
    MEM32(g_esp + 0x2C) = g_edx;
    g_edx = MEM32(g_esp + 0x10);
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm4.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] + MEMF(g_ecx + 0x270);
    MEMF(g_esp + 0x90) = g_xmm1.f[0];
    MEMF(g_esp + 0x98) = g_xmm1.f[0];
    MEMF(g_esp + 0x6C) = g_xmm0.f[0];
    MEM32(g_esp + 0x10) = g_edx;
    MEM32(g_esp + 0x14) = 3;
   for(;;){
    g_edx = MEM32(g_esp + 0x18);
    g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x68));
    g_xmm4 = XMM_SCALAR(MEMF(g_esp + 0x64));
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x6C));
    g_edx = g_edx >> 1;
    g_ebx = g_edx + g_eax;
    g_edx = g_ebx + g_ebx * 2;
    g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_ecx + g_edx * 4 + 0x3C);
    g_xmm3 = XMM_SCALAR(MEMF(g_ecx + g_edx * 4 + 0x13C));
    g_xmm4.f[0] = g_xmm4.f[0] - MEMF(g_ecx + g_edx * 4 + 0x38);
    g_xmm5 = XMM_SCALAR(MEMF(g_ecx + g_edx * 4 + 0x138));
    g_xmm6 = XMM_SCALAR(MEMF(g_ecx + g_edx * 4 + 0x134));
    g_xmm1.f[0] = g_xmm1.f[0] - MEMF(g_ecx + g_edx * 4 + 0x40);
    g_edx = g_ecx + g_edx * 4;
    g_xmm7 = g_xmm3;
    g_xmm7.f[0] = g_xmm7.f[0] * g_xmm0.f[0];
    g_xmm2 = g_xmm5;
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm1.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm6.f[0];
    g_xmm5.f[0] = g_xmm5.f[0] * g_xmm4.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] - g_xmm5.f[0];
    MEMF(g_esp + 0x3C) = g_xmm0.f[0];
    BEAM_PUSH(MEMF(g_esp + 0x3C));
    BEAM_TOP() = BEAM_TOP() * MEMF(g_esp + 0x3C);
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm4.f[0];
    g_xmm1.f[0] = g_xmm1.f[0] * g_xmm6.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] - g_xmm1.f[0];
    MEMF(g_esp + 0x38) = g_xmm3.f[0];
    BEAM_PUSH(MEMF(g_esp + 0x38));
    BEAM_TOP() = BEAM_TOP() * MEMF(g_esp + 0x38);
    g_xmm2.f[0] = g_xmm2.f[0] - g_xmm7.f[0];
    MEMF(g_esp + 0x34) = g_xmm2.f[0];
    BEAM_NEXT() = BEAM_NEXT() + BEAM_TOP(); BEAM_POP();
    BEAM_PUSH(MEMF(g_esp + 0x34));
    SET_LO8(g_ecx, MEM8(g_esp + 0x18));
    int odd=(LO8(g_ecx)&1)!=0;
    BEAM_TOP() = BEAM_TOP() * MEMF(g_esp + 0x34);
    g_xmm4 = XMM_ZERO();
    BEAM_NEXT() = BEAM_NEXT() + BEAM_TOP(); BEAM_POP();
    BEAM_TOP() = sqrt(BEAM_TOP());
    BEAM_TOP() = MEMF(0x44BF94) / BEAM_TOP();
    MEMF(g_esp + 0x48) = (float)BEAM_TOP(); BEAM_POP();
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x48));
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm1.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0];
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0];
    if(odd){
    g_xmm1 = g_xmm4;
    g_xmm1.f[0] = g_xmm1.f[0] - g_xmm2.f[0];
    MEMF(g_esp + 0x4C) = g_xmm1.f[0];
    g_ecx = MEM32(g_esp + 0x4C);
    g_xmm1 = g_xmm4;
    g_xmm1.f[0] = g_xmm1.f[0] - g_xmm3.f[0];
    MEMF(g_esp + 0x50) = g_xmm1.f[0];
    MEM32(g_esp + 0x20) = g_ecx;
    g_ecx = MEM32(g_esp + 0x50);
    g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x20));
    g_xmm1 = g_xmm4;
    g_xmm1.f[0] = g_xmm1.f[0] - g_xmm0.f[0];
    MEM32(g_esp + 0x24) = g_ecx;
    g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x24));
    MEMF(g_esp + 0x54) = g_xmm1.f[0];
    g_ecx = MEM32(g_esp + 0x54);
    MEM32(g_esp + 0x28) = g_ecx;
    g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x28));
    }
    g_ecx = MEM32(g_esp + 0x1C);
    g_xmm1 = XMM_SCALAR(MEMF(g_ecx + 0x254));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0];
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm1.f[0];
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(0x44C1F8));
    g_xmm0.f[0] = g_xmm0.f[0] * g_xmm1.f[0];
    MEMF(g_esp + 0x60) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_edx + 0x38));
    g_xmm2.f[0] = g_xmm2.f[0] * g_xmm1.f[0];
    MEMF(g_esp + 0x58) = g_xmm2.f[0];
    g_ecx = MEM32(g_esp + 0x58);
    MEM32(g_esp + 0x20) = g_ecx;
    g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0x20);
    MEMF(g_esp + 0x70) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_edx + 0x3C));
    g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0];
    MEMF(g_esp + 0x5C) = g_xmm3.f[0];
    g_ecx = MEM32(g_esp + 0x5C);
    MEM32(g_esp + 0x24) = g_ecx;
    g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0x24);
    g_ecx = MEM32(g_esp + 0x60);
    MEMF(g_esp + 0x74) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_edx + 0x40));
    g_edx = MEM32(g_esp + 0x10);
    MEM32(g_esp + 0x28) = g_ecx;
    g_ecx = MEM32(g_esp + 0x70);
    g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esp + 0x28);
    MEM32(g_edx) = g_ecx;
    g_ecx = MEM32(g_esp + 0x74);
    MEMF(g_esp + 0x78) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(0x44BF94));
    MEM32(g_edx + 4) = g_ecx;
    g_ecx = MEM32(g_esp + 0x78);
    MEM32(g_edx + 8) = g_ecx;
    if(g_ebx==0||g_ebx==20)g_xmm0=g_xmm4;
    g_ecx = MEM32(g_esp + 0x2C);
    g_ebx = MEM32(g_esp + 0x10);
    g_edx = g_esp + 0x7C;
    MEMF(g_esp + 0x7C) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(g_edx));
    g_edx = MEM32(g_edi);
    g_xmm1 = g_xmm0;
    g_ebx = g_ebx + 0xC;
    g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0);
    XMM_STORE(g_ecx, g_xmm1);
    MEM32(g_esi) = g_edx;
    g_edx = MEM32(g_edi + 4);
    MEM32(g_esi + 4) = g_edx;
    g_edx = MEM32(g_esp + 0x14);
    MEM32(g_esp + 0x10) = g_ebx;
    g_ebx = MEM32(g_esp + 0x18);
    g_ecx = g_ecx + 0x10;
    g_edi = g_edi + 8;
    g_esi = g_esi + 8;
    g_ebx++;
    g_edx--;
    MEM32(g_esp + 0x2C) = g_ecx;
    MEM32(g_esp + 0x18) = g_ebx;
    MEM32(g_esp + 0x14) = g_edx;
    if(!g_edx)break;
    g_ecx=MEM32(g_esp+0x1c);
   }
    g_eax = MEM32(g_esp + 0x10);
    g_ebx = MEM32(g_esp + 0x40);
    MEM32(g_esp + 0x10) = g_eax;
    g_eax = MEM32(g_esp + 0x30);
    g_ebx++;
    g_eax--;
    MEM32(g_esp + 0x40) = g_ebx;
    MEM32(g_esp + 0x14) = g_ecx;
    MEM32(g_esp + 0x30) = g_eax;
   if(!g_eax)break;
   g_ecx=MEM32(g_esp+0x1c);
  }
 }
    POP32(g_esp, g_edi);
    POP32(g_esp, g_esi);
    POP32(g_esp, g_ebx);
    g_esp = frame;
    POP32(g_esp, frame);
    g_esp += 28; return;
}

int uc_hybrid_lightning_beam_render(unsigned operation){
 if(!beam_render_enabled())return 0;
 uint32_t s=g_esp;
 if(operation>1||(s&3)||s<0x100c0u||!beam_render_span(s-192,224)||
    g_ecx<0x10024u||!beam_render_span(g_ecx-36,0x298)||
    !beam_render_span(0x51c178,1)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7)return 0;
 if(operation)beam_fill();else beam_prepare();return 1;
}
#undef BEAM_PUSH
#undef BEAM_POP
#undef BEAM_TOP
#undef BEAM_NEXT
