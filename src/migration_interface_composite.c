#include "migration_interface_composite.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D57FB(void),sub_00266C80(void);
static int composite_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_composite(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_COMPOSITE");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!composite_span(s-1024,1040))return 0;
 unsigned n=MEM32(s+(op?12:8)),items=MEM32(s+(op?8:4));if(n>32||(n&&!composite_span(items,n*(op?8:4)))||!composite_span(op?MEM32(s+4):g_ecx,op?8:136))return 0;
 if(!op){PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_eax=g_ecx;g_edx=0;MEM32(g_eax)=0x4877fc;MEM32(g_eax+132)=0;
  if(g_edi){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+16);do{g_ecx=MEM32(g_esi+g_edx*4);if(g_ecx){g_ebx=MEM32(g_eax+132);MEM32(g_eax+g_ebx*4+4)=g_ecx;MEM32(g_eax+132)++;}g_edx++;}while(g_edx<g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);}POP32(g_esp,g_edi);g_esp+=12;return 1;
 }
 g_esp-=128;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+148);g_ecx=0;g_esi=1;
 if(g_edi){g_edx=MEM32(g_esp+144);do{g_eax=MEM32(g_edx+g_ecx*8);MEM32(g_esp+g_ecx*4+8)=0;if(g_eax){MEM32(g_esp+g_ecx*4+8)=g_eax;g_eax=MEM32(g_edx+g_ecx*8+4);if(!(g_eax&1))g_esi=(g_esi&24)|g_eax;g_eax&=24;if(g_eax)g_esi=(g_esi&~24u)|g_eax;}g_ecx++;}while(g_ecx<g_edi);}
 PUSH32(g_esp,136);PUSH32(g_esp,0x266e5a);sub_002D57FB();g_esp+=4;
 if(g_eax){PUSH32(g_esp,g_edi);g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x266e6e);sub_00266C80();g_ecx=g_eax;g_eax=MEM32(g_esp+140);}else{g_eax=MEM32(g_esp+140);g_ecx=0;}
 POP32(g_esp,g_edi);MEM32(g_eax+4)=g_esi;MEM32(g_eax)=g_ecx;POP32(g_esp,g_esi);g_esp+=128;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
