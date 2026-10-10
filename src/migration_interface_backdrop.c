#include "migration_interface_backdrop.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_0005AAB0(void),sub_00018130(void),sub_00017060(void),sub_00017E40(void),sub_00017160(void),sub_00018DE0(void),sub_00018CE0(void),sub_00088040(void);
#define BD_CALL(name,ret) do{PUSH32(g_esp,ret);sub_##name();}while(0)
#define BD_TOP g_fp_stack[g_fp_top]
#define BD_POP() (g_fp_top=(g_fp_top+1)&7)
static int backdrop_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_backdrop(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_BACKDROP");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;unsigned s=g_esp;if(s<0x10400||(s&3)||!backdrop_span(s-1024,1032)||!backdrop_span(0x51dcb4,4)||!backdrop_span(MEM32(0x5096e0),5))return 0;
 unsigned mode=MEM32(0x51dcb4);if(mode<=3){unsigned target=MEM32(0x254278+mode*4);if(target!=0x2540b4&&target!=0x2540bb&&target!=0x2540c2&&target!=0x2540c9)return 0;}
 unsigned frame=g_seh_ebp;g_eax=MEM32(0);PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39ef88);PUSH32(g_esp,g_eax);g_eax=MEM32(0x5150ac);MEM32(0)=g_esp;g_esp-=40;
 int visible=!g_eax;if(visible){g_eax=MEM32(0x5096e0);visible=!(MEM8(g_eax+4)&16);}
 if(visible){
  PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);BD_CALL(0005AAB0,0x25409b);g_esi=MEM32(g_eax+324);g_eax=MEM32(0x51dcb4);g_edi=0;
  if(g_eax<=3){unsigned target=MEM32(g_eax*4+0x254278);switch(target){case 0x2540b4:g_edi=0x4846f8;break;case 0x2540bb:g_edi=0x4846e0;break;case 0x2540c2:g_edi=0x4846c8;break;case 0x2540c9:g_edi=0x4846b0;break;default:g_seh_ebp=frame;RECOMP_ITAIL(target);return 1;}}
  g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_xmm0.f[0]-=MEMF(0x4859f0);g_xmm0.f[0]+=MEMF(g_esp+64);g_ecx=MEM32(0x509704);MEMF(g_esp+8)=g_xmm0.f[0];BD_CALL(00018130,0x2540f5);BD_TOP=MEMF(0x4859fc)-BD_TOP;g_ecx=MEM32(0x509704);PUSH32(g_esp,2);BD_TOP-=MEMF(0x44ec38);MEMF(g_esp+16)=(float)BD_TOP;BD_POP();BD_CALL(00017060,0x254112);
  if(MEM32(0x51dcb4)==2&&g_esi){
   g_edx=MEM32(g_esi);unsigned saved=g_esp;PUSH32(g_esp,frame);g_eax=g_esp+24;PUSH32(g_esp,g_eax);g_ecx=g_esi;unsigned target=MEM32(g_edx+208);PUSH32(g_esp,0x254137);RECOMP_ICALL_SAFE(target,saved);g_esi=MEM32(g_esp+48);frame=MEM32(g_esp+28);MEM32(g_esp+60)=0;g_eax=frame;if(g_esi<16)g_eax=g_esp+28;g_edx=g_eax+1;do{SET_LO8(g_ecx,MEM8(g_eax));g_eax++;}while(LO8(g_ecx));g_eax-=g_edx;MEM32(g_esp+20)=g_eax;
   if(g_eax){g_eax=frame;if(g_esi<16)g_eax=g_esp+28;g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);BD_CALL(00017E40,0x254188);MEMF(g_esp+20)=(float)BD_TOP;BD_POP();g_ecx=MEM32(0x51cf28);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);BD_CALL(00017160,0x2541a3);g_eax=MEM32(g_esp+28);if(MEM32(g_esp+48)<16)g_eax=g_esp+28;g_esi=MEM32(g_esp+16);g_edx=MEM32(g_esp+12);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);BD_CALL(00018DE0,0x2541cc);
    double value=MEMF(g_esp+12);g_fp_top=(g_fp_top+7)&7;BD_TOP=value;BD_TOP-=MEMF(g_esp+20);g_eax=MEM32(0x51cf5c);g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);BD_TOP-=MEMF(0x46c31c);PUSH32(g_esp,g_eax);MEMF(g_esp+20)=(float)BD_TOP;BD_POP();BD_CALL(00017160,0x2541f4);g_ecx=MEM32(g_esp+12);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edi);BD_CALL(00018CE0,0x25420e);
   }
   g_ecx=g_esp+24;MEM32(g_esp+60)=0xffffffff;BD_CALL(00088040,0x25421f);POP32(g_esp,frame);
  }else{g_edx=MEM32(0x51cf5c);g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_edx);BD_CALL(00017160,0x254239);g_eax=MEM32(g_esp+12);g_ecx=MEM32(g_esp+8);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edi);BD_CALL(00018CE0,0x254257);}
  g_ecx=MEM32(0x509704);PUSH32(g_esp,2);BD_CALL(00017060,0x254264);POP32(g_esp,g_edi);POP32(g_esp,g_esi);
 }
 g_ecx=MEM32(g_esp+40);MEM32(0)=g_ecx;g_esp+=52;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
