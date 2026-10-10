#include "migration_interface_spriteconstruct.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273410(void),sub_000650D0(void);
int migration_interface_spriteconstruct(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_SPRITECONSTRUCT");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||(uint64_t)s+40>g_xbox_total_ram||g_ecx<0x10000||(uint64_t)g_ecx+80>g_xbox_total_ram)return 0;unsigned pair=MEM32(s+20);if(pair<0x10000||(uint64_t)pair+8>g_xbox_total_ram)return 0;
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x38b7e8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);g_eax=MEM32(g_esp+44);g_edx=MEM32(g_esp+24);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esp+32);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_ecx=g_esi;MEM32(g_esp+20)=g_esi;PUSH32(g_esp,0x70be8);sub_00273410();g_ecx=MEM32(g_esp+36);PUSH32(g_esp,g_ecx);MEM32(g_esp+20)=0;MEM32(g_esi)=0x452f1c;PUSH32(g_esp,0x70c00);sub_000650D0();g_ecx=MEM32(g_esp+48);MEM32(g_esi+60)=g_eax;g_eax=MEM32(g_esp+44);g_edx=MEM32(g_eax);MEM32(g_esi+64)=g_edx;g_eax=MEM32(g_eax+4);SET_LO8(g_edx,MEM8(g_esp+56));MEM32(g_esi+68)=g_eax;MEM32(g_esi+72)=g_ecx;g_ecx=MEM32(g_esp+12);g_esp+=4;MEM8(g_esi+76)=LO8(g_edx);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=36;return 1;
}
