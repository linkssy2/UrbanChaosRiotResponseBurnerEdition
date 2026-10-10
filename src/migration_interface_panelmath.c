#include "migration_interface_panelmath.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
static int pm_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_panelmath(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_PANELMATH");enabled=s&&!strcmp(s,"1");}if(!enabled||op>2)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!pm_span(s-1024,1056))return 0;
 if(!op){if(!pm_span(MEM32(s+4),8)||!pm_span(MEM32(s+8),8))return 0;
  g_esp-=8;g_ecx=MEM32(g_esp+12);g_eax=MEM32(g_esp+16);g_xmm0=XMM_SCALAR(MEMF(g_ecx));g_xmm1=XMM_SCALAR(MEMF(g_ecx+4));g_xmm0.f[0]+=MEMF(g_eax);g_xmm1.f[0]+=MEMF(g_eax+4);g_xmm2=XMM_SCALAR(MEMF(0x44c1f8));g_xmm3=XMM_SCALAR(MEMF(g_eax+4));g_xmm4=XMM_SCALAR(MEMF(g_esp+20));g_xmm1.f[0]*=g_xmm2.f[0];g_xmm0.f[0]*=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_eax));g_xmm2.f[0]-=g_xmm0.f[0];g_xmm2.f[0]*=g_xmm4.f[0];g_xmm3.f[0]-=g_xmm1.f[0];g_xmm3.f[0]*=g_xmm4.f[0];g_xmm4=g_xmm0;g_xmm4.f[0]-=g_xmm2.f[0];MEMF(g_esp)=g_xmm4.f[0];g_edx=MEM32(g_esp);MEM32(g_ecx)=g_edx;g_xmm4=g_xmm1;g_xmm4.f[0]-=g_xmm3.f[0];MEMF(g_esp+4)=g_xmm4.f[0];g_edx=MEM32(g_esp+4);MEM32(g_ecx+4)=g_edx;g_xmm2.f[0]+=g_xmm0.f[0];g_xmm3.f[0]+=g_xmm1.f[0];MEMF(g_esp)=g_xmm2.f[0];g_ecx=MEM32(g_esp);MEMF(g_esp+4)=g_xmm3.f[0];g_edx=MEM32(g_esp+4);MEM32(g_eax)=g_ecx;MEM32(g_eax+4)=g_edx;g_esp+=8;g_recomp_zf=!g_esp;g_esp+=4;return 1;
 }
 if(!pm_span(g_ecx,232))return 0;unsigned index=MEM32(g_ecx+228)+1,target=0;
 if(op==1&&index<=4){target=MEM32(0x25d900+index*4);if(target!=0x25d8c5&&target!=0x25d8ce&&target!=0x25d8d7&&target!=0x25d8e0&&target!=0x25d8fc)return 0;}
 int call=op==1?target&&target!=0x25d8fc:MEM32(g_ecx+228)==0xffffffff;
 if(call){unsigned engine=MEM32(0x51c4dc);if(!pm_span(engine,112)||!pm_span(MEM32(engine+108),4)||!pm_span(MEM32(MEM32(engine+108)),48))return 0;}
 int zf;
 if(op==1){g_eax=g_ecx;g_ecx=MEM32(g_eax+228)+1;zf=g_ecx==4;if(call){PUSH32(g_esp,0);PUSH32(g_esp,target==0x25d8c5?0x4873d8:target==0x25d8ce?0x4873d0:target==0x25d8d7?0x4873c8:0x4873bc);g_ecx=MEM32(0x51c4dc);g_ecx=MEM32(g_ecx+108);g_eax=MEM32(g_eax+224);g_edx=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,g_eax);target=MEM32(g_edx+44);PUSH32(g_esp,0x25d8fc);RECOMP_ICALL_SAFE(target,saved);}}
 else{zf=MEM32(g_ecx+228)==0xffffffff;if(call){g_eax=MEM32(0x51c4dc);g_ecx=MEM32(g_ecx+224);g_eax=MEM32(g_eax+108);g_edx=MEM32(g_eax);unsigned saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4873e0);PUSH32(g_esp,g_ecx);g_ecx=g_eax;target=MEM32(g_edx+44);PUSH32(g_esp,0x25d946);RECOMP_ICALL_SAFE(target,saved);}}
 g_recomp_zf=zf;g_esp+=4;return 1;
}
