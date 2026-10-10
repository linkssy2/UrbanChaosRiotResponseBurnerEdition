#include "migration_interface_plainlife.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D5CCD(void);
int migration_interface_plainlife(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_PLAINLIFE");enabled=v&&v[0]=='1'&&!v[1];}
 if(!enabled||operation>2||g_ecx<0x10000||(uint64_t)g_ecx+12>g_xbox_total_ram||g_esp<0x10100||(g_esp&3)||(uint64_t)g_esp+8>g_xbox_total_ram)return 0;
 static const unsigned table[]={0x452f14,0x485a44,0x487548},returns[]={0x70ba6,0x258426,0x2620d6};
 int release=MEM8(g_esp+4)&1;PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi)=table[operation];if(release){PUSH32(g_esp,g_esi);PUSH32(g_esp,returns[operation]);sub_002D5CCD();g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
}
