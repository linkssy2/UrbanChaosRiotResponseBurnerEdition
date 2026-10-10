#include "migration_interface_menu.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002745F0(void),sub_002743D0(void);
static int menu_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void menu_call(uint32_t target,uint32_t ret,uint32_t saved){PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static int menu_above(float a,float b){double x=a,y=b;return x==x&&y==y&&x>y;}
static void menu_route(void){
 g_xmm0=XMM_SCALAR(MEMF(g_esp+4));g_xmm0.f[0]*=MEMF(0x44bf18);g_xmm1=XMM_SCALAR(MEMF(0x4875e8));PUSH32(g_esp,g_esi);g_esi=g_ecx;g_xmm0.f[0]+=MEMF(g_esi+24);int above=menu_above(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_esi+24)=g_xmm0.f[0];if(above)MEMF(g_esi+24)=g_xmm1.f[0];
 SET_LO8(g_eax,MEM8(g_esp+12));if(LO8(g_eax)){
  g_eax=MEM32(g_esi);g_ecx=g_esi;uint32_t saved=g_esp;menu_call(MEM32(g_eax+28),0x26343c,saved);if(LO8(g_eax)){g_ecx=MEM32(g_esp+16);MEM32(g_ecx+4)=0x450990;}
  PUSH32(g_esp,22);PUSH32(g_esp,0x263452);sub_002745F0();RECOMP_TRACE_ESP("sub_00263400","after call 0x002745F0");g_esp+=4;
  if(LO8(g_eax)){g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;menu_call(MEM32(g_edx+32),0x263460,saved);if(LO8(g_eax)){g_eax=MEM32(g_esi+12);RECOMP_TRACE_ESP("sub_00263400","pop esi");POP32(g_esp,g_esi);g_recomp_zf=0;RECOMP_TRACE_EXIT("sub_00263400",0x263400);g_esp+=16;return;}}
  g_xmm0=XMM_SCALAR(MEMF(g_esi+28));g_xmm0.f[0]+=MEMF(g_esp+8);above=menu_above(g_xmm0.f[0],MEMF(0x4875ec));MEMF(g_esi+28)=g_xmm0.f[0];if(above){g_xmm0=XMM_ZERO();MEMF(g_esi+28)=g_xmm0.f[0];}
 }
 g_eax=0;RECOMP_TRACE_ESP("sub_00263400","pop esi");POP32(g_esp,g_esi);g_recomp_zf=1;RECOMP_TRACE_EXIT("sub_00263400",0x263400);g_esp+=16;
}
static void menu_update(void){
 g_esp-=88;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+100);PUSH32(g_esp,g_edi);g_esi=g_ecx;PUSH32(g_esp,0x272881);sub_002743D0();RECOMP_TRACE_ESP("sub_00272870","after call 0x002743D0");g_ecx=MEM32(g_esi+4);g_esp+=4;
 if(g_ecx){g_eax=MEM32(g_esi+24);MEM32(g_esp+20)=g_eax;g_edx=MEM32(g_esi+16);g_xmm0=XMM_SCALAR(MEMF(g_esi+28));g_eax=g_esp+11;MEM32(g_esp+36)=g_eax;g_eax=0;for(unsigned i=44;i<=88;i+=4)MEM32(g_esp+i)=0;MEM8(g_esp+92)=0;MEM32(g_esp+28)=g_edx;MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+32));g_eax=g_esp+20;g_edx=g_esi+36;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);MEM8(g_esp+15)=1;MEMF(g_esp+36)=g_xmm0.f[0];MEM32(g_esp+44)=g_edx;g_edx=MEM32(g_ecx);PUSH32(g_esp,g_edi);menu_call(MEM32(g_edx+4),0x272908,saved);
  SET_LO8(g_ecx,MEM8(g_esi+20));if(!LO8(g_ecx)&&g_eax){g_ecx=g_esp+12;saved=g_esp;PUSH32(g_esp,g_ecx);menu_call(g_eax,0x27291a,saved);g_edx=MEM32(g_eax);MEM32(g_esi+12)=g_edx;g_eax=MEM32(g_eax+4);g_esp+=4;MEM32(g_esi+16)=g_eax;MEM8(g_esi+20)=1;}
  SET_LO8(g_eax,MEM8(g_esi+20));int pending=LO8(g_eax)!=0;if(pending){SET_LO8(g_ecx,MEM8(g_esp+11));if(LO8(g_ecx)){g_ecx=MEM32(g_esi+4);if(g_ecx){g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,1);menu_call(MEM32(g_edx),0x272948,saved);}
    g_eax=MEM32(g_esi+8);g_ecx=MEM32(g_esi+12);g_edx=MEM32(g_esi+16);g_xmm0=XMM_ZERO();MEM32(g_esi+24)=g_eax;MEM32(g_esi+4)=g_ecx;g_eax=0;g_ecx=1;RECOMP_TRACE_ESP("sub_00272870","pop edi");POP32(g_esp,g_edi);MEM32(g_esi+8)=g_edx;MEM32(g_esi+12)=0;MEM32(g_esi+16)=1;MEM8(g_esi+20)=0;MEMF(g_esi+32)=MEMF(g_esi+28)=g_xmm0.f[0];goto pop_esi;}}
  pending=LO8(g_eax)!=0;g_xmm1=XMM_SCALAR(MEMF(g_esi+28));g_xmm0=XMM_SCALAR(MEMF(g_esp+100));g_xmm1.f[0]+=g_xmm0.f[0];MEMF(g_esi+28)=g_xmm1.f[0];if(pending){g_xmm1=XMM_SCALAR(MEMF(g_esi+32));g_xmm1.f[0]+=g_xmm0.f[0];MEMF(g_esi+32)=g_xmm1.f[0];}
 }
 RECOMP_TRACE_ESP("sub_00272870","pop edi");POP32(g_esp,g_edi);
pop_esi:RECOMP_TRACE_ESP("sub_00272870","pop esi");POP32(g_esp,g_esi);g_esp+=88;g_recomp_zf=!g_esp;RECOMP_TRACE_EXIT("sub_00272870",0x272870);g_esp+=8;
}
int migration_interface_menu(unsigned op){static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_MENU");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;uint32_t s=g_esp,m=g_ecx;if(s<0x10400||(s&3)||!menu_span(s-1024,1044)||!menu_span(m,40))return 0;
 if(!op){if(!menu_span(MEM32(m),36)||!menu_span(MEM32(s+12),8))return 0;menu_route();}else{uint32_t child=MEM32(m+4);if(child&&(!menu_span(child,4)||!menu_span(MEM32(child),8)))return 0;menu_update();}return 1;
}
