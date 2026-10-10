#include "migration_graphics_specular.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E88C0(void),sub_003EAA50(void),sub_003EAA60(void),sub_003EAAC0(void);
static int specular_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_SPECULAR");flag=v&&!strcmp(v,"1");}return flag;}
static int specular_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void spec_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void spec_pop(void){g_fp_top=(g_fp_top+1)&7;}
#define ST(n) g_fp_stack[(g_fp_top+(n))&7]
static void spec_store(uint32_t p){MEMF(p)=(float)ST(0);spec_pop();}
static void spec_status(void){g_eax=(g_eax&0xffff0000u)|((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);}
#define SPEC_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static void specular_coefficients(void){
 PUSH32(g_esp,g_ecx);spec_push(MEMF(g_esp+8));g_fp_cmp=RECOMP_FCMP(ST(0),MEMF(0x44bf94));spec_pop();spec_status();unsigned flags=(g_eax>>8)&5;
 if(!RECOMP_PARITY8(flags)){
  spec_push(MEMF(0x44c174));spec_push(MEMF(g_esp+8));g_fp_cmp=RECOMP_FCMP(ST(0),ST(1));spec_pop();spec_pop();spec_status();flags=(g_eax>>8)&0x44;
  if(!RECOMP_PARITY8(flags))spec_push(MEMF(0x44c174));
  else{spec_push(MEMF(0x455a64));PUSH32(g_esp,g_ecx);ST(0)/=MEMF(g_esp+12);spec_store(g_esp);SPEC_CALL(sub_003EAA60,0x3e8900);}
  g_eax=MEM32(g_esp+12);spec_push(ST(0));g_ecx=MEM32(g_esp+16);ST(0)=-ST(0);spec_store(g_eax);spec_push(MEMF(0x44bf94));ST(0)-=ST(1);ST(0)*=MEMF(g_esp+8);ST(0)=(double)MEMF(0x44bf94)-ST(0);spec_store(g_ecx);spec_pop();
 }else{
  g_edx=MEM32(g_esp+8);PUSH32(g_esp,g_edx);SPEC_CALL(sub_003EAAC0,0x3e8932);ST(0)*=MEMF(0x455a60);spec_store(g_esp+8);g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_eax);SPEC_CALL(sub_003EAA50,0x3e8946);
  g_ecx=MEM32(g_esp+12);MEM32(g_esp)=g_eax;spec_push((double)(int32_t)MEM32(g_esp));g_edx=MEM32(g_esp+16);ST(0)=(double)MEMF(g_esp+8)-ST(0);spec_push(MEMF(0x44bf94));ST(0)-=ST(1);
  /* Linear interpolation of the two original coefficient tables. */
  spec_push(ST(0));ST(0)*=MEMF(g_eax*4+0x3f2400);spec_push(ST(2));ST(0)*=MEMF(g_eax*4+0x3f2404);ST(1)+=ST(0);spec_pop();spec_store(g_ecx);ST(0)*=MEMF(g_eax*4+0x3f2380);double swap=ST(0);ST(0)=ST(1);ST(1)=swap;ST(0)*=MEMF(g_eax*4+0x3f2384);ST(1)+=ST(0);spec_pop();spec_store(g_edx);
 }
 POP32(g_esp,g_ecx);g_recomp_zf=flags==0;g_esp+=16;
}
static void specular_packets(void){
 g_esp-=24;PUSH32(g_esp,g_ebx);g_ebx=MEM32(0x3f41d4);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(0x3f4238);g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);g_esi=g_eax;g_eax=MEM32(g_edi+0xf40);g_edx=g_esp+20;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);MEM32(g_esp+24)=g_eax;MEM32(g_esi)=0x1809e0;SPEC_CALL(sub_003E88C0,0x3edef4);
 unsigned subsequent=0;
 for(;;){
  spec_push(MEMF(g_esp+16));ST(0)+=MEMF(0x44bf94);
  if(!subsequent){g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);g_edx=g_esp+28;ST(0)-=MEMF(g_esp+24);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);}
  else{g_eax=g_esp+28;PUSH32(g_esp,g_eax);g_ecx=g_esp+28;ST(0)-=MEMF(g_esp+24);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);}
  spec_store(g_esp+44);spec_push(MEMF(g_esp+24));ST(0)*=MEMF(0x44c1f8);spec_store(g_esp);if(!subsequent)SPEC_CALL(sub_003E88C0,0x3edf23);else SPEC_CALL(sub_003E88C0,0x3edfb1);
  spec_push(MEMF(g_esp+24));ST(0)+=MEMF(0x44bf94);
  if(!subsequent){g_eax=MEM32(g_esp+16);g_ecx=MEM32(g_esp+20);g_edx=MEM32(g_esp+32);MEM32(g_esi+4)=g_eax;ST(0)-=MEMF(g_esp+28);g_eax=MEM32(g_esp+24);MEM32(g_esi+8)=g_ecx;g_ecx=MEM32(g_esp+28);spec_store(g_esi+24);MEM32(g_esi+12)=g_edx;MEM32(g_esi+16)=g_eax;MEM32(g_esi+20)=g_ecx;}
  else{g_edx=MEM32(g_esp+16);g_eax=MEM32(g_esp+20);g_ecx=MEM32(g_esp+32);MEM32(g_esi+4)=g_edx;g_edx=MEM32(g_esp+24);ST(0)-=MEMF(g_esp+28);MEM32(g_esi+8)=g_eax;g_eax=MEM32(g_esp+28);MEM32(g_esi+12)=g_ecx;MEM32(g_esi+16)=g_edx;MEM32(g_esi+20)=g_eax;spec_store(g_esi+24);}
  g_esi+=28;if(subsequent)g_ebx--;if(!g_ebx)break;subsequent=1;
  g_edx=MEM32(g_edi+0xf84);g_eax=g_esp+20;PUSH32(g_esp,g_eax);g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);MEM32(g_esp+24)=g_edx;MEM32(g_esi)=0x181e28;SPEC_CALL(sub_003E88C0,0x3edf82);
 }
 POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_graphics_specular(uint32_t entry){
 if(!specular_enabled())return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!specular_span(s-64,84)||!specular_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7)return 0;
 if(entry==0x3e88c0){if((MEM32(s+4)&0x7fffffff)>0x43000000u||!specular_span(MEM32(s+8),4)||!specular_span(MEM32(s+12),4))return 0;specular_coefficients();return 1;}
 if(entry==0x3edec0){if(!specular_span(0x3f41d4,104)||MEM32(0x3f41d4)>16||!specular_span(MEM32(0x3f4238),0xf88)||!specular_span(g_eax,28ull*(MEM32(0x3f41d4)+1)))return 0;specular_packets();return 1;}return 0;
}
