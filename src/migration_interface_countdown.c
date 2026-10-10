#include "migration_interface_countdown.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_000871A0(void),sub_000658C0(void),sub_00017060(void),sub_000176D0(void),sub_00086FD0(void),sub_00017160(void),sub_00017180(void),sub_00018CE0(void),sub_00018130(void),sub_002D6780(void),sub_002D6322(void),sub_00275C80(void);
#define CD_CALL(n,r) do{g_ebp=frame;PUSH32(g_esp,r);sub_##n();}while(0)
#define CD_FP g_fp_stack[g_fp_top]
static unsigned countdown_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static void countdown_pop(unsigned out){MEMF(out)=(float)CD_FP;g_fp_top=(g_fp_top+1)&7;}
static int countdown_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void countdown_number(unsigned frame){
 g_xmm1.f[0]=(float)(int32_t)MEM32(g_esi+12);g_xmm1.f[0]-=MEMF(g_esi+8);g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_ecx=MEM32(0x509704);g_xmm2=g_xmm1;g_xmm2.f[0]*=MEMF(0x44eef8);g_xmm2.f[0]+=g_xmm0.f[0];g_xmm0.f[0]-=g_xmm1.f[0];g_xmm0.f[0]*=MEMF(0x452d80);g_xmm0.f[0]+=MEMF(0x44c260);MEMF(g_esp+24)=g_xmm2.f[0];MEMF(g_esp+28)=g_xmm0.f[0];CD_CALL(00018130,0x25aa5b);
 CD_FP*=MEMF(g_esp+24);PUSH32(g_esp,0xff000000);CD_FP*=MEMF(0x44c1f8);CD_FP=MEMF(g_esp+24)-CD_FP;countdown_pop(g_esp+24);double value=MEMF(g_esp+32);g_fp_top=(g_fp_top+7)&7;CD_FP=value;CD_CALL(002D6780,0x25aa7b);g_ecx=MEM32(0x509704);g_eax<<=24;g_eax+=0x888888;PUSH32(g_esp,g_eax);CD_CALL(00017160,0x25aa8f);
 g_xmm0=XMM_ZERO();if(!(countdown_cmp(g_xmm0.f[0],MEMF(g_esi+8))&0x41))MEMF(g_esi+8)=g_xmm0.f[0];g_eax=(int32_t)MEMF(g_esi+8);g_eax++;PUSH32(g_esp,g_eax);g_ecx=g_esp+20;PUSH32(g_esp,0x450a30);PUSH32(g_esp,g_ecx);CD_CALL(002D6322,0x25aab3);g_edx=MEM32(g_esp+36);g_eax=MEM32(g_esp+32);g_ecx=MEM32(0x4e839c);g_esp+=12;PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,390);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);g_edx=g_esp+40;PUSH32(g_esp,g_edx);CD_CALL(00018CE0,0x25aae0);
 g_xmm0=XMM_SCALAR(MEMF(g_esi+8));g_xmm0.f[0]-=MEMF(frame+8);g_eax=(int32_t)MEMF(g_esi+8);g_ecx=(int32_t)g_xmm0.f[0];MEMF(g_esi+8)=g_xmm0.f[0];if(g_eax!=g_ecx)MEM32(g_esi+12)=g_eax;
}
int migration_interface_countdown(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_COUNTDOWN");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation||(g_esp&3)||!countdown_span((uint64_t)g_esp-1024,1040)||!countdown_span(g_ecx,20))return 0;unsigned context=MEM32(g_esp+8);if(!countdown_span(context,20)||!countdown_span(MEM32(context+16),1))return 0;
 unsigned frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0;g_esp-=76;g_eax=MEM32(frame+12);g_xmm3=XMM_SCALAR(MEMF(0x46c320));g_xmm2=XMM_SCALAR(MEMF(g_eax+4));g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm2.f[0]*=g_xmm3.f[0];g_xmm1=g_xmm0;g_xmm1.f[0]-=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_eax+12));g_xmm2.f[0]*=g_xmm3.f[0];unsigned c=countdown_cmp(g_xmm2.f[0],g_xmm1.f[0]);PUSH32(g_esp,g_esi);g_esi=g_ecx;if(!(c&0x41))g_xmm1=g_xmm2;
 c=countdown_cmp(g_xmm0.f[0],g_xmm1.f[0]);g_xmm3=XMM_ZERO();if(c&0x41)g_xmm1=g_xmm0;else if(countdown_cmp(g_xmm1.f[0],g_xmm3.f[0])&0x41)g_xmm1=g_xmm3;c=countdown_cmp(g_xmm0.f[0],g_xmm2.f[0]);g_xmm4=g_xmm0;g_xmm4.f[0]-=g_xmm1.f[0];MEMF(g_esp+32)=g_xmm0.f[0];MEMF(g_esp+36)=g_xmm0.f[0];MEMF(g_esp+40)=g_xmm0.f[0];MEMF(g_esp+44)=g_xmm4.f[0];if(!(c&0x41)){g_eax=MEM32(g_eax+16);MEM8(g_eax)=0;}
 g_xmm0=XMM_SCALAR(MEMF(0x44d738));MEMF(g_esp+60)=g_xmm0.f[0];MEMF(g_esp+48)=g_xmm3.f[0];MEMF(g_esp+52)=g_xmm3.f[0];MEMF(g_esp+56)=g_xmm3.f[0];g_xmm0=XMM_MEM(g_esp+48);g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(g_esp+32));g_ecx=g_esp+48;PUSH32(g_esp,g_ecx);g_ecx=g_esp+28;XMM_STORE(g_esp+52,g_xmm0);CD_CALL(000871A0,0x25a94c);g_edx=g_esp+24;PUSH32(g_esp,g_edx);CD_CALL(000658C0,0x25a956);g_ecx=MEM32(0x509704);g_esp+=4;PUSH32(g_esp,3);CD_CALL(00017060,0x25a966);g_ecx=MEM32(0x509704);PUSH32(g_esp,3);CD_CALL(000176D0,0x25a973);
 CD_FP=MEMF(0x4e83a0)-CD_FP;g_eax=g_esp+64;PUSH32(g_esp,g_eax);g_ecx=0x51cf38;countdown_pop(g_esp+24);CD_CALL(00086FD0,0x25a98c);g_xmm0=XMM_MEM(g_eax);g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(g_esp+32));g_ecx=g_esp+48;PUSH32(g_esp,g_ecx);g_ecx=g_esp+32;XMM_STORE(g_esp+52,g_xmm0);CD_CALL(000871A0,0x25a9a7);g_edx=MEM32(g_eax);g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_edx);CD_CALL(00017160,0x25a9ba);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x44138000);PUSH32(g_esp,0x42480000);CD_CALL(00017180,0x25a9cf);
 g_eax=MEM32(g_esp+20);g_ecx=MEM32(0x4e839c);g_edx=MEM32(g_esi+4);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,18);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edx);CD_CALL(00018CE0,0x25a9f2);SET_LO8(g_eax,MEM8(g_esi+16));if(LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(0x485cc0));if(!(countdown_cmp(g_xmm0.f[0],MEMF(g_esi+8))&0x41))countdown_number(frame);SET_LO8(g_eax,MEM8(g_esi+16));}
 if(!LO8(g_eax)){PUSH32(g_esp,1);PUSH32(g_esp,0x485ca8);CD_CALL(00275C80,0x25ab12);g_esp+=8;if(LO8(g_eax))MEM8(g_esi+16)=1;}
 g_eax=0;POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=12;return 1;
}
