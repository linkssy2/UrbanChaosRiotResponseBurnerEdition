/* Unqualified controller candidate; isolated qualification precedes integration. */
#include "migration_interface_controller.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00274320(void),sub_00057FC0(void),sub_00064E30(void),sub_002745F0(void),sub_00274770(void),sub_00263230(void),sub_002633A0(void),sub_0005D9E0(void),sub_00258DF0(void);
extern void ucrr_menu_list_hover(void),ucrr_menu_list_leave(void);
extern void sub_00017060(void),sub_00018130(void),sub_002584C0(void),sub_00017730(void),sub_000643E0(void),sub_00055E00(void),sub_00254060(void),sub_00259370(void);
extern void sub_00065760(void),sub_000656D0(void),sub_00065550(void),sub_00259860(void),sub_00274300(void);
#define CT_TOP g_fp_stack[g_fp_top]
#define CT_ST1 g_fp_stack[(g_fp_top+1)&7]
#define CT_POP() (g_fp_top=(g_fp_top+1)&7)
#define CT_PUSH(v) do{double value=(v);g_fp_top=(g_fp_top+7)&7;CT_TOP=value;}while(0)
#define CT_CALL(name,ret) do{PUSH32(g_esp,ret);sub_##name();}while(0)
static void ct_indirect(unsigned target,unsigned ret,unsigned saved){PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void ct_return(unsigned *frame,int pushed){if(pushed)POP32(g_esp,*frame);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=88;g_recomp_zf=!g_esp;ucrr_menu_list_leave();g_esp+=12;}
static int controller_begin(unsigned *frame){
 g_eax=MEM32(g_esp+4);g_esp-=88;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;g_ecx=g_esi+220;PUSH32(g_esp,g_eax);CT_CALL(00274320,0x2659d8);
 g_eax=MEM32(g_esi+180);if((signed char)HI8(g_eax)<0){PUSH32(g_esp,0x450958);CT_CALL(00057FC0,0x2659ec);g_eax=MEM32(g_esi+180);g_esp+=4;g_eax&=0xffff7fff;MEM32(g_esi+180)=g_eax;}
 g_edi=MEM32(g_esp+108);SET_LO8(g_eax,MEM8(g_edi+72));g_ebx=0;if(LO8(g_eax))goto disabled;
 g_ecx=0x4e8364;CT_CALL(00064E30,0x265a1b);if(!g_eax)goto inactive;
 g_ecx=0x4e8364;CT_CALL(00064E30,0x265a2d);if(g_eax==1)goto inactive;
 g_ecx=0x4e8364;CT_CALL(00064E30,0x265a40);if(g_eax==4)goto inactive;
 MEM32(g_esp+32)=g_ebx;
 if(!(MEM32(g_esi+180)&0x120)){g_ecx=MEM32(g_esi+196);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+12),0x265a64,g_esp);if(!LO8(g_eax)){g_eax=MEM32(0x485a1c);if(g_eax!=11){MEM32(g_edi+g_eax*4+24)=0x4855c8;g_eax=MEM32(0x485a1c);}if((MEM32(g_esi+180)&0x40000)&&g_eax!=11)MEM32(g_edi+g_eax*4+24)=0x4877a8;}}
 g_eax=MEM32(g_esi+180);PUSH32(g_esp,*frame);
 if(g_eax&0x400){g_ecx=MEM32(g_esi+196);g_eax=MEM32(g_ecx);ct_indirect(MEM32(g_eax+12),0x265aaf,g_esp);if(!LO8(g_eax))MEM32(g_edi+36)=0x487788;PUSH32(g_esp,24);CT_CALL(002745F0,0x265ac1);g_esp+=4;
  if(LO8(g_eax)){if(MEM32(g_esi+88)>g_ebx){*frame=g_esi+8;do{g_ecx=MEM32(*frame);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+24),0x265ad8,g_esp);g_eax=MEM32(g_esi+88);g_ebx++;*frame+=4;}while(g_ebx<g_eax);}PUSH32(g_esp,1);PUSH32(g_esp,0xbf800000);CT_CALL(00263230,0x265aef);g_esp+=8;CT_CALL(002633A0,0x265af7);g_ebx=0;}}
 g_eax=MEM32(g_esi+180);if(g_eax&2){g_ecx=MEM32(g_edi+20);if(MEM8(g_ecx)&&MEM32(g_esi+172)!=g_ebx){MEM8(g_esi+176)=1;if(g_eax&0x4000){g_eax&=0xffffbfff;g_ecx=0x515114;MEM32(g_esi+180)=g_eax;CT_CALL(0005D9E0,0x265b38);}g_eax=MEM32(g_esi+172);ct_return(frame,1);return 0;}}
 g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi+88);if(MEM32(g_eax)>=g_ecx){g_ecx--;MEM32(g_eax)=g_ecx;}g_eax=MEM32(g_esi+200);MEM8(g_esp+18)=0;if(g_eax!=g_ebx){SET_LO8(g_edx,MEM8(g_eax));MEM8(g_esp+18)=LO8(g_edx);}
 const unsigned readiness[]={206,207,204,205},returns[]={0x265b7b,0x265b9b,0x265bbb,0x265bdb};
 for(unsigned i=0;i<4;i++){SET_LO8(g_eax,MEM8(g_esi+readiness[i]));if(!LO8(g_eax)){PUSH32(g_esp,30+i);PUSH32(g_esp,returns[i]);sub_00274770();g_esp+=4;SET_LO8(g_eax,!LO8(g_eax));MEM8(g_esi+readiness[i])=LO8(g_eax);}}
 if(MEM8(g_esi+180)&1){SET_LO8(g_eax,MEM8(g_esi+207));SET_LO8(g_ebx,MEM8(g_esi+206));*frame=30;g_edi=31;MEM8(g_esp+19)=LO8(g_eax);}else{SET_LO8(g_ecx,MEM8(g_esi+205));SET_LO8(g_ebx,MEM8(g_esi+204));*frame=32;g_edi=33;MEM8(g_esp+19)=LO8(g_ecx);}
 g_ecx=MEM32(g_esi+196);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+12),0x265c34,g_esp);int modal=LO8(g_eax)!=0;if(!modal){SET_LO8(g_eax,MEM8(g_esp+18));modal=LO8(g_eax)!=0;}
 if(modal){
  g_ecx=MEM32(g_esi+196);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+12),0x265e11,g_esp);if(!LO8(g_eax))return 1;g_eax=MEM32(g_esi+180);
  if(!(g_eax&0x100)){g_eax=MEM32(g_esi+88);*frame=0;if(g_eax){g_edi=MEM32(g_esp+112);g_edi+=24;g_ebx=g_esi+8;do{g_ecx=MEM32(g_ebx);g_edx=MEM32(g_esp+108);g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,g_edi);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);ct_indirect(MEM32(g_eax+4),0x265e42,saved);g_eax=MEM32(g_esi+88);(*frame)++;g_ebx+=4;}while(*frame<g_eax);}}
  g_edx=MEM32(g_esp+112);g_ecx=MEM32(g_esi+196);g_eax=MEM32(g_ecx);g_edx+=24;unsigned saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+112);PUSH32(g_esp,g_edx);ct_indirect(MEM32(g_eax+8),0x265e65,saved);MEM32(g_esp+36)=g_eax;if(!g_eax)return 1;if(g_eax!=0x284310){ct_return(frame,1);return 0;}MEM32(g_esp+36)=0;return 1;
 }
 ucrr_menu_list_hover();g_eax=MEM32(g_esi+88);if(!g_eax){g_ecx=MEM32(g_esi+4);MEM32(g_ecx)=0;return 1;}
 g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);MEM32(g_esp+48)=g_ecx;
 if(LO8(g_ebx)){PUSH32(g_esp,*frame);CT_CALL(002745F0,0x265c66);g_esp+=4;if(LO8(g_eax)){do{g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);if(g_ecx){g_ecx--;MEM32(g_eax)=g_ecx;}else if(MEM8(g_esi+180)&1){g_edx=MEM32(g_esi+88);g_edx--;MEM32(g_eax)=g_edx;}g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);g_edx=MEM32(g_esi+g_ecx*4+8);SET_LO8(g_eax,MEM8(g_edx+4));}while(!LO8(g_eax));}}
 SET_LO8(g_eax,MEM8(g_esp+19));if(LO8(g_eax)){PUSH32(g_esp,g_edi);CT_CALL(002745F0,0x265cab);g_esp+=4;if(LO8(g_eax)){do{g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi+88);g_edx=MEM32(g_eax);g_ecx--;if(g_edx<g_ecx){g_ecx=g_edx+1;MEM32(g_eax)=g_ecx;}else if(MEM8(g_esi+180)&1)MEM32(g_eax)=0;g_edx=MEM32(g_esi+4);g_eax=MEM32(g_edx);g_ecx=MEM32(g_esi+g_eax*4+8);SET_LO8(g_eax,MEM8(g_ecx+4));}while(!LO8(g_eax));}}
 g_edx=MEM32(g_esi+4);g_eax=MEM32(g_esp+48);if(g_eax!=MEM32(g_edx)){PUSH32(g_esp,0x450960);CT_CALL(00057FC0,0x265cfa);g_esp+=4;}
 g_eax=MEM32(g_esi+180);if(g_eax&0x100)return 1;g_eax=MEM32(g_esi+88);*frame=0;
 if(g_eax){g_edi=MEM32(g_esp+112);g_ebx=g_esi+8;do{g_ecx=MEM32(g_esi+4);if(*frame!=MEM32(g_ecx)){g_ecx=MEM32(g_ebx);g_edx=MEM32(g_ecx);g_eax=g_edi+24;unsigned saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+112);PUSH32(g_esp,0);PUSH32(g_esp,g_eax);ct_indirect(MEM32(g_edx+4),0x265d39,saved);}g_eax=MEM32(g_esi+88);(*frame)++;g_ebx+=4;}while(*frame<g_eax);}
 g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);if(g_ecx>=MEM32(g_esi+88))return 1;g_edx=g_ecx;g_ecx=MEM32(g_esi+g_edx*4+8);g_edx=MEM32(g_esp+112);g_eax=MEM32(g_ecx);g_edx+=24;unsigned saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+112);PUSH32(g_esp,1);PUSH32(g_esp,g_edx);ct_indirect(MEM32(g_eax+4),0x265d6c,saved);g_ebx=g_eax;MEM32(g_esp+36)=g_ebx;if(!g_ebx)return 1;
 g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);g_ecx=MEM32(g_esi+g_ecx*4+8);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+16),0x265d88,g_esp);if(!LO8(g_eax))return 1;
 g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);g_ecx=MEM32(g_esi+g_ecx*4+8);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+20),0x265d9e,g_esp);int text=g_eax!=0;PUSH32(g_esp,3);PUSH32(g_esp,0x284310);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0);
 if(text){g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);g_ecx=MEM32(g_esi+g_ecx*4+8);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+20),0x265dba,g_esp);PUSH32(g_esp,g_eax);g_eax=g_esp+116;PUSH32(g_esp,g_eax);CT_CALL(00258DF0,0x265dc5);g_ecx=MEM32(g_esi+196);g_edx=MEM32(g_ecx);}
 else{g_ecx=g_esp+112;PUSH32(g_esp,0x48776c);PUSH32(g_esp,g_ecx);CT_CALL(00258DF0,0x265dde);g_edx=MEM32(g_esi+196);g_edx=MEM32(g_edx);}
 g_ecx=MEM32(g_eax+4);g_eax=MEM32(g_eax);g_esp+=24;saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+196);PUSH32(g_esp,g_eax);ct_indirect(MEM32(g_edx+4),0x265df9,saved);MEM32(g_esp+36)=0;return 1;
