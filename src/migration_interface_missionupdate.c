#include "migration_interface_missionupdate.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <math.h>
extern void sub_00017060(void),sub_00017160(void),sub_00017E40(void),sub_00018130(void),sub_00018CE0(void),sub_00018DE0(void),sub_00057FC0(void),sub_00058000(void),sub_00063C20(void),sub_00063C70(void),sub_00063D40(void),sub_000643E0(void),sub_000656D0(void),sub_002584C0(void),sub_00258D40(void),sub_00259370(void),sub_00259860(void),sub_0026EF70(void),sub_0026F0B0(void),sub_0026F1B0(void),sub_0026F6A0(void),sub_00274300(void),sub_00274320(void),sub_002745F0(void),sub_002D5060(void),sub_002D7159(void),ucrr_level_thumb_begin(void);
extern int ucrr_level_hover(void);
#define MU_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
#define MU_FP g_fp_stack[g_fp_top]
static void mu_push(double x){g_fp_top=(g_fp_top+7)&7;MU_FP=x;}
static void mu_pop(unsigned out){MEMF(out)=(float)MU_FP;g_fp_top=(g_fp_top+1)&7;}
static unsigned mu_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static int mu_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void missionupdate_popup(void){
 PUSH32(g_esp,0x450958);MU_CALL(00057FC0,0x2707c4);g_esp+=4;PUSH32(g_esp,48);MEM32(g_esp+32)=0x270bf0;MEM32(g_esp+36)=0x270c10;MEM32(g_esp+40)=0x270c30;MEM32(g_esp+44)=0x270c50;g_edi=3;MU_CALL(000643E0,0x2707f3);g_ecx=g_eax;MU_CALL(00063C70,0x2707fa);int unlocked=LO8(g_eax)!=0;if(!unlocked){MU_CALL(000643E0,0x270803);SET_LO8(g_ecx,MEM8(g_eax+383));unlocked=LO8(g_ecx)!=0;}if(unlocked)g_edi=4;
 MU_CALL(000643E0,0x270817);if((int32_t)MEM32(g_eax+72)>=4){MU_CALL(000643E0,0x270822);MEM32(g_eax+72)=1;}MU_CALL(000643E0,0x27082e);g_ecx=MEM32(g_eax+72);PUSH32(g_esp,0);PUSH32(g_esp,3);PUSH32(g_esp,0x41000000);PUSH32(g_esp,124);PUSH32(g_esp,0x270c70);g_edx=g_esp+48;PUSH32(g_esp,g_edx);g_eax=g_esp+76;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);MEM32(0x51d0d0)=g_ecx;MU_CALL(000643E0,0x270857);g_ecx=MEM32(g_eax+72);PUSH32(g_esp,g_ecx);g_edx=g_esp+80;PUSH32(g_esp,0x488608);PUSH32(g_esp,g_edx);MU_CALL(00258D40,0x27086a);g_ecx=MEM32(g_esi+268);g_edx=MEM32(g_ecx);g_ecx=MEM32(g_eax+4);g_eax=MEM32(g_eax);g_esp+=44;unsigned saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+268);PUSH32(g_esp,g_eax);unsigned target=MEM32(g_edx+4);PUSH32(g_esp,0x270885);RECOMP_ICALL_SAFE(target,saved);
}
static int missionupdate_input(void){
 g_ecx=MEM32(g_esi+268);g_edx=MEM32(g_ecx);unsigned saved=g_esp,target=MEM32(g_edx+12);PUSH32(g_esp,0x2706d6);RECOMP_ICALL_SAFE(target,saved);g_ebx=MEM32(g_esp+92);
 if(!LO8(g_eax)){MEM32(g_ebx+36)=0x48861c;MEM32(g_ebx+32)=0x488608;g_eax=MEM32(0x485a1c);if(g_eax!=11)MEM32(g_ebx+g_eax*4+24)=0x4855c8;MEM32(g_ebx+28)=0x450990;MEM32(g_ebx+60)=0x450978;}
 g_ecx=MEM32(g_esi+268);g_eax=MEM32(g_ecx);saved=g_esp;target=MEM32(g_eax+12);PUSH32(g_esp,0x270717);RECOMP_ICALL_SAFE(target,saved);
 if(!LO8(g_eax)){
  if(ucrr_level_hover())return 1;
  PUSH32(g_esp,32);MU_CALL(002745F0,0x270726);g_esp+=4;if(LO8(g_eax)){g_eax=MEM32(g_esi+28);if(g_eax){g_eax--;PUSH32(g_esp,0);g_ecx=g_esi;MEM32(g_esi+28)=g_eax;MU_CALL(0026F6A0,0x270741);g_ecx=MEM32(g_esi+276);g_xmm0=XMM_ZERO();PUSH32(g_esp,0x485f20);PUSH32(g_esp,g_ecx);MEMF(g_esi+240)=g_xmm0.f[0];MU_CALL(00058000,0x27075d);g_esp+=8;}}
  PUSH32(g_esp,33);MU_CALL(002745F0,0x270767);g_esp+=4;if(LO8(g_eax)){g_edx=MEM32(g_esi+236)-1;g_eax=MEM32(g_esi+28);if(g_eax<g_edx){g_eax++;PUSH32(g_esp,0);g_ecx=g_esi;MEM32(g_esi+28)=g_eax;MU_CALL(0026F6A0,0x270789);g_eax=MEM32(g_esi+276);g_xmm0=XMM_ZERO();PUSH32(g_esp,0x485f20);PUSH32(g_esp,g_eax);MEMF(g_esi+240)=g_xmm0.f[0];MU_CALL(00058000,0x2707a5);g_esp+=8;}}
  PUSH32(g_esp,25);MU_CALL(002745F0,0x2707af);g_esp+=4;if(LO8(g_eax))missionupdate_popup();
 }
 g_xmm1=XMM_SCALAR(MEMF(g_esp+88));return 0;
}
static void missionupdate_refresh(void){PUSH32(g_esp,0);g_ecx=g_esi;MU_CALL(0026F6A0,0x2703cc);g_xmm0=XMM_ZERO();MEMF(g_esi+240)=g_xmm0.f[0];}
static void missionupdate_advance_sound(void){g_ecx=MEM32(g_esi+276);PUSH32(g_esp,0x485f20);PUSH32(g_esp,g_ecx);MU_CALL(00058000,0x2703e8);g_xmm1=XMM_SCALAR(MEMF(g_esp+96));g_esp+=8;}
static void missionupdate_scroll(void){
 g_eax=MEM32(g_esi+236)-1;g_ecx=MEM32(g_esi+28);if(g_ecx>=g_eax)g_ecx=g_eax;g_edx=0;g_eax=g_ecx?g_ecx:0;MEM32(g_esp+44)=g_eax;mu_push((double)SMEM32(g_esp+44));MEM32(g_esi+28)=g_eax;if((int32_t)g_eax<0)MU_FP+=MEMF(0x44c510);g_xmm3=XMM_SCALAR(MEMF(0x44bf18));mu_pop(g_esp+20);mu_push(MEMF(g_esi+24));g_xmm2=XMM_SCALAR(MEMF(g_esp+20));mu_push(MEMF(g_esp+20));g_fp_cmp=RECOMP_FCMP(MU_FP,g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;
 if(g_fp_cmp>0){g_xmm0=g_xmm1;g_xmm0.f[0]*=g_xmm3.f[0];g_xmm0.f[0]+=MEMF(g_esi+24);unsigned c=mu_cmp(g_xmm0.f[0],g_xmm2.f[0]);MEMF(g_esi+24)=g_xmm0.f[0];if(!(c&0x41))MEMF(g_esi+24)=g_xmm2.f[0];}
 g_xmm0=XMM_SCALAR(MEMF(g_esi+24));if(!(mu_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41)){g_xmm1.f[0]*=g_xmm3.f[0];g_xmm0.f[0]-=g_xmm1.f[0];unsigned c=mu_cmp(g_xmm2.f[0],g_xmm0.f[0]);MEMF(g_esi+24)=g_xmm0.f[0];if(!(c&0x41))MEMF(g_esi+24)=g_xmm2.f[0];}
 g_edi=g_eax*3;MU_CALL(000643E0,0x270484);g_ecx=MEM32(g_esi+g_edi*4+32);MEM32(g_eax+28)=g_ecx;g_eax=MEM32(g_esi+28)+3;g_edi=g_eax*3;MU_CALL(000643E0,0x270499);g_edx=MEM32(g_esi+g_edi*4);PUSH32(g_esp,0);MEM32(g_eax+32)=g_edx;g_xmm0=XMM_SCALAR(MEMF(0x51cf64));g_xmm0.f[0]-=MEMF(0x51cf4c);g_xmm0.f[0]-=MEMF(0x4704c4);g_eax=MEM32(0x51cf4c);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_eax);MU_CALL(00259370,0x2704cc);
}
static void missionupdate_labels(unsigned *frame){
 *frame=MEM32(g_esp+104);g_esp+=16;g_ecx=g_esi+244;PUSH32(g_esp,*frame);MU_CALL(00274320,0x2704df);g_ecx=g_esi+256;PUSH32(g_esp,*frame);MU_CALL(00274320,0x2704eb);g_ecx=MEM32(0x509704);PUSH32(g_esp,2);MU_CALL(00017060,0x2704f8);g_ecx=MEM32(0x51cf28);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MU_CALL(00017160,0x27050f);mu_push(MEMF(0x4e8394));MU_FP-=MEMF(0x4859f0);g_ecx=MEM32(0x509704);mu_pop(g_esp+20);MU_CALL(00018130,0x27052a);MU_FP=MEMF(0x51cf64)-MU_FP;PUSH32(g_esp,204);MU_FP+=MEMF(0x44c530);mu_pop(g_esp+48);MU_CALL(000643E0,0x270544);g_ecx=g_eax;MU_CALL(00063D40,0x27054b);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x485f68);g_edx=g_esp+80;PUSH32(g_esp,16);PUSH32(g_esp,g_edx);MU_CALL(002D7159,0x27055d);
 g_edi=MEM32(g_esp+64);g_eax=MEM32(g_esp+40);g_esp+=20;PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);g_ecx=g_esp+84;PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MU_CALL(00018DE0,0x27057e);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);g_edx=g_esp+80;PUSH32(g_esp,g_edx);MU_CALL(00017E40,0x270597);MU_FP=MEMF(g_esp+20)-MU_FP;g_eax=MEM32(0x51cf5c);g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);mu_pop(g_esp+24);PUSH32(g_esp,g_eax);MU_CALL(00017160,0x2705b5);
 PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=MEM32(g_esp+28);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x485f4c);MU_CALL(00018CE0,0x2705d3);g_edx=MEM32(0x4859f0);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_edi);g_eax=g_edx;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x488638);MEM32(g_esp+48)=g_edx;MU_CALL(00018CE0,0x2705f9);
 g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x450438);MU_CALL(00017E40,0x270612);mu_pop(g_esp+44);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x488638);MU_CALL(00017E40,0x27062f);MU_FP+=MEMF(g_esp+44);MU_FP+=MEMF(g_esp+20);mu_pop(g_esp+20);MU_CALL(000643E0,0x270640);int special=MEM32(g_eax+72)==3;g_eax=special?0x51cf54:0x51cf28;g_ecx=MEM32(g_eax);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MU_CALL(00017160,0x270663);g_edx=MEM32(g_esp+20);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);MU_CALL(000643E0,0x270676);g_eax=MEM32(g_eax+72);g_ecx=MEM32(g_esp+g_eax*4+76);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MU_CALL(00018CE0,0x270689);PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);MU_CALL(002584C0,0x270691);mu_pop(g_esp+52);g_eax=MEM32(g_esi+236);g_esp+=8;MEM32(g_esp+20)=0;
}

