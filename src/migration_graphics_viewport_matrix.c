#include "migration_graphics_viewport_matrix.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E87C0(void);
static int viewport_matrix_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_VIEWPORT_MATRIX");flag=v&&!strcmp(v,"1");}return flag;}
static int viewport_matrix_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define MTOP g_fp_stack[g_fp_top]
#define MST(i) g_fp_stack[(g_fp_top+(i))&7]
#define MPOP() (g_fp_top=(g_fp_top+1)&7)
static void matrix_push(double value){g_fp_top=(g_fp_top+7)&7;MTOP=value;}
int uc_migration_graphics_viewport_matrix(uint32_t entry){
 if(!viewport_matrix_enabled()||entry!=0x3e7240)return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!viewport_matrix_span(s-192,200)||!viewport_matrix_span(0x51c178,1)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7||!viewport_matrix_span(MEM32(0x3f4238),0xef8))return 0;
 uint32_t frame=g_ebp;PUSH32(esp,frame);frame=esp;g_ebp=frame;esp=(esp&~15u)-92;edx=MEM32(0x3f4238);eax=MEM32(edx+0xee8);matrix_push((double)SMEM32(edx+0xee8));PUSH32(esp,edi);if((int32_t)eax<0)MTOP+=MEMF(0x44c510);
 MTOP*=MEMF(edx+0x95c);ecx=MEM32(edx+0xeec);MTOP*=MEMF(0x44c1f8);MEMF(esp+20)=(float)MTOP;matrix_push((double)SMEM32(edx+0xeec));if((int32_t)ecx<0)MTOP+=MEMF(0x44c510);
 MTOP*=MEMF(edx+0x960);eax=MEM32(0x3f41dc);MTOP*=MEMF(0x44e868);MEMF(esp+12)=(float)MTOP;MTOP=-MTOP;MEMF(esp+24)=(float)MTOP;MPOP();matrix_push(MEMF(edx+0xef4));MTOP-=MEMF(edx+0xef0);MTOP*=MEMF(edx+0x948);MEMF(esp+16)=(float)MTOP;MPOP();matrix_push(MEMF(edx+0x948));MTOP*=MEMF(edx+0xef0);MEMF(esp+28)=(float)MTOP;MPOP();
 if(eax==2){matrix_push(MEMF(edx+0x944));MTOP*=MEMF(edx+0x948);matrix_push(MTOP);MST(2)*=MTOP;MPOP();matrix_push(MEMF(esp+12));MTOP*=MST(1);static const unsigned offsets[]={16,20,24,28};for(unsigned i=0;i<4;i++){matrix_push(MST(1));MTOP*=MEMF(esp+offsets[i]);MEMF(esp+offsets[i])=(float)MTOP;MPOP();}}
 else{matrix_push(MEMF(0x44bf94));matrix_push(MEMF(esp+12));}
 eax=0;{double value=MTOP;MTOP=MST(2);MST(2)=value;}ecx=16;edi=esp+32;for(unsigned i=0;i<ecx;i++)MEM32(edi+i*4)=eax;edi+=ecx*4;ecx=0;MEMF(esp+32)=(float)MTOP;MPOP();{double value=MTOP;MTOP=MST(1);MST(1)=value;}MEMF(esp+52)=(float)MTOP;MPOP();eax=MEM32(esp+16);MEMF(esp+92)=(float)MTOP;MPOP();ecx=MEM32(esp+20);MEM32(esp+72)=eax;eax=MEM32(esp+24);MEM32(esp+80)=ecx;ecx=MEM32(esp+28);MEM32(esp+84)=eax;eax=esp+32;MEM32(esp+88)=ecx;PUSH32(esp,eax);ecx=edx+0xca0;PUSH32(esp,ecx);edx+=0x980;PUSH32(esp,edx);g_ebp=frame;PUSH32(esp,0x3e7371);sub_003E87C0();MEM32(0x3f3d98)|=0x200;POP32(esp,edi);esp=frame;POP32(esp,frame);esp+=4;return 1;
}
#undef MTOP
#undef MST
#undef MPOP
