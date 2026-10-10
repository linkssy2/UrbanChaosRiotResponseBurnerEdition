#include "migration_services_unwind_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
int uc_migration_services_unwind(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_UNWIND");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x2d695d)return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10080||s>=0x80000000u||(uint64_t)s+24>g_xbox_total_ram||g_xbox_total_ram<=0x51c178||MEM8(0x51c178))return 0;
 unsigned count=MEM32(s+12),stride=MEM32(s+8);uint32_t end=MEM32(s+4);
 uint64_t bytes=(uint64_t)count*stride;
 if(count>4096||end<0x10000||end>g_xbox_total_ram||bytes>end-0x10000)return 0;
 if(bytes&&(uint64_t)end>s-128u&&(uint64_t)end-bytes<s+24u)return 0;
 uint32_t f=s-4,old=g_seh_ebp;
 PUSH32(g_esp,20);PUSH32(g_esp,0x496f68);PUSH32(g_esp,0x2d6969);PUSH32(g_esp,0x2d7ccc);
 g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(f)=old;g_esp-=20;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(f-24)=g_esp;MEM32(g_esp-4)=0x2d6969;
 MEM32(f-8)=0x496f68;MEM32(f-4)=0;g_eax=f-16;MEM32(0)=g_eax;g_seh_ebp=f;g_recomp_zf=0;
 /* The caller supplies the end of the successfully constructed prefix.
  * Reload every loop control field after a destructor callback. */
 while(1){
  uint32_t remaining=MEM32(f+16)-1;MEM32(f+16)=remaining;if((int32_t)remaining<0)break;
  g_ecx=MEM32(f+8)-MEM32(f+12);MEM32(f+8)=g_ecx;uint32_t saved=g_esp,target=MEM32(f+20);
  PUSH32(g_esp,0x2d697e);RECOMP_ICALL_SAFE(target,saved);
 }
 MEM32(f-4)=0xffffffffu;g_seh_ebp=f;PUSH32(g_esp,0x2d6989);MEM32(0)=MEM32(f-16);
 POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);MEM32(g_esp-4)=g_ecx;g_esp+=20;return 1;
}
