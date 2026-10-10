#include "migration_graphics_material_lifetime.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0022A710(void),sub_00090AD0(void),sub_002D7960(void),sub_002D57FB(void),sub_00090EA0(void);
#define LIFE_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int lifetime_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MATERIAL_LIFETIME");flag=v&&!strcmp(v,"1");}return flag;}
static int lifetime_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void material_base_destroy(void){
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x38c428);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=g_esi+4;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x45651c;MEM32(g_ecx)=0x456518;MEM32(g_esp+16)=0;LIFE_CALL(sub_0022A710,0x8e6c9);
 g_ecx=MEM32(g_esp+8);MEM32(g_esi)=0x4564fc;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void material_construct(void){
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x38c523);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi+4)=0x48d93c;g_eax=0;MEM32(g_esi)=0x4565b0;MEM32(g_esi+4)=0x4565ac;MEM32(g_esp+16)=g_eax;MEM32(g_esi+56)=g_eax;MEM32(g_esi+60)=g_eax;MEM32(g_esi+64)=g_eax;MEM32(g_esi+40)=g_eax;MEM32(g_esi+48)=g_eax;g_ecx=MEM32(g_esp+24);g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_esi;MEM8(g_esp+24)=1;LIFE_CALL(sub_00090AD0,0x90efc);
 g_ecx=MEM32(g_esp+8);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void material_create(void){
 g_eax=MEM32(0);PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x38c53b);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+32);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+32);
 static const uint32_t names[]={0x4565f4,0x4565ec,0x4565e4},returns[]={0x90f3b,0x90f4e,0x90f61};int matched=0;
 for(unsigned i=0;i<3;i++){PUSH32(g_esp,g_esi);PUSH32(g_esp,names[i]);PUSH32(g_esp,g_edi);LIFE_CALL(sub_002D7960,returns[i]);g_esp+=12;if(!g_eax){matched=1;break;}}
 if(matched){
  g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);{uint32_t saved=g_esp,target=MEM32(g_eax+60);PUSH32(g_esp,0x90f88);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_esp+28);g_edx=MEM32(g_eax);{uint32_t saved=g_esp;PUSH32(g_esp,2);PUSH32(g_esp,g_ecx);g_ecx=g_eax;uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x90f96);RECOMP_ICALL_SAFE(target,saved);}
  g_esi=g_eax;g_edi=0;
  if(g_esi){
   PUSH32(g_esp,80);LIFE_CALL(sub_002D57FB,0x90fa5);g_esp+=4;MEM32(g_esp+36)=g_eax;MEM32(g_esp+16)=g_edi;
   if(g_eax!=g_edi){g_edx=MEM32(g_esp+40);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);g_ecx=g_eax;LIFE_CALL(sub_00090EA0,0x90fc1);}else g_eax=0;
   MEM32(g_esp+16)=0xffffffff;if(g_eax!=g_edi)g_edi=g_eax+4;g_eax=MEM32(g_esi);{uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x90fdc);RECOMP_ICALL_SAFE(target,saved);}
   g_ecx=MEM32(g_esp+8);g_eax=g_edi;POP32(g_esp,g_edi);MEM32(0)=g_ecx;POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=24;return;
  }
 }
 g_eax=0;g_ecx=MEM32(g_esp+8);MEM32(0)=g_ecx;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=24;
}
int uc_migration_graphics_material_lifetime(uint32_t entry){
 if(!lifetime_enabled()||(entry!=0x8e690&&entry!=0x90ea0&&entry!=0x90f10))return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!lifetime_span(s-128,160)||!lifetime_span(0x51c178,1)||MEM8(0x51c178)||(entry!=0x90f10&&!lifetime_span(g_ecx,80)))return 0;
 if(entry==0x8e690)material_base_destroy();else if(entry==0x90ea0)material_construct();else material_create();return 1;
}
