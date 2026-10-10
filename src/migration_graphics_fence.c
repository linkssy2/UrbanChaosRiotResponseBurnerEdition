#include "migration_graphics_fence.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void),sub_003E9380(void);
static int active(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_FENCE");flag=v&&!strcmp(v,"1");}return flag;}
static int valid(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Publish the original fence packet, both tracking rings, then the next serial.
 * The public kick still owns host consumption; publishing never completes work. */
int uc_migration_graphics_fence(void){
 if(!active())return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10100||!valid(s-128,140)||!valid(0x51c178,1)||MEM8(0x51c178)||!valid(0x3f4238,4)||!valid(MEM32(0x3f4238),0x264))return 0;
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f4238);g_eax=MEM32(g_esi);int grow=g_eax>=MEM32(g_esi+4);PUSH32(g_esp,g_edi);
 if(grow){PUSH32(g_esp,0x3e94b4);sub_003E9810();}
 g_edi=MEM32(g_esi+44);MEM32(g_eax)=0x4a310;g_edx=g_edi&31;g_ecx=(g_eax*8)|g_edx;g_edx=MEM32(g_esi+64);g_ecx<<=2;g_edx&=3;g_ecx|=g_edx;SET_LO8(g_edx,MEM8(g_esp+12));
 MEM32(g_eax+4)=g_ecx;MEM32(g_eax+8)=0x41d70;MEM32(g_eax+12)=g_edi;g_ecx=0x41d90;MEM32(g_eax+16)=g_ecx;MEM32(g_eax+20)=0;MEM32(g_eax+24)=g_ecx;MEM32(g_eax+28)=0;
 g_ecx=g_eax+32;MEM32(g_esi)=g_ecx;g_ecx=(g_edi>>1)&63;MEM32(g_esi+g_ecx*8+0x64)=g_edi;MEM32(g_esi+g_ecx*8+0x68)=g_eax;
 if(g_edx&1){
  g_ecx=MEM32(g_esi+56);PUSH32(g_esp,frame);frame=MEM32(g_esi+52)+1;g_ecx&=frame;frame=MEM32(g_esi+72);MEM32(g_esi+52)=g_ecx;MEM32(frame+g_ecx*8)=g_edi;frame=MEM32(g_esi+72);MEM32(frame+g_ecx*8+4)=g_eax;POP32(g_esp,frame);
 }
 g_ecx=MEM32(g_esi+44)+2;int kick=(g_edx&2)==0;MEM32(g_esi+60)=0;MEM32(g_esi+44)=g_ecx;
 if(kick){g_ecx=g_esi;PUSH32(g_esp,0x3e9551);sub_003E9380();}
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=kick;g_esp+=8;return 1;
}
