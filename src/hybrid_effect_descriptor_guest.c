#include "hybrid_effect_descriptor_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_EFFECT_DESCRIPTOR");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static void vector_copy(unsigned offset){g_xmm0=XMM_MEM(g_ecx+offset);XMM_STORE(g_eax+offset,g_xmm0);}
static void words(unsigned begin,unsigned end){for(unsigned i=begin;i<end;i+=4){g_edx=MEM32(g_ecx+i);MEM32(g_eax+i)=g_edx;}}
static void byte_copy(unsigned i){SET_LO8(g_edx,MEM8(g_ecx+i));MEM8(g_eax+i)=LO8(g_edx);}
static void parameters(void){
 g_eax=g_ecx;g_ecx=MEM32(g_esp+4);words(0,0x1c);
 vector_copy(0x20);vector_copy(0x30);vector_copy(0x40);
 words(0x50,0x6c);byte_copy(0x6c);words(0x70,0x74);byte_copy(0x74);byte_copy(0x75);byte_copy(0x76);
 vector_copy(0x80);words(0x90,0x98);byte_copy(0x98);words(0x9c,0xa0);
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edx=g_ecx+0xa0;g_edi=MEM32(g_edx);g_esi=g_eax+0xa0;MEM32(g_esi)=g_edi;
 g_edi=MEM32(g_edx+4);MEM32(g_esi+4)=g_edi;g_edi=MEM32(g_edx+8);MEM32(g_esi+8)=g_edi;g_edx=MEM32(g_edx+12);MEM32(g_esi+12)=g_edx;
 words(0xb0,0xd0);g_ecx=MEM32(g_ecx+0xd0);POP32(g_esp,g_edi);MEM32(g_eax+0xd0)=g_ecx;POP32(g_esp,g_esi);g_esp+=8;
}
int uc_hybrid_effect_descriptor(unsigned entry){
 if(!enabled())return 0;
 unsigned n=entry==0x134510?0xd4:entry==0x134680?0x151:0;uint32_t s=g_esp,d=g_ecx;
 if(!n||(s&3)||s<0x10020u||!span(s-32,40)||!span(d,n)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t a=MEM32(s+4);
 if(!span(a,n)||!apart(a,n,s-32,40)||!apart(d,n,s-32,40)||
    (a!=d&&!apart(a,n,d,n)))return 0;
 /* Copy authored fields only: padding is intentionally untouched. Complete
  * nested parameter copy stays native; scratch writes retain the caller ABI. */
 if(entry==0x134510){parameters();return 1;}
 PUSH32(g_esp,g_ebx);g_ebx=g_ecx;PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);
 for(unsigned i=0;i<0x2c;i+=4){uint32_t*reg=i%12==0?&g_eax:i%12==4?&g_ecx:&g_edx;*reg=MEM32(g_esi+i);MEM32(g_ebx+i)=*reg;}
 g_edx=MEM32(g_esi+0x2c);PUSH32(g_esp,g_edi);g_eax=g_esi+0x30;PUSH32(g_esp,g_eax);g_ecx=g_ebx+0x30;MEM32(g_ebx+0x2c)=g_edx;
 PUSH32(g_esp,0x1346db);parameters();
 SET_LO8(g_ecx,MEM8(g_esi+0x110));MEM8(g_ebx+0x110)=LO8(g_ecx);g_esi+=0x111;g_edi=g_ebx+0x111;g_ecx=16;
 memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),64);g_esi+=64;g_edi+=64;g_ecx=0;
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=g_ebx;POP32(g_esp,g_ebx);g_recomp_zf=g_esi==0;g_esp+=8;return 1;
}
