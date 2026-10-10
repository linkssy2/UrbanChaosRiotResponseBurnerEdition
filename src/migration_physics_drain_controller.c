/* Original world pending-count reset and paired profiler records around drain. */
#include "migration_physics_drain_controller.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002FA2F0(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_physics_drain_controller(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_DRAIN_CONTROLLER");enabled=v&&!strcmp(v,"1");}
 if(!enabled||!span(g_esp-64,68)||!span(g_ecx,0x88)||!span(MEM32(g_ecx+0x80),16)||!span(0x520208,8))return 0;
 if(!((uint64_t)g_ecx+0x88<=g_esp-64||(uint64_t)g_esp+4<=g_ecx))return 0;
 uint32_t cursor=MEM32(0x520208),limit=MEM32(0x52020c);if(cursor<limit&&!span(cursor,24))return 0;
 PUSH32(g_esp,g_ecx);g_eax=MEM32(0x52020c);PUSH32(g_esp,g_esi);g_esi=MEM32(0x520208);
 if(g_esi<g_eax){MEM32(g_esi)=0x498ce0;uint64_t t=recomp_read_tsc();g_eax=(uint32_t)t;g_edx=(uint32_t)(t>>32);MEM32(g_esp+4)=g_eax;g_eax=MEM32(g_esp+4);MEM32(g_esi+4)=g_eax;g_esi+=12;MEM32(0x520208)=g_esi;}
 MEM32(g_ecx+0x84)=0;g_ecx=MEM32(g_ecx+0x80);PUSH32(g_esp,0x2ed4d2);sub_002FA2F0();g_ecx=MEM32(0x520208);int end=g_ecx<MEM32(0x52020c);POP32(g_esp,g_esi);
 if(end){MEM32(g_ecx)=0x49895c;uint64_t t=recomp_read_tsc();g_eax=(uint32_t)t;g_edx=(uint32_t)(t>>32);MEM32(g_esp)=g_eax;g_edx=MEM32(g_esp);MEM32(g_ecx+4)=g_edx;g_ecx+=12;MEM32(0x520208)=g_ecx;}
 POP32(g_esp,g_ecx);g_esp+=4;return 1;
}
