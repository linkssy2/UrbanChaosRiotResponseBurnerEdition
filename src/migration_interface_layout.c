#include "migration_interface_layout.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273300(void);
static int layout_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned layout_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
#define LY_TOP g_fp_stack[g_fp_top]
#define LY_POP() (g_fp_top=(g_fp_top+1)&7)
int migration_interface_layout(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_LAYOUT");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;unsigned s=g_esp;if(s<0x10400||(s&3)||!layout_span(s-1024,1052)||!layout_span(0x5011a4,16))return 0;
 if(op==1){if(!layout_span(MEM32(s+4),16)||(MEM32(s+8)&&!layout_span(MEM32(s+8),4)))return 0;g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+8);g_ecx=MEM32(g_eax+12);g_edx=MEM32(g_eax+4);PUSH32(g_esp,0x3e4ccccd);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x3e4ccccd);PUSH32(g_esp,g_edx);PUSH32(g_esp,2);PUSH32(g_esp,0x2584e2);sub_00273300();g_esp+=24;g_recomp_zf=!g_esp;g_esp+=4;return 1;}
 if(MEM32(s+4)>3||(MEM32(s+24)&&!layout_span(MEM32(s+24),4)))return 0;
 PUSH32(g_esp,g_ecx);g_eax=MEM32(g_esp+8);g_xmm0=XMM_SCALAR(MEMF(g_esp+12));g_xmm1=XMM_SCALAR(MEMF(g_esp+16));g_ecx=MEM32(g_eax*4+0x5011a4);g_xmm2=XMM_SCALAR(MEMF(g_esp+20));unsigned f=layout_cmp(g_xmm2.f[0],MEMF(0x44c174));g_xmm0.f[0]/=g_xmm1.f[0];MEM32(g_esp)=g_ecx;MEMF(g_esp+8)=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm1.f[0];
 if(!(f&0x41)){g_xmm3=XMM_SCALAR(MEMF(g_esp+24));g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm2.f[0]/=g_xmm3.f[0];g_xmm1.f[0]-=g_xmm2.f[0];if(!(layout_cmp(g_xmm0.f[0],g_xmm1.f[0])&0x41)){MEMF(g_esp+8)=g_xmm1.f[0];MEMF(g_esp+12)=g_xmm3.f[0];}}
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+32);
 if(g_esi){g_xmm0=XMM_SCALAR(MEMF(g_esp+12));g_xmm0.f[0]-=MEMF(0x44d024);unsigned saved=g_esp;PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];unsigned target=MEM32(g_esp+8);PUSH32(g_esp,0x273382);RECOMP_ICALL_SAFE(target,saved);MEMF(g_esp+24)=(float)LY_TOP;LY_POP();g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_xmm0.f[0]+=MEMF(0x44d024);MEMF(g_esp)=g_xmm0.f[0];saved=g_esp;target=MEM32(g_esp+8);PUSH32(g_esp,0x27339d);RECOMP_ICALL_SAFE(target,saved);LY_TOP-=MEMF(g_esp+24);g_esp+=4;LY_TOP/=MEMF(g_esp+16);LY_TOP*=MEMF(0x44c5e8);MEMF(g_esi)=(float)LY_TOP;LY_POP();}
 g_edx=MEM32(g_esp+12);unsigned saved=g_esp;PUSH32(g_esp,g_edx);unsigned target=MEM32(g_esp+8);PUSH32(g_esp,0x2733b9);RECOMP_ICALL_SAFE(target,saved);LY_TOP=MEMF(0x44bf94)-LY_TOP;g_esp+=4;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
