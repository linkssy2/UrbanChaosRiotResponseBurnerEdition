#include "migration_services_heap_support_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
int uc_migration_services_heap_support(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_HEAP_SUPPORT");enabled=p&&!strcmp(p,"1");}
 unsigned s=g_esp;if(!enabled||MEM8(0x51c178)||s<0x10010||s>=0x80000000u||(uint64_t)s+4>g_xbox_total_ram)return 0;
 if(entry==0x86cf0){if(g_ecx)g_eax=RECOMP_BSF32(g_ecx);g_recomp_zf=!g_ecx;g_esp+=4;return 1;}
 if(entry!=0x842c5&&entry!=0x844b9)return 0;
 unsigned offset=entry==0x842c5?26:25;if(g_seh_ebp<0x10020||g_seh_ebp>g_xbox_total_ram||g_ebx<0x10000||(uint64_t)g_ebx+0x584>g_xbox_total_ram)return 0;
 int empty=!MEM8(g_seh_ebp-offset);if(!empty){unsigned saved=g_esp;PUSH32(g_esp,MEM32(g_ebx+0x580));unsigned target=MEM32(0x44b3ec);PUSH32(g_esp,entry==0x842c5?0x842d7:0x844cb);RECOMP_ICALL_SAFE(target,saved);}
 g_recomp_zf=empty;g_esp+=4;return 1;
}
