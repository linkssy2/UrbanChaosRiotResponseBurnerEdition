#include "migration_interface_movie.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00387CBD(void),sub_00016BA0(void),sub_002D6780(void),sub_002D57FB(void),sub_00272AD0(void),sub_00273030(void),sub_00273700(void),sub_00266EA0(void);
static int movie_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_movie(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_MOVIE");enabled=s&&!strcmp(s,"1");}if(!enabled||op>2)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!movie_span(s-1024,1072))return 0;
 if(op==0){unsigned text=MEM32(s+4);if(!movie_span(g_ecx,72)||!movie_span(text,20)||!memchr((void*)XBOX_PTR(text),0,20))return 0;}
 else if(!movie_span(MEM32(s+4),8))return 0;
 if(op==0){
  unsigned frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39fd68);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+16)=g_esi;g_eax=MEM32(g_esp+40);g_ecx=MEM32(g_esp+48);SET_LO8(g_edx,MEM8(g_esp+56));g_xmm0=XMM_SCALAR(MEMF(g_esp+44));g_edi=MEM32(g_esp+36);frame=MEM32(g_esp+52);g_ebx=0;MEM32(g_esi+4)=g_eax;SET_LO8(g_eax,MEM8(g_esp+60));MEM8(g_esi+20)=LO8(g_edx);MEM32(g_esi+12)=g_ecx;g_ecx=MEM32(g_esp+64);MEMF(g_esi+8)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_edx=g_esi+36;MEM8(g_esi+21)=LO8(g_eax);MEM32(g_esp+28)=g_ebx;MEM32(g_esi)=0x489020;MEM32(g_esi+16)=frame;MEM8(g_esi+22)=MEM8(g_esi+23)=MEM8(g_esi+24)=LO8(g_ebx);MEMF(g_esi+28)=g_xmm0.f[0];MEM32(g_esi+32)=MEM32(g_esi+56)=g_ebx;MEM32(g_esi+68)=g_ecx;
  g_eax=g_edi;g_edx-=g_edi;do{SET_LO8(g_ecx,MEM8(g_eax));MEM8(g_edx+g_eax)=LO8(g_ecx);g_eax++;}while(LO8(g_ecx)!=LO8(g_ebx));
  g_edx=0x51d2a0;g_eax=g_edi;g_edx-=g_edi;do{SET_LO8(g_ecx,MEM8(g_eax));MEM8(g_edx+g_eax)=LO8(g_ecx);g_eax++;}while(LO8(g_ecx)!=LO8(g_ebx));
  PUSH32(g_esp,0x51d2a0);PUSH32(g_esp,0x272b7a);sub_00387CBD();g_esp+=4;
  if(frame!=g_ebx){PUSH32(g_esp,0x272b86);sub_00016BA0();PUSH32(g_esp,0x272b8b);sub_002D6780();MEM32(g_esi+60)=g_eax;}
  g_xmm0=XMM_ZERO();g_ecx=MEM32(g_esp+20);POP32(g_esp,g_edi);MEMF(g_esi+64)=g_xmm0.f[0];g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=36;
 }else if(op==1){
  PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39fd8b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,72);PUSH32(g_esp,0x27304d);sub_002D57FB();g_esp+=4;MEM32(g_esp)=g_eax;g_ecx=0;MEM32(g_esp+12)=g_ecx;
  if(g_eax){g_ecx=MEM32(g_esp+52);g_edx=MEM32(g_esp+48);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+48);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+48);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+48);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+48);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+48);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+48);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;PUSH32(g_esp,0x27308c);sub_00272AD0();g_ecx=g_eax;}
  g_eax=MEM32(g_esp+20);MEM32(g_eax)=g_ecx;g_ecx=MEM32(g_esp+4);MEM32(g_eax+4)=1;MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;
 }else{
  g_eax=MEM32(g_esp+36);g_ecx=MEM32(g_esp+32);g_edx=MEM32(g_esp+28);g_esp-=16;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+48);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+48);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+48);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+48);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+48);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_edx=g_esp+36;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x258546);sub_00273030();
  g_esi=g_eax;g_eax=g_esp+48;PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x258554);sub_00273700();g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi);g_esi=MEM32(g_esp+68);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_eax+4);PUSH32(g_esp,g_edx);g_edx=MEM32(g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x25856c);sub_00266EA0();g_esp+=64;g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;
 }return 1;
}
