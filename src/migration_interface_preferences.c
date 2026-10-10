#include "migration_interface_preferences.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_000643E0(void),sub_0005AAB0(void);
static int preferences_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void preferences_call(unsigned target,unsigned ret,unsigned saved){PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
int migration_interface_preferences(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_PREFERENCES");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!preferences_span(s-1024,1028)||!preferences_span(0x51c4e4,4))return 0;
 unsigned service=MEM32(0x51c4e4);if(!preferences_span(service,4)||!preferences_span(MEM32(service),4))return 0;
 g_ecx=service;g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0);PUSH32(g_esp,0x4e5308);preferences_call(MEM32(g_eax),0x16fc3,saved);
 g_esi=g_eax;PUSH32(g_esp,0x16fca);sub_000643E0();g_edi=MEM32(g_esi);PUSH32(g_esp,0x16fd1);sub_000643E0();g_ecx=MEM8(g_eax+69);saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_esi;preferences_call(MEM32(g_edi+12),0x16fdc,saved);
 PUSH32(g_esp,0x16fe1);sub_0005AAB0();g_esi=MEM32(g_eax+324);
 if(g_esi){g_edx=MEM32(g_esi);g_eax=MEM8(0x4e8354);g_ecx=g_esi;saved=g_esp;PUSH32(g_esp,g_eax);preferences_call(MEM32(g_edx+528),0x16ffd,saved);g_edi=MEM32(g_esi);PUSH32(g_esp,0x17004);sub_000643E0();SET_LO8(g_edx,MEM8(g_eax+71));g_ecx=!!LO8(g_edx);saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_esi;preferences_call(MEM32(g_edi+524),0x17017,saved);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;return 1;
}
