#include "migration_graphics_material_scalar.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
extern void sub_002D6802(void);
static int scalar_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MATERIAL_SCALAR");flag=v&&!strcmp(v,"1");}return flag;}
static int scalar_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void scalar_push(double value){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;}
#define TOP g_fp_stack[g_fp_top]
int uc_migration_graphics_material_scalar(uint32_t entry){
 if(!scalar_enabled()||entry!=0x8e840)return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!scalar_span(s-64,80)||!scalar_span(0x51c178,1)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7)return 0;
 uint32_t frame=g_seh_ebp;xmm0.f[0]/=MEMF(esp+4);xmm0.f[0]+=MEMF(esp+8);eax=LO8(eax);eax-=74;int equal=eax==10;MEMF(esp+4)=xmm0.f[0];
 if(eax<=10){eax=MEM8(eax+0x8e8fc);uint32_t target=MEM32(eax*4+0x8e8e4);
  switch(target){
  case 0x8e86b:case 0x8e878:scalar_push(MEMF(esp+4));TOP*=MEMF(0x44d95c);TOP=target==0x8e86b?sin(TOP):cos(TOP);esp+=4;return 1;
  case 0x8e885:case 0x8e895:case 0x8e8ab:
   scalar_push(MEMF(esp+4));scalar_push(MEMD(0x453190));PUSH32(esp,target==0x8e885?0x8e894:target==0x8e895?0x8e8a4:0x8e8ba);sub_002D6802();
   if(target==0x8e895)TOP=MEMF(0x44bf94)-TOP;
   if(target==0x8e8ab){scalar_push(TOP);scalar_push(MEMF(0x44c1f8));g_fp_cmp=RECOMP_FCMP(TOP,g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;if(g_fp_cmp<=0){TOP-=MEMF(0x44c1f8);TOP+=TOP;TOP=MEMF(0x44bf94)-TOP;}else TOP+=TOP;g_recomp_zf=g_fp_cmp==0;}
   esp+=4;return 1;
  case 0x8e8da:break;
  default:g_seh_ebp=frame;RECOMP_ITAIL(target);return 1;
  }
 }
 scalar_push(MEMF(0x44c174));g_recomp_zf=equal;esp+=4;return 1;
}
#undef TOP
