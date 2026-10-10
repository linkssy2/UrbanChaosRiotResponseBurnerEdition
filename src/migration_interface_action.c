#include "migration_interface_action.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00274740(void),sub_002745F0(void),sub_00064E30(void),sub_00058000(void),sub_00057FC0(void);
static int action_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_action(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_ACTION");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!action_span(s-1024,1040)||!action_span(g_ecx,32)||!action_span(MEM32(s+8),40))return 0;
 unsigned index=MEM32(g_ecx+4)-22,target=0;
 if(MEM32(g_ecx+12)&&index<4){target=MEM32(0x27678c+index*4);if(target!=0x2766ad&&target!=0x2766b4&&target!=0x2766b8&&target!=0x2766bf)return 0;}
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=MEM32(g_edi+12);
 if(g_ecx){g_eax=MEM32(g_edi+4)-22;if(g_eax<=3){g_eax=target==0x2766ad?1:target==0x2766b4?0:target==0x2766b8?3:2;g_edx=MEM32(g_esp+16);MEM32(g_edx+g_eax*4+24)=g_ecx;}}
 g_xmm0=XMM_SCALAR(MEMF(g_edi+28));SET_LO8(g_ebx,0);
 double a=g_xmm0.f[0],b=MEMF(0x44c174);unsigned comparison=(a!=a||b!=b)?0x47:a<b?3:a>b?2:0x42;
 if(!(comparison&0x41)){g_xmm0.f[0]-=MEMF(g_esp+12);MEMF(g_edi+28)=g_xmm0.f[0];}
 else{
  SET_LO8(g_eax,MEM8(g_edi+20));
  if(LO8(g_eax)){PUSH32(g_esp,g_esi);g_esi=0;do{g_eax=MEM32(g_edi+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x276702);sub_00274740();g_esp+=8;if(LO8(g_eax)){MEM32(0x51d2f4)=g_esi;SET_LO8(g_ebx,1);}g_esi++;}while((int32_t)g_esi<4);POP32(g_esp,g_esi);}
  else{g_ecx=MEM32(g_edi+4);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x276723);sub_002745F0();g_esp+=4;if(LO8(g_eax))SET_LO8(g_ebx,1);}
  g_ecx=0x4e8364;PUSH32(g_esp,0x276736);sub_00064E30();int ready=g_eax!=0;
  if(ready){g_ecx=0x4e8364;PUSH32(g_esp,0x276744);sub_00064E30();ready=g_eax!=1;}
  if(ready){g_ecx=0x4e8364;PUSH32(g_esp,0x276753);sub_00064E30();ready=g_eax!=4;}
  if(ready&&LO8(g_ebx)){
   g_ecx=MEM32(g_edi+16);
   if(g_ecx){g_eax=MEM32(g_edi+24);PUSH32(g_esp,g_ecx);
    if(g_eax){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x276771);sub_00058000();g_eax=MEM32(g_edi+8);g_esp+=8;POP32(g_esp,g_edi);POP32(g_esp,g_ebx);g_recomp_zf=!g_esp;g_esp+=12;return 1;}
    PUSH32(g_esp,0x276781);sub_00057FC0();g_esp+=4;
   }
   g_eax=MEM32(g_edi+8);POP32(g_esp,g_edi);POP32(g_esp,g_ebx);g_esp+=12;return 1;
  }
 }
 POP32(g_esp,g_edi);g_eax=0;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=12;return 1;
}
