#include "migration_interface_videoborder.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <math.h>
extern void sub_000656D0(void),sub_00065760(void);
#define VB_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
#define VB_FP g_fp_stack[g_fp_top]
static unsigned vb_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static void vb_pop(unsigned out){MEMF(out)=(float)VB_FP;g_fp_top=(g_fp_top+1)&7;}
static void videoborder_edges(void){
 g_xmm1=XMM_SCALAR(MEMF(0x4e8394));MEMF(g_esp+28)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+40));g_xmm0.f[0]-=MEMF(0x48903c);g_eax=g_esp+12;PUSH32(g_esp,g_eax);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_edx=g_esp+36;MEMF(g_esp+28)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x489040));MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0.f[0]+=MEMF(0x44c530);PUSH32(g_esp,g_edx);MEM32(g_esp+24)=g_esi;MEMF(g_esp+36)=g_xmm1.f[0];MEMF(g_esp+44)=g_xmm0.f[0];VB_CALL(000656D0,0x2740ff);
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));MEMF(g_esp+40)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+20));g_xmm0.f[0]+=MEMF(0x44c530);g_eax=g_esp+24;PUSH32(g_esp,g_eax);MEMF(g_esp+48)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_ecx=g_esp+44;PUSH32(g_esp,g_ecx);g_edx=g_esp+40;MEMF(g_esp+40)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x489044));PUSH32(g_esp,g_edx);MEM32(g_esp+36)=g_esi;MEMF(g_esp+48)=g_xmm0.f[0];VB_CALL(000656D0,0x274150);
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_edx=MEM32(0x51cf20);PUSH32(g_esp,0);MEMF(g_esp+56)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+68));PUSH32(g_esp,0x51cf30);g_eax=g_esp+60;PUSH32(g_esp,g_eax);MEMF(g_esp+68)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_ecx=g_esp+56;PUSH32(g_esp,g_ecx);MEMF(g_esp+60)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+56));PUSH32(g_esp,g_edx);MEMF(g_esp+68)=g_xmm0.f[0];VB_CALL(00065760,0x27419c);
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_edx=MEM32(0x51cf20);PUSH32(g_esp,0);PUSH32(g_esp,0x51cf30);g_eax=g_esp+80;MEMF(g_esp+80)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+60));PUSH32(g_esp,g_eax);g_ecx=g_esp+76;g_xmm1=g_xmm0;g_xmm1.f[0]+=MEMF(0x48903c);PUSH32(g_esp,g_ecx);MEMF(g_esp+92)=g_xmm1.f[0];g_xmm1=XMM_ZERO();PUSH32(g_esp,g_edx);MEMF(g_esp+84)=g_xmm1.f[0];MEMF(g_esp+88)=g_xmm0.f[0];VB_CALL(00065760,0x2741ed);g_xmm0=XMM_ZERO();g_esp+=64;
}
static void videoborder_bars(void){
 g_xmm1=XMM_SCALAR(MEMF(0x4e8394));g_eax=g_esp+16;PUSH32(g_esp,g_eax);g_ecx=g_esp+32;MEMF(g_esp+32)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esp+44));g_xmm1.f[0]-=MEMF(0x46c320);PUSH32(g_esp,g_ecx);g_edx=g_esp+28;PUSH32(g_esp,g_edx);MEM32(g_esp+28)=g_esi;MEMF(g_esp+44)=g_xmm1.f[0];MEMF(g_esp+32)=g_xmm0.f[0];MEMF(g_esp+36)=g_xmm0.f[0];VB_CALL(000656D0,0x274242);
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));MEMF(g_esp+40)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x4e8398));MEMF(g_esp+44)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_eax=g_esp+52;PUSH32(g_esp,g_eax);g_ecx=g_esp+44;MEMF(g_esp+36)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+24));g_xmm0.f[0]-=MEMF(0x46c320);PUSH32(g_esp,g_ecx);g_edx=g_esp+40;PUSH32(g_esp,g_edx);MEM32(g_esp+64)=g_esi;MEMF(g_esp+48)=g_xmm0.f[0];VB_CALL(000656D0,0x274293);g_esp+=24;
}
int migration_interface_videoborder(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_VIDEOBORDER");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation||g_esp<0x10400||(g_esp&3)||(uint64_t)g_esp+20>g_xbox_total_ram)return 0;unsigned context=MEM32(g_esp+8);if(context<0x10000||(uint64_t)context+12>g_xbox_total_ram)return 0;
 g_esp-=28;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+36);g_eax=g_ebx&2;if(g_eax||(LO8(g_ebx)&64)){
 g_ecx=MEM32(g_esp+40);g_xmm0=XMM_ZERO();g_xmm1=g_xmm0;MEMF(g_esp+36)=g_xmm1.f[0];if(MEM8(g_ecx)&2){g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm1.f[0]-=MEMF(g_esp+44);MEMF(g_esp+36)=g_xmm1.f[0];}g_xmm2=XMM_SCALAR(MEMF(g_esp+48));if(!(vb_cmp(g_xmm2.f[0],g_xmm0.f[0])&0x41)&&(MEM8(g_ecx+8)&2)&&!(vb_cmp(g_xmm2.f[0],g_xmm1.f[0])&0x41))MEMF(g_esp+36)=g_xmm2.f[0];
 double value=MEMF(g_esp+36);g_fp_top=(g_fp_top+7)&7;VB_FP=value;VB_FP*=MEMF(0x453010);PUSH32(g_esp,g_esi);g_esi=0xff000000;VB_FP=sin(VB_FP);VB_FP*=MEMF(0x489038);g_fp_top=(g_fp_top+7)&7;VB_FP=MEMF(0x489040);VB_FP-=g_fp_stack[(g_fp_top+1)&7];vb_pop(g_esp+40);VB_FP+=MEMF(0x489044);vb_pop(g_esp+8);if(g_eax)videoborder_edges();if(LO8(g_ebx)&64)videoborder_bars();POP32(g_esp,g_esi);}
 POP32(g_esp,g_ebx);g_esp+=28;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
