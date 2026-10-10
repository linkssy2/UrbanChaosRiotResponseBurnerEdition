#include "migration_interface_dialog.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00017060(void),sub_00017180(void),sub_00274300(void),sub_00274320(void),sub_002745F0(void),sub_002D37B0(void),sub_00282A60(void);
static int dialog_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned dialog_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static int dialog_target(unsigned t){return t==0x282802||t==0x28282c||t==0x282857||t==0x282966;}
static int dialog_groups(unsigned frame){
 for(;;){
  g_ecx=MEM32(g_esi+16);if(MEM32(g_ecx)==10)break;
  do{g_ecx=MEM32(g_esi+16);g_eax=MEM32(g_ecx);if(g_eax!=6&&g_eax!=5)break;g_ecx+=12;MEM32(g_esi+16)=g_ecx;}while(1);
  g_edx=MEM32(g_esi+16);g_eax=g_edx;do{g_ecx=MEM32(g_eax);if(g_ecx!=6&&g_ecx!=5)break;g_eax+=12;}while(1);
  g_ecx=MEM32(g_eax);g_edi=0;while(g_ecx!=6&&g_ecx!=5){g_ecx=MEM32(g_eax+12);g_eax+=12;g_edi++;}
  g_eax=MEM32(g_edx+8);g_xmm0=XMM_ZERO();SET_LO8(g_ebx,1);MEMF(g_esp+20)=MEMF(g_esp+16)=g_xmm0.f[0];MEM8(g_esp+15)=LO8(g_ebx);unsigned target=0;
  if(g_eax){g_eax--;if(g_eax<=3){target=MEM32(0x282a44+g_eax*4);if(!dialog_target(target)){g_seh_ebp=frame;RECOMP_ITAIL(target);return 1;}}}
  else if((int32_t)g_edi<4){
   PUSH32(g_esp,3);PUSH32(g_esp,0);PUSH32(g_esp,0x282890);sub_002D37B0();SET_LO8(g_ecx,MEM8(g_esp+g_eax+32));g_esp+=8;
   while(LO8(g_ecx)){PUSH32(g_esp,3);PUSH32(g_esp,0);PUSH32(g_esp,0x2828a9);sub_002D37B0();SET_LO8(g_ecx,MEM8(g_esp+g_eax+32));g_esp+=8;}
   g_ecx=g_eax;
   if(!g_ecx){g_xmm0=XMM_SCALAR(MEMF(0x5031f8));SET_LO8(g_ecx,MEM8(0x51d6fc));SET_LO8(g_ebx,MEM8(0x5031d8));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x5031fc));MEM8(g_esp+15)=LO8(g_ecx);MEMF(g_esp+16)=g_xmm0.f[0];}
   else if(!--g_ecx){g_xmm0=XMM_SCALAR(MEMF(0x503200));SET_LO8(g_edx,MEM8(0x5031d9));SET_LO8(g_ebx,MEM8(0x51d6fd));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x503204));MEM8(g_esp+15)=LO8(g_edx);MEMF(g_esp+16)=g_xmm0.f[0];}
   else if(!--g_ecx){g_xmm0=XMM_SCALAR(MEMF(0x503208));SET_LO8(g_ecx,MEM8(0x51d6ff));SET_LO8(g_ebx,MEM8(0x51d6fe));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x50320c));MEM8(g_esp+15)=LO8(g_ecx);MEMF(g_esp+16)=g_xmm0.f[0];}
   MEM8(g_esp+g_eax+24)=1;
  }else{MEM8(g_esp+27)=1;target=0x282966;}
  if(target){
   if(target==0x282802){g_xmm0=XMM_SCALAR(MEMF(0x503200));SET_LO8(g_eax,MEM8(0x5031d9));SET_LO8(g_ebx,MEM8(0x51d6fd));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x503204));MEM8(g_esp+15)=LO8(g_eax);}
   else if(target==0x28282c){g_xmm0=XMM_SCALAR(MEMF(0x5031f8));SET_LO8(g_ecx,MEM8(0x51d6fc));SET_LO8(g_ebx,MEM8(0x5031d8));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x5031fc));MEM8(g_esp+15)=LO8(g_ecx);}
   else if(target==0x282857){g_xmm0=XMM_SCALAR(MEMF(0x503208));SET_LO8(g_ebx,MEM8(0x51d6fe));SET_LO8(g_edx,MEM8(0x51d6ff));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x50320c));MEM8(g_esp+15)=LO8(g_edx);}
   else{g_xmm0=XMM_SCALAR(MEMF(0x503210));SET_LO8(g_edx,MEM8(0x5031db));SET_LO8(g_ebx,MEM8(0x5031da));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x503214));MEM8(g_esp+15)=LO8(g_edx);}
   MEMF(g_esp+16)=g_xmm0.f[0];
  }
  g_eax=ZX8(MEM8(g_esp+15));g_edx=MEM32(g_esp+16);PUSH32(g_esp,g_edi);g_esp-=8;g_xmm0.f[0]=(float)(int32_t)g_eax;g_eax=MEM32(g_esp+32);MEMF(g_esp+4)=g_xmm0.f[0];g_ecx=ZX8(LO8(g_ebx));g_xmm0.f[0]=(float)(int32_t)g_ecx;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x2829c2);sub_00282A60();
  g_ecx=MEM32(g_esi+16);if(MEM32(g_ecx)==6)MEM8(g_esi+36)=0;else{SET_LO8(g_eax,MEM8(g_esp+27));if(LO8(g_eax))MEM8(g_esi+36)=0;}SET_LO8(g_eax,MEM8(g_esi+36));if(!LO8(g_eax))break;
 }return 0;
}
int migration_interface_dialog(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_DIALOG");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!dialog_span(s-1024,1040)||!dialog_span(g_ecx,64))return 0;
 for(unsigned i=0;i<4;i++)if(!dialog_target(MEM32(0x282a44+i*4)))return 0;
 unsigned stream=MEM32(g_ecx+16),i=0;for(;i<4096;i++){if(!dialog_span((uint64_t)stream+i*12,12))return 0;if(MEM32(stream+i*12)==10)break;}if(i==4096)return 0;
 unsigned frame=g_seh_ebp;g_esp-=16;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x509704);PUSH32(g_esp,5);PUSH32(g_esp,0x2826a3);sub_00017060();g_xmm1=XMM_SCALAR(MEMF(g_esp+24));g_eax=MEM32(g_esi+40);g_xmm0=g_xmm1;g_xmm0.f[0]+=MEMF(g_esi+28);MEMF(g_esi+28)=g_xmm0.f[0];
 if((int32_t)g_eax>(int32_t)MEM32(0x5031c8)){MEM8(0x5154bc)=0;g_eax=MEM32(g_esi+4);POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=!g_esp;g_esp+=12;return 1;}
 g_xmm0=XMM_SCALAR(MEMF(g_esi+32));g_xmm0.f[0]+=g_xmm1.f[0];MEMF(g_esi+32)=g_xmm0.f[0];if(!(dialog_cmp(g_xmm0.f[0],MEMF(0x5031b0))&1)){MEM8(g_esi+36)=1;g_xmm0.f[0]-=MEMF(0x5031b0);MEMF(g_esi+32)=g_xmm0.f[0];}
 g_ecx=MEM32(0x503200);g_edx=MEM32(0x5031f8);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x282714);sub_00017180();SET_LO8(g_eax,MEM8(g_esi+36));MEM8(g_esp+20)=MEM8(g_esp+21)=MEM8(g_esp+22)=MEM8(g_esp+23)=0;int result=0;
 if(LO8(g_eax)){g_edx=MEM32(g_esi+40)+1;g_ecx=g_esi+52;MEM32(g_esi+40)=g_edx;PUSH32(g_esp,0x282742);sub_00274300();g_eax=MEM32(g_esi+40);
  if((int32_t)g_eax>(int32_t)MEM32(0x5031c8)){MEM8(0x5154bc)=0;g_eax=MEM32(g_esi+4);result=1;}
  else{SET_LO8(g_eax,MEM8(g_esi+36));if(LO8(g_eax)){PUSH32(g_esp,g_ebx);if(dialog_groups(frame))return 1;POP32(g_esp,g_ebx);}MEM8(g_esi+36)=0;}
 }
 if(!result){g_edx=MEM32(g_esp+28);g_edi=g_esi+52;PUSH32(g_esp,g_edx);g_ecx=g_edi;PUSH32(g_esp,0x2829f5);sub_00274320();g_ecx=MEM32(0x509704);PUSH32(g_esp,0x44138000);PUSH32(g_esp,0x42480000);PUSH32(g_esp,0x282a0a);sub_00017180();SET_LO8(g_eax,MEM8(g_esi+8));if(LO8(g_eax)){PUSH32(g_esp,22);PUSH32(g_esp,0x282a18);sub_002745F0();g_esp+=4;if(LO8(g_eax)){g_ecx=g_edi;MEM8(0x5154bc)=0;PUSH32(g_esp,0x282a2d);sub_00274300();g_eax=MEM32(g_esi+4);result=1;}}}
 POP32(g_esp,g_edi);if(!result)g_eax=0;POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=!g_esp;g_esp+=12;return 1;
}
