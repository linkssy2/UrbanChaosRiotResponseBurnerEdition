#include "hybrid_viewport_state_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <emmintrin.h>
#include <stdlib.h>
#include <string.h>
extern void sub_003E3610(void),sub_003E3CD0(void),sub_003E64E0(void),sub_003E6DF0(void),sub_003E7240(void),sub_003E7C50(void),sub_003E9810(void),sub_003E9820(void),sub_003EAA50(void);
#define VP_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define VP_PUSH(v) do{double vp_value=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=vp_value;}while(0)
#define VP_POP() (g_fp_top=(g_fp_top+1u)&7u)
#define VP_ST(i) g_fp_stack[(g_fp_top+(i))&7u]
static void vp_status(void){g_eax=(g_eax&0xffff0000u)|(uint16_t)(((g_fp_top&7u)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700u):g_fp_cmp==2?0x4500u:g_fp_cmp<0?0x100u:g_fp_cmp>0?0u:0x4000u));}
static int vp_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_VIEWPORT_STATE");f=p&&!strcmp(p,"1");}return f;}
static int vp_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int vp_apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* Original F3 0F 2C 44 24 04: load bits without a host float conversion,
 * then use the same CVTTSS2SI operation, including invalid/precision status. */
static __declspec(noinline) uint32_t vp_cvtt(uint32_t bits){return(uint32_t)_mm_cvttss_si32(_mm_castsi128_ps(_mm_cvtsi32_si128((int)bits)));}

/* Target-specific format bits are emitted even when the depth target is null. */
static void vp_depth_mode(void){
 g_eax=MEM32(0x3f4220);PUSH32(g_esp,g_esi);g_esi=g_eax?0x10100001u:0x100001u;
 if(MEM32(0x3f41dc)==2)g_esi|=0x10000;
 g_eax=MEM32(g_ecx+0x1a08);
 if(g_eax){g_eax=MEM8(g_eax+13);if(g_eax==0x2d||g_eax==0x2b||g_eax==0x31||g_eax==0x2f)g_esi|=0x1000;}
 MEM32(g_edx)=0x40290;MEM32(g_edx+4)=g_esi;g_eax=g_edx+8;POP32(g_esp,g_esi);g_esp+=4;
}
static void vp_target_mode(void){
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(0x3f4238);g_eax=MEM32(g_edi+0x1a04);g_ecx=MEM8(g_eax+13);
 SET_LO8(g_edx,MEM8(g_ecx+0x3f1d70));SET_LO8(g_edx,LO8(g_edx)&0x3c);g_esi=0;if(LO8(g_edx)==0x20)g_esi=g_ebx;
 g_eax=MEM32(g_edi+8);g_ecx=g_eax&1;
 if(g_esi!=g_ecx){
  g_ecx=MEM32(g_edi+4);g_eax^=1;MEM32(g_edi+8)=g_eax;g_eax=MEM32(g_edi);
  if(g_eax>=g_ecx)VP_CALL(sub_003E9810,0x3e7659);
  MEM32(g_eax)=0x40110;MEM32(g_eax+4)=0;g_esi<<=5;MEM32(g_eax+8)=0x40100;g_esi|=8;MEM32(g_eax+12)=g_esi;g_eax+=16;MEM32(g_edi)=g_eax;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0x3f421c)=g_ebx;POP32(g_esp,g_ebx);g_esp+=8;
}

/* Select the original program template, publish scale constants, and retain
 * the original program-copy call. No template or packet writes are batched. */
