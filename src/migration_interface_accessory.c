#include "migration_interface_accessory.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00266E00(void),sub_002C3740(void),sub_000170B0(void),sub_000871A0(void),sub_000658C0(void);
static int ac_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_accessory(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_ACCESSORY");enabled=s&&!strcmp(s,"1");}if(!enabled||op>2)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!ac_span(s-1024,1060))return 0;
 if(op==0?!ac_span(MEM32(s+4),8):!ac_span(g_ecx,op==1?64:32))return 0;
 if(op==0){
  g_esp-=24;g_eax=MEM32(g_esp+32);g_ecx=MEM32(g_esp+36);g_edx=MEM32(g_esp+40);MEM32(g_esp)=g_eax;g_eax=MEM32(g_esp+44);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+32);MEM32(g_esp+16)=g_eax;PUSH32(g_esp,3);g_eax=g_esp+8;MEM32(g_esp+12)=g_ecx;g_ecx=MEM32(g_esp+56);MEM32(g_esp+16)=g_edx;g_edx=MEM32(g_esp+60);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);MEM32(g_esp+32)=g_ecx;MEM32(g_esp+36)=g_edx;PUSH32(g_esp,0x266f24);sub_00266E00();g_esp+=12;g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=24;g_recomp_zf=!g_esp;g_esp+=4;
 }else if(op==1){
  PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x3a02c3);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;SET_LO8(g_ecx,MEM8(g_esp+32));g_xmm0=XMM_ZERO();g_eax=MEM32(g_esp+28);MEM8(g_esi+8)=LO8(g_ecx);g_edi=g_esi+52;g_ecx=g_edi;MEM32(g_esp+20)=0;MEM32(g_esi)=0x48cac4;MEM32(g_esi+4)=g_eax;MEM32(g_esi+12)=0x502688;for(unsigned i=20;i<=32;i+=4)MEMF(g_esi+i)=g_xmm0.f[0];MEM8(g_esi+36)=1;MEM32(g_esi+40)=0;PUSH32(g_esp,0x2824fd);sub_002C3740();MEM32(g_edi+4)=g_eax;MEM32(g_edi+8)=0;g_edx=MEM32(g_esi+12);MEM32(g_esi+16)=g_edx;g_ecx=MEM32(0x509704);MEM8(g_esp+20)=1;PUSH32(g_esp,0x28251d);sub_000170B0();g_ecx=MEM32(g_esp+12);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=12;
 }else{
  PUSH32(g_esp,g_ecx);g_ecx+=16;PUSH32(g_esp,g_ecx);g_ecx=g_esp+4;PUSH32(g_esp,0x25864e);sub_000871A0();g_eax=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x258657);sub_000658C0();g_eax=0;g_esp+=8;g_recomp_zf=!g_esp;g_esp+=12;
 }return 1;
}
