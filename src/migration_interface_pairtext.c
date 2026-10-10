#include "migration_interface_pairtext.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273300(void),sub_00017060(void),sub_00017730(void),sub_00018130(void),sub_00017160(void),sub_00018CE0(void),sub_00017E40(void),sub_00018DE0(void);
static int pt_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
#define PT_TOP g_fp_stack[g_fp_top]
#define PT_POP() (g_fp_top=(g_fp_top+1)&7)
int migration_interface_pairtext(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_PAIRTEXT");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!pt_span(s-1024,1040)||!pt_span(g_ecx,12)||!pt_span(MEM32(s+8),16))return 0;
 g_esp-=8;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=g_esp+8;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+28);g_edx=MEM32(g_eax+4);PUSH32(g_esp,0x3e4ccccd);g_esi=g_ecx;g_ecx=MEM32(g_eax+12);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x3e4ccccd);PUSH32(g_esp,g_edx);PUSH32(g_esp,2);PUSH32(g_esp,0x259049);sub_00273300();
 PT_TOP*=MEMF(0x451290);g_ecx=MEM32(0x509704);g_esp+=24;PT_TOP=MEMF(0x4859f0)-PT_TOP;PUSH32(g_esp,3);MEMF(g_esp+28)=(float)PT_TOP;PT_POP();g_fp_top=(g_fp_top+7)&7;PT_TOP=MEMF(g_esp+12);PT_TOP*=MEMF(0x451290);MEMF(g_esp+12)=(float)PT_TOP;PT_POP();PUSH32(g_esp,0x259077);sub_00017060();
 g_eax=MEM32(g_esp+8);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x259087);sub_00017730();g_ecx=MEM32(0x509704);PUSH32(g_esp,0x259092);sub_00018130();
 PT_TOP=MEMF(0x4859fc)-PT_TOP;g_eax=MEM32(g_esi+4);MEMF(g_esp+12)=(float)PT_TOP;PT_POP();g_edi=MEM32(g_esp+12);
 if(g_eax){
  g_ecx=MEM32(0x51cf90);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x2590c2);sub_00017160();
  g_edx=MEM32(g_esp+24);g_eax=MEM32(g_esi+4);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,64);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2590df);sub_00018CE0();
  g_ecx=MEM32(g_esi+4);PUSH32(g_esp,0);PUSH32(g_esp,64);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x2590f7);sub_00017E40();
  PT_TOP+=MEMF(g_esp+24);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PT_TOP+=MEMF(0x46c320);PUSH32(g_esp,0);PUSH32(g_esp,g_edi);MEMF(g_esp+36)=(float)PT_TOP;PT_POP();g_edx=MEM32(g_esp+36);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x44c3f4);PUSH32(g_esp,0x25911f);sub_00018DE0();
  g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x44c3f4);PUSH32(g_esp,0x259138);sub_00017E40();PT_TOP+=MEMF(g_esp+24);PT_TOP+=MEMF(0x45080c);MEMF(g_esp+24)=(float)PT_TOP;PT_POP();
 }
 g_eax=MEM32(g_esi+8);
 if(g_eax){g_eax=MEM32(g_esi+4);g_eax=g_eax?0x51cf68:0x51cf90;g_eax=MEM32(g_eax);g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x259171);sub_00017160();g_ecx=MEM32(g_esp+24);g_edx=MEM32(g_esi+8);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,64);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x25918e);sub_00018CE0();}
 g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0x25919b);sub_00017730();POP32(g_esp,g_edi);g_eax=0;POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=!g_esp;g_esp+=12;return 1;
}
#undef PT_TOP
#undef PT_POP
