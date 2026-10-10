#include "migration_services_fp_errors_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002DB935(void),sub_002DAA8E(void);
static void error_number(void){
 g_eax=MEM32(g_esp+4);uint32_t type=g_eax;
 if(type>=1&&type<=3){PUSH32(g_esp,type==1?0x2da768:0x2da75c);sub_002DB935();MEM32(g_eax)=type==1?33:34;if(type!=1)g_recomp_zf=type==3;}
 g_esp+=4;
}
int uc_migration_services_fp_errors(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_FP_ERRORS");enabled=p&&!strcmp(p,"1");}
 uint32_t s=g_esp;if(!enabled||MEM8(0x51c178)||s<0x10080||s>=0x80000000u||(uint64_t)s+48>g_xbox_total_ram)return 0;
 switch(entry){
 case 0x2de6d5:g_eax=0;g_recomp_zf=1;g_esp+=4;return 1;
 case 0x2da747:error_number();return 1;
 case 0x2da76f:{
  SET_LO8(g_eax,MEM8(s+4));unsigned flags=(uint8_t)g_eax;
  unsigned code=flags&32?5:flags&8?1:flags&4?2:flags&1?3:0;
  if(code==1){g_eax=1;g_recomp_zf=0;}else if(code){PUSH32(g_esp,code);POP32(g_esp,g_eax);}else{g_eax=(flags&2)<<1;g_recomp_zf=!g_eax;}
  g_esp+=4;return 1;
 }
 case 0x2daa77:case 0x2daa82:
  PUSH32(g_esp,g_ecx);MEM16(g_esp)=(uint16_t)(((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?g_fp_cmp&0x4700:g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000));
  /* The current guest contract has no separate pending x87 exception state. */
  g_eax=(uint32_t)(int32_t)SMEM16(g_esp);POP32(g_esp,g_ecx);g_esp+=4;return 1;
 case 0x2da79c:break;
 default:return 0;
 }
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp-=32;
 for(g_eax=0;g_eax<29;g_eax++){g_ecx=MEM32(0x504a30+8*g_eax);if(g_ecx==MEM32(frame+12))break;}
 g_eax=g_eax<29?MEM32(0x504a34+8*g_eax):0;MEM32(frame-28)=g_eax;
 if(g_eax){
  g_eax=MEM32(frame+16);MEM32(frame-24)=g_eax;g_eax=MEM32(frame+20);MEM32(frame-20)=g_eax;g_eax=MEM32(frame+24);MEM32(frame-16)=g_eax;g_eax=MEM32(frame+28);
  PUSH32(g_esp,g_esi);g_esi=MEM32(frame+8);MEM32(frame-12)=g_eax;g_eax=MEM32(frame+32);MEM32(frame-8)=g_eax;g_eax=MEM32(frame+36);
  PUSH32(g_esp,0xffff);PUSH32(g_esp,MEM32(frame+40));MEM32(frame-32)=g_esi;MEM32(frame-4)=g_eax;g_ebp=frame;PUSH32(g_esp,0x2da7f7);sub_002DAA8E();
  g_eax=frame-32;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x2da800);g_eax=0;g_recomp_zf=1;g_esp+=4;
  g_esp+=12;PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x2da80d);error_number();POP32(g_esp,g_ecx);
  g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMD(frame-8);POP32(g_esp,g_esi);g_esp=frame+8;g_recomp_zf=1;
 }else{
  PUSH32(g_esp,0xffff);PUSH32(g_esp,MEM32(frame+40));g_ebp=frame;PUSH32(g_esp,0x2da82a);sub_002DAA8E();
  PUSH32(g_esp,MEM32(frame+8));g_ebp=frame;PUSH32(g_esp,0x2da832);error_number();g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMD(frame+32);g_esp=frame+8;g_recomp_zf=frame+4==0;
 }
 return 1;
}
