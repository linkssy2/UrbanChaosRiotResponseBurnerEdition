#include "migration_animation_asset_owner_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D7290(void),sub_002D76F3(void),sub_001F24B0(void),sub_001F0C60(void),sub_001F35F0(void),sub_001F0340(void),sub_001F0AF0(void);
static int asset_owner_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int asset_owner_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_migration_animation_asset_owner(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_ASSET_OWNER");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;if(!enabled||entry!=0x1f3ff0||(s&3)||s<0x12000||!asset_owner_span(s-8192,8204)||!asset_owner_span(p,120)||!asset_owner_apart(p,120,s-8192,8204)||!asset_owner_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t name=MEM32(s+4),path=MEM32(s+8),length=0;if(!asset_owner_span(name,16)||!asset_owner_apart(name,16,s-8192,8204))return 0;
 while(length<4096){if(!asset_owner_span(path+length,1))return 0;if(!MEM8(path+length))break;++length;}
 if(length<3||length==4096||!asset_owner_apart(path,length+1,s-8192,8204)||!asset_owner_apart(path,length+1,p,120))return 0;
 /* Keep loading, name fallback and close synchronous. The owner reads their
  * actual return state instead of replaying effects after a callback. */
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39b919);g_eax=MEM32(0);PUSH32(g_esp,g_eax);g_eax=0x1004;MEM32(0)=g_esp;PUSH32(g_esp,0x1f400f);sub_002D7290();
 PUSH32(g_esp,g_ebx);g_ebx=0;PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+12)=g_esi;MEM32(g_esi+8)=0;MEM32(g_esi)=0;MEM32(g_esi+4)=0;MEM32(g_esi+12)=0;g_ecx=g_esi+76;PUSH32(g_esp,0);MEM32(g_esp+0x101c)=0;MEM32(g_esi+16)=0;PUSH32(g_esp,0x1f4038);sub_001F24B0();
 g_eax=MEM32(g_esp+0x1024);g_edx=g_esp+16;MEM8(g_esp+0x1018)=1;g_edx-=g_eax;
 do{SET_LO8(g_ecx,MEM8(g_eax));MEM8(g_edx+g_eax)=LO8(g_ecx);++g_eax;}while(LO8(g_ecx)!=LO8(g_ebx));
 g_eax=g_esp+16;g_edx=g_eax+1;do{SET_LO8(g_ecx,MEM8(g_eax));++g_eax;}while(LO8(g_ecx)!=LO8(g_ebx));g_eax-=g_edx;g_eax+=g_esp+13;PUSH32(g_esp,0x47faf8);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f4079);sub_002D76F3();g_esp+=8;
 if(!g_eax){g_eax=g_esp+16;g_edx=g_eax+1;do{SET_LO8(g_ecx,MEM8(g_eax));++g_eax;}while(LO8(g_ecx)!=LO8(g_ebx));g_eax-=g_edx;MEM32(g_esp+g_eax+13)=0x61666b;}
 g_ecx=g_esp+16;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f40a2);sub_001F0C60();g_esp+=4;g_edi=g_eax;PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x1f40af);sub_001F35F0();
 if(MEM32(g_esi)==g_ebx){g_edx=MEM32(g_esp+0x1020);PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,0x1f40c2);sub_001F0340();}
 PUSH32(g_esp,g_edi);PUSH32(g_esp,0x1f40c8);sub_001F0AF0();g_ecx=MEM32(g_esp+0x1014);g_esp+=4;POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=0x1010;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
