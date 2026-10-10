#include "migration_services_fp_mask_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002DAAB2(void),sub_002DA9C3(void);
static void mask_push(double x){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=x;}
static unsigned mask_status(void){return ((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?g_fp_cmp&0x4700:g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);}
static void mask_raise(uint32_t f,unsigned flags,unsigned pc){PUSH32(g_esp,flags);g_ebp=f;PUSH32(g_esp,pc);sub_002DAAB2();POP32(g_esp,g_ecx);}
int uc_migration_services_fp_mask(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_FP_MASK");enabled=p&&!strcmp(p,"1");}
 uint32_t s=g_esp;if(!enabled||entry!=0x2da55a||MEM8(0x51c178)||s<0x10200||s>=0x80000000u||(uint64_t)s+16>g_xbox_total_ram)return 0;
 uint32_t result=MEM32(s+8);if(result<0x10000||(uint64_t)result+8>g_xbox_total_ram)return 0;
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;g_esp-=16;g_eax=MEM32(f+8);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=1;g_esi=g_eax&31;MEM32(f-4)=g_esi;
 if((g_eax&8)&&(MEM8(f+16)&1)){mask_raise(f,g_ebx,0x2da57f);g_esi&=~8u;}
 else if((g_eax&4)&&(MEM8(f+16)&4)){mask_raise(f,4,0x2da599);g_esi&=~4u;}
 else if((g_eax&1)&&(MEM8(f+16)&8)){
  mask_raise(f,8,0x2da5bb);g_eax=MEM32(f+16);g_ecx=0xc00;g_eax&=g_ecx;unsigned rounding=g_eax;
  g_ecx=MEM32(f+12);mask_push(MEMD(g_ecx));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMD(0x463af0));g_fp_top=(g_fp_top+1)&7;
  if(rounding==0xc00)mask_push(MEMD(0x504b28));SET_LO16(g_eax,mask_status());int negative=!!(HI8(g_eax)&0x41);
  if(rounding!=0xc00){unsigned address=(rounding==0x800?negative:rounding==0x400?!negative:0)?0x504b28:0x504b18;mask_push(MEMD(address));}
  if(negative)g_fp_stack[g_fp_top]=-g_fp_stack[g_fp_top];MEMD(g_ecx)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_esi&=~1u;
 }
 else if((g_eax&2)&&(MEM8(f+16)&16)){
  g_esi=(g_eax&16)?g_ebx:0;mask_push(MEMD(0x463af0));PUSH32(g_esp,g_edi);g_edi=MEM32(f+12);mask_push(MEMD(g_edi));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;SET_LO16(g_eax,mask_status());
  if(RECOMP_PARITY8(HI8(g_eax)&0x44)){
   mask_push(MEMD(g_edi));g_eax=f-8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);MEMD(g_esp)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_ebp=f;PUSH32(g_esp,0x2da697);sub_002DA9C3();
   g_ecx=MEM32(f-8);MEMD(f-16)=g_fp_stack[g_fp_top];g_ecx-=1536;g_esp+=12;
   if((int32_t)g_ecx < -1074){g_fp_stack[g_fp_top]*=MEMD(0x463af0);g_esi=g_ebx;}
   else{
    g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMD(0x463af0));g_fp_top=(g_fp_top+1)&7;SET_LO16(g_eax,mask_status());g_edx=RECOMP_PARITY8(HI8(g_eax)&5)?0:g_ebx;
    g_eax=(MEM8(f-10)&15)|16;MEM16(f-10)=(uint16_t)g_eax;g_eax=0xfffffc03;
    if((int32_t)g_ecx < (int32_t)g_eax){g_eax-=g_ecx;do{if((MEM8(f-16)&(uint8_t)g_ebx)&&!g_esi)g_esi=g_ebx;MEM64(f-16)>>=1;g_eax--;}while(g_eax);}
    mask_push(MEMD(f-16));if(g_edx)g_fp_stack[g_fp_top]=-g_fp_stack[g_fp_top];
   }
   MEMD(g_edi)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
  }else g_esi=g_ebx;
  int inexact=!!g_esi;POP32(g_esp,g_edi);if(inexact)mask_raise(f,16,0x2da71d);MEM32(f-4)&=~2u;g_esi=MEM32(f-4);
 }
 if((MEM8(f+8)&16)&&(MEM8(f+16)&32)){mask_raise(f,32,0x2da738);g_esi&=~16u;}
 g_eax=!g_esi;g_recomp_zf=!g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f+8;return 1;
}
