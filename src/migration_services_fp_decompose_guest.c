#include "migration_services_fp_decompose_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static unsigned fp_status(void){return ((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?g_fp_cmp&0x4700:g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);}
static void push_fp(double value){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;}
static void replace_exponent(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);g_eax=MEM32(f+16);push_fp(MEMD(f+8));g_ecx=MEM32(f+14);MEMD(f-8)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
 g_eax=((g_eax+0x3fe)<<4)|(g_ecx&0xffff800f);g_ecx&=0xffff800f;MEM16(f-2)=(uint16_t)g_eax;push_fp(MEMD(f-8));g_esp=f+8;g_recomp_zf=!g_eax;
}
int uc_migration_services_fp_decompose(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_FP_DECOMPOSE");enabled=p&&!strcmp(p,"1");}
 uint32_t s=g_esp;if(!enabled||MEM8(0x51c178)||s<0x10080||s>=0x80000000u||(uint64_t)s+20>g_xbox_total_ram)return 0;
 if(entry==0x2da93e){replace_exponent();return 1;}
 if(entry==0x2daab2){
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);SET_LO8(g_ecx,MEM8(g_esp+12));unsigned flags=(uint8_t)g_ecx;
  if(flags&1){push_fp(MEMF(0x504b40));SMEM32(g_esp+12)=(int32_t)llrint(g_fp_stack[g_fp_top]);g_fp_top=(g_fp_top+1)&7;}
  if(flags&8){SET_LO16(g_eax,fp_status());push_fp(MEMF(0x504b40));MEMD(g_esp)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;SET_LO16(g_eax,fp_status());}
  if(flags&16){push_fp(MEMF(0x504b4c));MEMD(g_esp)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
  if(flags&4){push_fp(0);push_fp(1);g_fp_stack[(g_fp_top+1)&7]=g_fp_stack[g_fp_top]/g_fp_stack[(g_fp_top+1)&7];g_fp_top=(g_fp_top+2)&7;}
  if(flags&32){push_fp(3.14159265358979323846);MEMD(g_esp)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
  POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);g_recomp_zf=!(flags&32);g_esp+=4;return 1;
 }
 if(entry!=0x2da9c3)return 0;
 uint32_t dest=MEM32(s+12);if(dest<0x10000||(uint64_t)dest+4>g_xbox_total_ram)return 0;
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;push_fp(MEMD(0x463af0));push_fp(MEMD(f+8));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;SET_LO16(g_eax,fp_status());
 if(!RECOMP_PARITY8(HI8(g_eax)&0x44)){push_fp(0);g_edx=0;}
 else{
  g_ecx=0;int subnormal=!(MEM16(f+14)&0x7ff0)&&((MEM32(f+12)&0xfffff)||MEM32(f+8));
  if(subnormal){
   push_fp(MEMD(f+8));g_edx=0xfffffc03;g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMD(0x463af0));g_fp_top=(g_fp_top+1)&7;SET_LO16(g_eax,fp_status());g_eax=!RECOMP_PARITY8(HI8(g_eax)&5);
   while(!(MEM8(f+14)&16)){MEM32(f+12)=(MEM32(f+12)<<1)|(MEM32(f+8)>>31);MEM32(f+8)<<=1;g_edx--;}
   MEM8(f+14)&=0xef;if(g_eax!=g_ecx)MEM8(f+15)|=0x80;
  }
  push_fp(MEMD(f+8));PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);MEMD(g_esp)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_ebp=f;PUSH32(g_esp,subnormal?0x2daa48:0x2daa5b);replace_exponent();
  if(!subnormal)g_edx=((MEM32(f+14)>>4)&0x7ff)-0x3fe;g_esp+=12;
 }
 g_eax=MEM32(f+16);MEM32(g_eax)=g_edx;g_esp+=8;return 1;
}
