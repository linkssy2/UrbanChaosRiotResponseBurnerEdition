#include "migration_services_crt_thread_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00084C7C(void),sub_00083B5D(void),sub_002D86E8(void),sub_002DB134(void);
static void zero_allocate(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8)*MEM32(g_esp+12);if(!g_esi)g_esi=1;
 for(;;){
  g_eax=0;
  if(g_esi<=0xffffffe0u){
   if(MEM32(0x520a40)==3)g_esi=(g_esi+15)&~15u;
   PUSH32(g_esp,g_esi);PUSH32(g_esp,8);PUSH32(g_esp,0x2d8525);sub_00084C7C();PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2d852b);sub_00083B5D();if(g_eax)break;
  }
  if(!MEM32(0x51e6cc))break;
  PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2d853e);sub_002D86E8();int retry=!!g_eax;POP32(g_esp,g_ecx);if(!retry)break;
 }
 POP32(g_esp,g_esi);g_esp+=4;
}
static void thread_data(void){
 g_eax=MEM8(0x24);
 if((uint8_t)g_eax>=2){uint32_t saved=g_esp;PUSH32(g_esp,10);uint32_t target=MEM32(0x44b48c);PUSH32(g_esp,0x2d93d0);RECOMP_ICALL_SAFE(target,saved);}
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=MEM32(0x28);
 if(MEM32(g_eax+0x28)){g_eax=MEM32(0x519c50);g_ecx=MEM32(4);g_edi=MEM32(g_ecx+g_eax*4)+12;}else g_edi=0x520a4c;
 g_esi=MEM32(g_edi);
 if(!g_esi){
  PUSH32(g_esp,136);PUSH32(g_esp,1);PUSH32(g_esp,0x2d940c);zero_allocate();g_esi=g_eax;POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
  if(g_esi){MEM32(g_edi)=g_esi;MEM32(g_esi+0x54)=0x505030;MEM32(g_esi+0x14)=1;PUSH32(g_esp,0x2d9429);g_eax=MEM32(MEM32(0x28)+0x12c);g_esp+=4;MEM32(g_esi+4)=0xffffffff;MEM32(g_esi)=g_eax;}
  else{PUSH32(g_esp,16);PUSH32(g_esp,0x2d9438);sub_002DB134();POP32(g_esp,g_ecx);}
 }
 POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=4;
}
int uc_migration_services_crt_thread(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_CRT_THREAD");enabled=p&&!strcmp(p,"1");}
 uint32_t s=g_esp;if(!enabled||MEM8(0x51c178)||s<0x10080||s>=0x80000000u||(uint64_t)s+16>g_xbox_total_ram)return 0;
 if(entry==0x2d84f8){zero_allocate();return 1;}
 if(entry!=0x2d93bc&&entry!=0x2db935&&entry!=0x86c5b)return 0;
 uint32_t thread=MEM32(0x28);if(thread<0x10000||(uint64_t)thread+0x130>g_xbox_total_ram)return 0;
 if(entry==0x86c5b){g_eax=MEM32(thread+0x12c);g_esp+=4;return 1;}
 if(MEM32(thread+0x28)){
  uint64_t slot=(uint64_t)MEM32(4)+4ull*MEM32(0x519c50);if(slot<0x10000||slot+4>g_xbox_total_ram)return 0;
  uint32_t tls=MEM32((uint32_t)slot);if(tls<0x10000||(uint64_t)tls+16>g_xbox_total_ram)return 0;
 }
 if(entry==0x2db935){PUSH32(g_esp,0x2db93a);thread_data();g_eax+=8;g_recomp_zf=!g_eax;g_esp+=4;}else thread_data();return 1;
}
