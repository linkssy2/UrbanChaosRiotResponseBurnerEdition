#include "migration_interface_tween.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273410(void),sub_002D57FB(void);
static int tween_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void tween_construct(void){
 g_esp-=104;g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm1=XMM_ZERO();g_xmm2=XMM_SCALAR(MEMF(0x44bf80));g_edx=MEM32(g_esp+108);g_xmm4=XMM_SCALAR(MEMF(g_esp+112));g_xmm4.f[0]*=MEMF(0x44c72c);
 const unsigned offsets[]={8,12,40,48,56,64};for(unsigned i=0;i<6;i++)MEMF(g_esp+offsets[i])=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(0x46c31c));MEM32(g_ecx)=0x452f14;MEM8(g_ecx+28)=0;g_eax=MEM32(g_edx);MEM32(g_ecx+4)=g_eax;g_eax=MEM32(g_esp+116);MEMF(g_esp+88)=MEMF(g_esp+92)=g_xmm2.f[0];MEMF(g_esp+16)=MEMF(g_esp+20)=g_xmm0.f[0];for(unsigned i=24;i<=36;i+=4)MEMF(g_esp+i)=g_xmm1.f[0];g_xmm2=XMM_SCALAR(MEMF(g_esp+g_eax*4+8));g_xmm2.f[0]*=MEMF(0x4e8394);MEMF(g_ecx+12)=g_xmm2.f[0];for(unsigned i=44;i<=68;i+=8)MEMF(g_esp+i)=g_xmm0.f[0];g_xmm2=XMM_SCALAR(MEMF(g_esp+g_eax*4+40));g_xmm2.f[0]*=MEMF(0x4e8394);MEMF(g_ecx+16)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_esp+120));g_xmm5=g_xmm0;g_xmm5.f[0]-=g_xmm4.f[0];g_xmm4=XMM_SCALAR(MEMF(g_ecx+12));MEMF(g_ecx+8)=g_xmm5.f[0];g_xmm5=XMM_SCALAR(MEMF(g_ecx+4));for(unsigned i=72;i<=84;i+=4)MEMF(g_esp+i)=g_xmm1.f[0];MEMF(g_esp+96)=MEMF(g_esp+100)=g_xmm0.f[0];g_xmm3=XMM_SCALAR(MEMF(g_esp+g_eax*4+72));g_xmm2.f[0]-=g_xmm0.f[0];MEMF(g_ecx+20)=g_xmm2.f[0];g_xmm5.f[0]+=g_xmm4.f[0];MEMF(g_ecx+24)=g_xmm3.f[0];MEMF(g_ecx+40)=g_xmm1.f[0];MEMF(g_ecx+48)=g_xmm4.f[0];g_xmm4=XMM_SCALAR(MEMF(g_edx+4));MEMF(g_esp+4)=g_xmm4.f[0];g_eax=MEM32(g_esp+4);double a=g_xmm3.f[0],b=g_xmm1.f[0];unsigned flags=a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;MEM32(g_ecx+36)=g_eax;SET_HI8(g_eax,flags);MEMF(g_esp)=g_xmm5.f[0];g_edx=MEM32(g_esp);g_xmm2.f[0]+=g_xmm0.f[0];MEM32(g_ecx+32)=g_edx;MEMF(g_ecx+44)=g_xmm1.f[0];MEMF(g_ecx+52)=g_xmm2.f[0];if(RECOMP_PARITY8(HI8(g_eax)&0x44))g_xmm0=g_xmm1;MEMF(g_ecx+56)=g_xmm0.f[0];g_eax=g_ecx;g_esp+=104;g_recomp_zf=!g_esp;g_esp+=20;
}
int migration_interface_tween(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_TWEEN");enabled=s&&!strcmp(s,"1");}if(!enabled||op>3)return 0;unsigned s=g_esp;
 if(s<0x10400||(s&3)||!tween_span(s-1024,1044)||!tween_span(0x4e8394,4))return 0;
 if(op<2){if(!tween_span(g_ecx,64)||!tween_span(MEM32(s+4),8)||MEM32(s+12)>7)return 0;}else if(!tween_span(MEM32(s+4),8))return 0;
 if(!op){tween_construct();return 1;}
 if(op==1){g_eax=MEM32(g_esp+12);g_edx=MEM32(g_esp+8);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+12);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x273649);sub_00273410();g_edx=MEM32(g_esp+16);MEM32(g_ecx)=0x489064;MEM32(g_ecx+60)=g_edx;g_eax=g_ecx;g_esp+=20;return 1;}
 PUSH32(g_esp,op==2?12:16);PUSH32(g_esp,op==2?0x273707:0x2733d7);sub_002D57FB();g_edx=MEM32(g_esp+12);g_ecx=0;g_esp+=4;int failed=!g_eax;
 if(g_eax){if(op==2){MEM32(g_eax+8)=0;MEM32(g_eax)=0x48906c;MEM32(g_eax+4)=g_edx;g_ecx=g_eax;}else{PUSH32(g_esp,g_ebx);SET_LO8(g_ebx,MEM8(g_esp+20));PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+20);MEM32(g_eax+8)=g_esi;POP32(g_esp,g_esi);MEM8(g_eax+12)=LO8(g_ebx);MEM8(g_eax+13)=LO8(g_ecx);MEM32(g_eax)=0x489050;MEM32(g_eax+4)=g_edx;g_ecx=g_eax;POP32(g_esp,g_ebx);}}
 g_eax=MEM32(g_esp+4);MEM32(g_eax)=g_ecx;MEM32(g_eax+4)=g_edx;g_recomp_zf=failed;g_esp+=4;return 1;
}
