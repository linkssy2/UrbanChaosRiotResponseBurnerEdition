#include "migration_services_power_special_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002DA968(void),sub_002DA92D(void);
static void ps_push(double x){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=x;}
static void ps_pop(void){g_fp_top=(g_fp_top+1)&7;}
static double ps_top(void){return g_fp_stack[g_fp_top];}
static void ps_status(void){SET_LO16(g_eax,((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?g_fp_cmp&0x4700:g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000));}
static void ps_classify(void){
 PUSH32(g_esp,g_ebp);unsigned f=g_esp;g_ebp=f;g_eax=MEM32(f+14);g_ecx=0x7ff0;g_edx=g_eax&g_ecx;
 if(LO16(g_edx)==LO16(g_ecx)){
  ps_push(MEMD(f+8));PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);MEMD(g_esp)=ps_top();ps_pop();g_ebp=f;PUSH32(g_esp,0x2d866d);sub_002DA968();g_eax--;POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
  if(!g_eax){g_eax=0x200;g_recomp_zf=1;}
  else if(!--g_eax){PUSH32(g_esp,4);POP32(g_esp,g_eax);g_recomp_zf=1;}
  else if(!--g_eax){PUSH32(g_esp,2);POP32(g_esp,g_eax);g_recomp_zf=1;}
  else{g_eax=1;g_recomp_zf=0;}
 }else{
  g_eax&=0x8000;g_ecx=g_eax;
  if(!LO16(g_edx)&&((MEM32(f+12)&0xfffff)||MEM32(f+8)))g_eax=g_ecx?0x10:0x80;
  else{ps_push(MEMD(0x463af0));ps_push(MEMD(f+8));g_fp_cmp=RECOMP_FCMP(ps_top(),g_fp_stack[(g_fp_top+1)&7]);ps_pop();ps_pop();ps_status();unsigned even=RECOMP_PARITY8(HI8(g_eax)&0x44);g_eax=g_ecx;g_eax=even?(g_eax?8:0x100):(g_eax?0x20:0x40);}
  g_recomp_zf=!g_eax;
 }
 g_esp=f+8;
}
static void ps_integer(void){
 PUSH32(g_esp,g_ebp);unsigned f=g_esp;g_ebp=f;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);ps_push(MEMD(f+8));PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);MEMD(g_esp)=ps_top();ps_pop();g_ebp=f;PUSH32(g_esp,0x2dcd05);ps_classify();unsigned reject=LO8(g_eax)&0x90;POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
 if(reject)goto not_integer;
 ps_push(MEMD(f+8));PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);MEMD(g_esp)=ps_top();ps_pop();g_ebp=f;PUSH32(g_esp,0x2dcd18);sub_002DA92D();ps_push(MEMD(f+8));g_fp_cmp=RECOMP_FCMP(ps_top(),g_fp_stack[(g_fp_top+1)&7]);ps_pop();ps_pop();POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);ps_status();if(RECOMP_PARITY8(HI8(g_eax)&0x44))goto not_integer;
 ps_push(MEMD(f+8));PUSH32(g_esp,g_ecx);g_fp_stack[g_fp_top]*=MEMD(0x450bc8);PUSH32(g_esp,g_ecx);MEMD(f-8)=ps_top();MEMD(g_esp)=ps_top();ps_pop();g_ebp=f;PUSH32(g_esp,0x2dcd3c);sub_002DA92D();ps_push(MEMD(f-8));POP32(g_esp,g_ecx);g_fp_cmp=RECOMP_FCMP(ps_top(),g_fp_stack[(g_fp_top+1)&7]);ps_pop();ps_pop();POP32(g_esp,g_ecx);ps_status();
 if(RECOMP_PARITY8(HI8(g_eax)&0x44)){g_eax=1;g_recomp_zf=0;}
 else{unsigned zf=!(HI8(g_eax)&0x44);PUSH32(g_esp,2);POP32(g_esp,g_eax);g_recomp_zf=zf;}g_esp=f+8;return;
