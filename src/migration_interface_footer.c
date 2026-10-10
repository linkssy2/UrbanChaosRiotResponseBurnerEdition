#include "migration_interface_footer.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_0005AAB0(void),sub_00017060(void),sub_00018130(void),sub_00065820(void),sub_00017160(void),sub_00018DE0(void),sub_002BC1F0(void),sub_002BF3A0(void),sub_002BB850(void),sub_002BC350(void),sub_002BB760(void),sub_00015AE0(void);
static int footer_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned footer_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
#define FT_TOP g_fp_stack[g_fp_top]
#define FT_POP() (g_fp_top=(g_fp_top+1)&7)
#define FT_CALL(fn,ret) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
static void footer_icons(unsigned frame){
 g_eax=MEM32(g_edi);g_ecx=g_edi;unsigned saved=g_esp,target=MEM32(g_eax+384);PUSH32(g_esp,0x254390);RECOMP_ICALL_SAFE(target,saved);
 if(LO8(g_eax)){
  g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_ecx=MEM32(0x509704);MEMF(g_esp+28)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(frame+8));MEM32(g_esp+24)=g_esi;MEMF(g_esp+32)=g_xmm0.f[0];FT_CALL(sub_00018130,0x2543be);FT_TOP=MEMF(frame+8)-FT_TOP;g_xmm0=XMM_SCALAR(MEMF(g_esp+20));PUSH32(g_esp,g_ebx);g_ecx=g_esp+28;MEMF(g_esp+44)=(float)FT_TOP;FT_POP();PUSH32(g_esp,g_ecx);g_edx=g_esp+36;PUSH32(g_esp,g_edx);g_eax=g_esp+48;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x484784);MEMF(g_esp+56)=g_xmm0.f[0];FT_CALL(sub_00065820,0x2543eb);
  g_xmm0=XMM_SCALAR(MEMF(g_esp+32));g_xmm0.f[0]-=MEMF(0x44c530);g_xmm1=XMM_SCALAR(MEMF(g_esp+40));g_xmm1.f[0]-=g_xmm0.f[0];MEMF(g_esp+40)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esp+36));g_xmm1.f[0]-=g_xmm0.f[0];g_esp+=20;MEMF(g_esp+16)=g_xmm1.f[0];
 }
 g_edx=MEM32(g_edi);g_ecx=g_edi;saved=g_esp;target=MEM32(g_edx+388);PUSH32(g_esp,0x254426);RECOMP_ICALL_SAFE(target,saved);
 if(LO8(g_eax)){
  g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_ecx=MEM32(0x509704);MEMF(g_esp+36)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(frame+8));MEM32(g_esp+24)=g_esi;MEMF(g_esp+40)=g_xmm0.f[0];FT_CALL(sub_00018130,0x254450);FT_TOP=MEMF(frame+8)-FT_TOP;g_xmm0=XMM_SCALAR(MEMF(g_esp+20));PUSH32(g_esp,g_ebx);g_eax=g_esp+28;MEMF(g_esp+36)=(float)FT_TOP;FT_POP();PUSH32(g_esp,g_eax);g_ecx=g_esp+44;PUSH32(g_esp,g_ecx);g_edx=g_esp+40;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x48474c);MEMF(g_esp+48)=g_xmm0.f[0];FT_CALL(sub_00065820,0x25447d);g_esp+=20;
 }
}
static void footer_row(unsigned frame){
 g_ecx=MEM32(0x509704);MEM32(g_esp+44)=g_edi;FT_CALL(sub_00018130,0x254591);FT_TOP+=MEMF(g_esp+12);g_xmm0=XMM_SCALAR(MEMF(g_esp+16));PUSH32(g_esp,g_ebx);g_ecx=g_esp+48;MEMF(g_esp+72)=(float)FT_TOP;FT_POP();PUSH32(g_esp,g_ecx);g_edx=g_esp+72;PUSH32(g_esp,g_edx);MEMF(g_esp+76)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+32));g_eax=g_esp+84;PUSH32(g_esp,g_eax);MEMF(g_esp+88)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+28));PUSH32(g_esp,0x484710);MEMF(g_esp+96)=g_xmm0.f[0];FT_CALL(sub_00065820,0x2545d7);
 g_eax=MEM32(g_esp+232);g_esp+=20;g_eax-=g_ebx;PUSH32(g_esp,0xff000000);
 if(!g_eax){g_eax=MEM32(0x51cf54);PUSH32(g_esp,g_eax);}else{g_eax--;if(!g_eax){g_edx=MEM32(0x51cf3c);PUSH32(g_esp,g_edx);}else{g_ecx=MEM32(0x51cf5c);PUSH32(g_esp,g_ecx);}}
 g_ecx=MEM32(0x509704);FT_CALL(sub_00017160,0x254610);g_eax=MEM32(g_esp+176);if(MEM32(g_esp+196)<16)g_eax=g_esp+176;g_ecx=MEM32(g_esp+12);g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_xmm0.f[0]+=MEMF(0x44ec38);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_eax);FT_CALL(sub_00018DE0,0x25464f);g_ecx=MEM32(0x509704);FT_CALL(sub_00018130,0x25465a);FT_TOP+=MEMF(g_esp+12);FT_TOP+=MEMF(0x44bf94);MEMF(g_esp+12)=(float)FT_TOP;FT_POP();
}
static void footer_rows(unsigned frame){
 FT_CALL(sub_0005AAB0,0x2544a8);if(MEM32(g_eax+328)==g_ebx)return;FT_CALL(sub_0005AAB0,0x2544b9);g_ecx=MEM32(g_eax+328);FT_CALL(sub_002BC1F0,0x2544c4);if(!LO8(g_eax))return;
 MEM32(g_esp+52)=g_ebx;MEM32(g_esp+56)=g_ebx;MEM32(g_esp+60)=g_ebx;g_eax=g_esp+48;PUSH32(g_esp,g_eax);MEM32(g_esp+312)=g_ebx;FT_CALL(sub_0005AAB0,0x2544e9);g_ecx=MEM32(g_eax+328);FT_CALL(sub_002BF3A0,0x2544f4);g_edi=MEM32(g_esp+56);g_ecx=MEM32(0x509704);g_esi=MEM32(g_esp+52);MEM32(g_esp+28)=g_edi;FT_CALL(sub_00018130,0x25450b);FT_TOP+=MEMF(0x484748);MEM32(g_esp+24)=g_ebx;MEMF(g_esp+12)=(float)FT_TOP;FT_POP();
 if(g_esi!=g_edi)do{
  g_edi=MEM32(0x51cf5c);g_eax=g_esi;PUSH32(g_esp,g_eax);g_ecx=g_esp+84;g_esi+=216;FT_CALL(sub_002BB850,0x254539);MEM8(g_esp+308)=1;
  if(MEM8(g_esp+201)==LO8(g_ebx)&&MEM8(g_esp+220)!=LO8(g_ebx)&&MEM8(g_esp+202)!=LO8(g_ebx)){FT_CALL(sub_0005AAB0,0x25456d);g_ecx=MEM32(g_eax+328);FT_CALL(sub_002BC350,0x254578);if(MEM32(g_esp+24)!=g_eax)footer_row(frame);}
  MEM32(g_esp+24)++;g_ecx=g_esp+80;MEM8(g_esp+308)=LO8(g_ebx);FT_CALL(sub_002BB760,0x25467c);
 }while(g_esi!=MEM32(g_esp+28));
 g_ecx=g_esp+48;MEM32(g_esp+308)=0xffffffff;FT_CALL(sub_00015AE0,0x25469a);
}
int migration_interface_footer(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_FOOTER");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10800||(s&3)||!footer_span(s-2048,2080))return 0;
 unsigned frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff8;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39efb6);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=288;g_eax=MEM32(0x5150ac);PUSH32(g_esp,g_ebx);g_ebx=0;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if(g_eax==g_ebx){FT_CALL(sub_0005AAB0,0x2542c8);g_edi=MEM32(g_eax+324);
  if(g_edi!=g_ebx){g_ecx=MEM32(0x509704);PUSH32(g_esp,2);FT_CALL(sub_00017060,0x2542e3);g_ecx=MEM32(0x509704);FT_CALL(sub_00018130,0x2542ee);MEMF(g_esp+12)=(float)FT_TOP;FT_POP();g_xmm0=XMM_SCALAR(MEMF(0x451290));
   if(!(footer_cmp(g_xmm0.f[0],MEMF(frame+8))&0x41)){g_xmm0=XMM_SCALAR(MEMF(0x51cf7c));g_xmm2=XMM_SCALAR(MEMF(0x46c320));g_xmm1=g_xmm0;g_xmm1.f[0]-=MEMF(g_esp+12);g_xmm1.f[0]+=g_xmm2.f[0];MEMF(g_esp+20)=g_xmm1.f[0];g_xmm0.f[0]+=g_xmm2.f[0];}else{g_xmm0=XMM_SCALAR(MEMF(0x4859f0));g_xmm0.f[0]+=MEMF(0x44ff78);MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0.f[0]+=MEMF(g_esp+12);}
   MEMF(g_esp+16)=g_xmm0.f[0];FT_CALL(sub_0005AAB0,0x254350);g_xmm0=XMM_SCALAR(MEMF(g_eax+336));g_esi=MEM32(0x51cf5c);g_eax=(int32_t)g_xmm0.f[0];g_xmm1.f[0]=(float)(int32_t)g_eax;g_xmm0.f[0]-=g_xmm1.f[0];if(!(footer_cmp(g_xmm0.f[0],MEMF(0x44c174))&1)){g_xmm1=XMM_SCALAR(MEMF(0x44c1f8));if(!(footer_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41))g_esi=MEM32(0x51cf60);}
   footer_icons(frame);g_xmm0=XMM_SCALAR(MEMF(0x4859f0));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0.f[0]+=MEMF(g_esp+12);MEMF(g_esp+16)=g_xmm0.f[0];if(MEM8(frame+20)!=LO8(g_ebx))footer_rows(frame);
  }
 }
 g_ecx=MEM32(g_esp+300);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=4;return 1;
}
#undef FT_TOP
#undef FT_POP
#undef FT_CALL
