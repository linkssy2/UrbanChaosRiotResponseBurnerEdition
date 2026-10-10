#include "migration_interface_textupdate.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D7960(void),sub_00017060(void),sub_00017160(void),sub_00017180(void),sub_00017EE0(void),sub_00018130(void),sub_00018CE0(void);
#define TEXT_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
static unsigned text_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static int text_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_textupdate(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_TEXTUPDATE");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation||(g_esp&3)||!text_span((uint64_t)g_esp-512,528)||!text_span(g_ecx,32))return 0;
 unsigned text=MEM32(g_ecx+4);if(text&&(!text_span(text,22)||!text_span(MEM32(0x5096e0),20)))return 0;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);if(!g_eax){POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=12;return 1;}
 g_xmm0=XMM_ZERO();unsigned c=text_cmp(g_xmm0.f[0],MEMF(g_esi+20));PUSH32(g_esp,g_ebx);if(c&1){g_xmm0=XMM_SCALAR(MEMF(g_esi+20));g_xmm0.f[0]-=MEMF(g_esp+16);MEMF(g_esi+20)=g_xmm0.f[0];goto done;}
 PUSH32(g_esp,22);PUSH32(g_esp,0x485b48);PUSH32(g_esp,g_eax);TEXT_CALL(002D7960,0x2586cb);g_esp+=12;g_ebx=0xffffffff;
 if(!g_eax){SET_LO8(g_eax,MEM8(0x5154bd));if(LO8(g_eax)){POP32(g_esp,g_ebx);g_eax=0x271840;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=0;g_esp+=12;return 1;}
  g_ecx=MEM32(0x5096e0);MEM8(0x5154bc)=0;SET_LO8(g_edx,MEM8(g_ecx+16));SET_LO8(g_eax,0);if(LO8(g_edx)!=LO8(g_ebx))SET_LO8(g_eax,1);if(MEM8(g_ecx+17)!=LO8(g_ebx))SET_LO8(g_eax,1);if(MEM8(g_ecx+18)!=LO8(g_ebx))SET_LO8(g_eax,1);if(MEM8(g_ecx+19)==LO8(g_ebx)&&!LO8(g_eax))goto done;
 }
 g_ecx=MEM32(0x509704);g_eax=0;SET_LO8(g_eax,MEM8(g_esi+16));PUSH32(g_esp,g_eax);TEXT_CALL(00017060,0x25872d);g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ebx);TEXT_CALL(00017160,0x25873e);
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_xmm0.f[0]-=MEMF(0x4859f0);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,0x42480000);TEXT_CALL(00017180,0x258764);g_ecx=MEM32(g_esi+4);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);TEXT_CALL(00017EE0,0x258777);
 g_edx=MEM32(g_esi+12);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);MEM32(g_esp+20)=g_eax;g_eax=g_esi+24;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,18);MEM32(g_esp+24)=g_edx;TEXT_CALL(00018130,0x258797);g_fp_stack[g_fp_top]*=(double)SMEM32(g_esp+32);g_edx=MEM32(g_esi+4);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+8);g_fp_stack[g_fp_top]*=MEMF(0x44c1f8);g_fp_stack[g_fp_top]=MEMF(g_esp+28)-g_fp_stack[g_fp_top];MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edx);TEXT_CALL(00018CE0,0x2587bc);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x44138000);PUSH32(g_esp,0x42480000);TEXT_CALL(00017180,0x2587d1);
done: POP32(g_esp,g_ebx);g_eax=0;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=12;return 1;
}
