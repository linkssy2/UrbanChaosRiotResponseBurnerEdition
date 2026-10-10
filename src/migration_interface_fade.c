#include "migration_interface_fade.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273C20(void),sub_00087030(void),sub_000658C0(void);
static int fade_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned fade_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
int migration_interface_fade(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_FADE");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!fade_span(s-1024,1048)||!fade_span(MEM32(s+8),20))return 0;
 if(!op&&(!fade_span(g_ecx,12)||!fade_span(MEM32(MEM32(s+8)+16),1)))return 0;
 if(!op){
  PUSH32(g_esp,g_ecx);g_edx=MEM32(g_esp+12);g_xmm0=XMM_SCALAR(MEMF(g_edx+4));g_xmm3=XMM_SCALAR(MEMF(0x46c320));g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm2=XMM_ZERO();g_xmm0.f[0]*=g_xmm3.f[0];
  if(fade_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41){g_xmm0=g_xmm1;MEMF(g_esp)=g_xmm0.f[0];}else MEMF(g_esp)=(fade_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41)?g_xmm2.f[0]:g_xmm0.f[0];
  g_xmm0=XMM_SCALAR(MEMF(g_edx+12));int clear=0;
  if(fade_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41){MEMF(g_esp+12)=g_xmm2.f[0];clear=1;}
  else{g_xmm0.f[0]*=g_xmm3.f[0];if(fade_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41)g_xmm0=g_xmm1;else if(fade_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41)g_xmm0=g_xmm2;SET_HI8(g_eax,fade_cmp(g_xmm0.f[0],g_xmm1.f[0]));MEMF(g_esp+12)=g_xmm0.f[0];clear=RECOMP_PARITY8(HI8(g_eax)&0x44);if(!clear&&(MEM8(g_edx+8)&1)){g_eax=MEM32(g_ecx+8)+1;MEM32(g_ecx+8)=g_eax;clear=g_eax<=3;}}
  if(clear){g_eax=MEM32(g_edx+16);MEM8(g_eax)=0;}
  g_eax=MEM32(g_esp+12);g_ecx=MEM32(g_ecx+4);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x273c0c);sub_00273C20();g_eax=0;g_esp+=20;g_recomp_zf=!g_esp;g_esp+=12;
 }else{
  g_eax=MEM32(g_esp+8);SET_LO8(g_edx,MEM8(g_eax));g_xmm2=XMM_ZERO();SET_LO8(g_ecx,1);g_xmm0=g_xmm2;if((LO8(g_edx)&LO8(g_ecx))||(MEM8(g_esp+4)&LO8(g_ecx))){g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm0.f[0]-=MEMF(g_esp+12);}g_xmm1=g_xmm2;if(MEM8(g_eax+8)&LO8(g_ecx))g_xmm1=XMM_SCALAR(MEMF(g_esp+16));if(!(fade_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41))g_xmm0=g_xmm1;unsigned cmp=fade_cmp(g_xmm0.f[0],g_xmm2.f[0]);MEMF(g_esp+8)=g_xmm0.f[0];
  if(!(cmp&0x41)){g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_esp+24;PUSH32(g_esp,0x273c7b);sub_00087030();PUSH32(g_esp,g_eax);PUSH32(g_esp,0x273c81);sub_000658C0();POP32(g_esp,g_ecx);}
  g_recomp_zf=!!(cmp&0x40);g_esp+=4;
 }return 1;
}
