#include "migration_graphics_depth_bias.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E7080(void),sub_003E70B0(void);
static int depth_bias_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_DEPTH_BIAS");flag=v&&!strcmp(v,"1");}return flag;}
static int depth_bias_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define FTOP g_fp_stack[g_fp_top]
#define FCALL(fn,pc) do{PUSH32(esp,pc);fn();}while(0)
static void depth_bias_push(double value){g_fp_top=(g_fp_top+7)&7;FTOP=value;}
int uc_migration_graphics_depth_bias(uint32_t entry){
 if(!depth_bias_enabled()||(entry!=0x3e70b0&&entry!=0x3e7690))return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!depth_bias_span(s-128,140)||!depth_bias_span(0x51c178,1)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7)return 0;

 if(entry==0x3e70b0){uint32_t state=MEM32(s+4);if(state>255)return 0;PUSH32(esp,esi);esi=MEM32(esp+8);
  if((int32_t)esi<92){ecx=MEM32(esi*4+0x44b5d8);PUSH32(esp,edi);edi=MEM32(esp+16);edx=edi;FCALL(sub_003E7080,0x3e70cd);MEM32(esi*4+0x3f3fa0)=edi;POP32(esp,edi);POP32(esp,esi);g_recomp_zf=0;esp+=12;return 1;}
  if((int32_t)esi<136){eax=MEM32(esi*4+0x44b3b8);MEM32(0x3f3d98)|=eax;ecx=MEM32(esp+12);MEM32(esi*4+0x3f3fa0)=ecx;POP32(esp,esi);esp+=12;return 1;}
  int zero=esi==136;edx=MEM32(esp+12);uint32_t saved=esp;PUSH32(esp,edx);uint32_t target=MEM32(esi*4+0x3f20d8);PUSH32(esp,0x3e7109);RECOMP_ICALL_SAFE(target,saved);POP32(esp,esi);g_recomp_zf=zero;esp+=12;return 1;
 }
 PUSH32(esp,ecx);PUSH32(esp,esi);eax=0;PUSH32(esp,edi);edi=MEM32(esp+16);SET_LO8(eax,edi!=0);int zero=edi==0;MEM32(esp+16)=edi;depth_bias_push((double)SMEM32(esp+16));esi=eax;if((int32_t)edi<0)FTOP+=MEMF(0x44c510);FTOP=-FTOP;MEMF(esp+16)=(float)FTOP;FTOP*=MEMF(0x44d660);MEMF(esp+8)=(float)FTOP;g_fp_top=(g_fp_top+1)&7;
 ecx=MEM32(esp+8);PUSH32(esp,ecx);PUSH32(esp,0x4d);FCALL(sub_003E70B0,0x3e76ce);edx=MEM32(esp+16);eax=edx;PUSH32(esp,eax);PUSH32(esp,0x4e);MEM32(esp+24)=edx;FCALL(sub_003E70B0,0x3e76e0);
 static const uint32_t pcs[]={0x3e76e8,0x3e76f0,0x3e76f8};for(unsigned i=0;i<3;i++){PUSH32(esp,esi);PUSH32(esp,0x4f+i);FCALL(sub_003E70B0,pcs[i]);}
 MEM32(0x3f41f4)=edi;POP32(esp,edi);POP32(esp,esi);POP32(esp,ecx);g_recomp_zf=zero;esp+=8;return 1;
}
#undef FCALL
#undef FTOP
