#include "migration_graphics_material_pass.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0008EC20(void),sub_003E7080(void),sub_002D631D(void),sub_002D5CCD(void),sub_0008E690(void);
#define PASS_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int pass_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MATERIAL_PASS");flag=v&&!strcmp(v,"1");}return flag;}
static int pass_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void pass_device(unsigned offset,uint32_t pc){g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+offset);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
static void pass_object(unsigned offset,uint32_t pc){g_edx=MEM32(g_eax);g_ecx=g_eax;uint32_t saved=g_esp,target=MEM32(g_edx+offset);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
static void material_pass(void){
 PUSH32(g_esp,g_ecx);SET_LO8(g_eax,MEM8(0x519f88));PUSH32(g_esp,g_esi);g_esi=g_ecx;int draw=!(g_eax&255);
 if(draw){SET_LO8(g_eax,MEM8(0x519f89));draw=!(g_eax&255);}
 if(draw&&(MEM8(g_esi+77)&2)){pass_device(0x60,0x8fdaf);pass_object(0x74,0x8fdb6);draw=(g_eax&255)!=0;}
 if(draw){
  pass_device(0x50,0x8fdc9);pass_object(0x30,0x8fdd0);
  if(MEM8(g_esi+76)&8){g_edx=0x1010101;g_ecx=0x40358;PASS_CALL(sub_003E7080,0x8fde5);MEM32(0x3f40ac)=0x1010101;}
  SET_LO8(g_eax,MEM8(0x51a014));g_xmm0=XMM_ZERO();if(!(g_eax&255))g_xmm0=XMM_SCALAR(MEMF(0x4565e0));
  g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);MEMF(g_esp+4)=g_xmm0.f[0];{uint32_t saved=g_esp,target=MEM32(g_eax+0xa4);PUSH32(g_esp,0x8fe17);RECOMP_ICALL_SAFE(target,saved);}pass_object(0x1c,0x8fe1e);SET_LO8(g_ecx,MEM8(g_eax+4));
  if(g_ecx&255){pass_device(0xa4,0x8fe33);pass_object(0x1c,0x8fe3a);g_xmm0=XMM_SCALAR(MEMF(g_eax+8));MEMF(g_esp+4)=g_xmm0.f[0];}
  else{
   g_eax=MEM32(g_esi+28);
   if(g_eax){g_eax=MEM32(g_esi+32);g_ecx=MEM32(g_eax+16);if(g_ecx){g_ecx=g_eax;g_ecx=MEM32(g_ecx+16);g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+44);PUSH32(g_esp,0x8fe5c);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x519f60);if(g_eax==MEM32(g_ecx+4)){g_xmm0=XMM_SCALAR(MEMF(0x4535a4));MEMF(g_esp+4)=g_xmm0.f[0];}}}
  }
  g_eax=MEM32(0x3f3d98);g_ecx=MEM32(g_esp+12);g_edx=MEM32(g_esp+4);g_eax|=1;MEM32(0x3f3d98)=g_eax;g_eax=MEM32(g_esp+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=g_esi;MEM32(0x3f3db8)=g_edx;PASS_CALL(sub_0008EC20,0x8fea1);
  if(MEM8(g_esi+76)&8){g_edx=0x10101;g_ecx=0x40358;PASS_CALL(sub_003E7080,0x8feb6);MEM32(0x3f40ac)=0x10101;}
 }
 SET_LO8(g_eax,1);POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_esp+=12;
}
static void material_destroy(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+32);PUSH32(g_esp,g_eax);MEM32(g_esi)=0x4565b0;MEM32(g_esi+4)=0x4565ac;PASS_CALL(sub_002D631D,0x8fc49);
 g_ecx=MEM32(g_esi+20);PUSH32(g_esp,g_ecx);PASS_CALL(sub_002D631D,0x8fc52);g_edx=MEM32(g_esi+24);PUSH32(g_esp,g_edx);PASS_CALL(sub_002D631D,0x8fc5b);g_eax=MEM32(g_esi+56);g_esp+=12;
 if(g_eax){PUSH32(g_esp,g_eax);PASS_CALL(sub_002D5CCD,0x8fc6b);g_esp+=4;}
 MEM32(g_esi+56)=0;MEM32(g_esi+60)=0;MEM32(g_esi+64)=0;g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_0008E690();
}
int uc_migration_graphics_material_pass(uint32_t entry){
 if(!pass_enabled()||(entry!=0x8fd80&&entry!=0x8fc30))return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!pass_span(s-128,144)||!pass_span(g_ecx,80)||!pass_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x8fd80)material_pass();else material_destroy();return 1;
}
