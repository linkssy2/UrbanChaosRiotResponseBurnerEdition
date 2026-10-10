#include "migration_interface_tweenstep.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273270(void),sub_00273730(void);
static int ts_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned ts_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
#define TS_TOP g_fp_stack[g_fp_top]
#define TS_POP() (g_fp_top=(g_fp_top+1)&7)
#define TS_PUSH(v) do{double value=(v);g_fp_top=(g_fp_top+7)&7;TS_TOP=value;}while(0)
static void ts_ease(void){
 PUSH32(g_esp,g_ecx);g_eax=MEM32(g_esp+8);g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm0.f[0]/=MEMF(g_esp+16);g_ecx=MEM32(g_eax*4+0x5011a4);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+24);MEMF(g_esp+4)=g_xmm0.f[0];g_xmm0.f[0]*=MEMF(g_esp+16);MEM32(g_esp+12)=g_ecx;MEMF(g_esp+20)=g_xmm0.f[0];
 if(g_esi){g_xmm0.f[0]-=MEMF(0x44d024);unsigned saved=g_esp;PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];unsigned target=MEM32(g_esp+16);PUSH32(g_esp,0x2732bb);RECOMP_ICALL_SAFE(target,saved);MEMF(g_esp+20)=(float)TS_TOP;TS_POP();g_xmm0=XMM_SCALAR(MEMF(g_esp+24));g_xmm0.f[0]+=MEMF(0x44d024);MEMF(g_esp)=g_xmm0.f[0];saved=g_esp;target=MEM32(g_esp+16);PUSH32(g_esp,0x2732d6);RECOMP_ICALL_SAFE(target,saved);TS_TOP-=MEMF(g_esp+20);g_esp+=4;TS_TOP*=MEMF(g_esp+4);TS_TOP*=MEMF(0x44c5e8);MEMF(g_esi)=(float)TS_TOP;TS_POP();}
 g_edx=MEM32(g_esp+20);unsigned saved=g_esp;PUSH32(g_esp,g_edx);unsigned target=MEM32(g_esp+16);PUSH32(g_esp,0x2732f2);RECOMP_ICALL_SAFE(target,saved);TS_TOP=MEMF(0x44bf94)-TS_TOP;g_esp+=4;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=!g_esp;g_esp+=4;
}
static void ts_update(void){
 g_esp-=12;g_xmm0=XMM_ZERO();PUSH32(g_esp,g_esi);g_esi=g_ecx;SET_LO8(g_eax,MEM8(g_esi+28));MEMF(g_esp+8)=MEMF(g_esp+12)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+20));
 if(LO8(g_eax)){g_xmm0.f[0]*=MEMF(0x453014);g_xmm1=XMM_SCALAR(MEMF(g_esi+8));g_xmm1.f[0]-=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+16));MEMF(g_esi+8)=g_xmm1.f[0];MEMF(g_esp+20)=g_xmm0.f[0];}
 else{g_xmm0.f[0]*=MEMF(0x44c72c);g_xmm0.f[0]+=MEMF(g_esi+8);g_xmm1=XMM_SCALAR(MEMF(0x44bf94));if(!(ts_cmp(g_xmm0.f[0],g_xmm1.f[0])&0x41))g_xmm0=g_xmm1;MEMF(g_esi+8)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+12));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+20));MEMF(g_esp+8)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+24));MEMF(g_esp+12)=g_xmm0.f[0];}
 g_eax=MEM32(g_esi+8);PUSH32(g_esp,g_edi);g_edi=g_esi+44;PUSH32(g_esp,g_edi);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);PUSH32(g_esp,1);PUSH32(g_esp,0x2737cd);sub_00273270();MEMF(g_esp+24)=(float)TS_TOP;TS_POP();TS_PUSH(MEMF(g_esp+24));g_xmm2=XMM_SCALAR(MEMF(0x44bf94));TS_TOP*=MEMF(g_esp+28);g_xmm1=XMM_SCALAR(MEMF(g_esp+24));g_xmm0=g_xmm2;g_xmm0.f[0]-=g_xmm1.f[0];TS_TOP+=MEMF(0x44bf94);g_xmm1.f[0]*=MEMF(g_esp+40);MEMF(g_esi+40)=g_xmm0.f[0];MEMF(g_esi+48)=g_xmm1.f[0];MEMF(g_esi+52)=(float)TS_TOP;TS_POP();g_xmm3=XMM_SCALAR(MEMF(0x4e8394));g_xmm3.f[0]*=MEMF(g_edi);g_esp+=16;MEMF(g_edi)=g_xmm3.f[0];g_xmm3=XMM_SCALAR(MEMF(g_esp+16));SET_HI8(g_eax,ts_cmp(g_xmm3.f[0],MEMF(0x44c174)));POP32(g_esp,g_edi);
 if(RECOMP_PARITY8(HI8(g_eax)&0x44)){g_xmm0.f[0]*=g_xmm3.f[0];if(!(ts_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41))g_xmm0=g_xmm2;MEMF(g_esi+56)=g_xmm0.f[0];}
 g_xmm0=XMM_SCALAR(MEMF(g_esi+4));g_xmm0.f[0]+=g_xmm1.f[0];MEMF(g_esi+32)=g_xmm0.f[0];POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=!g_esp;g_esp+=8;
}
int migration_interface_tweenstep(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_TWEENSTEP");enabled=s&&!strcmp(s,"1");}if(!enabled||op>3)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!ts_span(s-1024,1044)||!ts_span(0x5011a4,16))return 0;
 if(op==3){g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm0=XMM_SCALAR(MEMF(g_esp+4));unsigned f=ts_cmp(g_xmm1.f[0],g_xmm0.f[0]);if(f&0x41){g_xmm0=g_xmm1;MEMF(g_esp+4)=g_xmm0.f[0];}else{g_xmm1=XMM_ZERO();f=ts_cmp(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_esp+4)=(f&0x41)?g_xmm1.f[0]:g_xmm0.f[0];}TS_PUSH(MEMF(g_esp+4));g_recomp_zf=!!(f&0x40);g_esp+=4;}
 else if(op==0){if(MEM32(s+4)>3||(MEM32(s+16)&&!ts_span(MEM32(s+16),4)))return 0;ts_ease();}
 else{if(!ts_span(g_ecx,64)||(op==2&&!ts_span(MEM32(g_ecx+60),4)))return 0;if(op==1)ts_update();else{g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_esi=g_ecx;PUSH32(g_esp,0x27398d);sub_00273730();g_ecx=MEM32(g_esi+60);g_edx=MEM32(g_esi+32);MEM32(g_ecx)=g_edx;POP32(g_esp,g_esi);g_esp+=8;}}
 return 1;
}
