#include "migration_animation_modifier_lifetime_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void),sub_002D5CCD(void),sub_002D68FB(void),sub_002D69BB(void);
static int modifier_life_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int modifier_life_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_migration_animation_modifier_lifetime(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_MODIFIER_LIFETIME");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;if(!enabled||(s&3)||s<0x10400||!modifier_life_span(s-1024,1032)||!modifier_life_span(p,12)||!modifier_life_apart(p,12,s-1024,1032)||!modifier_life_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x1f4a60){MEM32(p)=0x47fafc;g_esp+=4;return 1;}
 if(entry==0x1f4ac0){g_eax=p;MEM32(p)=0x47fb08;MEM8(p+4)=0;g_esp+=4;return 1;}
 if(entry==0x1f4a40||entry==0x1f4af0){
  int deleting=MEM8(s+4)&1;PUSH32(g_esp,g_esi);g_esi=p;
  if(entry==0x1f4af0){PUSH32(g_esp,0x1f4af8);MEM32(g_ecx)=0x47fafc;g_esp+=4;deleting=MEM8(g_esp+8)&1;}else MEM32(g_esi)=0x47fafc;
  if(deleting){PUSH32(g_esp,g_esi);PUSH32(g_esp,entry==0x1f4af0?0x1f4b05:0x1f4a56);sub_002D5CCD();g_esp+=4;}
  g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
 }
 if(entry==0x1f4b20){
  PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39b97b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;PUSH32(g_esp,0x18010);MEM32(g_edi)=0x47fb30;PUSH32(g_esp,0x1f4b4a);sub_002D57FB();g_esp+=4;MEM32(g_esp+8)=g_eax;g_esi=0;MEM32(g_esp+20)=0;
  if(g_eax){PUSH32(g_esp,0x11030);PUSH32(g_esp,0x1f4b10);PUSH32(g_esp,0x800);g_esi=g_eax+16;PUSH32(g_esp,48);PUSH32(g_esp,g_esi);MEM32(g_eax)=0x800;PUSH32(g_esp,0x1f4b7b);sub_002D68FB();}
  g_ecx=MEM32(g_esp+12);MEM32(g_edi+8)=g_esi;g_esi+=0x18000;MEM32(g_edi+4)=g_esi;g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(entry!=0x1f4eb0)return 0;
 uint32_t buffer=MEM32(p+8);if(buffer&&(!modifier_life_span(buffer-16,16)||MEM32(buffer-16)>2048||!modifier_life_span(buffer,MEM32(buffer-16)*48)||!modifier_life_apart(buffer-16,16+MEM32(buffer-16)*48,s-1024,1032)))return 0;
 PUSH32(g_esp,g_esi);g_esi=p;g_eax=MEM32(g_esi+8);MEM32(g_esi)=0x47fb30;
 if(g_eax){g_ecx=MEM32(g_eax-16);PUSH32(g_esp,g_edi);g_edi=g_eax-16;PUSH32(g_esp,0x11030);PUSH32(g_esp,g_ecx);PUSH32(g_esp,48);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f4ed5);sub_002D69BB();PUSH32(g_esp,g_edi);PUSH32(g_esp,0x1f4edb);sub_002D5CCD();g_esp+=4;POP32(g_esp,g_edi);}
 if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1f4eec);sub_002D5CCD();g_esp+=4;}
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
}
