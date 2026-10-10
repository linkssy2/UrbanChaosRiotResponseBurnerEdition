#include "migration_interface_popupupdate.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_000871A0(void),sub_000656D0(void),sub_002745F0(void),sub_00057FC0(void),sub_00017060(void),sub_00017180(void),sub_00017EE0(void),sub_000176D0(void),sub_00086FD0(void),sub_00017160(void),sub_00018CE0(void),sub_00018DE0(void);
static int pu_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned pu_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
#define PU_TOP g_fp_stack[g_fp_top]
#define PU_POP() (g_fp_top=(g_fp_top+1)&7)
#define PU_CALL(fn,ret) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
static void popup_background(unsigned frame){
 g_xmm3=XMM_SCALAR(MEMF(0x46c320));g_xmm2=XMM_SCALAR(MEMF(g_eax+4));g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm2.f[0]*=g_xmm3.f[0];g_xmm1=g_xmm0;g_xmm1.f[0]-=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_eax+12));g_xmm2.f[0]*=g_xmm3.f[0];if(!(pu_cmp(g_xmm2.f[0],g_xmm1.f[0])&0x41))g_xmm1=g_xmm2;unsigned cmp=pu_cmp(g_xmm0.f[0],g_xmm1.f[0]);g_xmm3=XMM_ZERO();if(cmp&0x41)g_xmm1=g_xmm0;else if(pu_cmp(g_xmm1.f[0],g_xmm3.f[0])&0x41)g_xmm1=g_xmm3;cmp=pu_cmp(g_xmm0.f[0],g_xmm2.f[0]);g_xmm4=g_xmm0;g_xmm4.f[0]-=g_xmm1.f[0];MEMF(g_esp+48)=MEMF(g_esp+52)=MEMF(g_esp+56)=g_xmm0.f[0];MEMF(g_esp+60)=g_xmm4.f[0];if(!(cmp&0x41)){g_eax=MEM32(g_eax+16);MEM8(g_eax)=0;}
 g_xmm0=XMM_SCALAR(MEMF(0x47df9c));MEMF(g_esp+76)=g_xmm0.f[0];g_ecx=g_esp+96;MEMF(g_esp+64)=MEMF(g_esp+68)=MEMF(g_esp+72)=g_xmm3.f[0];g_xmm0=XMM_MEM(g_esp+64);g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(g_esp+48));PUSH32(g_esp,g_ecx);g_ecx=g_esp+32;XMM_STORE(g_esp+100,g_xmm0);PU_CALL(sub_000871A0,0x25a3ed);
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));MEMF(g_esp+64)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x51cf64));g_xmm0.f[0]+=MEMF(0x450090);MEMF(g_esp+68)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_edx=g_esp+28;PUSH32(g_esp,g_edx);g_eax=g_esp+68;MEMF(g_esp+44)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x489040));g_xmm0.f[0]-=MEMF(0x44bf94);PUSH32(g_esp,g_eax);g_ecx=g_esp+48;PUSH32(g_esp,g_ecx);MEMF(g_esp+56)=g_xmm0.f[0];PU_CALL(sub_000656D0,0x25a444);g_eax=MEM32(g_ebx+8);g_esp+=12;
}
static void popup_header(unsigned frame){
 g_ecx=MEM32(0x509704);g_eax=0;SET_LO8(g_eax,MEM8(g_ebx+92));PUSH32(g_esp,g_eax);PU_CALL(sub_00017060,0x25a4ae);g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_xmm0.f[0]-=MEMF(0x458800);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,0x42fa0000);PU_CALL(sub_00017180,0x25a4d4);g_ecx=MEM32(g_ebx+4);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PU_CALL(sub_00017EE0,0x25a4e7);MEM32(g_esp+20)=g_eax;
 if((int32_t)g_eax>=4){g_ecx=MEM32(0x509704);PUSH32(g_esp,0x44138000);PUSH32(g_esp,0x42480000);PU_CALL(sub_00017180,0x25a505);g_edx=MEM32(g_ebx+4);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PU_CALL(sub_00017EE0,0x25a518);MEM32(g_esp+20)=g_eax;}
 g_fp_top=(g_fp_top+7)&7;PU_TOP=(double)SMEM32(g_esp+20);g_eax=MEM32(g_ebx+92);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_eax);MEMF(g_esp+24)=(float)PU_TOP;PU_POP();PU_CALL(sub_000176D0,0x25a533);PU_TOP*=MEMF(g_esp+20);g_ecx=MEM32(g_ebx+92);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PU_TOP*=MEMF(0x44c1f8);PU_TOP=MEMF(0x4e83a0)-PU_TOP;PU_TOP-=MEMF(0x44c13c);MEMF(g_esp+32)=(float)PU_TOP;PU_POP();PU_CALL(sub_000176D0,0x25a55c);PU_TOP*=MEMF(g_esp+20);g_edx=g_esp+112;PUSH32(g_esp,g_edx);g_ecx=0x51cf94;PU_TOP*=MEMF(0x44c1f8);PU_TOP+=MEMF(0x4e83a0);PU_TOP-=MEMF(0x4584e4);MEMF(g_esp+68)=(float)PU_TOP;PU_POP();PU_CALL(sub_00086FD0,0x25a585);
 g_xmm0=XMM_MEM(g_eax);g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(g_esp+48));g_eax=g_esp+96;PUSH32(g_esp,g_eax);g_ecx=g_esp+44;XMM_STORE(g_esp+100,g_xmm0);PU_CALL(sub_000871A0,0x25a5a0);g_ecx=MEM32(g_eax);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PU_CALL(sub_00017160,0x25a5b3);g_edx=MEM32(g_esp+28);g_eax=MEM32(0x4e839c);g_ecx=MEM32(g_ebx+4);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,18);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PU_CALL(sub_00018CE0,0x25a5d5);
}
static void popup_row_colors(unsigned frame){
 g_eax=g_esp+192;PUSH32(g_esp,g_eax);g_ecx=0x51cf5c;PU_CALL(sub_00086FD0,0x25a623);g_xmm0=XMM_MEM(g_eax);g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(g_esp+48));g_ecx=g_esp+96;PUSH32(g_esp,g_ecx);g_ecx=g_esp+32;XMM_STORE(g_esp+100,g_xmm0);PU_CALL(sub_000871A0,0x25a63e);g_edx=g_esp+160;PUSH32(g_esp,g_edx);g_ecx=0x51cf38;PU_CALL(sub_00086FD0,0x25a650);g_xmm0=XMM_MEM(g_eax);g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(g_esp+48));g_eax=g_esp+144;PUSH32(g_esp,g_eax);g_ecx=g_esp+40;XMM_STORE(g_esp+148,g_xmm0);PU_CALL(sub_000871A0,0x25a671);
 g_ecx=MEM32(g_esp+20);g_esi=MEM32(g_ecx-88);g_edi=0x485c94;g_ecx=18;g_edx=0;int equal=1;while(g_ecx){equal=MEM8(g_esi)==MEM8(g_edi);g_esi++;g_edi++;g_ecx--;if(!equal)break;}
 if(equal){g_eax=g_esp+176;PUSH32(g_esp,g_eax);g_ecx=0x51cf54;PU_CALL(sub_00086FD0,0x25a69a);g_xmm0=XMM_MEM(g_eax);g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(g_esp+48));g_ecx=g_esp+128;PUSH32(g_esp,g_ecx);g_ecx=g_esp+96;XMM_STORE(g_esp+132,g_xmm0);PU_CALL(sub_000871A0,0x25a6bb);g_edx=MEM32(g_esp+92);g_eax=g_esp+208;PUSH32(g_esp,g_eax);g_ecx=0x51cf74;MEM32(g_esp+32)=g_edx;PU_CALL(sub_00086FD0,0x25a6d5);g_xmm0=XMM_MEM(g_eax);g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(g_esp+48));g_ecx=g_esp+112;PUSH32(g_esp,g_ecx);g_ecx=g_esp+44;XMM_STORE(g_esp+116,g_xmm0);PU_CALL(sub_000871A0,0x25a6f0);g_edx=MEM32(g_esp+40);MEM32(g_esp+36)=g_edx;}
}
static void popup_rows(unsigned frame){
 g_xmm1=XMM_SCALAR(MEMF(g_ebx+136));g_eax=MEM32(g_ebx+12);g_xmm1.f[0]*=MEMF(0x44c1f8);g_xmm0=XMM_SCALAR(MEMF(0x4e839c));g_xmm0.f[0]-=g_xmm1.f[0];MEMF(g_esp+24)=g_xmm0.f[0];MEM32(g_esp+32)=0;
 if(!g_eax)return;g_edx=g_ebx+104;MEM32(g_esp+20)=g_edx;
 do{
  popup_row_colors(frame);g_eax=MEM32(g_esp+32);int selected=g_eax==MEM32(g_ebx+8);g_eax=g_esp+(selected?28:36);g_ecx=MEM32(g_eax);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PU_CALL(sub_00017160,0x25a71c);g_edi=MEM32(g_esp+64);g_esi=MEM32(g_esp+20);g_xmm0=XMM_SCALAR(MEMF(g_esi));g_xmm0.f[0]*=MEMF(0x44c1f8);g_xmm0.f[0]+=MEMF(g_esp+24);g_edx=MEM32(g_esi-88);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,2);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_edx);PU_CALL(sub_00018CE0,0x25a754);g_eax=MEM32(g_ebx+12);g_ecx=MEM32(g_esp+32);g_eax--;
  if(g_ecx!=g_eax){SET_LO8(g_eax,MEM8(g_ebx+84));g_xmm0=XMM_SCALAR(MEMF(g_esi));g_xmm0.f[0]+=MEMF(g_esp+24);MEMF(g_esp+24)=g_xmm0.f[0];
   if(LO8(g_eax)){g_ecx=MEM32(g_esp+36);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PU_CALL(sub_00017160,0x25a790);g_xmm0=XMM_SCALAR(MEMF(0x44c1f8));g_xmm1=XMM_SCALAR(MEMF(g_ebx+96));g_xmm2=XMM_SCALAR(MEMF(g_ebx+88));PUSH32(g_esp,0);PUSH32(g_esp,2);PUSH32(g_esp,g_edi);g_xmm1.f[0]*=g_xmm0.f[0];PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);g_xmm2.f[0]*=g_xmm0.f[0];g_xmm1.f[0]+=g_xmm2.f[0];g_xmm1.f[0]+=MEMF(g_esp+40);g_edx=g_ebx+100;MEMF(g_esp)=g_xmm1.f[0];PUSH32(g_esp,g_edx);PU_CALL(sub_00018DE0,0x25a7ce);g_xmm0=XMM_SCALAR(MEMF(g_ebx+96));g_xmm0.f[0]+=MEMF(g_esp+24);g_xmm0.f[0]+=MEMF(g_ebx+88);}else g_xmm0.f[0]+=MEMF(0x485a40);MEMF(g_esp+24)=g_xmm0.f[0];
  }
  g_eax=MEM32(g_esp+32);g_ecx=MEM32(g_ebx+12);g_eax++;g_esi+=4;MEM32(g_esp+32)=g_eax;MEM32(g_esp+20)=g_esi;
 }while(g_eax<g_ecx);
}
int migration_interface_popupupdate(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_POPUPUPDATE");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10800||(s&3)||!pu_span(s-2048,2080)||!pu_span(g_ecx,140)||!pu_span(MEM32(s+8),64))return 0;
 unsigned ctx=MEM32(s+8),n=MEM32(g_ecx+12);if(n>8||MEM32(g_ecx+8)>=8||!pu_span(MEM32(ctx+16),1))return 0;for(unsigned i=0;i<n;i++)if(!pu_span(MEM32(g_ecx+16+i*4),18))return 0;
 unsigned frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0;g_esp-=212;g_eax=MEM32(frame+12);PUSH32(g_esp,g_ebx);MEM32(g_eax+28)=0x450990;g_ebx=g_ecx;MEM32(g_eax+60)=0x450978;g_ecx=MEM32(g_ebx+80);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);if(g_ecx)MEM32(g_eax+24)=0x4855c8;popup_background(frame);
 if(g_eax){PUSH32(g_esp,32);PU_CALL(sub_002745F0,0x25a455);g_esp+=4;if(LO8(g_eax)){PUSH32(g_esp,0x450960);PU_CALL(sub_00057FC0,0x25a466);g_eax=MEM32(g_ebx+8);g_esp+=4;g_eax--;MEM32(g_ebx+8)=g_eax;}}
 g_edx=MEM32(g_ebx+12);g_eax=MEM32(g_ebx+8);g_edx--;if(g_eax<g_edx){PUSH32(g_esp,33);PU_CALL(sub_002745F0,0x25a482);g_esp+=4;if(LO8(g_eax)){PUSH32(g_esp,0x450960);PU_CALL(sub_00057FC0,0x25a493);g_eax=MEM32(g_ebx+8);g_esp+=4;g_eax++;MEM32(g_ebx+8)=g_eax;}}
 popup_header(frame);popup_rows(frame);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x44138000);PUSH32(g_esp,0x42480000);PU_CALL(sub_00017180,0x25a81e);PUSH32(g_esp,22);PU_CALL(sub_002745F0,0x25a825);g_esp+=4;int result=0;
 if(LO8(g_eax)){PUSH32(g_esp,0x450958);PU_CALL(sub_00057FC0,0x25a836);g_eax=MEM32(g_ebx+8);g_eax=MEM32(g_ebx+g_eax*4+48);g_esp+=4;result=1;}
 else{g_eax=MEM32(g_ebx+80);if(g_eax){PUSH32(g_esp,23);PU_CALL(sub_002745F0,0x25a857);g_esp+=4;if(LO8(g_eax)){PUSH32(g_esp,0x4509a8);PU_CALL(sub_00057FC0,0x25a868);g_eax=MEM32(g_ebx+80);g_esp+=4;result=1;}}}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);if(!result)g_eax=0;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=result?!g_esp:1;g_esp+=12;return 1;
}
#undef PU_TOP
#undef PU_POP
#undef PU_CALL
