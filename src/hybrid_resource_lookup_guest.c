#include "hybrid_resource_lookup_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00011EF0(void);
extern void sub_00088210(void);
extern void sub_002D5CCD(void);
static int lookup_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_RESOURCE_LOOKUP");flag=v&&!strcmp(v,"1");}return flag;}
/* Complete88620 temporary-key and result ownership. Original tree/string
 * services remain authoritative; callbacks may mutate the live owner. */
static int lookup_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_resource_lookup(void){
 if(!lookup_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10400u||!lookup_span(s-1024,1044)||!lookup_span(p,8)||!lookup_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t name=MEM32(s+4),out=MEM32(s+8);
 if(!lookup_span(name,1)||!lookup_span(out,4))return 0;
 size_t bound=g_xbox_total_ram-name;if(bound>4096)bound=4096;
 const unsigned char*end=memchr((void*)XBOX_PTR(name),0,bound);if(!end)return 0;
 size_t count=(size_t)(end-(unsigned char*)XBOX_PTR(name))+1;
 if(!((uint64_t)name+count<=s-1024||(uint64_t)s+20<=name)||!((uint64_t)p+8<=s-1024||(uint64_t)s+20<=p))return 0;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38bd78);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=28;
 g_edx=MEM32(g_esp+44);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=0;g_eax=g_edx;PUSH32(g_esp,g_edi);g_edi=g_ecx;
 MEM32(g_esp+36)=15;MEM32(g_esp+32)=g_ebx;MEM8(g_esp+16)=(uint8_t)g_ebx;g_esi=g_eax+1;
 do{SET_LO8(g_ecx,MEM8(g_eax));g_eax++;}while((uint8_t)g_ecx!=(uint8_t)g_ebx);
 g_eax-=g_esi;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);g_ecx=g_esp+20;PUSH32(g_esp,0x8866c);sub_00011EF0();
 g_eax=g_esp+12;PUSH32(g_esp,g_eax);g_ecx=g_esp+60;PUSH32(g_esp,g_ecx);g_ecx=g_edi;MEM32(g_esp+56)=g_ebx;PUSH32(g_esp,0x88681);sub_00088210();
 g_esi=MEM32(g_eax);
 if(MEM32(g_esp+36)>=16){g_edx=MEM32(g_esp+16);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x88694);sub_002D5CCD();g_esp+=4;}
 int found=g_esi!=MEM32(g_edi+4);MEM32(g_esp+36)=15;MEM32(g_esp+32)=g_ebx;MEM8(g_esp+16)=(uint8_t)g_ebx;
 if(!found){POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=0;POP32(g_esp,g_ebx);g_ecx=MEM32(g_esp+28);MEM32(0)=g_ecx;}
 else{
  g_eax=MEM32(g_esi+68);g_ecx=MEM32(g_esp+60);MEM32(g_ecx)=g_eax;
  if(MEM32(g_esi+64)>=16){g_eax=MEM32(g_esi+44);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_ecx=MEM32(g_esp+28);MEM32(0)=g_ecx;}
  else{g_ecx=MEM32(g_esp+40);POP32(g_esp,g_edi);g_eax=g_esi+44;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;}
 }
 g_esp+=40;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
