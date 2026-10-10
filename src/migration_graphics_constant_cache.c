#include "migration_graphics_constant_cache.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003EEE90(void);
static int cache_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_CONSTANT_CACHE");flag=v&&!strcmp(v,"1");}return flag;}
static int cache_span(uint64_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&p+n<=g_xbox_total_ram;}
int uc_migration_graphics_constant_cache(uint32_t entry){
 if(!cache_enabled()||entry!=0x9c830)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!cache_span(s-80,96)||!cache_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t index=MEM32(s+4);if(!cache_span(0x51a7e4ull+index,1)||!cache_span(0x51a800ull+(uint64_t)index*16,16)||!cache_span(MEM32(s+8),16))return 0;
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-8;PUSH32(g_esp,g_esi);g_esi=MEM32(frame+8);SET_LO8(g_eax,MEM8(g_esi+0x51a7e4));PUSH32(g_esp,g_edi);g_edi=MEM32(frame+12);
 if(LO8(g_eax)){
  g_xmm0=XMM_MEM(g_edi);g_eax=(g_esi<<4)+0x51a800;g_xmm1=XMM_MEM(g_eax);g_xmm1=XMM_CMP_NEQ(g_xmm1,g_xmm0);g_ecx=XMM_MOVEMASK(g_xmm1);
  if(!g_ecx){SET_LO8(g_eax,0);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=4;return 1;}
 }
 g_xmm0=XMM_MEM(g_edi);g_ecx=MEM32(0x519f60);g_edx=(g_esi<<4)+0x51a800;XMM_STORE(g_edx,g_xmm0);MEM8(g_esi+0x51a7e4)=1;g_eax=MEM32(g_ecx);
 {uint32_t saved=g_esp,target=MEM32(g_eax+0xa4);PUSH32(g_esp,0x9c894);RECOMP_ICALL_SAFE(target,saved);}
 g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp,target=MEM32(g_edx+28);PUSH32(g_esp,0x9c89b);RECOMP_ICALL_SAFE(target,saved);}
 PUSH32(g_esp,1);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x9c8a4);sub_003EEE90();POP32(g_esp,g_edi);SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_edx==0;g_esp+=4;return 1;
}
