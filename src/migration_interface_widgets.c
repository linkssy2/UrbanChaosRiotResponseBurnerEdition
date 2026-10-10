#include "migration_interface_widgets.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D57FB(void),sub_00387C82(void),sub_00259A30(void),sub_00266E00(void),sub_0005AAB0(void),sub_001F4F10(void);
static int widgets_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void widgets_call(unsigned target,unsigned ret,unsigned saved){PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
int migration_interface_widgets(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_WIDGETS");enabled=s&&!strcmp(s,"1");}if(!enabled||op>8)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!widgets_span(s-1024,1056)||!widgets_span(0x51d0d4,4))return 0;
 if((op==1||op==8)&&!widgets_span(g_ecx,32))return 0;
 if(op==1){unsigned p=MEM32(s+4);if(p){if(!widgets_span(p,1))return 0;unsigned end=p;while(end<g_xbox_total_ram&&MEM8(end))end++;if(end==g_xbox_total_ram)return 0;}}
 if(op==2&&!widgets_span(MEM32(s+4),8))return 0;
 if(op==8&&(unsigned)g_fp_top>7)return 0;
 switch(op){
 case 0:
  PUSH32(g_esp,32);PUSH32(g_esp,0x263527);sub_002D57FB();g_esp+=4;
  if(g_eax){g_ecx=MEM32(g_esp+4);g_edx=MEM32(g_esp+8);g_xmm0=XMM_ZERO();MEM32(g_eax+8)=g_ecx;SET_LO8(g_ecx,MEM8(g_esp+12));MEM32(g_eax+12)=g_edx;SET_LO8(g_edx,MEM8(g_esp+16));MEM8(g_eax+16)=LO8(g_ecx);g_ecx=MEM32(g_esp+20);MEM8(g_eax+4)=1;MEM32(g_eax)=0x487608;MEM8(g_eax+17)=LO8(g_edx);MEM32(g_eax+20)=g_ecx;MEMF(g_eax+24)=MEMF(g_eax+28)=0;g_recomp_zf=0;}else{g_eax=0;g_recomp_zf=1;}break;
 case 1:
  g_edx=MEM32(g_esp+4);SET_LO8(g_eax,MEM8(g_esp+8));PUSH32(g_esp,g_ebx);g_ebx=0;PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi)=0x485c10;MEM8(g_esi+4)=LO8(g_eax);MEM32(g_esi+8)=0;
  if(g_edx){g_eax=g_edx;PUSH32(g_esp,g_edi);g_edi=g_eax+1;do{SET_LO8(g_ecx,MEM8(g_eax++));}while(LO8(g_ecx));g_eax-=g_edi;POP32(g_esp,g_edi);if(g_eax>1){PUSH32(g_esp,g_edx);PUSH32(g_esp,0x259a69);sub_00387C82();g_esp+=4;MEM32(g_esi+8)=g_eax;}}
  MEM32(g_esi+12)=g_ebx;g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;return 1;
 case 2:
  PUSH32(g_esp,16);PUSH32(g_esp,0x259be7);sub_002D57FB();g_esp+=4;
  if(g_eax){g_ecx=MEM32(g_esp+12);g_edx=MEM32(g_esp+8);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;PUSH32(g_esp,0x259bff);sub_00259A30();g_ecx=g_eax;g_eax=MEM32(g_esp+4);g_recomp_zf=0;}else{g_eax=MEM32(g_esp+4);g_ecx=0;g_recomp_zf=1;}MEM32(g_eax)=g_ecx;MEM32(g_eax+4)=1;break;
 case 3:
  g_esp-=16;g_ecx=MEM32(g_esp+28);g_eax=MEM32(g_esp+24);g_edx=MEM32(g_esp+32);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+24);MEM32(g_esp+8)=g_ecx;PUSH32(g_esp,2);g_ecx=g_esp+8;MEM32(g_esp+8)=g_eax;g_eax=MEM32(g_esp+44);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);MEM32(g_esp+24)=g_edx;MEM32(g_esp+28)=g_eax;PUSH32(g_esp,0x266ed5);sub_00266E00();g_esp+=12;g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=!g_esp;break;
 case 4:MEM8(0x4ff0f5)=1;break;
 case 5:MEM32(0x51d0d4)=4;break;
 case 6:{unsigned frame=g_seh_ebp;PUSH32(g_esp,0x254c25);sub_0005AAB0();g_ecx=MEM32(g_eax+324);if(g_ecx){PUSH32(g_esp,0x254c34);sub_0005AAB0();g_eax=MEM32(g_eax+324);g_edx=MEM32(g_eax);g_ecx=g_eax;g_seh_ebp=frame;RECOMP_ITAIL(MEM32(g_edx+268));return 1;}g_recomp_zf=1;break;}
 case 7:
  PUSH32(g_esp,g_esi);PUSH32(g_esp,0x254c56);sub_0005AAB0();g_esi=MEM32(g_eax+324);
  if(g_esi){do{g_eax=MEM32(g_esi);unsigned saved=g_esp;PUSH32(g_esp,0x3d088889);g_ecx=g_esi;widgets_call(MEM32(g_eax+220),0x254c6f,saved);g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;widgets_call(MEM32(g_edx+276),0x254c79,saved);}while(g_eax==2);
   g_eax=MEM32(g_esi);unsigned saved=g_esp;PUSH32(g_esp,0x3d088889);g_ecx=g_esi;widgets_call(MEM32(g_eax+220),0x254c8d,saved);do{g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;widgets_call(MEM32(g_edx+280),0x254c9a,saved);}while(g_eax==2);}
  POP32(g_esp,g_esi);break;
 case 8:
  PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_esi=g_ecx;
  if(g_edi==4){g_eax=MEM32(g_esi+8);if(g_eax<=1){POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=12;return 1;}}
  if(MEM32(g_esi+8)==3||g_edi==3){g_xmm0=XMM_ZERO();g_ecx=0x515468;MEMF(g_esi+16)=0;PUSH32(g_esp,0x64df6);sub_001F4F10();MEMF(g_esi+20)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
  MEM32(g_esi+8)=g_edi;if(g_edi<=1){g_eax=MEM32(g_esp+16);MEM32(g_esi+12)=g_eax?g_eax:0x4523d8;if(g_eax)g_recomp_zf=0;}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=12;return 1;
 }
 g_esp+=4;return 1;
}
