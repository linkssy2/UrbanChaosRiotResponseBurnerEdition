#include "migration_interface_widgetstate.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002745F0(void),sub_00274770(void),sub_00057FC0(void),sub_000643E0(void),sub_00017060(void),sub_00017160(void),sub_00018CE0(void),sub_00272FE0(void),sub_002D5CCD(void),sub_000877B0(void);
static int ws_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_widgetstate(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_WIDGETSTATE");enabled=s&&!strcmp(s,"1");}if(!enabled||op>2)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!ws_span(s-1024,1040)||!ws_span(g_ecx,72))return 0;
 if(!op&&MEM8(g_ecx+4)&&MEM32(g_ecx+12)>7)return 0;
 if(op==1){PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x272fc8);sub_00272FE0();if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x272fd5);sub_002D5CCD();g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 if(op==2){PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39fd68);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x489020;MEM32(g_esp+16)=0;PUSH32(g_esp,0x273010);sub_000877B0();g_ecx=MEM32(g_esp+8);MEM32(g_esi)=0x450708;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;return 1;}
 unsigned frame=g_seh_ebp;g_esp-=32;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);g_eax=25;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ebx=31;MEM32(g_esp+28)=g_eax;MEM32(g_esp+40)=g_eax;SET_LO8(g_eax,MEM8(g_esi+4));PUSH32(g_esp,g_edi);g_edi=30;frame=0;MEM32(g_esp+16)=g_edi;MEM32(g_esp+20)=g_edi;MEM32(g_esp+24)=g_ebx;MEM32(g_esp+28)=g_ebx;MEM32(g_esp+36)=g_ebx;MEM32(g_esp+40)=g_edi;
 if(LO8(g_eax)){
  // Preserve the original authored input sequence and unlock result.
  g_eax=MEM32(g_esi+12);g_ecx=MEM32(g_esp+g_eax*4+16);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x259ace);sub_002745F0();g_esp+=4;
  if(LO8(g_eax)){g_eax=MEM32(g_esi+12);if(g_eax==7){PUSH32(g_esp,0x450958);PUSH32(g_esp,0x259ae7);sub_00057FC0();g_esp+=4;PUSH32(g_esp,0x259aef);sub_000643E0();MEM8(g_eax+376)=1;frame=0x280dd0;}else{g_eax++;MEM32(g_esi+12)=g_eax;}}
  else{g_edx=MEM32(g_esi+12);unsigned key=MEM32(g_esp+g_edx*4+16)==g_edi?g_ebx:g_edi;PUSH32(g_esp,key);PUSH32(g_esp,0x259b15);sub_002745F0();g_esp+=4;PUSH32(g_esp,32);SET_LO8(g_ebx,LO8(g_eax));PUSH32(g_esp,0x259b21);sub_00274770();PUSH32(g_esp,33);SET_LO8(g_ebx,LO8(g_ebx)|LO8(g_eax));PUSH32(g_esp,0x259b2a);sub_00274770();g_esp+=8;SET_LO8(g_eax,LO8(g_eax)|LO8(g_ebx));if(LO8(g_eax))MEM32(g_esi+12)=0;}
 }
 g_eax=MEM32(g_esi+8);
 if(g_eax){g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0x259b4c);sub_00017060();g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);PUSH32(g_esp,0x30ffffff);PUSH32(g_esp,0x259b61);sub_00017160();g_eax=MEM32(g_esi+8);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,128);PUSH32(g_esp,0x43ac8000);PUSH32(g_esp,0x41f00000);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x259b85);sub_00018CE0();}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=frame;POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=32;g_recomp_zf=!g_esp;g_esp+=12;return 1;
}