static void vp_program_constants(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x794);
 if(MEM8(g_eax+4)&2){
  g_eax=MEM32(0x3f4114);PUSH32(g_esp,g_ebx);g_ebx=0;PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);
  if(!g_eax){frame=0x3f2208;g_edi=0x30;}
  else if(MEM8(g_esi+8)&2){g_edi=0x2c;frame=0x3f2158;}
  else{frame=0x3f2098;g_edi=0x30;}
  g_ecx=g_edi+0x1e;PUSH32(g_esp,g_ecx);VP_CALL(sub_003E9820,0x3e6e3d);
  MEM32(g_eax)=0x41ea4;MEM32(g_eax+4)=g_ebx;MEM32(g_eax+8)=0x200b80;
  if(MEM32(0x3f41dc)==2){VP_PUSH(MEMF(g_esi+0x948));VP_ST(0)*=MEMF(g_esi+0x944);}else VP_PUSH(MEMF(0x44bf94));
  g_edx=MEM32(g_esi+0x95c);MEM32(g_eax+12)=g_edx;g_ecx=MEM32(g_esi+0x960);MEM32(g_eax+16)=g_ecx;g_edx=MEM32(g_esi+0x948);MEMF(g_eax+24)=(float)VP_ST(0);VP_POP();
  VP_PUSH(MEMF(0x44c174));MEM32(g_eax+20)=g_edx;g_ecx=MEM32(g_esi+8);
  if((int8_t)HI8(g_ecx)<0&&MEM32(0x3f4200)!=g_ebx){VP_POP();VP_PUSH(MEMF(0x44c1f8));}
  VP_PUSH(MEMF(g_esi+0xef8));PUSH32(g_esp,g_edi);VP_ST(0)-=VP_ST(1);PUSH32(g_esp,frame);g_eax+=0x34;PUSH32(g_esp,g_eax);MEMF(g_eax-24)=(float)VP_ST(0);VP_POP();
  VP_PUSH(MEMF(g_esi+0xefc));MEM32(g_eax-16)=g_ebx;VP_ST(0)-=VP_ST(1);MEM32(g_eax-12)=g_ebx;MEMF(g_eax-20)=(float)VP_ST(0);VP_POP();MEM32(g_eax-8)=0x41e9c;MEM32(g_eax-4)=g_ebx;VP_POP();
  VP_CALL(sub_003E64E0,0x3e6ed8);POP32(g_esp,g_edi);POP32(g_esp,frame);MEM32(g_esi)=g_eax;POP32(g_esp,g_ebx);
 }
 POP32(g_esp,g_esi);g_esp+=4;
}

/* Clamp a requested viewport using the live target, then retain the original
 * transform/viewport command writers. Arithmetic scratch remains guest RAM. */
