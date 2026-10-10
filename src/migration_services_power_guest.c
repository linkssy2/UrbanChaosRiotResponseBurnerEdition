#include "migration_services_power_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
extern void sub_002DAFBD(void),sub_002DAFED(void),sub_002DAFA8(void),sub_002D7099(void),sub_002DB046(void),sub_002DAF54(void),sub_002DAF6B(void),sub_002DCD58(void);
static void pw_push(double x){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=x;}
static void pw_pop(void){g_fp_top=(g_fp_top+1)&7;}
static double pw_top(void){return g_fp_stack[g_fp_top];}
static void pw_status(void){SET_LO16(g_eax,((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?g_fp_cmp&0x4700:g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000));}
static void pw_limits(void){
 g_esp-=8;MEMD(g_esp)=pw_top();g_eax=MEM32(g_esp+4);g_esp+=8;g_eax&=0x7ff00000;
 if(g_eax&&g_eax!=0x7ff00000){
  SET_LO16(g_eax,MEM16(g_esp));if(LO16(g_eax)!=0x27f){SET_LO16(g_eax,LO16(g_eax)&32);if(!LO16(g_eax)){pw_status();SET_LO16(g_eax,LO16(g_eax)&32);if(LO16(g_eax)){g_eax=8;goto report;}}g_fp_control_word=MEM16(g_esp);}POP32(g_esp,g_edx);g_esp+=4;return;
 }
 int overflow=g_eax!=0;pw_push(MEMD(overflow?0x497240:0x497248));double x=pw_top();g_fp_stack[g_fp_top]=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[(g_fp_top+1)&7]=x;g_fp_stack[g_fp_top]=ldexp(pw_top(),(int)g_fp_stack[(g_fp_top+1)&7]);g_fp_stack[(g_fp_top+1)&7]=pw_top();pw_pop();pw_push(pw_top());g_fp_stack[g_fp_top]=fabs(pw_top());g_fp_cmp=RECOMP_FCMP(pw_top(),MEMD(overflow?0x497230:0x497238));pw_pop();pw_status();g_eax=overflow?3:4;
 if(overflow?g_fp_cmp>0:g_fp_cmp<0)g_fp_stack[g_fp_top]*=MEMD(overflow?0x497250:0x497258);
report:{int two=g_edx==29;PUSH32(g_esp,two?0x2db0db:0x2db0d4);if(two)sub_002DAF54();else sub_002DAF6B();POP32(g_esp,g_edx);g_recomp_zf=two;g_esp+=4;}
}
static void pw_evaluate(void){
 unsigned frame=g_seh_ebp;g_ecx=g_eax;PUSH32(g_esp,g_eax);MEM16(g_esp)=g_fp_control_word;if(MEM16(g_esp)!=0x27f){PUSH32(g_esp,0x2d6ee8);sub_002DAFBD();}g_ecx&=0x7ff00000;g_edx=g_esp+8;
 if(g_ecx==0x7ff00000){
  g_ecx=0;g_eax=(g_eax&0xfffff)|MEM32(g_esp+16);
  if(g_eax){g_edx=g_esp+8;PUSH32(g_esp,0x2d6f74);sub_002DAFED();if(MEM8(g_esp+22)&8)goto nan_sum;g_ecx++;goto special;}
  g_edx=g_esp+8;PUSH32(g_esp,0x2d6fb1);sub_002DAFED();goto special;
 }
 PUSH32(g_esp,0x2d6f03);sub_002DAFED();
 /* Preserve the separately qualified INTERNAL103 producer-ZF correction. */
 if(g_recomp_zf){g_ecx=0;goto special;}
 if(!(g_eax&0x7ff00000)){
  g_eax=(MEM32(g_esp+12)&0xfffff)|MEM32(g_esp+8);if(g_eax)goto finite;
  pw_pop();g_eax=(MEM32(g_esp+20)&0x7fffffff)|MEM32(g_esp+16);
  if(!g_eax){pw_pop();pw_push(1.0);goto restore;}
  PUSH32(g_esp,0x2d7033);sub_002D7099();SET_HI8(g_ecx,MEM8(g_esp+15));SET_HI8(g_ecx,HI8(g_ecx)>>7);
  if(MEM32(g_esp+23)&0x80){pw_push(MEMF(0x504f88));if(HI8(g_ecx)&LO8(g_ecx))g_fp_stack[g_fp_top]=-pw_top();g_eax=2;goto error;}
  pw_push(0.0);if(HI8(g_ecx)&LO8(g_ecx))g_fp_stack[g_fp_top]=-pw_top();goto restore;
 }
finite:
 SET_LO8(g_ecx,MEM8(g_esp+15));SET_LO8(g_ecx,LO8(g_ecx)&0x80);
 if(LO8(g_ecx)){
  pw_push(g_fp_stack[(g_fp_top+1)&7]);PUSH32(g_esp,0x2d707b);sub_002D7099();g_fp_stack[g_fp_top]=-pw_top();
  if(!LO8(g_ecx)){pw_pop();pw_pop();pw_push(MEMF(0x504b58));g_eax=1;goto error;}
 }
 g_fp_stack[(g_fp_top+1)&7]=g_fp_stack[(g_fp_top+1)&7]*log2(pw_top());pw_pop();PUSH32(g_esp,0x2d6f28);sub_002DAFA8();if(LO8(g_ecx)==1)g_fp_stack[g_fp_top]=-pw_top();
 if(MEM32(0x51e4d4))goto restore;g_ecx=0x5048b0;g_edx=29;g_seh_ebp=frame;pw_limits();return;
special:
 g_eax=MEM32(g_esp+12);g_edx=g_eax;g_eax&=0x7ff00000;g_edx&=0xfffff;
 if(g_eax==0x7ff00000){g_edx|=MEM32(g_esp+8);if(g_edx){if(MEM8(g_esp+14)&8)goto nan_sum;goto invalid_sum;}}
 if(g_ecx)goto invalid_sum;
 g_esp-=116;g_ecx=g_esp;PUSH32(g_esp,g_ecx);g_esp-=16;MEMD(g_esp)=pw_top();pw_pop();MEMD(g_esp+8)=pw_top();pw_pop();PUSH32(g_esp,0x2d6fec);sub_002DCD58();g_esp+=16;POP32(g_esp,g_ecx);pw_push(MEMD(g_ecx));g_esp+=116;if(!g_eax)goto restore;g_eax=1;goto error;
nan_sum:g_fp_stack[(g_fp_top+1)&7]=g_fp_stack[(g_fp_top+1)&7]+pw_top();pw_pop();g_eax=1;goto error;
invalid_sum:g_fp_stack[(g_fp_top+1)&7]=g_fp_stack[(g_fp_top+1)&7]+pw_top();pw_pop();g_eax=7;
error:
 if(MEM32(0x51e4d4))goto restore;g_ecx=0x5048b0;g_edx=29;PUSH32(g_esp,0x2d6f69);sub_002DAF54();POP32(g_esp,g_edx);g_recomp_zf=1;g_esp+=4;return;
restore:g_seh_ebp=frame;sub_002DB046();
}
int uc_migration_services_power(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_POWER");enabled=p&&!strcmp(p,"1");}
 unsigned s=g_esp;if(!enabled||MEM8(0x51c178)||s<0x10200||s>=0x80000000u||(uint64_t)s+40>g_xbox_total_ram)return 0;
 if(entry==0x2db091){pw_limits();return 1;}if(entry!=0x2d6eb0&&entry!=0x2d6ed4)return 0;
 if(entry==0x2d6eb0){g_esp-=20;double x=pw_top();g_fp_stack[g_fp_top]=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[(g_fp_top+1)&7]=x;MEMD(g_esp)=pw_top();pw_pop();MEMD(g_esp+8)=pw_top();g_eax=MEM32(g_esp+12);PUSH32(g_esp,0x2d6ec7);pw_evaluate();g_esp+=20;g_recomp_zf=!g_esp;g_esp+=4;}else pw_evaluate();return 1;
}
