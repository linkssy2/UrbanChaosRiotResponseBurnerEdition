#include "migration_graphics_float_states.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003EAA50(void),sub_003E9810(void);
static int float_state_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_FLOAT_STATES");flag=v&&!strcmp(v,"1");}return flag;}
static int float_state_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define FTOP g_fp_stack[g_fp_top]
#define FCALL(fn,pc) do{PUSH32(esp,pc);fn();}while(0)
static void float_state_push(double value){g_fp_top=(g_fp_top+7)&7;FTOP=value;}
int uc_migration_graphics_float_states(uint32_t entry){
 if(!float_state_enabled()||entry!=0x3e75a0)return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!float_state_span(s-128,140)||!float_state_span(0x51c178,1)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7)return 0;
 uint32_t device=MEM32(0x3f4238);if(!float_state_span(device,0x968)||!float_state_span(MEM32(device),16))return 0;
 if(entry==0x3e75a0){PUSH32(esp,ebx);ebx=MEM32(esp+8);PUSH32(esp,esi);esi=MEM32(0x3f4238);MEM32(esp+12)=ebx;float_state_push(MEMF(esp+12));PUSH32(esp,edi);FTOP*=MEMF(esi+0x964);PUSH32(esp,ecx);FTOP*=MEMF(0x44eef8);FTOP+=MEMF(0x44c1f8);MEMF(esp)=(float)FTOP;g_fp_top=(g_fp_top+1)&7;FCALL(sub_003EAA50,0x3e75d0);edi=eax;if(edi>511)edi=511;eax=MEM32(esi);if(eax>=MEM32(esi+4)){FCALL(sub_003E9810,0x3e75eb);}MEM32(eax)=0x40380;MEM32(eax+4)=edi;eax+=8;POP32(esp,edi);MEM32(esi)=eax;POP32(esp,esi);MEM32(0x3f4214)=ebx;POP32(esp,ebx);g_recomp_zf=eax==0;esp+=8;return 1;}
 return 0;
}
#undef FCALL
#undef FTOP
