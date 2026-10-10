#include "migration_animation_controller_lifetime_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F3580(void),sub_002D5CCD(void);
static int controller_life_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_animation_controller_lifetime(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_CONTROLLER_LIFETIME");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;if(!enabled||(s&3)||s<0x10400||!controller_life_span(s-1024,1032)||!controller_life_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x1f23c0){
  p=MEM32(s+4);if(!controller_life_span(p,4)||!controller_life_span(MEM32(p),60)||!((uint64_t)p+4<=s-1024||(uint64_t)s+8<=p))return 0;
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi);g_ecx=g_esi;
  {uint32_t target=MEM32(g_eax+56);PUSH32(g_esp,0x1f23cc);RECOMP_ICALL_SAFE(target,s);}
  g_edx=MEM32(g_esi);{uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x1f23d4);RECOMP_ICALL_SAFE(target,saved);}
  POP32(g_esp,g_esi);g_esp+=8;return 1;
 }
 if(entry!=0x1f23e0||!controller_life_span(p,56)||!((uint64_t)p+56<=s-1024||(uint64_t)s+8<=p))return 0;
 /* Cache destruction and heap release stay synchronous; the next read sees
  * callback changes, and no committed destruction is replayed. */
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b878);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;MEM32(g_esi)=0x457d20;g_edi=MEM32(g_esi+52);MEM32(g_esp+20)=0;
 if(g_edi){g_ecx=g_edi;PUSH32(g_esp,0x1f241a);sub_001F3580();PUSH32(g_esp,g_edi);PUSH32(g_esp,0x1f2420);sub_002D5CCD();g_esp+=4;}
 g_ecx=MEM32(g_esp+12);MEM32(g_esi+52)=0;MEM32(g_esi)=0x47fa80;POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
