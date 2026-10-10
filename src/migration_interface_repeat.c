#include "migration_interface_repeat.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
static int repeat_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void repeat_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void repeat_compare(void){g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;}
static unsigned repeat_flags(float a,float b){double x=a,y=b;return x!=x||y!=y?0x47:x<y?3:x>y?2:0x42;}
static void repeat_input(unsigned action,unsigned mode,unsigned ret,int edx_table){g_ecx=MEM32(0x51c4dc);if(edx_table)g_edx=MEM32(g_ecx);else g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,mode);PUSH32(g_esp,action);PUSH32(g_esp,g_ebx);uint32_t target=MEM32((edx_table?g_edx:g_eax)+8);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static int repeat_pressed(void){repeat_push(MEMF(0x44c174));repeat_compare();SET_HI8(g_eax,g_fp_cmp==2?0x47:g_fp_cmp<0?3:g_fp_cmp>0?2:0x42);return RECOMP_PARITY8(HI8(g_eax)&0x44);}
int migration_interface_repeat(unsigned op){static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_REPEAT");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;uint32_t s=g_esp;if(s<0x10100||(s&3)||!repeat_span(s-256,264)||!repeat_span(0x51d2bc,16)||!repeat_span(0x51c4dc,4))return 0;uint32_t owner=MEM32(0x51c4dc);if(!repeat_span(owner,4)||!repeat_span(MEM32(owner),12)||g_fp_top<0||g_fp_top>7)return 0;
 SET_LO8(g_eax,MEM8(0x51d2ca));g_esp-=8;PUSH32(g_esp,g_ebx);g_ebx=0;
 if(LO8(g_eax)){MEM8(0x51d2c8)=MEM8(0x51d2ca)=MEM8(0x51d2bd)=0;}
 else{repeat_input(29,1,0x2743fd,0);MEM8(0x51d2c8)=(uint8_t)repeat_pressed();repeat_input(28,1,0x27442a,1);MEM8(0x51d2bd)=(uint8_t)repeat_pressed();}
 repeat_input(18,3,0x274457,0);MEMF(g_esp+4)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;repeat_input(19,3,0x27446b,1);MEMF(g_esp+8)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
 g_xmm0=XMM_SCALAR(MEMF(0x51d2c4));g_xmm1=XMM_ZERO();unsigned flags=repeat_flags(g_xmm0.f[0],g_xmm1.f[0]);MEM8(0x51d2bc)=MEM8(0x51d2c9)=0;if(!(flags&0x41)){g_xmm0.f[0]-=MEMF(g_esp+16);MEMF(0x51d2c4)=g_xmm0.f[0];}
 for(unsigned axis=0;axis<2;axis++){
  unsigned held=axis?0x51d2bf:0x51d2c1,trigger=axis?0x51d2bc:0x51d2c9,direction=axis?0x51d2be:0x51d2c0;
  repeat_push(MEMF(g_esp+4+axis*4));SET_LO8(g_eax,MEM8(held));g_fp_stack[g_fp_top]=fabs(g_fp_stack[g_fp_top]);if(!axis){g_xmm3=XMM_SCALAR(MEMF(0x44c538));g_xmm2=XMM_SCALAR(MEMF(0x44c5f0));}
  if(LO8(g_eax)){
   repeat_push(MEMF(0x44ec20));repeat_compare();if(g_fp_cmp>0){MEM8(held)=0;continue;}
   if(repeat_flags(g_xmm1.f[0],g_xmm0.f[0])&1)continue;
   g_xmm0=XMM_SCALAR(MEMF(g_esp+4+axis*4));flags=repeat_flags(g_xmm0.f[0],g_xmm1.f[0]);MEM8(trigger)=MEM8(held)=MEM8(direction)=1;if(flags&0x41)MEM8(direction)=0;
   if(!axis)g_xmm0=g_xmm3;MEMF(0x51d2c4)=axis?g_xmm3.f[0]:g_xmm0.f[0];
  }else{
   repeat_push(MEMF(0x44c5f0));double temp=g_fp_stack[g_fp_top];g_fp_stack[g_fp_top]=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[(g_fp_top+1)&7]=temp;repeat_compare();if(g_fp_cmp<=0)continue;
   if(!axis){g_xmm4=XMM_SCALAR(MEMF(g_esp+4));flags=repeat_flags(g_xmm4.f[0],g_xmm1.f[0]);g_xmm0=g_xmm2;MEMF(0x51d2c4)=g_xmm0.f[0];}else{g_xmm0=XMM_SCALAR(MEMF(g_esp+8));flags=repeat_flags(g_xmm0.f[0],g_xmm1.f[0]);MEMF(0x51d2c4)=g_xmm2.f[0];}
   MEM8(trigger)=MEM8(held)=MEM8(direction)=1;if(flags&0x41)MEM8(direction)=0;
  }
 }
 POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
