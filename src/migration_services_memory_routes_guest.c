#include "migration_services_memory_routes_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00083B5D(void),sub_000842D8(void);
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void set_error(void){
 unsigned level=MEM8(0x24);g_eax=level;if(level<2)g_eax=MEM32(0x28);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;
}
static void normal_allocate(void){
 g_eax=MEM32(g_esp+4);PUSH32(g_esp,MEM32(g_esp+8));g_eax=(g_eax>>3)&8;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(0x5206c4));PUSH32(g_esp,0x84c9c);sub_00083B5D();g_recomp_zf=!g_eax;g_esp+=12;
}
static void normal_release(void){
 PUSH32(g_esp,MEM32(g_esp+4));PUSH32(g_esp,0);PUSH32(g_esp,MEM32(0x5206c4));PUSH32(g_esp,0x84cc4);sub_000842D8();g_eax=LO8(g_eax)?0:MEM32(g_esp+4);g_recomp_zf=!g_eax;g_esp+=8;
}
static void physical_allocate(unsigned entry){
 uint32_t saved=g_esp;PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;
 if(entry==0x86c68){g_eax=MEM32(f+12);g_ecx=MEM32(f+8);if(g_eax!=0xffffffff){MEM32(f+16)=0;g_edx=g_eax;g_eax=g_ecx+g_eax-1;}else{g_edx=0;g_eax=0xffffffff;}saved=g_esp;}
 PUSH32(g_esp,g_esi);
 if(entry==0x86cab){for(unsigned offset=24;offset>=8;offset-=4)PUSH32(g_esp,MEM32(f+offset));}
 else{PUSH32(g_esp,MEM32(f+20));PUSH32(g_esp,MEM32(f+16));PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);}
 uint32_t target=MEM32(0x44b358);PUSH32(g_esp,entry==0x86cab?0x86cc4:0x86c97);RECOMP_ICALL_SAFE(target,saved);g_esi=g_eax;unsigned zero=!g_esi;
 if(zero){PUSH32(g_esp,8);g_ebp=f;PUSH32(g_esp,entry==0x86cab?0x86cd1:0x86ca4);set_error();}
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=4;g_recomp_zf=zero;g_esp+=entry==0x86cab?24:20;
}
static void allocate_route(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;PUSH32(g_esp,g_esi);g_esi=MEM32(f+12);
 if((int32_t)g_esi<0){
  if(!(g_esi&0xf000000))g_esi=(g_esi&0xfcffffff)|0xc000000;
  g_ecx=(g_esi>>24)&15;g_eax=(g_esi>>28)&3;PUSH32(g_esp,MEM32(g_eax*4+0x4ea4c0));
  if(g_ecx==1){PUSH32(g_esp,0x1000);PUSH32(g_esp,0xffffff);PUSH32(g_esp,0);PUSH32(g_esp,MEM32(f+8));g_ebp=f;PUSH32(g_esp,0x82577);physical_allocate(0x86cab);}
  else{g_eax=1u<<LO8(g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0xffffffff);PUSH32(g_esp,MEM32(f+8));g_ebp=f;PUSH32(g_esp,0x82589);physical_allocate(0x86c68);}
  g_edx=g_eax;
  if(g_edx&&(g_esi&0x40000000)){g_ecx=MEM32(f+8);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_eax=0;g_edi=g_edx;memset((void*)XBOX_PTR(g_edi),0,g_ecx);g_edi+=g_ecx;g_ecx=0;POP32(g_esp,g_edi);}
 }else{PUSH32(g_esp,MEM32(f+8));g_esi=(g_esi>>24)&64;PUSH32(g_esp,g_esi);g_ebp=f;PUSH32(g_esp,0x825bf);normal_allocate();g_edx=g_eax;}
 g_eax=g_edx;POP32(g_esp,g_esi);g_esp+=16;
}
int uc_migration_services_memory_routes(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_MEMORY_ROUTES");enabled=p&&!strcmp(p,"1");}
 if(!enabled||(g_esp&3)||!span(g_esp-128,160)||MEM8(0x51c178))return 0;
 switch(entry){
 case 0x820c1:set_error();break;
 case 0x82528:allocate_route();break;
 case 0x825c8:{unsigned physical=MEM8(g_esp+11)&128;PUSH32(g_esp,MEM32(g_esp+4));PUSH32(g_esp,physical?0x825d8:0x825df);if(physical){RECOMP_ITAIL(MEM32(0x44b35c));}else normal_release();g_recomp_zf=!physical;g_esp+=12;break;}
 case 0x84c82:normal_allocate();break;
 case 0x84cb3:normal_release();break;
 case 0x86c68:case 0x86cab:physical_allocate(entry);break;
 case 0x86cde:RECOMP_ITAIL(MEM32(0x44b35c));break;
 default:return 0;
 }
 return 1;
}