static void vp_set_viewport(void){
 uint32_t frame=g_seh_ebp;g_esp-=12;PUSH32(g_esp,frame);frame=MEM32(g_esp+20);PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f4238);
 if(frame){
  g_eax=MEM32(g_esi+0x1a04);
  if(g_eax==MEM32(g_esi+0x1a14)){g_ecx=MEM32(g_esi+0x1a18);MEM32(g_esp+8)=g_ecx;}else{MEM32(g_esp+8)=g_eax;g_ecx=g_eax;}
  g_edx=MEM32(frame);g_eax=0;g_eax=(g_eax<g_edx?0xffffffffu:0u)&g_edx;MEM32(g_esp+12)=g_eax;g_eax=0;PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+4);g_eax=(g_eax<g_ebx?0xffffffffu:0u)&g_ebx;MEM32(g_esp+20)=g_eax;
  g_eax=MEM32(g_ecx+16);PUSH32(g_esp,g_edi);
  if(!g_eax){g_ecx=(MEM32(g_ecx+12)>>20)&15;g_edi=1u<<g_ecx;g_ecx=MEM32(g_esp+16);}else g_edi=(g_eax&0xfff)+1;
  frame=MEM32(frame+8);g_edx+=frame;
  if(g_edx>=g_edi){if(!g_eax){g_ecx=(MEM32(g_ecx+12)>>20)&15;g_edx=1u<<g_ecx;}else g_edx=(g_eax&0xfff)+1;}
  frame=g_edx;
  if(!g_eax){g_ecx=MEM32(g_esp+16);g_ecx=MEM8(g_ecx+15)&15;g_edx=1u<<g_ecx;}else g_edx=((g_eax>>12)&0xfff)+1;
  g_ecx=MEM32(g_esp+32);g_ecx=MEM32(g_ecx+12);g_ebx+=g_ecx;
  if(g_ebx<g_edx)g_eax=g_ebx;
  else if(!g_eax){g_edx=MEM32(g_esp+16);g_ecx=0;SET_LO8(g_ecx,MEM8(g_edx+15));g_ecx&=15;g_eax=1u<<g_ecx;}
  else g_eax=((g_eax>>12)&0xfff)+1;
  MEM32(g_esp+16)=frame;VP_PUSH((double)SMEM32(g_esp+16));g_edi=g_eax;if((int32_t)frame<0)VP_ST(0)+=MEMF(0x44c510);
  VP_ST(0)*=MEMF(g_esi+0x95c);g_ebx=MEM32(g_esi+0x954);PUSH32(g_esp,g_ecx);VP_ST(0)+=MEMF(0x44c1f8);MEMF(g_esp)=(float)VP_ST(0);VP_POP();VP_CALL(sub_003EAA50,0x3e373d);
  if(g_eax>g_ebx){MEM32(g_esp+16)=g_ebx;VP_PUSH((double)SMEM32(g_esp+16));if((int32_t)g_ebx<0)VP_ST(0)+=MEMF(0x44c510);VP_ST(0)/=MEMF(g_esi+0x95c);PUSH32(g_esp,g_ecx);MEMF(g_esp)=(float)VP_ST(0);VP_POP();VP_CALL(sub_003EAA50,0x3e3762);frame=g_eax;}
  MEM32(g_esp+16)=g_edi;VP_PUSH((double)SMEM32(g_esp+16));if((int32_t)g_edi<0)VP_ST(0)+=MEMF(0x44c510);
  VP_ST(0)*=MEMF(g_esi+0x960);g_ebx=MEM32(g_esi+0x958);PUSH32(g_esp,g_ecx);VP_ST(0)+=MEMF(0x44c1f8);MEMF(g_esp)=(float)VP_ST(0);VP_POP();VP_CALL(sub_003EAA50,0x3e3791);
  if(g_eax>g_ebx){MEM32(g_esp+16)=g_ebx;VP_PUSH((double)SMEM32(g_esp+16));if((int32_t)g_ebx<0)VP_ST(0)+=MEMF(0x44c510);VP_ST(0)/=MEMF(g_esi+0x960);PUSH32(g_esp,g_ecx);MEMF(g_esp)=(float)VP_ST(0);VP_POP();VP_CALL(sub_003EAA50,0x3e37b6);g_edi=g_eax;}
  g_eax=MEM32(g_esp+20);g_ecx=MEM32(g_esp+24);MEM32(g_esi+0xee0)=g_eax;frame-=g_eax;g_eax=MEM32(g_esp+32);g_edi-=g_ecx;MEM32(g_esi+0xeec)=g_edi;MEM32(g_esi+0xee4)=g_ecx;MEM32(g_esi+0xee8)=frame;
  g_ecx=MEM32(g_eax+16);MEM32(g_esi+0xef0)=g_ecx;g_edx=MEM32(g_eax+20);MEM32(g_esi+0xef4)=g_edx;g_eax=MEM32(0x3f3d98);g_eax|=0x100;POP32(g_esp,g_edi);MEM32(0x3f3d98)=g_eax;POP32(g_esp,g_ebx);
 }
 PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);VP_CALL(sub_003E3CD0,0x3e380e);VP_CALL(sub_003E7240,0x3e3813);
 g_eax=MEM32(g_esi);if(g_eax>=MEM32(g_esi+4))VP_CALL(sub_003E9810,0x3e381f);
 g_edx=g_eax;g_ecx=g_esi;VP_CALL(sub_003E7C50,0x3e3828);MEM32(g_esi)=g_eax;POP32(g_esp,g_esi);POP32(g_esp,frame);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=8;
}

/* Reconcile target dimensions and scaling, then emit the original format and
 * filtering commands before updating program constants and the viewport. */
