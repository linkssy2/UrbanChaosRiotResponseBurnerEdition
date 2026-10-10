#include "hybrid_particle_tree_links_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_TREE_LINKS");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_particle_tree_links(unsigned op){
 if(!enabled())return 0;
 uint32_t s=g_esp;
 if(op>2||(s&3)||s<0x11000||!span(s-64,76)||!span(0x51c178,1)||MEM8(0x51c178)||!span(g_ecx,8))return 0;
 if(op<2){
  unsigned side=op?0:8,other=8-side;uint32_t node=MEM32(s+4);
  if(!span(node,0x192)||!span(MEM32(node+side),0x192)||!span(MEM32(node+4),0x192)||!span(MEM32(g_ecx+4),0x192)||!span(MEM32(MEM32(node+side)+other),0x192))return 0;
  g_edx=MEM32(g_esp+4);g_eax=MEM32(g_edx+side);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax+other);MEM32(g_edx+side)=g_esi;g_esi=MEM32(g_eax+other);SET_LO8(g_ebx,MEM8(g_esi+0x191));if(!LO8(g_ebx))MEM32(g_esi+4)=g_edx;
  g_esi=MEM32(g_edx+4);MEM32(g_eax+4)=g_esi;g_ecx=MEM32(g_ecx+4);int zero=g_edx==MEM32(g_ecx+4);
  if(zero)MEM32(g_ecx+4)=g_eax;else{g_ecx=MEM32(g_edx+4);zero=g_edx==MEM32(g_ecx+other);MEM32(g_ecx+(zero?other:side))=g_eax;}
  POP32(g_esp,g_esi);MEM32(g_eax+other)=g_edx;MEM32(g_edx+4)=g_eax;POP32(g_esp,g_ebx);g_recomp_zf=zero;g_esp+=8;return 1;
 }
 if(!span(MEM32(g_ecx),0x192))return 0;
 g_eax=MEM32(g_ecx);SET_LO8(g_edx,MEM8(g_eax+0x191));
 if(LO8(g_edx)){g_eax=MEM32(g_eax+8);MEM32(g_ecx)=g_eax;g_recomp_zf=0;g_esp+=4;return 1;}
 g_edx=MEM32(g_eax);PUSH32(g_esp,g_ebx);SET_LO8(g_ebx,MEM8(g_edx+0x191));
 if(!LO8(g_ebx)){
  g_eax=MEM32(g_edx+8);SET_LO8(g_ebx,MEM8(g_eax+0x191));while(!LO8(g_ebx)){g_edx=g_eax;g_eax=MEM32(g_edx+8);SET_LO8(g_ebx,MEM8(g_eax+0x191));}
  int zero=LO8(g_ebx)==0;MEM32(g_ecx)=g_edx;POP32(g_esp,g_ebx);g_recomp_zf=zero;g_esp+=4;return 1;
 }
 g_eax=MEM32(g_eax+4);SET_LO8(g_edx,MEM8(g_eax+0x191));
 while(!LO8(g_edx)){g_edx=MEM32(g_ecx);if(g_edx!=MEM32(g_eax))break;MEM32(g_ecx)=g_eax;g_eax=MEM32(g_eax+4);SET_LO8(g_edx,MEM8(g_eax+0x191));}
 SET_LO8(g_edx,MEM8(g_eax+0x191));if(!LO8(g_edx))MEM32(g_ecx)=g_eax;
 POP32(g_esp,g_ebx);g_recomp_zf=LO8(g_edx)==0;g_esp+=4;return 1;
}
