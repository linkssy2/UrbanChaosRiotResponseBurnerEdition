#include "hybrid_resource_provider_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00088620(void);
extern void sub_002D65B0(void);
extern void sub_002D6E2C(void);
extern void sub_00081265(void);
extern void sub_00080F6E(void);
extern void sub_002D57FB(void);
extern void sub_00087C70(void);
static int provider_enabled(void){static int value=-1;if(value<0){const char*s=getenv("UCRR_NATIVE_RESOURCE_PROVIDER");value=s&&!strcmp(s,"1");}return value;}
/* Complete88700 routing owner. All map, string, file and allocator services
 * retain their original frontiers. No parallel native cache or handle owner. */
static int provider_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void provider_open_file(uint32_t name,uint32_t return_address){
 PUSH32(g_esp,0);PUSH32(g_esp,0x8000000);PUSH32(g_esp,3);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,0x80000000u);PUSH32(g_esp,name);PUSH32(g_esp,return_address);sub_00081265();
}
int uc_hybrid_resource_provider_open(void){
 if(!provider_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10400u||!provider_span(s-1024,1040)||!provider_span(p,12)||
 !((uint64_t)p+12<=s-1024||(uint64_t)s+16<=p)||!provider_span(0x51c178,1)||MEM8(0x51c178)||!provider_span(0x519e44,24))return 0;
 PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+4);
 if(!g_eax){g_eax=0;POP32(g_esp,g_edi);g_recomp_zf=1;g_esp+=12;return 1;}
 g_ecx=MEM32(g_edi+8);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);
 if(g_ecx){
  g_eax=g_esp+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x8872b);sub_00088620();
  if(g_eax){
   PUSH32(g_esp,0x40);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x8873b);sub_002D65B0();g_esp+=8;
   if(g_eax){
    g_eax++;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x8874e);sub_002D6E2C();g_ebx=g_eax;g_eax=MEM32(0x519e58);g_esp+=4;
    uint32_t capacity=g_eax;g_eax=MEM32(0x519e44);if(capacity<16)g_eax=0x519e44;
    provider_open_file(g_eax,0x8877f);g_esi=g_eax;
    if(g_esi==0xffffffffu){
     g_edx=MEM32(g_edi);g_ecx=g_edi;uint32_t saved=g_esp,target=MEM32(g_edx+32);PUSH32(g_esp,0x8878d);RECOMP_ICALL_SAFE(target,saved);
     POP32(g_esp,g_ebx);POP32(g_esp,g_esi);g_eax=0;POP32(g_esp,g_edi);g_recomp_zf=1;g_esp+=12;return 1;
    }
    PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x887a0);sub_00080F6E();
    if((int32_t)g_eax<0){g_eax=MEM32(g_edi);g_ecx=g_edi;uint32_t saved=g_esp,target=MEM32(g_eax+32);PUSH32(g_esp,0x887ab);RECOMP_ICALL_SAFE(target,saved);}
    PUSH32(g_esp,20);PUSH32(g_esp,0x887b2);sub_002D57FB();g_esp+=4;
    if(!g_eax){POP32(g_esp,g_ebx);POP32(g_esp,g_esi);g_eax=0;POP32(g_esp,g_edi);g_recomp_zf=1;g_esp+=12;return 1;}
    g_ecx=MEM32(g_esp+16);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x887c7);sub_00087C70();
    POP32(g_esp,g_ebx);POP32(g_esp,g_esi);POP32(g_esp,g_edi);g_recomp_zf=0;g_esp+=12;return 1;
   }
   provider_open_file(g_esi,0x887e5);g_esi=g_eax;
   if(g_esi==0xffffffffu){POP32(g_esp,g_esi);g_eax=0;POP32(g_esp,g_edi);g_recomp_zf=1;g_esp+=12;return 1;}
   PUSH32(g_esp,20);PUSH32(g_esp,0x887fa);sub_002D57FB();g_esp+=4;
   if(!g_eax){POP32(g_esp,g_esi);g_eax=0;POP32(g_esp,g_edi);g_recomp_zf=1;g_esp+=12;return 1;}
   g_edx=MEM32(g_esp+12);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x8880f);sub_00087C70();
   POP32(g_esp,g_esi);POP32(g_esp,g_edi);g_recomp_zf=0;g_esp+=12;return 1;
  }
 }
 g_ecx=MEM32(g_edi+4);g_eax=MEM32(g_ecx);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0x8881d);RECOMP_ICALL_SAFE(target,saved);}
 POP32(g_esp,g_esi);POP32(g_esp,g_edi);g_esp+=12;return 1;
}