static void missionupdate_thumbnails(unsigned *frame){
 if(g_eax){g_edi=g_esi+32;do{
  g_eax=MEM32(g_edi);*frame=g_eax==0xffffffff?0x500ba0:g_eax*24+0x500a98;
  g_edx=MEM32(g_esp+20);mu_push((double)SMEM32(g_esp+20));if((int32_t)g_edx<0)MU_FP+=MEMF(0x44c510);MU_FP-=MEMF(g_esi+24);MEMF(g_esp+28)=(float)MU_FP;MU_FP=fabs(MU_FP);MEMF(g_esp+24)=(float)MU_FP;mu_push(MEMF(0x44bf94));g_fp_cmp=RECOMP_FCMP(MU_FP,g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;
  if(g_fp_cmp>0){g_xmm0=XMM_SCALAR(MEMF(g_esp+24));g_xmm1=XMM_ZERO();if(mu_cmp(g_xmm0.f[0],g_xmm1.f[0])&0x41)g_xmm0=g_xmm1;}else g_xmm0=XMM_SCALAR(MEMF(0x44bf94));
  g_xmm1=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,g_ecx);g_xmm1.f[0]-=g_xmm0.f[0];MEMF(g_esp)=g_xmm1.f[0];MU_CALL(002D5060,0x2708ec);mu_pop(g_esp+28);g_eax=MEM32(g_edi+4);g_ecx=MEM32(g_edi);g_esp+=4;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);MU_CALL(000643E0,0x2708ff);g_ecx=g_eax;MU_CALL(00063C20,0x270906);ucrr_level_thumb_begin();
  g_eax=MEM32(g_eax);g_edx=MEM32(g_esi+240);g_ecx=MEM32(g_esp+44);g_ebx=MEM32(g_esp+24);PUSH32(g_esp,g_eax);int empty=g_eax==0xffffffff;SET_LO8(g_eax,empty);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+36);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,*frame);MU_CALL(0026F1B0,0x27092c);
  mu_push(MEMF(0x44c174));mu_push(MEMF(g_esp+52));g_esp+=28;g_fp_cmp=RECOMP_FCMP(MU_FP,g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;if(g_fp_cmp>0){g_eax=MEM32(*frame+12);PUSH32(g_esp,0xc1300000);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);MU_CALL(0026F0B0,0x27094e);g_esp+=12;}
  g_eax=MEM32(g_esp+20);g_ecx=MEM32(g_esi+236);g_eax++;g_edi+=12;MEM32(g_esp+20)=g_eax;
 }while(g_eax<g_ecx);*frame=MEM32(g_esp+88);g_ebx=MEM32(g_esp+92);}
}
static void missionupdate_overlay(void){
 g_xmm0=XMM_SCALAR(MEMF(g_esi+24));if(!(mu_cmp(g_xmm0.f[0],MEMF(0x500a4c))&1)){g_xmm0=XMM_SCALAR(MEMF(0x4859ec));g_xmm0.f[0]+=MEMF(0x4859f0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x43ca0000);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,2);MU_CALL(00259860,0x2709aa);g_esp+=20;}
 g_ecx=MEM32(g_esi+236);mu_push((double)SMEM32(g_esi+236));if((int32_t)g_ecx<0)MU_FP+=MEMF(0x44c510);MU_FP-=MEMF(0x500a50);MU_FP-=MEMF(0x44bf94);mu_push(MEMF(g_esi+24));double t=MU_FP;MU_FP=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[(g_fp_top+1)&7]=t;g_fp_cmp=RECOMP_FCMP(MU_FP,g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;
 if(g_fp_cmp>0){g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_xmm1=XMM_SCALAR(MEMF(0x4859ec));g_xmm0.f[0]-=MEMF(0x4859f0);g_xmm1.f[0]*=MEMF(0x44c1f8);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x43ca0000);PUSH32(g_esp,g_ecx);g_xmm0.f[0]-=g_xmm1.f[0];MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,3);MU_CALL(00259860,0x270a17);g_esp+=20;}
 g_xmm0=XMM_SCALAR(MEMF(g_esp+44));g_xmm2=XMM_ZERO();if(!(mu_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41)){g_xmm1=XMM_SCALAR(MEMF(0x4e8394));g_xmm0.f[0]*=MEMF(0x485f2c);g_edx=g_esp+88;PUSH32(g_esp,g_edx);g_eax=g_esp+48;MEMF(g_esp+48)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x485cc8));PUSH32(g_esp,g_eax);g_ecx=g_esp+36;g_xmm0.f[0]+=g_xmm1.f[0];PUSH32(g_esp,g_ecx);MEM32(g_esp+100)=0xff000000;MEMF(g_esp+60)=g_xmm0.f[0];MEMF(g_esp+40)=g_xmm2.f[0];MEMF(g_esp+44)=g_xmm1.f[0];MU_CALL(000656D0,0x270a78);g_esp+=12;}
}
static void missionupdate_finish(unsigned frame){
 SET_LO8(g_eax,MEM8(g_esp+19));int modal=LO8(g_eax)!=0;if(!modal){g_ecx=MEM32(g_esi+268);g_edx=MEM32(g_ecx);unsigned saved=g_esp,target=MEM32(g_edx+12);PUSH32(g_esp,0x270a92);RECOMP_ICALL_SAFE(target,saved);modal=LO8(g_eax)!=0;}
 if(modal){g_esi=MEM32(g_esi+268);g_edx=MEM32(g_esi);g_ebx+=24;unsigned saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);g_ecx=g_esi;unsigned target=MEM32(g_edx+8);PUSH32(g_esp,0x270bca);RECOMP_ICALL_SAFE(target,saved);if(g_eax){g_ecx=g_esp+44;saved=g_esp;PUSH32(g_esp,g_ecx);target=g_eax;PUSH32(g_esp,0x270bd5);RECOMP_ICALL_SAFE(target,saved);g_esp+=4;}}
 else{
  g_eax=MEM32(0x485a18);PUSH32(g_esp,g_eax);MU_CALL(002745F0,0x270aa5);g_esp+=4;if(LO8(g_eax)){g_ecx=g_esi+244;MU_CALL(00274300,0x270ab7);g_ecx=g_esi+256;MU_CALL(00274300,0x270ac2);PUSH32(g_esp,0x4509a8);MU_CALL(00057FC0,0x270acc);g_eax=MEM32(g_esi+20);g_esp+=4;goto keep;}
  PUSH32(g_esp,24);MU_CALL(002745F0,0x270af0);g_esp+=4;if(LO8(g_eax)){g_ecx=g_esi+244;MU_CALL(00274300,0x270b02);g_ecx=g_esi+256;MU_CALL(00274300,0x270b0d);PUSH32(g_esp,0x450958);MU_CALL(00057FC0,0x270b17);g_eax=MEM32(g_esi+12);g_esp+=4;goto keep;}
  PUSH32(g_esp,22);MU_CALL(002745F0,0x270b2e);g_esp+=4;if(LO8(g_eax)){PUSH32(g_esp,0x450958);MU_CALL(00057FC0,0x270b43);g_eax=MEM32(g_esi+28);g_edx=MEM32(g_esi+4);g_ecx=g_eax*3;g_eax=MEM32(g_esi+g_ecx*4+32);MEM32(g_edx)=g_eax;g_eax=MEM32(g_esi+28);g_edx=MEM32(g_esi+8);g_eax+=3;g_ecx=g_eax*3;g_eax=MEM32(g_esi+g_ecx*4);g_esp+=4;g_ecx=g_esi+244;MEM32(g_edx)=g_eax;MU_CALL(00274300,0x270b71);g_ecx=g_esi+256;MU_CALL(00274300,0x270b7c);g_eax=MEM32(g_esi+28);g_ecx=g_eax*3+9;g_edx=MEM32(g_esi+g_ecx*4);g_eax*=3;g_ecx=MEM32(g_esi+g_eax*4+32);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);MU_CALL(000643E0,0x270b94);g_ecx=g_eax;MU_CALL(00063C20,0x270b9b);g_edx=MEM32(g_eax);MEM32(0x51d0d4)=g_edx;g_eax=MEM32(g_eax+4);POP32(g_esp,g_edi);MEM32(0x51d0d8)=g_eax;g_eax=MEM32(g_esi+16);goto rest;}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=0;goto end;
