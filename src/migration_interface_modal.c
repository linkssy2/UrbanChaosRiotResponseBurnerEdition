#include "migration_interface_modal.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D57FB(void),sub_002D5CCD(void),sub_0025A290(void);
static int modal_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void modal_update(void){
 unsigned frame=g_seh_ebp;g_esp-=80;PUSH32(g_esp,frame);frame=g_ecx;g_eax=MEM32(frame+4);
 if(g_eax){
  g_xmm0=XMM_SCALAR(MEMF(frame+8));unsigned saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+100);PUSH32(g_esp,g_edi);MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(frame+12));g_ecx=g_esp+18;MEM32(g_esp+36)=g_ecx;g_ebx=1;g_edx=g_esp+19;MEM32(g_esp+20)=g_ebx;MEM32(g_esp+28)=g_ebx;MEMF(g_esp+32)=g_xmm0.f[0];MEM32(g_esp+40)=g_edx;g_ecx=13;g_edi=g_esp+44;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),52);g_esi+=52;g_edi+=52;g_ecx=0;g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+104);MEM8(g_esp+22)=LO8(g_ebx);MEM8(g_esp+23)=0;g_edx=MEM32(g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_eax;unsigned target=MEM32(g_edx+4);PUSH32(g_esp,0x25a1e0);RECOMP_ICALL_SAFE(target,saved);
  g_esi=g_eax;if(g_esi)MEM8(frame+16)=LO8(g_ebx);SET_LO8(g_eax,MEM8(frame+16));int remove=0;
  if(LO8(g_eax)){SET_LO8(g_ecx,MEM8(g_esp+18));remove=LO8(g_ecx)!=0;}
  if(remove){g_ecx=MEM32(frame+4);if(g_ecx){g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,g_ebx);target=MEM32(g_edx);PUSH32(g_esp,0x25a204);RECOMP_ICALL_SAFE(target,saved);}}
  else{int closing=LO8(g_eax)!=0;g_xmm0=XMM_SCALAR(MEMF(g_esp+100));g_xmm1=g_xmm0;g_xmm1.f[0]+=MEMF(frame+8);MEMF(frame+8)=g_xmm1.f[0];if(closing){g_xmm1=XMM_SCALAR(MEMF(frame+12));g_xmm1.f[0]+=g_xmm0.f[0];MEMF(frame+12)=g_xmm1.f[0];}}
  POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);if(remove)MEM32(frame+4)=0;
 }else g_eax=0;
 POP32(g_esp,frame);g_esp+=80;g_recomp_zf=!g_esp;g_esp+=12;
}
int migration_interface_modal(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_MODAL");enabled=s&&!strcmp(s,"1");}if(!enabled||op>5)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!modal_span(s-1024,1044))return 0;
 if(op&& !modal_span(g_ecx,20))return 0;
 if(op==2&&!modal_span(MEM32(s+8),52))return 0;
 if(op==2||op==4||op==5){unsigned child=MEM32(g_ecx+4);if(child&&(!modal_span(child,4)||!modal_span(MEM32(child),8)))return 0;}
 switch(op){
 case 0:PUSH32(g_esp,20);PUSH32(g_esp,0x25a2e7);sub_002D57FB();g_ecx=0;g_esp+=4;if(g_eax){g_xmm0=XMM_ZERO();MEM32(g_eax)=0x485c84;MEM32(g_eax+4)=0;MEMF(g_eax+8)=MEMF(g_eax+12)=g_xmm0.f[0];MEM8(g_eax+16)=0;}g_recomp_zf=!g_eax;g_esp+=4;break;
 case 1:g_eax=MEM32(g_esp+4);g_xmm0=XMM_ZERO();MEM32(g_ecx+4)=g_eax;MEMF(g_ecx+8)=MEMF(g_ecx+12)=g_xmm0.f[0];MEM8(g_ecx+16)=0;g_esp+=12;break;
 case 2:modal_update();break;
 case 3:g_edx=MEM32(g_ecx+4);g_eax=g_edx!=0;g_recomp_zf=!g_edx;g_esp+=4;break;
 case 4:PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x25a278);sub_0025A290();if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x25a285);sub_002D5CCD();g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;break;
 case 5:{PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39f278);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x485c84;g_ecx=MEM32(g_esi+4);MEM32(g_esp+16)=0;if(g_ecx){g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,1);unsigned target=MEM32(g_eax);PUSH32(g_esp,0x25a2c8);RECOMP_ICALL_SAFE(target,saved);}g_ecx=MEM32(g_esp+8);MEM32(g_esi)=0x485a44;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;break;}
 }return 1;
}