inactive:
 {const unsigned offsets[]={28,24,36,32,56,60,64,52,40};for(unsigned i=0;i<9;i++)MEM32(g_edi+offsets[i])=g_ebx;}
disabled:
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=0;POP32(g_esp,g_ebx);g_esp+=88;g_recomp_zf=!g_esp;ucrr_menu_list_leave();g_esp+=12;return 0;
}
static void controller_layout_setup(void){
 g_ecx=MEM32(0x509704);g_edi=MEM32(g_esp+112);PUSH32(g_esp,2);CT_CALL(00017060,0x265e91);g_xmm0=XMM_ZERO();g_eax=MEM32(g_esi+180);MEMF(g_esp+40)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x4859f8));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x51cf7c));MEMF(g_esp+28)=g_xmm0.f[0];
 if(g_eax&1){g_xmm0=XMM_SCALAR(MEMF(0x51cf64));g_xmm0.f[0]-=MEMF(0x44fe44);MEMF(g_esp+20)=g_xmm0.f[0];}
 if(g_eax&4){g_ecx=MEM32(0x509704);CT_CALL(00018130,0x265ee5);CT_PUSH((double)SMEM32(g_esi+88));g_eax=MEM32(g_esi+88);if((int32_t)g_eax<0)CT_TOP+=MEMF(0x44c510);CT_ST1*=CT_TOP;CT_POP();CT_TOP+=MEMF(0x51cf4c);MEMF(g_esp+20)=(float)CT_TOP;CT_POP();}
 if(!(MEM8(g_esi+180)&2)){g_ecx=g_esp+56;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);CT_CALL(002584C0,0x265f15);CT_TOP*=MEMF(0x4523d0);SET_LO8(g_eax,MEM8(g_esi+180));g_esp+=8;MEMF(g_esp+40)=(float)CT_TOP;CT_POP();if(!(LO8(g_eax)&2)){g_edx=MEM32(g_esp+56);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edx);CT_CALL(00017730,0x265f3c);}}
 SET_LO8(g_eax,MEM8(g_esi+180));if((signed char)LO8(g_eax)<0){CT_CALL(000643E0,0x265f4b);SET_LO8(g_ecx,MEM8(g_esi+192));if(LO8(g_ecx)!=MEM8(g_eax+48)){CT_CALL(000643E0,0x265f5b);SET_LO8(g_edx,MEM8(g_eax+48));MEM8(g_esi+192)=LO8(g_edx);g_ecx=MEM32(0x51c4dc);CT_CALL(00055E00,0x265f6f);g_edx=MEM32(g_eax);g_ecx=0;SET_LO8(g_ecx,MEM8(g_esi+192));unsigned saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0);g_ecx=g_eax;ct_indirect(MEM32(g_edx+16),0x265f81,saved);g_ecx=MEM32(0x51c4dc);CT_CALL(00055E00,0x265f8c);g_edx=MEM32(g_eax);saved=g_esp;PUSH32(g_esp,2);PUSH32(g_esp,0x3f19999a);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);g_ecx=g_eax;ct_indirect(MEM32(g_edx+24),0x265fa1,saved);}}
 g_eax=MEM32(g_esi+180);if(g_eax&0x200){g_eax=MEM32(g_esp+40);PUSH32(g_esp,g_eax);CT_CALL(00254060,0x265fb6);g_esp+=4;}g_eax=MEM32(g_esi+180);
}
static void controller_vertical(unsigned *frame){
 if(!(g_eax&0x100)){g_ecx=MEM32(g_esi+196);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+12),0x265fd7,g_esp);if(!LO8(g_eax)){g_eax=MEM32(g_esi+88);if(g_eax){MEM32(g_edi+64)=0x450978;if(!(MEM32(g_esi+180)&0x10000))MEM32(g_edi+60)=0x450990;}}}
 if(MEM8(g_esi+180)&16){g_xmm0=XMM_SCALAR(MEMF(0x51cf64));g_xmm0.f[0]-=MEMF(0x51cf4c);g_eax=MEM32(0x51cf4c);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_eax);CT_CALL(00259370,0x26602a);g_esp+=16;}
 g_xmm0=XMM_ZERO();if(!(MEM8(g_esi+180)&64))g_xmm0=XMM_SCALAR(MEMF(0x46c320));g_ebx=MEM32(g_esi+88);g_ebx--;MEMF(g_esp+108)=g_xmm0.f[0];if((int32_t)g_ebx<0)return;g_edi=g_esi+g_ebx*4+8;
 do{g_ecx=MEM32(0x509704);CT_CALL(00018130,0x266060);CT_TOP+=MEMF(g_esp+108);int striped=(MEM8(g_esi+180)&8)!=0;CT_TOP=MEMF(g_esp+20)-CT_TOP;MEMF(g_esp+20)=(float)CT_TOP;CT_POP();*frame=MEM32(g_esp+20);
  if(striped){g_ecx=0;SET_LO8(g_ecx,LO8(g_ebx));PUSH32(g_esp,0);g_ecx&=0xffffff01;PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);CT_CALL(00018130,0x266091);CT_TOP+=MEMF(g_esp+116);PUSH32(g_esp,g_ecx);MEMF(g_esp)=(float)CT_TOP;CT_POP();PUSH32(g_esp,*frame);CT_CALL(00259370,0x26609f);g_esp+=16;}
  int variable=(MEM8(g_esi+180)&64)!=0;g_ecx=MEM32(g_edi);g_edx=MEM32(g_ecx);g_eax=MEM32(variable?g_esp+28:g_edi+84);unsigned saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+4);int selected=g_ebx==MEM32(g_eax);PUSH32(g_esp,0);SET_LO8(g_eax,selected);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+40);PUSH32(g_esp,*frame);PUSH32(g_esp,g_eax);ct_indirect(MEM32(g_edx+8),variable?0x2660c8:0x2660ea,saved);if(variable){CT_TOP=MEMF(g_esp+20)-CT_TOP;MEMF(g_esp+20)=(float)CT_TOP;}CT_POP();g_ebx--;g_edi-=4;
 }while((int32_t)g_ebx>=0);
}
static unsigned ct_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static void controller_horizontal(unsigned *frame){
 if(!(g_eax&0x100)){g_ecx=MEM32(g_esi+196);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+12),0x26610d,g_esp);if(!LO8(g_eax)){g_eax=MEM32(g_esi+88);if(g_eax){MEM32(g_edi+60)=0x450978;if(!(MEM32(g_esi+180)&0x10000))MEM32(g_edi+28)=0x450990;}}}
 g_eax=MEM32(g_esi+88);g_xmm0=XMM_SCALAR(MEMF(0x51cf58));MEMF(g_esp+24)=g_xmm0.f[0];
 if(g_eax){g_eax=MEM32(g_esi+4);*frame=MEM32(g_eax);g_ecx=MEM32(g_eax);g_ecx=MEM32(g_esi+g_ecx*4+8);g_edx=MEM32(g_ecx);g_edi=0;g_eax=g_edi<*frame;unsigned saved=g_esp;PUSH32(g_esp,g_eax);ct_indirect(MEM32(g_edx+12),0x266160,saved);CT_TOP*=MEMF(0x44c1f8);g_eax=MEM32(g_esi+88);CT_TOP=MEMF(g_esp+24)-CT_TOP;MEMF(g_esp+24)=(float)CT_TOP;CT_POP();g_xmm0=XMM_SCALAR(MEMF(g_esp+24));
  if(g_eax){g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);g_ebx=0;if(g_eax){*frame=g_esi+8;do{g_edx=MEM32(g_esi+88);g_eax=0;g_edx--;if(g_ebx!=g_edx)g_eax=1;g_ecx=MEM32(*frame);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,g_eax);ct_indirect(MEM32(g_edx+12),0x2661a8,saved);CT_TOP=MEMF(g_esp+24)-CT_TOP;g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);g_ebx++;MEMF(g_esp+24)=(float)CT_TOP;CT_POP();*frame+=4;}while(g_ebx<g_ecx);g_xmm0=XMM_SCALAR(MEMF(g_esp+24));}}
 }
 g_eax=MEM32(g_esi+88);g_xmm1=XMM_ZERO();MEMF(g_esp+32)=g_xmm1.f[0];if(g_eax){g_ebx=0;*frame=g_esi+8;do{g_ecx=MEM32(g_esi+88);g_eax=0;g_ecx--;if(g_ebx!=g_ecx)g_eax=1;g_ecx=MEM32(*frame);g_edx=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,g_eax);ct_indirect(MEM32(g_edx+12),0x2661f8,saved);CT_TOP+=MEMF(g_esp+32);g_eax=MEM32(g_esi+88);g_ebx++;*frame+=4;MEMF(g_esp+32)=(float)CT_TOP;CT_POP();}while(g_ebx<g_eax);g_xmm1=XMM_SCALAR(MEMF(g_esp+32));g_xmm0=XMM_SCALAR(MEMF(g_esp+24));}
 g_xmm3=XMM_SCALAR(MEMF(0x51cf7c));g_xmm2=XMM_SCALAR(MEMF(0x4859f0));g_xmm4=g_xmm3;g_xmm4.f[0]-=g_xmm2.f[0];
 if(!(ct_cmp(g_xmm4.f[0],g_xmm1.f[0])&1)){g_xmm2=XMM_SCALAR(MEMF(0x44c1f8));g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_xmm0.f[0]*=g_xmm2.f[0];g_xmm1.f[0]*=g_xmm2.f[0];g_xmm0.f[0]-=g_xmm1.f[0];MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+24));}
 else{if(!(ct_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41)){MEMF(g_esp+24)=g_xmm2.f[0];g_xmm0=g_xmm2;}g_xmm3.f[0]-=g_xmm1.f[0];g_xmm1=g_xmm3;if(!(ct_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41)){MEMF(g_esp+24)=g_xmm1.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+24));}}
 SET_LO8(g_eax,MEM8(g_esi+216));if(!LO8(g_eax)){MEMF(g_esi+212)=g_xmm0.f[0];MEM8(g_esi+216)=1;}
 else{g_xmm1=XMM_SCALAR(MEMF(0x487768));unsigned f=ct_cmp(g_xmm1.f[0],MEMF(g_esp+108));g_xmm1=XMM_SCALAR(MEMF(g_esi+212));if(!(f&0x41)){g_xmm1.f[0]*=MEMF(0x47df9c);g_xmm0.f[0]*=MEMF(0x44c5f0);}else{g_xmm1.f[0]*=MEMF(0x44c5fc);g_xmm0.f[0]*=MEMF(0x44e594);}g_xmm1.f[0]+=g_xmm0.f[0];MEMF(g_esi+212)=g_xmm1.f[0];}
 g_xmm0=XMM_SCALAR(MEMF(g_esi+212));g_eax=MEM32(g_esi+88);MEMF(g_esp+76)=g_xmm0.f[0];g_xmm0.f[0]+=MEMF(g_esp+40);g_edi=0;MEMF(g_esp+28)=g_xmm0.f[0];
 if(g_eax){*frame=g_esi+8;do{g_eax=MEM32(g_esi+88);g_eax--;g_edx=g_edi==g_eax?4:5;CT_PUSH(MEMF(g_esi+212));CT_TOP-=MEMF(g_esp+24);CT_TOP=fabs(CT_TOP);CT_PUSH(MEMF(0x4875e8));g_fp_cmp=RECOMP_FCMP(CT_TOP,CT_ST1);CT_POP();CT_POP();if(g_fp_cmp>0)g_edx|=2;g_eax=MEM32(g_esp+28);g_ecx=MEM32(*frame);g_ebx=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+4);SET_LO8(g_edx,g_edi==MEM32(g_edx));PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+32);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);ct_indirect(MEM32(g_ebx+8),0x26634d,saved);CT_TOP+=MEMF(g_esp+28);g_eax=MEM32(g_esi+88);g_edi++;*frame+=4;MEMF(g_esp+28)=(float)CT_TOP;CT_POP();}while(g_edi<g_eax);}
}
static void controller_scroll_edges(unsigned *frame){
 g_xmm0=XMM_SCALAR(MEMF(0x51cf7c));g_ecx=MEM32(0x509704);g_xmm1=g_xmm0;g_xmm1.f[0]-=MEMF(0x44c260);MEMF(g_esp+40)=g_xmm0.f[0];g_xmm0.f[0]-=MEMF(0x4859ec);g_xmm0.f[0]+=MEMF(0x44ec38);MEMF(g_esp+48)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x4e8394));MEMF(g_esp+60)=g_xmm1.f[0];MEMF(g_esp+24)=g_xmm0.f[0];CT_CALL(00018130,0x2663ae);CT_TOP+=MEMF(g_esp+20);g_xmm0=XMM_SCALAR(MEMF(0x51cf7c));g_xmm0.f[0]-=MEMF(g_esp+32);g_xmm0.f[0]+=MEMF(0x4875ec);MEMF(g_esp+108)=(float)CT_TOP;CT_POP();g_xmm1=XMM_SCALAR(MEMF(g_esi+212));g_ebx=0xffffffff;unsigned f=ct_cmp(g_xmm1.f[0],g_xmm0.f[0]);g_edi=0xff000000;
 if(!(f&0x41)){
  g_xmm0=XMM_SCALAR(MEMF(g_esp+40));PUSH32(g_esp,0);g_eax=g_esp+48;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+208);MEMF(g_esp+76)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+116));g_ecx=g_esp+76;PUSH32(g_esp,g_ecx);MEMF(g_esp+84)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+60));g_edx=g_esp+60;PUSH32(g_esp,g_edx);MEMF(g_esp+64)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+36));PUSH32(g_esp,g_eax);MEM32(g_esp+64)=g_ebx;MEMF(g_esp+72)=g_xmm0.f[0];CT_CALL(00065760,0x266436);
  g_xmm0=XMM_SCALAR(MEMF(g_esp+80));MEMF(g_esp+88)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+128));g_ecx=g_esp+68;PUSH32(g_esp,g_ecx);MEMF(g_esp+96)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+64));g_edx=g_esp+92;PUSH32(g_esp,g_edx);g_eax=g_esp+88;MEMF(g_esp+88)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+48));PUSH32(g_esp,g_eax);MEM32(g_esp+80)=g_edi;MEMF(g_esp+96)=g_xmm0.f[0];CT_CALL(000656D0,0x266481);g_esp+=32;
 }
 g_xmm0=XMM_SCALAR(MEMF(0x4859f0));g_xmm1=g_xmm0;g_xmm1.f[0]+=MEMF(0x44c260);MEMF(g_esp+68)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x4859ec));g_xmm2=g_xmm0;g_xmm1.f[0]+=g_xmm0.f[0];g_xmm0.f[0]-=MEMF(0x4875ec);f=ct_cmp(g_xmm0.f[0],MEMF(g_esi+212));g_xmm1.f[0]-=MEMF(0x44ec38);MEMF(g_esp+44)=g_xmm2.f[0];MEMF(g_esp+48)=g_xmm1.f[0];
 if(!(f&0x41)){
  g_xmm0=XMM_SCALAR(MEMF(g_esp+108));g_ecx=g_esp+60;PUSH32(g_esp,g_ecx);MEMF(g_esp+88)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_edx=g_esp+84;PUSH32(g_esp,g_edx);g_eax=g_esp+96;MEMF(g_esp+96)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+28));PUSH32(g_esp,g_eax);MEM32(g_esp+72)=g_edi;MEMF(g_esp+92)=g_xmm2.f[0];MEMF(g_esp+104)=g_xmm0.f[0];CT_CALL(000656D0,0x266514);
  g_xmm0=XMM_SCALAR(MEMF(0x4875ec));g_xmm1=XMM_ZERO();PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_esp+80;PUSH32(g_esp,g_ecx);MEMF(g_esp+116)=g_xmm0.f[0];MEMF(g_esp+104)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+92));g_edx=g_esp+112;PUSH32(g_esp,g_edx);g_eax=g_esp+108;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+208);MEMF(g_esp+100)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+140));g_ecx=g_esp+100;PUSH32(g_esp,g_ecx);MEMF(g_esp+108)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+80));g_edx=g_esp+132;PUSH32(g_esp,g_edx);MEMF(g_esp+136)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+60));PUSH32(g_esp,g_eax);MEM32(g_esp+104)=g_ebx;MEMF(g_esp+132)=g_xmm1.f[0];MEMF(g_esp+128)=g_xmm1.f[0];MEMF(g_esp+144)=g_xmm0.f[0];CT_CALL(00065550,0x2665a6);g_esp+=44;
 }
 g_eax=MEM32(g_esi+184);g_xmm0=XMM_SCALAR(MEMF(0x485a10));g_xmm0.f[0]+=MEMF(g_esp+20);MEMF(g_esp+20)=g_xmm0.f[0];g_ecx=(int32_t)g_eax>=255?255:(int32_t)g_eax>0?g_eax:0;g_eax=MEM32(g_esi+188);MEM32(g_esi+184)=g_ecx;g_edx=(int32_t)g_eax>=255?255:(int32_t)g_eax>0?g_eax:0;g_xmm0=XMM_SCALAR(MEMF(g_esp+28));g_edi=MEM32(g_esp+20);g_eax=(g_edx<<24)|0xffffff;g_ecx=(g_ecx<<24)|0xffffff;f=ct_cmp(g_xmm0.f[0],MEMF(g_esp+40));MEM32(g_esi+188)=g_edx;g_ebx=g_ecx;*frame=0xffffffe0;
 int right=!(f&0x41);if(right){g_xmm0=XMM_SCALAR(MEMF(0x51cf7c));g_xmm0.f[0]-=MEMF(g_esp+32);g_xmm0.f[0]+=MEMF(0x4875ec);g_xmm1=XMM_SCALAR(MEMF(g_esi+212));right=!(ct_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41);}
 if(right){if((int32_t)g_edx<255){g_edx+=32;MEM32(g_esi+188)=g_edx;}g_ecx=MEM32(g_esp+24);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,3);CT_CALL(00259860,0x26666d);g_esp+=20;}
 else if(g_edx){g_edx=MEM32(g_esp+24);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);PUSH32(g_esp,3);CT_CALL(00259860,0x26669d);g_eax=MEM32(g_esi+188);g_esp+=20;g_eax+=*frame;MEM32(g_esi+188)=g_eax;}
 g_xmm0=XMM_SCALAR(MEMF(g_esp+44));int left=!(ct_cmp(g_xmm0.f[0],MEMF(g_esp+76))&0x41);if(left){g_xmm0=XMM_SCALAR(MEMF(0x4859f0));g_xmm0.f[0]-=MEMF(0x4875ec);left=!(ct_cmp(g_xmm0.f[0],MEMF(g_esi+212))&0x41);}
 if(left){g_eax=MEM32(g_esi+184);if((int32_t)g_eax<255){g_eax+=32;MEM32(g_esi+184)=g_eax;}g_eax=MEM32(g_esp+48);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,2);CT_CALL(00259860,0x2666fd);g_esp+=20;}
 else{g_eax=MEM32(g_esi+184);if(g_eax){g_ecx=MEM32(g_esp+48);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,2);CT_CALL(00259860,0x26671f);g_eax=MEM32(g_esi+184);g_esp+=20;g_eax+=*frame;MEM32(g_esi+184)=g_eax;}}
}
static void controller_release_depth(void){
 if(g_eax&0x4000){g_eax&=0xffffbfff;MEM32(g_esi+180)=g_eax;g_eax=MEM32(0x515124);if(g_eax!=g_edi){g_eax--;MEM32(0x515124)=g_eax;if(!g_eax)MEM32(0x515120)=g_edi;}}
}
static void controller_finish(unsigned *frame){
 SET_LO8(g_eax,MEM8(g_esi+180));g_edi=0;if(!(LO8(g_eax)&2)){g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edi);CT_CALL(00017730,0x266748);}
 g_ecx=MEM32(g_esi+196);g_edx=MEM32(g_ecx);ct_indirect(MEM32(g_edx+12),0x266753,g_esp);int available=!LO8(g_eax);if(available){SET_LO8(g_eax,MEM8(g_esp+18));available=!LO8(g_eax);}
 if(available&&(MEM8(g_esi+180)&2)){
  int back=0;if(MEM32(g_esi+172)!=g_edi){PUSH32(g_esp,29);CT_CALL(002745F0,0x266783);g_esp+=4;back=LO8(g_eax)!=0;}
  if(!back&&(MEM8(g_esi+180)&2)){g_eax=MEM32(0x5096e0);SET_LO8(g_ecx,MEM8(g_eax+1144));if(LO8(g_ecx)){PUSH32(g_esp,28);CT_CALL(002745F0,0x2667ad);g_esp+=4;back=LO8(g_eax)!=0;}}
  if(back){SET_LO8(g_eax,MEM8(g_esi+180));if((signed char)LO8(g_eax)<0)MEM32(0x5154b8)=g_edi;g_eax=MEM32(g_esi+180);controller_release_depth();g_ecx=g_esi+220;CT_CALL(00274300,0x2667fc);g_ecx=MEM32(g_esp+112);g_edx=MEM32(g_ecx+20);MEM8(g_edx)=1;MEM8(g_esi+176)=1;g_eax=MEM32(g_esi+172);ct_return(frame,1);return;}
 }
 g_ecx=MEM32(g_esi+196);g_eax=MEM32(g_ecx);ct_indirect(MEM32(g_eax+12),0x266828,g_esp);available=!LO8(g_eax);if(available){SET_LO8(g_eax,MEM8(g_esp+18));available=!LO8(g_eax);}
 if(available&&MEM32(g_esi+172)!=g_edi){g_ecx=MEM32(0x485a18);PUSH32(g_esp,g_ecx);CT_CALL(002745F0,0x266848);g_esp+=4;if(LO8(g_eax)){g_eax=MEM32(g_esi+180);controller_release_depth();SET_LO8(g_eax,MEM8(g_esi+180));if((signed char)LO8(g_eax)<0)MEM32(0x5154b8)=g_edi;g_ecx=g_esi+220;CT_CALL(00274300,0x266897);PUSH32(g_esp,0x4509a8);CT_CALL(00057FC0,0x2668a1);g_esp+=4;MEM8(g_esi+176)=1;g_eax=MEM32(g_esi+172);ct_return(frame,1);return;}}
 g_eax=MEM32(g_esp+36);ct_return(frame,1);
}
static int controller_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_controller(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_CONTROLLER");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp,obj=g_ecx;if(s<0x10800||(s&3)||!controller_span(s-2048,2064)||!controller_span(obj,232)||!controller_span(0x51d0d4,4))return 0;
 unsigned context=MEM32(s+8),n=MEM32(obj+88),manager=MEM32(obj+196),busy=MEM32(obj+200);if(!controller_span(context,76)||!controller_span(MEM32(context+20),1)||!controller_span(MEM32(obj+4),4)||n>20||!controller_span(manager,20)||!controller_span(MEM32(manager),16)||(busy&&!controller_span(busy,1))||MEM32(0x485a1c)>11||!controller_span(MEM32(0x5096e0),1145))return 0;
 int selectable=0;for(unsigned i=0;i<n;i++){unsigned child=MEM32(obj+8+i*4);if(!controller_span(child,8)||!controller_span(MEM32(child),28))return 0;if(MEM8(child+4))selectable=1;}
 if(n&&(!selectable||(!(MEM8(obj+180)&1)&&(!MEM8(MEM32(obj+8)+4)||!MEM8(MEM32(obj+8+(n-1)*4)+4)))))return 0;
 unsigned frame=g_seh_ebp;if(!controller_begin(&frame))return 1;controller_layout_setup();if(g_eax&1)controller_vertical(&frame);else{controller_horizontal(&frame);controller_scroll_edges(&frame);}controller_finish(&frame);return 1;
}
