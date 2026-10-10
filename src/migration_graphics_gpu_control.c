#include "migration_graphics_gpu_control.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void);
static int control_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_GPU_CONTROL");flag=v&&!strcmp(v,"1");}return flag;}
static int control_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_graphics_gpu_control(uint32_t entry){
 if(!control_enabled()||entry!=0x3e7ea0)return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!control_span(s-64,72)||!control_span(0x51c178,1)||MEM8(0x51c178))return 0;uint32_t device=MEM32(0x3f4238);if(!control_span(device,0x2430)||!control_span(MEM32(device),48))return 0;
 PUSH32(esp,esi);esi=MEM32(0x3f4238);edx=MEM32(esi+0x2428)&0xfffffff7;MEM32(esi+0x2428)=edx;ecx=MEM32(0x3f4234);eax=edx;if(ecx){eax|=8;MEM32(esi+0x2428)=eax;}
 edx=MEM32(esi+0x242c)&0xe7efffff;MEM32(esi+0x242c)=edx;ecx=MEM32(0x3f422c);eax=edx;if(ecx){eax|=0x100000;MEM32(esi+0x242c)=eax;}eax=MEM32(0x3f4230);if(eax)MEM32(esi+0x242c)|=0x8000000;
 eax=MEM32(esi);if(eax>=MEM32(esi+4)){PUSH32(esp,0x3e7f13);sub_003E9810();}
 MEM32(eax)=0x40110;MEM32(eax+4)=0;ecx=MEM32(esi+0x2428);MEM32(eax+8)=0x81d8c;MEM32(eax+12)=0x400094;MEM32(eax+16)=ecx;MEM32(eax+20)=0x40100;edx=9;MEM32(eax+24)=edx;ecx=MEM32(esi+0x242c);MEM32(eax+28)=0x81d8c;MEM32(eax+32)=0x400b80;MEM32(eax+36)=ecx;MEM32(eax+40)=0x40100;MEM32(eax+44)=edx;eax+=48;MEM32(esi)=eax;POP32(esp,esi);g_recomp_zf=eax==0;esp+=4;return 1;
}