not_integer:g_eax=0;g_recomp_zf=1;g_esp=f+8;
}
static void ps_special(void){
 PUSH32(g_esp,g_ebp);unsigned f=g_esp;g_ebp=f;ps_push(MEMD(f+8));PUSH32(g_esp,g_esi);g_fp_cmp=RECOMP_FCMP(ps_top(),MEMD(0x463af0));g_edx=0;g_esi=0;ps_status();if(!RECOMP_PARITY8(HI8(g_eax)&5))g_fp_stack[g_fp_top]=-ps_top();g_eax=0x7ff00000;g_ecx=0xfff00000;
 unsigned exponent=MEM32(f+20);
 if((exponent==g_eax||exponent==g_ecx)&&!MEM32(f+16)){
  g_fp_cmp=RECOMP_FCMP(ps_top(),MEMD(0x453190));ps_status();
  if(!(HI8(g_eax)&0x41)){ps_pop();ps_push(exponent==0x7ff00000?MEMD(0x504b18):0.0);g_eax=MEM32(f+24);}
  else{g_fp_cmp=RECOMP_FCMP(ps_top(),MEMD(0x453190));ps_pop();ps_status();unsigned even=RECOMP_PARITY8(HI8(g_eax)&5);g_eax=MEM32(f+24);if(even){ps_push(MEMD(0x504b20));g_esi=1;}else ps_push(exponent==0x7ff00000?0.0:MEMD(0x504b18));}
  MEMD(g_eax)=ps_top();ps_pop();goto done;
 }
 ps_pop();
 if(MEM32(f+12)==0x7ff00000&&!MEM32(f+8)){
  ps_push(MEMD(f+16));g_fp_cmp=RECOMP_FCMP(ps_top(),MEMD(0x463af0));ps_pop();ps_status();
  if(!(HI8(g_eax)&0x41)){ps_push(MEMD(0x504b18));g_eax=MEM32(f+24);}
  else{ps_push(MEMD(f+16));g_fp_cmp=RECOMP_FCMP(ps_top(),MEMD(0x463af0));ps_pop();ps_status();unsigned even=RECOMP_PARITY8(HI8(g_eax)&5);g_eax=MEM32(f+24);ps_push(even?1.0:0.0);}
  MEMD(g_eax)=ps_top();ps_pop();
 }else if(MEM32(f+12)==g_ecx&&!MEM32(f+8)){
  ps_push(MEMD(f+16));PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);MEMD(g_esp)=ps_top();ps_pop();g_ebp=f;PUSH32(g_esp,0x2dce51);ps_integer();ps_push(MEMD(f+16));POP32(g_esp,g_ecx);g_fp_cmp=RECOMP_FCMP(ps_top(),MEMD(0x463af0));ps_pop();POP32(g_esp,g_ecx);g_ecx=g_eax;ps_status();
  int twice=1;
  if(!(HI8(g_eax)&0x41)){ps_push(MEMD(0x504b18));if(g_ecx==1)g_fp_stack[g_fp_top]=-ps_top();}
  else{ps_push(MEMD(f+16));g_fp_cmp=RECOMP_FCMP(ps_top(),MEMD(0x463af0));ps_pop();ps_status();if(RECOMP_PARITY8(HI8(g_eax)&5)){ps_push(1.0);twice=0;}else ps_push(g_ecx==1?MEMD(0x504b38):0.0);}
  g_eax=MEM32(f+24);if(twice)ps_push(ps_top());MEMD(g_eax)=ps_top();ps_pop();if(twice)ps_pop();
 }
done:g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;
}
int uc_migration_services_power_special(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_POWER_SPECIAL");enabled=p&&!strcmp(p,"1");}
 unsigned s=g_esp;if(!enabled||MEM8(0x51c178)||s<0x10080||s>=0x80000000u||(uint64_t)s+32>g_xbox_total_ram)return 0;
 if(entry==0x2d864c)ps_classify();else if(entry==0x2dccf3)ps_integer();else if(entry==0x2dcd58){unsigned p=MEM32(s+20);if(p<0x10000||p>=0x80000000u||(uint64_t)p+8>g_xbox_total_ram)return 0;ps_special();}else return 0;return 1;
}
