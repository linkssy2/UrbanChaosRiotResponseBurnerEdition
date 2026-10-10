#include "hybrid_gun_flare_render_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_002D3780(void);
static int flare_render_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_GUN_FLARE_RENDER");enabled=v&&!strcmp(v,"1");}return enabled;}
static int flare_render_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}

int uc_hybrid_gun_flare_render(unsigned operation){
 if(!flare_render_enabled())return 0;
 uint32_t start=g_esp;
 if(operation>1||(start&3)||start<0x10080u||!flare_render_span(start-128,160)||
    !flare_render_span(g_ecx,0x24)||!flare_render_span(0x51c178,1)||MEM8(0x51c178)||
    (operation==1&&(g_fp_top<0||g_fp_top>7)))return 0;
 if(operation==0){
  SET_LO8(g_eax,MEM8(g_ecx+0x18));int zero=LO8(g_eax)==0;
  if(!zero){SET_LO8(g_eax,MEM8(g_ecx+0x20));zero=LO8(g_eax)==0;}
  if(!zero){g_eax=MEM32(g_esp+4);MEM32(g_eax)=1;}
  else{g_edx=MEM32(g_esp+4);MEM32(g_edx)=0;}
  g_eax=MEM32(g_esp+8);MEM32(g_eax)=4;
  g_edx=MEM32(g_ecx+8);g_eax=MEM32(g_esp+12);MEM32(g_eax)=g_edx;
  MEM8(g_ecx+0x18)=0;g_recomp_zf=zero;g_esp+=16;return 1;
 }
 /* Output aliases deliberately retain the source read/write sequence. The
  * filler always emits one record and advances the original RNG once. */
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;
 g_esp=(g_esp&0xfffffff0u)-16;
 g_xmm0=XMM_SCALAR(MEMF(g_ecx+12));g_eax=MEM32(frame+0x14);MEMF(g_esp)=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(g_ecx+16));MEMF(g_esp+4)=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(g_ecx+20));MEMF(g_esp+8)=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(g_ecx+0x1c));MEMF(g_esp+12)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp);
 PUSH32(g_esp,0x40c90fdb);PUSH32(g_esp,0);XMM_STORE(g_eax,g_xmm0);
 g_ebp=frame;PUSH32(g_esp,0x1868aa);sub_002D3780();
 g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_ecx=MEM32(frame+0x18);g_edx=MEM32(frame+0x1c);
 MEMF(g_ecx)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
 MEMF(g_esp+8)=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm0.f[0];MEMF(g_esp+16)=g_xmm0.f[0];MEMF(g_esp+20)=g_xmm0.f[0];
 g_xmm0=XMM_MEM(g_esp+8);g_esp+=8;XMM_STORE(g_edx,g_xmm0);
 g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=28;return 1;
}
