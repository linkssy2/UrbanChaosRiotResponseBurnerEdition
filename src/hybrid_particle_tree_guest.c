#include "hybrid_particle_tree_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023DC00(void),sub_002D5CCD(void),sub_002D57FB(void),sub_00134680(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_TREE");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_particle_tree(unsigned op){
 if(!enabled())return 0;
 uint32_t s=g_esp;
 if(op>3||(s&3)||s<0x11000||!span(s-4096,4120)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(op==0){
  if(!span(g_ecx,4)||!span(MEM32(g_ecx),0x192))return 0;
  g_eax=MEM32(g_ecx);SET_LO8(g_edx,MEM8(g_eax+0x191));
  if(!LO8(g_edx)){
   g_edx=MEM32(g_eax+8);PUSH32(g_esp,g_ebx);SET_LO8(g_ebx,MEM8(g_edx+0x191));
   if(!LO8(g_ebx)){
    g_eax=MEM32(g_edx);SET_LO8(g_ebx,MEM8(g_eax+0x191));
    while(!LO8(g_ebx)){g_edx=g_eax;g_eax=MEM32(g_edx);SET_LO8(g_ebx,MEM8(g_eax+0x191));}
    int zero=LO8(g_ebx)==0;MEM32(g_ecx)=g_edx;POP32(g_esp,g_ebx);g_recomp_zf=zero;g_esp+=4;return 1;
   }
   g_eax=MEM32(g_eax+4);SET_LO8(g_edx,MEM8(g_eax+0x191));
   while(!LO8(g_edx)){
    g_edx=MEM32(g_ecx);if(g_edx!=MEM32(g_eax+8))break;
    MEM32(g_ecx)=g_eax;g_eax=MEM32(g_eax+4);SET_LO8(g_edx,MEM8(g_eax+0x191));
   }
   MEM32(g_ecx)=g_eax;POP32(g_esp,g_ebx);
  }
  g_esp+=4;return 1;
 }
 if(op==1){
  if(!span(g_ecx,4)||!span(MEM32(s+4),0x192))return 0;
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);SET_LO8(g_eax,MEM8(g_edi+0x191));g_ebx=g_ecx;g_esi=g_edi;
  if(!LO8(g_eax))do{
   g_eax=MEM32(g_esi+8);PUSH32(g_esp,g_eax);g_ecx=g_ebx;PUSH32(g_esp,0x23dc20);sub_0023DC00();g_esi=MEM32(g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x23dc28);sub_002D5CCD();SET_LO8(g_eax,MEM8(g_esi+0x191));g_esp+=4;g_edi=g_esi;
  }while(!LO8(g_eax));
  int zero=LO8(g_eax)==0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=zero;g_esp+=8;return 1;
 }
 if(op==2){
  PUSH32(g_esp,0x1a0);PUSH32(g_esp,0x23dc4a);sub_002D57FB();g_esp+=4;
  if(g_eax)MEM32(g_eax)=0;g_ecx=g_eax+4;if(g_ecx)MEM32(g_ecx)=0;g_ecx=g_eax+8;if(g_ecx)MEM32(g_ecx)=0;MEM8(g_eax+0x190)=1;MEM8(g_eax+0x191)=0;g_recomp_zf=g_ecx==0;g_esp+=4;return 1;
 }
 if(!span(g_ecx,0x171)||!span(MEM32(s+4),4)||!span(MEM32(s+8),0x161))return 0;
 g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_eax);MEM32(g_esi)=g_ecx;g_ecx=MEM32(g_esp+16);PUSH32(g_esp,g_edi);g_edx=g_ecx;g_ebx=MEM32(g_edx);g_eax=g_esi+16;g_edi=g_eax;MEM32(g_edi)=g_ebx;g_ebx=MEM32(g_edx+4);MEM32(g_edi+4)=g_ebx;g_ebx=MEM32(g_edx+8);g_ecx+=16;MEM32(g_edi+8)=g_ebx;g_edx=MEM32(g_edx+12);PUSH32(g_esp,g_ecx);g_ecx=g_eax+16;MEM32(g_edi+12)=g_edx;PUSH32(g_esp,0x23dd9a);sub_00134680();POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=g_ecx==0;g_esp+=12;return 1;
}
