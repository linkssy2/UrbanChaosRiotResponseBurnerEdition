#include "migration_graphics_light_math.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E8990(void);
static int light_math_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_LIGHT_MATH");flag=v&&!strcmp(v,"1");}return flag;}
static int light_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define FP_BACK(n) g_fp_stack[(top+8-(n))&7]
static void reciprocal_length(void){
 unsigned top=(unsigned)g_fp_top;PUSH32(g_esp,g_ecx);
 g_eax=(uint32_t)((int32_t)(0xbe800000u-MEM32(g_esp+8))>>1);MEM32(g_esp)=g_eax;
 /* Original two refinement steps, including the intervening float spill
  * and absolute-value bit operation. Retain inactive x87 slots too. */
 double half=(double)MEMF(0x4eaa78)*MEMF(g_esp+8),guess=MEMF(g_esp);
 FP_BACK(4)=half;FP_BACK(3)=(guess*guess)*half;FP_BACK(2)=guess;
 FP_BACK(1)=((double)MEMF(0x4eaa7c)-FP_BACK(3))*guess;
 MEMF(g_esp)=(float)FP_BACK(1);g_eax=MEM32(g_esp)&0x7fffffff;MEM32(g_esp)=g_eax;
 double value=MEMF(g_esp);FP_BACK(1)=value*value;FP_BACK(1)*=MEMF(g_esp+8);FP_BACK(1)=(double)MEMF(0x46c31c)-FP_BACK(1);FP_BACK(1)*=MEMF(g_esp);FP_BACK(1)*=MEMF(0x44c1f8);
 g_fp_top=(top+7)&7;POP32(g_esp,g_ecx);g_recomp_zf=g_eax==0;g_esp+=8;
}
static void normalize_light(void){
 unsigned top=(unsigned)g_fp_top;g_edx=MEM32(g_esp+4);FP_BACK(1)=MEMF(g_edx+8);PUSH32(g_esp,g_ecx);FP_BACK(2)=MEMF(g_edx+4);FP_BACK(3)=MEMF(g_edx);
 FP_BACK(4)=FP_BACK(3)*FP_BACK(3);FP_BACK(5)=FP_BACK(2)*FP_BACK(2);FP_BACK(4)+=FP_BACK(5);FP_BACK(5)=FP_BACK(1)*FP_BACK(1);FP_BACK(4)+=FP_BACK(5);MEMF(g_esp)=(float)FP_BACK(4);
 PUSH32(g_esp,0x3e8a1b);sub_003E8990();
 unsigned at=(unsigned)g_fp_top;double factor=g_fp_stack[at];
 g_fp_stack[(at+7)&7]=factor*MEMF(g_edx);MEMF(g_edx)=(float)g_fp_stack[(at+7)&7];
 g_fp_stack[(at+7)&7]=factor*MEMF(g_edx+4);MEMF(g_edx+4)=(float)g_fp_stack[(at+7)&7];
 g_fp_stack[at]*=MEMF(g_edx+8);MEMF(g_edx+8)=(float)g_fp_stack[at];g_fp_top=(at+1)&7;g_esp+=8;
}
static void light_mask(void){
 g_ecx=MEM32(0x3f4144);g_eax=0;
 if(g_ecx){g_eax=MEM32(0x3f4148);for(unsigned i=1;i<8;i++){g_ecx=MEM32(0x3f4148+4*i);g_eax=(g_eax<<2)|g_ecx;}
  g_ecx=MEM32(0x3f4238);g_ecx=MEM32(g_ecx+0x794);g_ecx=MEM32(g_ecx+4);
  const uint32_t masks[]={0xffffffaa,0xffffff55,0xffffaaff,0xffff55ff};for(unsigned i=0;i<4;i++)if(!(g_ecx&(0x400u<<i)))g_eax&=masks[i];
 }g_esp+=4;
}
int uc_migration_graphics_light_math(uint32_t entry){
 if(!light_math_enabled())return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!light_span(s-16,28)||!light_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7)return 0;
 switch(entry){
 case 0x3e8990:if(!light_span(0x4eaa78,8)||!light_span(0x46c31c,4)||!light_span(0x44c1f8,4))return 0;reciprocal_length();break;
 case 0x3e89f0:if(!light_span(MEM32(s+4),12))return 0;normalize_light();break;
 case 0x3ecef0:if(!light_span(0x3f4144,0xf8)||!light_span(MEM32(0x3f4238),0x798)||!light_span(MEM32(MEM32(0x3f4238)+0x794),8))return 0;light_mask();break;
 default:return 0;
 }return 1;
}
