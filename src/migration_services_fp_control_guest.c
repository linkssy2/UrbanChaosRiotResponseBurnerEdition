#include "migration_services_fp_control_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
int uc_migration_services_fp_control(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_FP_CONTROL");enabled=p&&!strcmp(p,"1");}
 uint32_t s=g_esp;if(!enabled||MEM8(0x51c178)||s<0x10010||s>=0x80000000u||(uint64_t)s+20>g_xbox_total_ram)return 0;
 if(entry==0x2da92d){
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMD(g_esp+12);g_fp_stack[g_fp_top]=RECOMP_X87_ROUND(g_fp_stack[g_fp_top]);MEMD(g_esp)=g_fp_stack[g_fp_top];g_fp_stack[g_fp_top]=MEMD(g_esp);POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);g_esp+=4;return 1;
 }
 if(entry!=0x2daa8e&&entry!=0x2da968)return 0;
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;
 if(entry==0x2daa8e){
  PUSH32(g_esp,g_ecx);MEM16(frame-4)=g_fp_control_word;g_eax=MEM32(frame+12);g_ecx=MEM32(frame+8)&MEM32(frame+12);g_eax=(~g_eax&MEM32(frame-4))|g_ecx;MEM32(frame+12)=g_eax;g_fp_control_word=MEM16(frame+12);g_eax=(uint32_t)(int32_t)SMEM16(frame-4);g_recomp_zf=!g_eax;
 }else{
  g_edx=0;uint32_t high=MEM32(frame+12),low=MEM32(frame+8);
  if(high==0x7ff00000u&&!low){g_eax=1;g_recomp_zf=0;}
  else if(high==0xfff00000u&&!low){PUSH32(g_esp,2);POP32(g_esp,g_eax);}
  else{g_ecx=MEM32(frame+14);g_eax=0x7ff8;g_ecx&=g_eax;
   if(g_ecx==0x7ff8){PUSH32(g_esp,3);POP32(g_esp,g_eax);}
   else if(g_ecx==0x7ff0&&((high&0x7ffff)||low)){PUSH32(g_esp,4);POP32(g_esp,g_eax);}
   else{g_eax=0;g_recomp_zf=1;}
  }
 }
 g_esp=frame+8;return 1;
}
