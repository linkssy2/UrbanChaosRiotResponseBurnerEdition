#include "migration_graphics_flush.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E3A80(void);
static int flush_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_FLUSH");flag=v&&!strcmp(v,"1");}return flag;}
static int flush_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* The host notification orders RAM and acknowledges PFB_WBC.FLUSH only.
 * Pushbuffer consumption, DMA progress and public kick wrappers stay intact. */
static void flush_barrier(int use_edx){
 g_eax=MEM32(MEM32(0x3f4238)+0x1c28);
 if(use_edx){g_edx=MEM32(g_eax+0x100410)|0x10000;MEM32(g_eax+0x100410)=g_edx;}
 else{g_ecx=MEM32(g_eax+0x100410)|0x10000;MEM32(g_eax+0x100410)=g_ecx;}
 RECOMP_PFB_FLUSH_WRITE(g_eax+0x100410);
 while(MEM32(g_eax+0x100410)&0x10000){}
}
int uc_migration_graphics_flush(uint32_t entry){
 if(!flush_enabled()||(entry!=0x3e9320&&entry!=0x3e9380&&entry!=0x3e9430))return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10100||!flush_span(s-32,40)||!flush_span(0x51c178,1)||MEM8(0x51c178)||!flush_span(0x3f4238,4))return 0;
 uint32_t device=MEM32(0x3f4238);if(!flush_span(device,0x1c2c)||MEM32(device+0x1c28)!=0xfd000000u)return 0;
 if(entry!=0x3e9320){if(!flush_span(g_ecx,0x2480))return 0;uint32_t dma=MEM32(g_ecx+0x1c20);if(!(dma>=0xfd000000u&&dma<=0xfe000000u-72)&&!flush_span(dma,72))return 0;if((MEM32(g_ecx+8)&0x2000)&&!flush_span(MEM32(g_ecx+48),4))return 0;}
 if(entry==0x3e9320){flush_barrier(0);g_recomp_zf=1;g_esp+=4;return 1;}
 if(entry==0x3e9380){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+8);g_edx=MEM32(g_esi+((g_eax&4)?0x770:0));
  if(!(g_eax&0x2000)){
   flush_barrier(0);g_ecx=MEM32(g_esi+0x1c20);g_edx&=0xfffffff;MEM32(g_ecx+64)=g_edx;g_eax=MEM32(0x3f3d90);
   if(g_eax){do{PUSH32(g_esp,0x3e93e9);sub_003E3A80();}while(g_eax);MEM32(g_esi+8)|=0x2000;}
  }else{
   g_eax=MEM32(g_esi);g_ecx=0x3f3d40;MEM32(g_esi+0x1c20)=g_ecx;g_eax&=0xfffffff;MEM32(g_ecx+64)=g_eax;g_edx=MEM32(g_esi+0x1c20);MEM32(g_edx+68)=g_eax;g_eax=MEM32(g_esi+44);g_ecx=MEM32(g_esi+48);g_eax-=2;MEM32(g_ecx)=g_eax;g_edx=MEM32(g_esi+0x2478);MEM32(g_esi+0x1de4)=g_edx;
  }
 }else{
  g_eax=MEM32(g_ecx+8);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+((g_eax&4)?0x770:0));
  if(!(g_eax&0x2000)){flush_barrier(1);g_ecx=MEM32(g_ecx+0x1c20);g_esi&=0xfffffff;MEM32(g_ecx+64)=g_esi;do{PUSH32(g_esp,0x3e9495);sub_003E3A80();}while(g_eax);}
 }
 POP32(g_esp,g_esi);g_esp+=4;return 1;
}
