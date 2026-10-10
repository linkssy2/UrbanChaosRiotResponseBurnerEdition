#include "migration_services_string_storage_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void),sub_002D5CCD(void);
static int storage_span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_services_string_storage(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_STRING_STORAGE");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x11c80||MEM8(0x51c178)||!storage_span(g_esp-256,272)||!storage_span(g_ecx,28))return 0;
 uint32_t bytes=MEM32(g_esp+8),buffer=MEM32(g_ecx+24)<16?g_ecx+4:MEM32(g_ecx+4);if(bytes&&!storage_span(buffer,bytes))return 0;
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x389700);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=12;g_eax=MEM32(f+8);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=g_eax|15;PUSH32(g_esp,g_edi);g_esi=g_ecx;MEM32(f-16)=g_esp;MEM32(f-20)=g_esi;
 if(g_ebx>0xfffffffe)g_ebx=g_eax;
 else{
  g_edi=MEM32(g_esi+24);uint64_t product=0xaaaaaaabull*g_ebx;g_eax=(uint32_t)product;g_edx=(uint32_t)(product>>32);g_ecx=g_edi>>1;g_edx>>=1;
  if(g_edx<g_ecx){g_eax=0xfffffffe-g_ecx;if(g_edi<=g_eax)g_ebx=g_ecx+g_edi;}
 }
 g_ecx=g_ebx+1;PUSH32(g_esp,g_ecx);MEM32(f-4)=0;g_ebp=f;PUSH32(g_esp,0x11ce9);sub_002D57FB();g_edx=g_eax;g_esp+=4;MEM32(f+8)=g_edx;g_ecx=MEM32(f+12);
 if(g_ecx){
  g_esi=MEM32(g_esi+24)<16?g_esi+4:MEM32(g_esi+4);g_eax=g_ecx;g_ecx>>=2;g_edi=g_edx;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),4*g_ecx);g_esi+=4*g_ecx;g_edi+=4*g_ecx;g_ecx=g_eax&3;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx);g_esi+=g_ecx;g_edi+=g_ecx;g_ecx=0;g_esi=MEM32(f-20);g_ecx=MEM32(f+12);
 }
 if(MEM32(g_esi+24)>=16){g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_ecx);g_ebp=f;PUSH32(g_esp,0x11d56);sub_002D5CCD();g_edx=MEM32(f+8);g_ecx=MEM32(f+12);g_esp+=4;}
 g_eax=g_esi+4;MEM8(g_eax)=0;MEM32(g_eax)=g_edx;MEM32(g_esi+24)=g_ebx;MEM32(g_esi+20)=g_ecx;int equal=g_ebx==16;POP32(g_esp,g_edi);if(g_ebx>=16)g_eax=g_edx;MEM8(g_eax+g_ecx)=0;g_ecx=MEM32(f-12);POP32(g_esp,g_esi);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=f+16;g_recomp_zf=equal;return 1;
}