static void vp_target_scale(void){
 uint32_t frame=g_seh_ebp;g_esp-=16;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ebx=MEM32(g_esi+0x1a04);unsigned same=g_ebx==MEM32(g_esi+0x1a14);PUSH32(g_esp,g_edi);MEM32(g_esp+24)=g_edx;
 if(same){
  g_ecx=MEM32(g_esi+0x970);g_eax=MEM32(g_esi+0x96c);frame=MEM32(0x3f4208);MEM32(g_esp+20)=g_ecx;g_ecx=MEM32(g_esi+0x1a18);MEM32(g_esp+16)=g_eax;g_eax=MEM32(g_ecx+16);
  if(!g_eax){g_ecx=(MEM32(g_ecx+12)>>20)&15;g_eax=1u<<g_ecx;}else g_eax=(g_eax&0xfff)+1;
  MEM32(g_esp+28)=g_eax;VP_PUSH((double)SMEM32(g_esp+28));if((int32_t)g_eax<0)VP_ST(0)+=MEMF(0x44c510);VP_ST(0)*=MEMF(g_esp+16);PUSH32(g_esp,g_ecx);VP_ST(0)+=MEMF(0x44c1f8);MEMF(g_esp)=(float)VP_ST(0);VP_POP();VP_CALL(sub_003EAA50,0x3e7ff3);
  g_ecx=MEM32(g_esi+0x1a18);g_edi=g_eax;g_eax=MEM32(g_ecx+16);
  if(!g_eax){g_ecx=MEM8(g_ecx+15)&15;g_eax=1u<<g_ecx;}else g_eax=((g_eax>>12)&0xfff)+1;
  MEM32(g_esp+28)=g_eax;VP_PUSH((double)SMEM32(g_esp+28));if((int32_t)g_eax<0)VP_ST(0)+=MEMF(0x44c510);VP_ST(0)*=MEMF(g_esp+20);PUSH32(g_esp,g_ecx);VP_ST(0)+=MEMF(0x44c1f8);MEMF(g_esp)=(float)VP_ST(0);VP_POP();VP_CALL(sub_003EAA50,0x3e8040);
  g_edx=MEM32(g_esp+16);VP_PUSH(MEMF(g_esp+20));MEM32(g_esp+16)=g_edx;g_edx=MEM32(g_esi+0x1a14);g_ecx=MEM32(g_edx+16);
  if(!g_ecx){g_ecx=(MEM32(g_edx+12)>>20)&15;g_ebx=1u<<g_ecx;}else{g_ecx=(g_ecx&0xfff)+1;g_ebx=g_ecx;}
  if(g_edi>=g_ebx){g_edi=MEM32(g_edx+16);if(!g_edi){g_ecx=(MEM32(g_edx+12)>>20)&15;g_edi=1u<<g_ecx;}else g_edi=(g_edi&0xfff)+1;}
  g_ecx=MEM32(g_edx+16);
  if(!g_ecx){g_ecx=0;SET_LO8(g_ecx,MEM8(g_edx+15));g_ecx&=15;g_ebx=1u<<g_ecx;}else{g_ecx=((g_ecx>>12)&0xfff)+1;g_ebx=g_ecx;}
  if(g_eax>=g_ebx){g_eax=MEM32(g_edx+16);if(!g_eax){g_ecx=0;SET_LO8(g_ecx,MEM8(g_edx+15));g_ecx&=15;g_eax=1u<<g_ecx;}else g_eax=((g_eax>>12)&0xfff)+1;}
 }else{
  g_eax=MEM32(g_ebx+16);VP_PUSH(MEMF(0x44bf94));frame=MEM32(0x3f420c);MEM32(g_esp+16)=0x3f800000;
  if(!g_eax){g_ecx=(MEM32(g_ebx+12)>>20)&15;g_edx=1u<<g_ecx;}else g_edx=(g_eax&0xfff)+1;
  g_edi=g_edx;
  if(!g_eax){g_ecx=0;SET_LO8(g_ecx,MEM8(g_ebx+15));g_ecx&=15;g_eax=1u<<g_ecx;}else g_eax=((g_eax>>12)&0xfff)+1;
 }
 g_ebx=MEM32(g_esi+0x1a0c);g_ecx=g_eax;g_eax=MEM32(g_esi+8);g_eax&=0xffff7fff;MEM32(g_esi+8)=g_eax;
 if(frame&&!(HI8(g_ebx)&2)){
  VP_PUSH(MEMF(g_esp+16));g_eax|=0x8000;VP_ST(0)*=MEMF(0x44c1f8);g_edi=(g_edi+1u)>>1;unsigned both=frame==2;MEMF(g_esp+16)=(float)VP_ST(0);VP_POP();MEM32(g_esi+8)=g_eax;
  if(both){VP_ST(0)*=MEMF(0x44c1f8);g_ecx=(g_ecx+1u)>>1;g_ebx|=0x2000;}else g_ebx|=0x1000;
 }
 g_eax=MEM32(g_esp+16);MEMF(g_esi+0x960)=(float)VP_ST(0);VP_PUSH(MEMF(g_esp+16));MEM32(g_esi+0x95c)=g_eax;g_fp_cmp=RECOMP_FCMP(VP_ST(0),VP_ST(1));VP_POP();MEM32(g_esi+0x954)=g_edi;MEM32(g_esi+0x958)=g_ecx;vp_status();
 if(RECOMP_PARITY8(HI8(g_eax)&5)){MEMF(g_esp+16)=(float)VP_ST(0);VP_POP();}else{g_ecx=MEM32(g_esp+16);VP_POP();MEM32(g_esp+16)=g_ecx;}
 g_eax=MEM32(g_esi);if(g_eax>=MEM32(g_esi+4))VP_CALL(sub_003E9810,0x3e81c4);
 VP_PUSH(MEMF(g_esp+16));MEM32(g_eax)=0x40208;MEM32(g_eax+4)=g_ebx;VP_PUSH(MEMF(g_esi+0x964));g_fp_cmp=RECOMP_FCMP(VP_ST(0),VP_ST(1));VP_POP();VP_POP();g_ebx=g_eax+8;vp_status();
 if(RECOMP_PARITY8(HI8(g_eax)&0x44)){
  VP_PUSH(MEMF(g_esp+16));g_edx=MEM32(g_esp+16);VP_ST(0)+=VP_ST(0);PUSH32(g_esp,g_ecx);MEM32(g_esi+0x964)=g_edx;VP_ST(0)+=MEMF(0x44c1f8);MEMF(g_esp)=(float)VP_ST(0);VP_POP();VP_CALL(sub_003EAA50,0x3e8202);
  g_eax=MEM32(g_eax*4+0x3f22d8);MEM32(g_esi+0x968)=g_eax;g_ecx=MEM32(0x3f4214);g_edi=MEM32(0x3f3d98);MEM32(g_esp+20)=g_ecx;VP_PUSH(MEMF(g_esp+20));PUSH32(g_esp,g_ecx);VP_ST(0)*=MEMF(g_esp+20);g_edi|=0x10f;MEM32(0x3f3d98)=g_edi;VP_ST(0)*=MEMF(0x44eef8);VP_ST(0)+=MEMF(0x44c1f8);MEMF(g_esp)=(float)VP_ST(0);VP_POP();VP_CALL(sub_003EAA50,0x3e8248);
  if(g_eax>0x1ff)g_eax=0x1ff;MEM32(g_ebx)=0x40380;MEM32(g_ebx+4)=g_eax;g_ebx+=8;
 }
 g_eax=MEM32(g_esi+8);MEM32(g_esi)=g_ebx;g_edi=MEM32(0x3f4204);g_ecx=MEM32(0x3f4218);g_edi<<=16;g_edi|=g_ecx;
 if((int8_t)HI8(g_eax)<0){g_eax=MEM32(0x3f4200);if(g_eax)g_edi|=1;}
 unsigned grow=g_ebx>=MEM32(g_esi+4);g_eax=g_ebx;if(grow)VP_CALL(sub_003E9810,0x3e8292);
 MEM32(g_eax)=0x41d7c;MEM32(g_eax+4)=g_edi;g_edx=MEM32(g_esi+0x794);g_eax+=8;MEM32(g_esi)=g_eax;
 if(MEM8(g_edx+4)&2){g_ecx=g_esi;VP_CALL(sub_003E6DF0,0x3e82b3);}
 g_eax=MEM32(g_esp+24);PUSH32(g_esp,g_eax);VP_CALL(sub_003E3610,0x3e82bd);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}

