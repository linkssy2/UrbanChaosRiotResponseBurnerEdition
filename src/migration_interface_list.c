#include "migration_interface_list.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002C3740(void),sub_002D57FB(void),sub_00273630(void),sub_002BCFA0(void),sub_000722A0(void),sub_000643E0(void),sub_0025A2E0(void),sub_000650D0(void),sub_00274770(void),sub_002669F0(void);
static int list_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void list_construct(void){
 unsigned frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39f841);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=12;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+16)=g_esi;g_eax=MEM32(g_esp+52);g_ecx=MEM32(g_esp+56);g_edx=MEM32(g_esp+60);g_edi=MEM32(g_esp+44);MEM32(g_esi+88)=g_eax;g_eax=MEM32(g_esp+64);MEM32(g_esi+172)=g_ecx;g_ebx=g_esi+220;frame=0;g_ecx=g_ebx;MEM32(g_esp+36)=frame;MEM32(g_esi)=0x4877b4;MEM32(g_esi+4)=g_edi;MEM32(g_esi+180)=g_edx;MEM32(g_esi+200)=g_eax;PUSH32(g_esp,0x266a57);sub_002C3740();
 MEM32(g_ebx+4)=g_eax;MEM32(g_ebx+8)=frame;g_xmm0=XMM_ZERO();MEM8(g_esp+36)=1;MEM8(g_esi+176)=0;MEMF(g_esi+212)=g_xmm0.f[0];MEM8(g_esi+216)=0;MEM32(g_esi+184)=frame;MEM32(g_esi+188)=frame;MEM32(g_esp+52)=frame;
 if(MEM32(g_esi+88)){g_edi=g_esi+92;MEM32(g_esp+56)=g_edi;for(;;){g_ecx=MEM32(g_esp+48);g_edx=MEM32(g_esp+52);g_eax=MEM32(g_ecx+g_edx*4);PUSH32(g_esp,64);MEM32(g_edi-84)=g_eax;PUSH32(g_esp,0x266ab8);sub_002D57FB();g_esp+=4;MEM32(g_esp+64)=g_eax;MEM8(g_esp+36)=2;
   if(g_eax!=frame){g_xmm0=XMM_SCALAR(MEMF(0x4859f0));PUSH32(g_esp,g_edi);PUSH32(g_esp,frame);PUSH32(g_esp,0x3e4ccccd);g_ecx=g_esp+32;MEMF(g_esp+32)=g_xmm0.f[0];g_xmm0=XMM_ZERO();PUSH32(g_esp,g_ecx);g_ecx=g_eax;MEMF(g_esp+40)=g_xmm0.f[0];PUSH32(g_esp,0x266af2);sub_00273630();}else g_eax=0;
   g_edi=MEM32(g_ebx+4);g_edx=g_esp+64;MEM32(g_esp+64)=g_eax;g_eax=MEM32(g_edi+4);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);g_ecx=g_ebx;MEM8(g_esp+48)=1;PUSH32(g_esp,0x266b13);sub_002BCFA0();PUSH32(g_esp,1);g_ecx=g_ebx;frame=g_eax;PUSH32(g_esp,0x266b1e);sub_000722A0();g_eax=MEM32(g_esp+52);g_edx=MEM32(g_esp+56);MEM32(g_edi+4)=frame;g_ecx=MEM32(frame+4);MEM32(g_ecx)=frame;g_ecx=MEM32(g_esi+88);g_eax++;g_edx+=4;MEM32(g_esp+52)=g_eax;MEM32(g_esp+56)=g_edx;if(g_eax>=g_ecx)break;g_edi=MEM32(g_esp+56);frame=0;
  }g_edi=MEM32(g_esp+44);}
 g_eax=MEM32(g_esi+88);g_ecx=MEM32(g_edi);g_eax--;if(g_ecx<g_eax)g_eax=g_ecx;g_edx=0;g_ecx=g_eax?g_eax:0;SET_LO8(g_eax,MEM8(g_esp+60));MEM32(g_edi)=g_ecx;if((int8_t)LO8(g_eax)<0){PUSH32(g_esp,0x266b6b);sub_000643E0();SET_LO8(g_eax,MEM8(g_eax+48));MEM8(g_esi+192)=LO8(g_eax);}PUSH32(g_esp,0x266b79);sub_0025A2E0();MEM32(g_esi+196)=g_eax;PUSH32(g_esp,0x4877c4);MEM32(0x51cfa0)=g_esi;PUSH32(g_esp,0x266b8f);sub_000650D0();PUSH32(g_esp,30);MEM32(g_esi+208)=g_eax;PUSH32(g_esp,0x266b9c);sub_00274770();SET_LO8(g_eax,!LO8(g_eax));PUSH32(g_esp,31);MEM8(g_esi+206)=LO8(g_eax);PUSH32(g_esp,0x266baf);sub_00274770();SET_LO8(g_eax,!LO8(g_eax));PUSH32(g_esp,32);MEM8(g_esi+207)=LO8(g_eax);PUSH32(g_esp,0x266bc2);sub_00274770();SET_LO8(g_eax,!LO8(g_eax));PUSH32(g_esp,33);MEM8(g_esi+204)=LO8(g_eax);PUSH32(g_esp,0x266bd5);sub_00274770();g_ecx=MEM32(g_esp+48);g_esp+=20;SET_LO8(g_eax,!LO8(g_eax));MEM8(g_esi+205)=LO8(g_eax);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=24;g_recomp_zf=!g_esp;g_esp+=28;
}
int migration_interface_list(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_LIST");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;unsigned s=g_esp;
 if(s<0x10400||(s&3)||!list_span(s-1024,1060)||!list_span(0x51cfa0,4))return 0;
 unsigned n=MEM32(s+(op?16:12));if(n>20||!list_span(MEM32(s+(op?8:4)),4)||(n&&!list_span(MEM32(s+(op?12:8)),n*4))||!list_span(op?MEM32(s+4):g_ecx,op?8:232))return 0;
 if(!op){list_construct();return 1;}
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39f85b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,232);PUSH32(g_esp,0x266c20);sub_002D57FB();g_esp+=4;MEM32(g_esp)=g_eax;g_ecx=0;MEM32(g_esp+12)=0;
 if(g_eax){g_ecx=MEM32(g_esp+44);g_edx=MEM32(g_esp+40);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+40);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+40);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+40);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+40);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;PUSH32(g_esp,0x266c55);sub_002669F0();g_ecx=g_eax;}
 g_eax=MEM32(g_esp+20);MEM32(g_eax)=g_ecx;g_ecx=MEM32(g_esp+4);MEM32(g_eax+4)=1;MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
