#include "hybrid_particle_asset_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023DA10(void),sub_003878F0(void);
static int asset_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_ASSET");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_particle_asset(unsigned operation){
 if(!asset_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if(operation>1||(s&3)||s<0x10400||!span(s-1024,1040)||!span(p,12)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(operation==0){
  if(!span(MEM32(p+4),0x192))return 0;
  g_eax=MEM32(g_ecx+4);PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax+4);PUSH32(g_esp,g_edi);g_edi=g_eax;SET_LO8(g_eax,MEM8(g_esi+0x191));
  if(!LO8(g_eax)){
   PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+16);
   do{
    g_eax=MEM32(g_ebx);g_ecx=MEM32(g_esi+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23da3c);sub_003878F0();g_esp+=8;
    if((int32_t)g_eax<0)g_esi=MEM32(g_esi+8);else{g_edi=g_esi;g_esi=MEM32(g_esi);}
    SET_LO8(g_eax,MEM8(g_esi+0x191));
   }while(!LO8(g_eax));
   POP32(g_esp,g_ebx);
  }
  g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=0;g_esp+=8;return 1;
 }
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=g_esp+16;g_esi=g_edi+4;PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x23de34);sub_0023DA10();
 int missing=g_eax==MEM32(g_esi+4);MEM32(g_esp+8)=g_eax;
 if(!missing){
  g_eax=MEM32(g_eax+16);g_ecx=MEM32(g_esp+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23de4b);sub_003878F0();g_esp+=8;missing=(int32_t)g_eax<0;
 }
 if(missing){g_edx=MEM32(g_esi+4);MEM32(g_esp+16)=g_edx;g_eax=g_esp+16;}else g_eax=g_esp+8;
 g_ecx=MEM32(g_edi+8);g_eax=MEM32(g_eax);missing=g_eax==g_ecx;POP32(g_esp,g_edi);POP32(g_esp,g_esi);
 if(missing)g_eax=0;else g_eax+=32;
 POP32(g_esp,g_ecx);g_recomp_zf=g_eax==0;g_esp+=8;return 1;
}
