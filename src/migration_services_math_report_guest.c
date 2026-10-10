#include "migration_services_math_report_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002DA55A(void),sub_002DA2B6(void),sub_002DAA8E(void),sub_002DE6D5(void),sub_002DA747(void),sub_002DE6D8(void);
static int mr_span(unsigned p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Entry 2DAF54 prepares the two-operand fields then jumps inside 2DAF6B at retail 2DAF74. */
static void mr_tail(unsigned f){
 MEMD(f-8)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;MEM32(f-28)=g_ecx;g_eax=MEM32(f+16);g_ecx=MEM32(f+20);MEM32(f-24)=g_eax;MEM32(f-20)=g_ecx;g_eax=f+8;g_ecx=f-32;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ebp=f;PUSH32(g_esp,0x2daf94);sub_002DE6D8();
 g_esp+=12;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMD(f-8);int same=MEM16(f+8)==0x27f;if(!same)g_fp_control_word=MEM16(f+8);g_esp=f+8;g_recomp_zf=same;
}
void uc_migration_services_math_report_tail(void){mr_tail(g_seh_ebp);}
int uc_migration_services_math_report(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_MATH_REPORT");enabled=p&&!strcmp(p,"1");}
 unsigned s=g_esp;if(!enabled||MEM8(0x51c178)||!mr_span(s-512,544))return 0;
 if(entry==0x2daf54||entry==0x2daf6b){PUSH32(g_esp,g_ebp);unsigned f=g_esp;g_ebp=f;g_esp-=32;MEM32(f-32)=g_eax;if(entry==0x2daf54){g_eax=MEM32(f+24);MEM32(f-16)=g_eax;g_eax=MEM32(f+28);MEM32(f-12)=g_eax;g_seh_ebp=f;}mr_tail(f);return 1;}
 if(entry!=0x2de6d8||!mr_span(MEM32(s+8),32)||!mr_span(MEM32(s+12),4))return 0;
 PUSH32(g_esp,g_ebp);unsigned f=g_esp;g_ebp=f;g_esp=(g_esp&~15u)-116;g_eax=MEM32(f+16);g_ecx=MEM16(g_eax);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(f+12);g_eax=MEM32(g_esi)-1;PUSH32(g_esp,g_edi);MEM32(g_esp+12)=g_ecx;
 unsigned code=MEM32(g_esi),mask=code==1||code==5?8:code==2?4:code==3?17:code==4?18:code==8?16:0;
 if(mask){
  g_eax=0;PUSH32(g_esp,mask);POP32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);g_edi=g_esi+24;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);g_ebp=f;PUSH32(g_esp,0x2de72f);sub_002DA55A();g_esp+=12;
  if(!g_eax){
   g_eax=MEM32(f+8);
   if(g_eax==16||g_eax==22||g_eax==29){g_ecx=MEM32(g_esp+80);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMD(g_esi+16);g_ecx&=0xffffffe3;MEMD(g_esp+64)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_ecx|=3;MEM32(g_esp+80)=g_ecx;}
   else MEM32(g_esp+80)&=0xfffffffe;
   PUSH32(g_esp,g_edi);g_ecx=g_esi+8;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);g_eax=g_esp+28;PUSH32(g_esp,g_eax);g_eax=g_esp+36;PUSH32(g_esp,g_eax);g_ebp=f;PUSH32(g_esp,0x2de77a);sub_002DA2B6();g_esp+=24;
  }
 }else{g_eax=code-8;if(code==7){g_eax=0;MEM32(g_esi)=1;}}
 PUSH32(g_esp,0xffff);PUSH32(g_esp,MEM32(g_esp+16));g_ebp=f;PUSH32(g_esp,0x2de78b);sub_002DAA8E();POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
 int report=1;
 if(MEM32(g_esi)!=8&&!MEM32(0x505220)){PUSH32(g_esp,g_esi);g_ebp=f;PUSH32(g_esp,0x2de7a1);sub_002DE6D5();report=!g_eax;POP32(g_esp,g_ecx);}
 if(report){PUSH32(g_esp,MEM32(g_esi));g_ebp=f;PUSH32(g_esp,0x2de7ad);sub_002DA747();POP32(g_esp,g_ecx);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f+8;return 1;
}
