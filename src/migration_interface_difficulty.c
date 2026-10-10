#include "migration_interface_difficulty.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_000643E0(void);
int migration_interface_difficulty(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_DIFFICULTY");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation>4||g_esp<0x10100||(g_esp&3)||(uint64_t)g_esp+8>g_xbox_total_ram)return 0;
 unsigned out=MEM32(g_esp+4);if(out<0x10000||(uint64_t)out+8>g_xbox_total_ram)return 0;
 static const unsigned returns[]={0x270bf5,0x270c15,0x270c35,0x270c55,0x270c75};PUSH32(g_esp,returns[operation]);sub_000643E0();if(operation==1){g_ecx=1;MEM32(g_eax+72)=g_ecx;}else if(operation==4){g_ecx=MEM32(0x51d0d0);MEM32(g_eax+72)=g_ecx;}else MEM32(g_eax+72)=operation;g_eax=MEM32(g_esp+4);MEM32(g_eax)=0;MEM32(g_eax+4)=operation==1?g_ecx:1;g_esp+=4;return 1;
}
