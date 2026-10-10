#include "migration_graphics_progress.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9120(void);
static int progress_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_PROGRESS");flag=v&&!strcmp(v,"1");}return flag;}
static int progress_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_graphics_progress(uint32_t entry){
 if(!progress_enabled()||entry!=0x3e92a0)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!progress_span(s-32,48)||!progress_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t d=MEM32(s+4);if(!progress_span(d,0x1c24)||!progress_span(MEM32(d+48),4))return 0;
 uint32_t dma=MEM32(d+0x1c20);if(!(dma>=0xfd000000u&&dma<=0xfe000000u-72)&&!progress_span(dma,72))return 0;
 if(MEM32(s+8)){uint32_t aperture=MEM32(d+0x934);if(aperture!=0xfd000000u&&!progress_span(aperture,0x400b14))return 0;}
 g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);
 if(g_eax){
  g_edx=MEM32(g_esi+0x934);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+48);
  do{RECOMP_GPU_PROGRESS_READ(g_edx+0x400b10);g_eax=MEM32(g_edx+0x400b10);g_ecx=(MEM32(g_edi)<<2)^g_eax;}while(g_ecx&0x7c);
  POP32(g_esp,g_edi);
 }
 g_edx=MEM32(g_esi+0x1c20);g_eax=MEM32(g_edx+68)|0x80000000u;g_ecx=MEM32(g_esi+36);
 if(g_eax<g_ecx||g_eax>=MEM32(g_esi+40)){
  g_eax=MEM32(MEM32(g_esi+48));g_ecx=MEM32(g_esi+44);g_eax+=2;
  if(g_eax==g_ecx){g_eax=MEM32(g_esi);g_recomp_zf=1;}
  else{PUSH32(g_esp,0x3e9307);sub_003E9120();if(g_eax){g_eax=MEM32(g_eax+4);g_recomp_zf=0;}else g_eax=MEM32(g_esi)+4;}
 }
 POP32(g_esp,g_esi);g_esp+=12;return 1;
}