int uc_hybrid_viewport_state(uint32_t entry){
 if(!vp_enabled())return 0;
 if(entry!=0x3e3610&&entry!=0x3e6df0&&entry!=0x3e7610&&entry!=0x3e7e40&&entry!=0x3e7f70&&entry!=0x3eaa50)return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10400u||!vp_span(s-1024,1060)||!vp_span(0x51c178,1)||MEM8(0x51c178)||!vp_apart(s-1024,1060,0x51c178,1))return 0;
 if(entry==0x3eaa50){g_eax=vp_cvtt(MEM32(g_esp+4));g_esp+=8;return 1;}
 if((entry==0x3e3610||entry==0x3e6df0||entry==0x3e7f70)&&(unsigned)g_fp_top>7)return 0;
 if(!vp_span(0x3f4238,4)||!vp_apart(s-1024,1060,0x3f4238,4))return 0;
 uint32_t device=(entry==0x3e3610||entry==0x3e7610)?MEM32(0x3f4238):g_ecx;
 if(!vp_span(device,0x1a1c)||!vp_apart(device,0x1a1c,0x51c178,1))return 0;
 if(entry==0x3e7e40&&(!vp_span(g_edx,8)||!vp_apart(g_edx,8,0x51c178,1)))return 0;
 if(entry==0x3e3610&&MEM32(s+4)&&!vp_span(MEM32(s+4),24))return 0;
 switch(entry){case 0x3e3610:vp_set_viewport();break;case 0x3e6df0:vp_program_constants();break;case 0x3e7610:vp_target_mode();break;case 0x3e7e40:vp_depth_mode();break;default:vp_target_scale();break;}
 return 1;
}
#undef VP_CALL
#undef VP_PUSH
#undef VP_POP
#undef VP_ST
