#include "migration_interface_missionlife.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00270280(void),sub_002D5CCD(void),sub_000643E0(void),sub_00063C20(void),sub_0006E040(void);
static int ml_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_missionlife(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_MISSIONLIFE");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!ml_span(s-1024,1040)||!ml_span(g_ecx,280)||MEM32(g_ecx+28)>16)return 0;unsigned child=MEM32(g_ecx+268);if(child&&(!ml_span(child,4)||!ml_span(MEM32(child),4)))return 0;
 if(!op){PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x270268);sub_00270280();if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x270275);sub_002D5CCD();g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39fcd4);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x4885fc;
 g_eax=MEM32(g_esi+28);g_ecx=g_eax*3+9;g_edx=MEM32(g_esi+g_ecx*4);g_eax*=3;g_ecx=MEM32(g_esi+g_eax*4+32);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);MEM32(g_esp+24)=2;PUSH32(g_esp,0x2702c3);sub_000643E0();g_ecx=g_eax;PUSH32(g_esp,0x2702ca);sub_00063C20();g_edx=MEM32(g_eax);MEM32(0x51d0d4)=g_edx;g_eax=MEM32(g_eax+4);MEM32(0x51d0d8)=g_eax;
 g_ecx=MEM32(g_esi+268);if(g_ecx){g_edx=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,1);unsigned target=MEM32(g_edx);PUSH32(g_esp,0x2702ea);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=g_esi+256;MEM8(g_esp+16)=1;PUSH32(g_esp,0x2702fa);sub_0006E040();g_ecx=g_esi+244;MEM8(g_esp+16)=0;PUSH32(g_esp,0x27030a);sub_0006E040();g_ecx=MEM32(g_esp+8);MEM32(g_esi)=0x450708;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