keep: POP32(g_esp,g_edi);
rest: POP32(g_esp,g_esi);POP32(g_esp,frame);
end: POP32(g_esp,g_ebx);g_esp+=68;g_recomp_zf=!g_esp;g_esp+=12;
}
int migration_interface_missionupdate(unsigned operation){
 static int enabled=-1;if(enabled<0){const char *v=getenv("UCRR_NATIVE_INTERFACE_MISSIONUPDATE");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation||(g_esp&3)||!mu_span(g_ecx,280)||!mu_span((uint64_t)g_esp-4096,4112)||MEM32(0x485a1c)>11)return 0;
 unsigned obj=g_ecx,modal=MEM32(obj+268);if(!mu_span(modal,4)||!mu_span(MEM32(modal),16)||!mu_span(MEM32(g_esp+8),88)||!mu_span(MEM32(obj+4),4)||!mu_span(MEM32(obj+8),4)||MEM32(obj+28)>16)return 0;
 unsigned frame=g_seh_ebp;g_esp-=68;unsigned saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+268);MEM32(g_esp+48)=0x488674;MEM32(g_esp+52)=0x488660;MEM32(g_esp+56)=0x48864c;MEM32(g_esp+60)=0x485c94;g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edi);unsigned target=MEM32(g_eax+12);PUSH32(g_esp,0x270354);RECOMP_ICALL_SAFE(target,saved);MEM8(g_esp+19)=LO8(g_eax);g_eax=g_esi+32;PUSH32(g_esp,g_eax);MU_CALL(0026EF70,0x270361);
 g_xmm0=XMM_SCALAR(MEMF(g_esi+240));g_xmm1=XMM_SCALAR(MEMF(g_esp+92));g_xmm2=XMM_ZERO();g_xmm0.f[0]+=g_xmm1.f[0];MEMF(g_esi+240)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+272));g_esp+=4;MEM32(g_esi+236)=g_eax;
 if(!(mu_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41)){g_xmm0.f[0]-=g_xmm1.f[0];unsigned c=mu_cmp(g_xmm2.f[0],g_xmm0.f[0]);MEM8(g_esp+19)=1;MEMF(g_esi+272)=g_xmm0.f[0];if(!(c&1)&&MEM32(0x51d0d4)==0xffffffff){g_ecx=MEM32(g_esi+28);g_eax--;if(g_ecx<g_eax){g_ecx++;MEM32(g_esi+28)=g_ecx;missionupdate_refresh();}missionupdate_advance_sound();}g_ebx=MEM32(g_esp+92);}
 else if(missionupdate_input()){missionupdate_refresh();missionupdate_advance_sound();g_ebx=MEM32(g_esp+92);}
 missionupdate_scroll();missionupdate_labels(&frame);missionupdate_thumbnails(&frame);missionupdate_overlay();missionupdate_finish(frame);return 1;
}
