#include "migration_interface_panelconstruct.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D57FB(void),sub_0025D7C0(void),sub_0025CF20(void),sub_0025D4C0(void),sub_00273410(void),sub_000650D0(void),sub_002D3780(void),sub_002D3800(void);
#define PC_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
static int pc_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void panelconstruct_seed(void){
 g_xmm0=XMM_ZERO();PUSH32(g_esp,g_esi);g_esi=g_ecx;MEMF(g_esi)=MEMF(g_esi+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3dcccccd);MEMF(g_esi+8)=MEMF(g_esi+12)=g_xmm0.f[0];PC_CALL(002D3780,0x25d4f0);MEMF(g_esi+16)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
 static const unsigned hi[]={0x4019999a,0x40c90fdb,0x3f000000},lo[]={0x3fe66666,0,0xbf000000},ret[]={0x25d502,0x25d511,0x25d523};for(unsigned i=0;i<3;i++){PUSH32(g_esp,hi[i]);PUSH32(g_esp,lo[i]);PC_CALL(002D3780,ret[i]);MEMF(g_esi+20+i*4)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
 g_esp+=32;PC_CALL(002D3800,0x25d52e);if(LO8(g_eax)){g_eax=MEM32(g_esi+8);g_xmm0=XMM_SCALAR(MEMF(g_esi));MEM32(g_esi)=g_eax;MEMF(g_esi+8)=g_xmm0.f[0];}PC_CALL(002D3800,0x25d545);int swap=LO8(g_eax)!=0;g_eax=g_esi;if(swap){g_ecx=MEM32(g_esi+12);g_xmm0=XMM_SCALAR(MEMF(g_esi+4));MEM32(g_esi+4)=g_ecx;MEMF(g_esi+12)=g_xmm0.f[0];}POP32(g_esp,g_esi);g_recomp_zf=!swap;g_esp+=4;
}
int migration_interface_panelconstruct(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_PANELCONSTRUCT");enabled=s&&!strcmp(s,"1");}if(!enabled||op>3)return 0;
 unsigned s=g_esp;if(s<0x10800||(s&3)||!pc_span(s-2048,2096)||(op&&!pc_span(g_ecx,op==3?32:op==2?84:232)))return 0;
 if(op==3){panelconstruct_seed();return 1;}
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,op?0x39f478:0x39f49b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);
 if(!op){PUSH32(g_esp,232);PC_CALL(002D57FB,0x25db20);g_esp+=4;MEM32(g_esp)=g_eax;MEM32(g_esp+12)=0;if(g_eax){g_ecx=MEM32(g_esp+40);g_edx=MEM32(g_esp+36);PUSH32(g_esp,0xffffffff);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+40);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+40);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+40);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+40);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;PC_CALL(0025D7C0,0x25db59);}else g_eax=0;g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;return 1;}
 if(op==2){
  g_eax=MEM32(g_esp+28);g_edx=MEM32(g_esp+20);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x3f800000);g_esi=g_ecx;g_ecx=MEM32(g_esp+36);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_esi;MEM32(g_esp+24)=g_esi;PC_CALL(00273410,0x25cf59);g_eax=MEM32(g_esp+40);g_ebx=0;PUSH32(g_esp,g_eax);MEM32(g_esp+24)=0;MEM32(g_esi)=0x487348;PC_CALL(000650D0,0x25cf6f);
  g_xmm0=XMM_ZERO();g_ecx=MEM32(g_esp+16);MEMF(g_esi+64)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+48));MEMF(g_esi+68)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+52));MEMF(g_esi+72)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+56));MEM32(g_esi+60)=g_eax;g_esp+=4;MEM8(g_esi+80)=MEM8(g_esi+81)=MEM8(g_esi+82)=LO8(g_ebx);MEMF(g_esi+76)=g_xmm0.f[0];g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
 }else{
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;MEM32(g_esp+12)=g_esi;g_xmm1=XMM_SCALAR(MEMF(0x4873b8));g_eax=MEM32(g_esp+40);g_edx=MEM32(g_esp+32);g_edi=MEM32(g_esp+48);PUSH32(g_esp,g_ecx);g_xmm0.f[0]=(float)(int32_t)g_edi;g_xmm0.f[0]*=MEMF(0x44c538);g_xmm1.f[0]-=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+56));g_xmm0.f[0]*=MEMF(0x487328);MEMF(g_esp)=g_xmm1.f[0];PUSH32(g_esp,0x40000000);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_eax);g_ecx=MEM32(g_esp+52);PUSH32(g_esp,4);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_esi;PC_CALL(0025CF20,0x25d832);
  g_xmm0=XMM_SCALAR(MEMF(g_esp+52));g_eax=0;double a=g_xmm0.f[0],b=MEMF(0x44bf94);int below=a!=a||b!=b||a<b;MEM32(g_esp+24)=0;MEM32(g_esi)=0x4873a4;MEM32(g_esi+84)=g_edi;if(!below)g_eax=1;MEM8(g_esi+88)=LO8(g_eax);g_edi=g_esi+92;g_ebx=4;do{g_ecx=g_edi;PC_CALL(0025D4C0,0x25d867);g_edi+=32;g_ebx--;}while(g_ebx);PUSH32(g_esp,0x487378);PC_CALL(000650D0,0x25d877);g_ecx=MEM32(g_esp+60);MEM32(g_esi+220)=g_eax;g_eax=MEM32(g_esp+48);g_esp+=4;MEM32(g_esi+224)=g_eax;MEM32(g_esi+228)=g_ecx;g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
 }
 MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=32;return 1;
}
