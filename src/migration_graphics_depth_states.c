#include "migration_graphics_depth_states.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void),sub_003E7240(void),sub_003E6DF0(void),sub_003E7E40(void),sub_003E7C50(void);
static int depth_state_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_DEPTH_STATES");flag=v&&!strcmp(v,"1");}return flag;}
static int depth_state_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define DCALL(fn,pc) do{PUSH32(esp,pc);fn();}while(0)
int uc_migration_graphics_depth_states(uint32_t entry){
 if(!depth_state_enabled()||(entry!=0x3e82d0&&entry!=0x3e8360&&entry!=0x3e8400&&entry!=0x3e84a0&&entry!=0x3e8510))return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!depth_state_span(s-128,140)||!depth_state_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t device=MEM32(0x3f4238);if(!depth_state_span(device,0x1a0c)||!depth_state_span(MEM32(device),32))return 0;
 if(entry==0x3e82d0){
  PUSH32(esp,esi);esi=MEM32(0x3f4238);eax=MEM32(esi);if(eax>=MEM32(esi+4)){DCALL(sub_003E9810,0x3e82e3);}edx=MEM32(esp+8);if(edx){ecx=MEM32(esi+0x1a08);ecx=ecx?1:0;}else ecx=0;
  MEM32(eax)=0x4030c;MEM32(eax+4)=ecx;ecx=MEM32(0x3f40e8);MEM32(eax+8)=0x41d78;MEM32(eax+12)=ecx;eax+=16;MEM32(esi)=eax;eax=MEM32(0x3f41dc);MEM32(0x3f41dc)=edx;
  if(eax==2||edx==2){DCALL(sub_003E7240,0x3e8336);ecx=esi;DCALL(sub_003E6DF0,0x3e833d);eax=MEM32(esi);if(eax>=MEM32(esi+4)){DCALL(sub_003E9810,0x3e8349);}edx=eax;ecx=esi;DCALL(sub_003E7E40,0x3e8352);edx=eax;DCALL(sub_003E7C50,0x3e8359);MEM32(esi)=eax;}
  POP32(esp,esi);esp+=8;return 1;
 }
 int saved_edi=entry==0x3e8360||entry==0x3e8400;
 if(saved_edi){PUSH32(esp,esi);esi=MEM32(0x3f4238);PUSH32(esp,edi);edi=MEM32(esp+12);MEM32(entry==0x3e8360?0x3f41e0:0x3f41e4)=edi;}
 else{eax=MEM32(esp+4);PUSH32(esp,esi);esi=MEM32(0x3f4238);MEM32(entry==0x3e84a0?0x3f4224:0x3f4228)=eax;}
 eax=MEM32(esi);if(eax>=MEM32(esi+4)){PUSH32(esp,entry+(saved_edi?30:28));sub_003E9810();}
 edx=MEM32(0x3f4228);ecx=0;if(edx)ecx=2;edx=MEM32(0x3f4224);if(edx){edx=MEM32(0x3f41e0);if(!edx||MEM32(0x3f41e4)==0x1e00)ecx|=1;}
 MEM32(eax)=0x41d84;MEM32(eax+4)=ecx;
 if(entry==0x3e8360){if(edi){ecx=MEM32(esi+0x1a08);ecx=ecx?1:0;}else ecx=0;MEM32(eax+8)=0x4032c;MEM32(eax+12)=ecx;eax+=16;}
 else if(entry==0x3e8400){MEM32(eax+8)=0x40370;MEM32(eax+12)=edi;eax+=16;}
 else eax+=8;
 if(saved_edi)POP32(esp,edi);MEM32(esi)=eax;POP32(esp,esi);g_recomp_zf=eax==0;esp+=8;return 1;
}
#undef DCALL
