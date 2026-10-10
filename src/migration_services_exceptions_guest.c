#include "migration_services_exceptions_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D83F8(void);
static int exception_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int exception_string(uint32_t p){return exception_span(p,1)&&memchr((void*)XBOX_PTR(p),0,(size_t)(g_xbox_total_ram-p)<4096?(size_t)(g_xbox_total_ram-p):4096)!=NULL;}
static void raise_record(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;g_esp-=80;g_eax=MEM32(f+8);MEM32(f-72)=0;MEM32(f-80)=g_eax;g_eax=MEM32(f+12);PUSH32(g_esp,g_esi);g_esi=MEM32(f+20);g_eax&=1;MEM32(f-76)=g_eax;MEM32(f-68)=0x86a0a;
 if(g_esi){g_ecx=MEM32(f+16);if(g_ecx>15){PUSH32(g_esp,15);POP32(g_esp,g_ecx);}MEM32(f-64)=g_ecx;if(g_ecx){PUSH32(g_esp,g_edi);g_edi=f-60;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),4*g_ecx);g_esi+=4*g_ecx;g_edi+=4*g_ecx;g_ecx=0;POP32(g_esp,g_edi);}}else MEM32(f-64)=0;
 g_eax=f-80;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(0x44b3fc);PUSH32(g_esp,0x86a5b);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);g_esp=f+24;
}
int uc_migration_services_exceptions(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_EXCEPTIONS");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!exception_span(g_esp-256,280))return 0;
 if(entry==0x86a0a){uint32_t data=MEM32(g_esp+16),n=MEM32(g_esp+12);if(data&&!exception_span(data,4*(n>15?15:n)))return 0;raise_record();return 1;}
 if(entry==0x2d619e){
  PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;g_esp-=32;g_eax=MEM32(f+8);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,8);POP32(g_esp,g_ecx);g_esi=0x496ef4;g_edi=f-32;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),32);g_esi+=32;g_edi+=32;g_ecx=0;
  MEM32(f-8)=g_eax;g_eax=MEM32(f+12);MEM32(f-4)=g_eax;g_eax=f-12;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(f-16));PUSH32(g_esp,MEM32(f-28));PUSH32(g_esp,MEM32(f-32));g_ebp=f;PUSH32(g_esp,0x2d61d1);raise_record();POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp=f+16;g_recomp_zf=f+4==0;return 1;
 }
 if(entry!=0x2d61d7&&entry!=0x2d61e8&&entry!=0x2d622d)return 0;
 if(!exception_span(g_ecx,12))return 0;
 if(entry==0x2d61d7){g_eax=g_ecx;MEM32(g_eax+4)=0;MEM32(g_eax+8)=0;MEM32(g_eax)=0x496f14;g_esp+=4;return 1;}
 uint32_t arg=MEM32(g_esp+4);if(!exception_span(arg,entry==0x2d61e8?4:12))return 0;
 if((entry==0x2d61e8||MEM32(arg+8))&&!exception_string(MEM32(arg+(entry==0x2d61e8?0:4))))return 0;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);MEM32(g_esi)=0x496f14;
 int copying=entry==0x2d622d;int owns=1;
 if(copying){g_eax=MEM32(g_edi+8);MEM32(g_esi+8)=g_eax;owns=!!g_eax;g_eax=MEM32(g_edi+4);}else g_eax=MEM32(g_edi);
 if(owns){
  g_ecx=g_eax+1;do{SET_LO8(g_edx,MEM8(g_eax));g_eax++;}while((uint8_t)g_edx);g_eax=g_eax-g_ecx+1;PUSH32(g_esp,g_eax);PUSH32(g_esp,copying?0x2d625b:0x2d620b);sub_002D83F8();POP32(g_esp,g_ecx);MEM32(g_esi+4)=g_eax;
  if(g_eax){if(copying){g_edi=MEM32(g_edi+4);do{SET_LO8(g_ecx,MEM8(g_edi));g_edi++;MEM8(g_eax)=(uint8_t)g_ecx;g_eax++;}while((uint8_t)g_ecx);}else{g_edx=MEM32(g_edi);do{SET_LO8(g_ecx,MEM8(g_edx));g_edx++;MEM8(g_eax)=(uint8_t)g_ecx;g_eax++;}while((uint8_t)g_ecx);}}
 }else MEM32(g_esi+4)=g_eax;
 POP32(g_esp,g_edi);if(!copying)MEM32(g_esi+8)=1;g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
}
