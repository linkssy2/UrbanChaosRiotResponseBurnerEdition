#include "migration_interface_decorated.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273300(void),sub_00017060(void),sub_00018130(void),sub_00017160(void),sub_00018CE0(void),sub_00017E40(void),sub_00018DE0(void),sub_000656D0(void),sub_00065760(void);
static int decorated_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned decorated_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
#define PT_TOP g_fp_stack[g_fp_top]
#define PT_POP() (g_fp_top=(g_fp_top+1)&7)
static void decorated_background(void){
 g_fp_top=(g_fp_top+7)&7;PT_TOP=MEMF(g_esp+12);g_xmm1=XMM_SCALAR(MEMF(0x489044));PT_TOP*=MEMF(0x453010);g_xmm0=XMM_SCALAR(MEMF(0x489040));g_eax=g_esp+16;PUSH32(g_esp,g_eax);PT_TOP=sin(PT_TOP);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_edx=g_esp+36;PUSH32(g_esp,g_edx);MEM32(g_esp+28)=0xff000000;MEMF(g_esp+40)=g_xmm2.f[0];MEMF(g_esp+44)=g_xmm2.f[0];PT_TOP*=MEMF(0x489038);MEMF(g_esp+56)=(float)PT_TOP;PT_POP();g_xmm3=XMM_SCALAR(MEMF(g_esp+56));g_xmm1.f[0]+=g_xmm3.f[0];MEMF(g_esp+56)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x4e8394));g_xmm0.f[0]-=g_xmm3.f[0];MEMF(g_esp+32)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x48903c));g_xmm1.f[0]*=MEMF(0x44c1f8);MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0.f[0]-=g_xmm1.f[0];g_xmm0.f[0]+=MEMF(0x44c530);MEMF(g_esp+36)=g_xmm0.f[0];PUSH32(g_esp,0x25aed5);sub_000656D0();
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));MEMF(g_esp+40)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x4e8398));MEMF(g_esp+44)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_eax=g_esp+28;PUSH32(g_esp,g_eax);g_ecx=g_esp+44;MEMF(g_esp+36)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+60));g_xmm0.f[0]-=MEMF(0x46c31c);PUSH32(g_esp,g_ecx);g_edx=g_esp+40;PUSH32(g_esp,g_edx);MEM32(g_esp+40)=0xff000000;MEMF(g_esp+48)=g_xmm0.f[0];PUSH32(g_esp,0x25af2a);sub_000656D0();
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_edx=MEM32(g_esi+20);g_xmm1=XMM_ZERO();PUSH32(g_esp,0);PUSH32(g_esp,0x51cf30);g_eax=g_esp+60;PUSH32(g_esp,g_eax);MEMF(g_esp+64)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+48));g_ecx=g_esp+56;PUSH32(g_esp,g_ecx);MEMF(g_esp+72)=g_xmm0.f[0];g_xmm0.f[0]-=MEMF(0x48903c);PUSH32(g_esp,g_edx);MEMF(g_esp+64)=g_xmm1.f[0];MEMF(g_esp+68)=g_xmm0.f[0];PUSH32(g_esp,0x25af75);sub_00065760();
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_xmm1=XMM_SCALAR(MEMF(0x48903c));g_edx=MEM32(g_esi+24);PUSH32(g_esp,0);PUSH32(g_esp,0x51cf30);g_eax=g_esp+80;MEMF(g_esp+80)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+96));PUSH32(g_esp,g_eax);g_xmm1.f[0]+=g_xmm0.f[0];g_ecx=g_esp+76;MEMF(g_esp+88)=g_xmm1.f[0];g_xmm1=XMM_ZERO();PUSH32(g_esp,g_ecx);MEMF(g_esp+80)=g_xmm1.f[0];MEMF(g_esp+84)=g_xmm0.f[0];PUSH32(g_esp,g_edx);PUSH32(g_esp,0x25afc4);sub_00065760();
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_esp+=64;MEMF(g_esp+28)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+44));g_xmm0.f[0]-=MEMF(0x44c530);PUSH32(g_esp,0);g_eax=g_esp+20;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+28);MEMF(g_esp+40)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_ecx=g_esp+36;PUSH32(g_esp,g_ecx);g_edx=g_esp+32;MEMF(g_esp+32)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+24));g_xmm0.f[0]-=MEMF(0x44ec38);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);MEM32(g_esp+36)=0xffffffff;MEMF(g_esp+44)=g_xmm0.f[0];PUSH32(g_esp,0x25b028);sub_00065760();
}
int migration_interface_decorated(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_DECORATED");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!decorated_span(s-1024,1040)||!decorated_span(g_ecx,32)||!decorated_span(MEM32(s+8),16))return 0;
 unsigned child=MEM32(g_ecx+4);if(!decorated_span(child,4)||!decorated_span(MEM32(child),8))return 0;
 unsigned frame=g_seh_ebp;g_esp-=24;g_xmm2=XMM_ZERO();PUSH32(g_esp,frame);frame=MEM32(g_esp+36);PUSH32(g_esp,g_esi);g_esi=g_ecx;SET_LO8(g_eax,MEM8(g_esi+16));PUSH32(g_esp,g_edi);MEMF(g_esp+12)=g_xmm2.f[0];
 if(LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(frame+4));g_xmm0.f[0]*=MEMF(0x46c320);g_xmm1=XMM_SCALAR(MEMF(0x44bf94));if(decorated_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41)g_xmm0=g_xmm1;else if(decorated_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41)g_xmm0=g_xmm2;g_xmm1.f[0]-=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm1.f[0];}
 decorated_background();g_xmm0=XMM_SCALAR(MEMF(0x4859f0));g_ecx=MEM32(0x509704);g_esp+=20;PUSH32(g_esp,3);MEMF(g_esp+48)=g_xmm0.f[0];PUSH32(g_esp,0x25b046);sub_00017060();SET_LO8(g_eax,MEM8(g_esi+16));
 if(LO8(g_eax)){g_edx=MEM32(frame+12);g_eax=MEM32(frame+4);g_ecx=g_esp+44;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x3e4ccccd);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x3e4ccccd);PUSH32(g_esp,g_eax);PUSH32(g_esp,2);PUSH32(g_esp,0x25b06b);sub_00273300();PT_TOP*=MEMF(0x451290);g_esp+=24;PT_TOP=MEMF(0x4859f0)-PT_TOP;MEMF(g_esp+44)=(float)PT_TOP;PT_POP();}
 g_ecx=MEM32(0x509704);PUSH32(g_esp,0x25b089);sub_00018130();
 PT_TOP=MEMF(0x4859fc)-PT_TOP;g_eax=MEM32(g_esi+8);MEMF(g_esp+16)=(float)PT_TOP;PT_POP();g_edi=MEM32(g_esp+16);
 if(g_eax){
  g_ecx=MEM32(0x51cf90);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x25b0b9);sub_00017160();
  g_edx=MEM32(g_esp+44);g_eax=MEM32(g_esi+8);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,64);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x25b0d6);sub_00018CE0();
  g_ecx=MEM32(g_esi+8);PUSH32(g_esp,0);PUSH32(g_esp,64);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x25b0ee);sub_00017E40();
  PT_TOP+=MEMF(g_esp+44);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PT_TOP+=MEMF(0x46c320);PUSH32(g_esp,64);PUSH32(g_esp,g_edi);MEMF(g_esp+56)=(float)PT_TOP;PT_POP();g_edx=MEM32(g_esp+56);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x44c3f4);PUSH32(g_esp,0x25b116);sub_00018DE0();
  g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x44c3f4);PUSH32(g_esp,0x25b12f);sub_00017E40();PT_TOP+=MEMF(g_esp+44);PT_TOP+=MEMF(0x45080c);MEMF(g_esp+44)=(float)PT_TOP;PT_POP();
 }
 g_eax=MEM32(g_esi+12);
 if(g_eax){g_eax=MEM32(g_esi+8);g_eax=g_eax?0x51cf68:0x51cf90;g_eax=MEM32(g_eax);g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x25b168);sub_00017160();g_ecx=MEM32(g_esp+44);g_edx=MEM32(g_esi+12);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,64);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x25b185);sub_00018CE0();}

 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esp+40);g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,frame);PUSH32(g_esp,g_edx);unsigned target=MEM32(g_eax+4);PUSH32(g_esp,0x25b193);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_esp+=24;g_recomp_zf=!g_esp;g_esp+=12;return 1;
}
#undef PT_TOP
#undef PT_POP
