#include "migration_interface_widgetlife.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00253F80(void),sub_002554C0(void),sub_002592B0(void),sub_00282640(void),sub_002D5CCD(void),sub_0006E040(void);
static int wl_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_widgetlife(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_WIDGETLIFE");enabled=s&&!strcmp(s,"1");}if(!enabled||op>7)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!wl_span(s-1024,1040)||!wl_span(g_ecx,64))return 0;
 if(op<2||op==4||op==5){unsigned child=MEM32(g_ecx+4);if(child&&(!wl_span(child,4)||!wl_span(MEM32(child),4)))return 0;}
 if(!(op&1)){
  static const unsigned returns[]={0x253f68,0x2554a8,0x259298,0x282628},frees[]={0x253f75,0x2554b5,0x2592a5,0x282635};
  static void(*const destroy[])(void)={sub_00253F80,sub_002554C0,sub_002592B0,sub_00282640};
  PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,returns[op/2]);destroy[op/2]();
  if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,frees[op/2]);sub_002D5CCD();g_esp+=4;}
  g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
 }
 if(op==3){MEM32(g_ecx)=0x450708;g_esp+=4;return 1;}
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,op==1?0x39ef68:op==5?0x39f238:0x3a02f8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;
 if(op==7){g_ecx=g_esi+52;MEM32(g_esp+16)=0;PUSH32(g_esp,0x28266d);sub_0006E040();}
 else{
  MEM32(g_esi)=op==1?0x4846a4:0x485c04;
  if(op==1){g_ecx=MEM32(0x51cebc)-1;g_eax=0;MEM32(g_esp+16)=0;MEM32(0x51cebc)=(int32_t)g_ecx<0?0:g_ecx;if(MEM8(g_esi+8))MEM32(0x51cec0)--;}
  g_ecx=MEM32(g_esi+4);if(op==5)MEM32(g_esp+16)=0;
  if(g_ecx){g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,1);unsigned target=MEM32(g_eax);PUSH32(g_esp,op==1?0x253fd5:0x2592e8);RECOMP_ICALL_SAFE(target,saved);}
 }
 g_ecx=MEM32(g_esp+8);MEM32(g_esi)=0x450708;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
