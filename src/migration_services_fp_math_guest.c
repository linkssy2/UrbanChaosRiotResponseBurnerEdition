#include "migration_services_fp_math_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002DAA8E(void),sub_002DA92D(void),sub_002DA968(void),sub_002DA79C(void),sub_002DB935(void),sub_002DA55A(void),sub_002DA2B6(void),sub_002DA76F(void),sub_002DA747(void);
#define MATH_CALL(fn,pc) do{g_ebp=frame;PUSH32(g_esp,pc);fn();}while(0)
#define MATH_TOP g_fp_stack[g_fp_top]
static void math_push(double value){g_fp_top=(g_fp_top+7)&7;MATH_TOP=value;}
static void math_pop(void){g_fp_top=(g_fp_top+1)&7;}
static void math_store(uint32_t p){MEMD(p)=MATH_TOP;math_pop();}
static void math_compare(void){g_fp_cmp=RECOMP_FCMP(MATH_TOP,g_fp_stack[(g_fp_top+1)&7]);math_pop();math_pop();g_eax=(g_eax&0xffff0000u)|((g_fp_top&7)<<11)|(g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);}
static void math_copy_sign(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);g_eax=MEM32(frame+8);MEM32(frame-8)=g_eax;g_eax=(MEM32(frame+12)&0x7fffffffu)|(MEM32(frame+20)&0x80000000u);MEM32(frame-4)=g_eax;math_push(MEMD(frame-8));g_esp=frame+8;g_recomp_zf=!g_eax;
}
static void math_nan_result(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;
 if(!MEM32(0x505220)){PUSH32(g_esp,MEM32(frame+20));math_push(MEMD(frame+12));g_esp-=24;math_store(g_esp+16);math_push(0);math_store(g_esp+8);math_push(MEMD(frame+12));math_store(g_esp);PUSH32(g_esp,MEM32(frame+8));PUSH32(g_esp,1);MATH_CALL(sub_002DA79C,0x2da869);g_esp+=40;g_recomp_zf=g_esp==0;g_esp+=4;}
 else{MATH_CALL(sub_002DB935,0x2da873);PUSH32(g_esp,0xffff);PUSH32(g_esp,MEM32(frame+20));MEM32(g_eax)=33;MATH_CALL(sub_002DAA8E,0x2da886);math_push(MEMD(frame+12));POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);g_esp+=8;g_recomp_zf=0;}
}
static void math_exception_result(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-112;PUSH32(g_esp,MEM32(frame+32));g_eax=frame+24;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));MATH_CALL(sub_002DA55A,0x2da8a5);g_esp+=12;
 if(!g_eax){MEM32(g_esp+64)&=~1u;g_eax=frame+24;PUSH32(g_esp,g_eax);g_eax=frame+16;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+12));g_eax=frame+32;PUSH32(g_esp,MEM32(frame+8));PUSH32(g_esp,g_eax);g_eax=g_esp+20;PUSH32(g_esp,g_eax);MATH_CALL(sub_002DA2B6,0x2da8cd);g_esp+=24;}
 PUSH32(g_esp,MEM32(frame+8));MATH_CALL(sub_002DA76F,0x2da8d8);g_esp+=4;
 if(!MEM32(0x505220)&&g_eax){PUSH32(g_esp,MEM32(frame+32));math_push(MEMD(frame+24));g_esp-=24;math_store(g_esp+16);math_push(0);math_store(g_esp+8);math_push(MEMD(frame+16));math_store(g_esp);PUSH32(g_esp,MEM32(frame+12));PUSH32(g_esp,g_eax);MATH_CALL(sub_002DA79C,0x2da90a);g_esp+=36;}
 else{PUSH32(g_esp,g_eax);MATH_CALL(sub_002DA747,0x2da915);MEM32(g_esp)=0xffff;PUSH32(g_esp,MEM32(frame+32));MATH_CALL(sub_002DAA8E,0x2da924);math_push(MEMD(frame+24));POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);}
 g_esp=frame+8;
}
static void math_ceil(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=0xffff;PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(0x5048c0));MATH_CALL(sub_002DAA8E,0x2d72e9);
 math_push(MEMD(frame+8));POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);g_ebx=g_eax;g_eax=MEM32(frame+14);PUSH32(g_esp,g_ecx);SET_LO16(g_eax,LO16(g_eax)&0x7ff0);PUSH32(g_esp,g_ecx);math_store(g_esp);
 if(LO16(g_eax)==0x7ff0){
  MATH_CALL(sub_002DA968,0x2d7307);POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
  if((int32_t)g_eax>0&&g_eax<=2){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);MATH_CALL(sub_002DAA8E,0x2d7333);math_push(MEMD(frame+8));POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);}
  else if(g_eax==3){math_push(MEMD(frame+8));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);math_store(g_esp);PUSH32(g_esp,12);g_ebp=frame;PUSH32(g_esp,0x2d7327);math_nan_result();g_esp+=16;}
  else{math_push(MEMD(frame+8));PUSH32(g_esp,g_ebx);MATH_TOP+=MEMD(0x453190);g_esp-=16;math_store(g_esp+8);math_push(MEMD(frame+8));math_store(g_esp);PUSH32(g_esp,12);PUSH32(g_esp,8);g_ebp=frame;PUSH32(g_esp,0x2d739d);math_exception_result();g_esp+=28;}
 }else{
  MATH_CALL(sub_002DA92D,0x2d735c);math_store(frame-8);math_push(MEMD(frame+8));POP32(g_esp,g_ecx);math_push(MEMD(frame-8));POP32(g_esp,g_ecx);math_compare();
  if(!RECOMP_PARITY8(HI8(g_eax)&0x44)||(g_ebx&0x20)){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);MATH_CALL(sub_002DAA8E,0x2d7377);math_push(MEMD(frame-8));POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);}
  else{math_push(MEMD(frame-8));PUSH32(g_esp,g_ebx);g_esp-=16;math_store(g_esp+8);math_push(MEMD(frame+8));math_store(g_esp);PUSH32(g_esp,12);PUSH32(g_esp,16);g_ebp=frame;PUSH32(g_esp,0x2d739d);math_exception_result();g_esp+=28;}
 }
 POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+8;
}
static void math_modf(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0);PUSH32(g_esp,0);MATH_CALL(sub_002DAA8E,0x2d8130);g_esi=0xffff;PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(0x5048d8));g_ebx=g_eax;MATH_CALL(sub_002DAA8E,0x2d8143);
 g_edi=MEM32(frame+14);g_esp+=16;g_eax=g_edi;SET_LO16(g_eax,LO16(g_eax)&0x7ff0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);
 if(LO16(g_eax)==0x7ff0){
  math_push(MEMD(0x504b20));g_edi=MEM32(frame+16);math_store(g_edi);math_push(MEMD(frame+8));math_store(g_esp);MATH_CALL(sub_002DA968,0x2d8171);POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
  if(g_eax==3){math_push(MEMD(frame+8));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);math_store(g_edi);math_push(MEMD(frame+8));PUSH32(g_esp,g_ecx);math_store(g_esp);PUSH32(g_esp,28);g_ebp=frame;PUSH32(g_esp,0x2d8196);math_nan_result();g_esp+=16;}
  else if((int32_t)g_eax>0&&g_eax<=2){math_push(MEMD(frame+8));g_esp-=16;math_store(g_edi);math_push(MEMD(frame+8));math_store(g_esp+8);math_push(0);math_store(g_esp);g_ebp=frame;PUSH32(g_esp,0x2d81b7);math_copy_sign();math_store(frame+8);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);MATH_CALL(sub_002DAA8E,0x2d81c1);g_esp+=24;math_push(MEMD(frame+8));}
  else{math_push(MEMD(frame+8));PUSH32(g_esp,g_ebx);MATH_TOP+=MEMD(0x453190);g_esp-=16;MEMD(g_edi)=MATH_TOP;math_store(g_esp+8);math_push(MEMD(frame+8));math_store(g_esp);PUSH32(g_esp,28);PUSH32(g_esp,8);g_ebp=frame;PUSH32(g_esp,0x2d81e8);math_exception_result();g_esp+=28;}
 }else{
  math_push(MEMD(frame+8));math_store(g_esp);MATH_CALL(sub_002DA92D,0x2d81f8);g_eax=MEM32(frame+16);double integer=MATH_TOP;math_push(integer);math_store(g_eax);POP32(g_esp,g_ecx);math_push(MEMD(frame+8));POP32(g_esp,g_ecx);MATH_TOP-=g_fp_stack[(g_fp_top+1)&7];math_store(frame+8);math_pop();math_push(MEMD(0x463af0));math_push(MEMD(frame+8));math_compare();
  if(!RECOMP_PARITY8(HI8(g_eax)&0x44)){SET_LO16(g_edi,LO16(g_edi)&0x8000);MEM16(frame+14)|=LO16(g_edi);}PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);MATH_CALL(sub_002DAA8E,0x2d822d);POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);math_push(MEMD(frame+8));
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=8;
}
int uc_migration_services_fp_math(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_FP_MATH");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||g_esp<0x10200||g_esp>=0x80000000u||(uint64_t)g_esp+40>g_xbox_total_ram)return 0;
 if(entry==0x2d8121){uint32_t out=MEM32(g_esp+12);if(out<0x10000||(uint64_t)out+8>g_xbox_total_ram)return 0;}
 switch(entry){case 0x2d72d1:math_ceil();break;case 0x2d8121:math_modf();break;case 0x2d85fd:math_copy_sign();break;case 0x2da83a:math_nan_result();break;case 0x2da88d:math_exception_result();break;default:return 0;}return 1;
}
#undef MATH_CALL
#undef MATH_TOP
