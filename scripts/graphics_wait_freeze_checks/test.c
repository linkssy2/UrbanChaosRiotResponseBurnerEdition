#include "recomp_types.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
enum{RAM=0xa00000,Q=0x610000,F=0x620000,B=0x680000,S=0x980000,R=0x630000};
static unsigned char ram[RAM];
ptrdiff_t g_xbox_mem_offset;size_t g_xbox_total_ram=RAM;
RECOMP_TLS uint32_t g_eax,g_ecx,g_edx,g_esp,g_ebx,g_esi,g_edi,g_ebp,g_seh_ebp;
RECOMP_TLS uint64_t g_mmx[8];RECOMP_TLS double g_fp_stack[8];
RECOMP_TLS int g_fp_top,g_fp_cmp,g_recomp_zf;RECOMP_TLS uint16_t g_fp_control_word;
RECOMP_TLS RecompXmm g_xmm0,g_xmm1,g_xmm2,g_xmm3,g_xmm4,g_xmm5,g_xmm6,g_xmm7;

static unsigned polls,irqs,flushes,progress,limit,target_value;
void sub_003E94A0(void){assert(0);}
void sub_003E9120(void){g_eax=F;g_esp+=4;}
void sub_003E92A0(void){progress++;g_eax=B+0x100;g_esp+=12;}
void sub_003E8FB0(void){g_eax=0x3000;g_esp+=8;}
void sub_003E9320(void){flushes++;g_esp+=4;}
void xbox_NotifyMemoryReadPoll(uint32_t address){assert(address==R);assert(++polls<=limit);if(polls==limit)MEM32(R)=target_value;}
static void irq(void){irqs++;g_eax=0;g_esp+=24;}
#undef RECOMP_ICALL_SAFE
#define RECOMP_ICALL_SAFE(address,saved) do{assert((address)==0x123456);irq();}while(0)
void recomp_icall_fail_log(uint32_t address){assert(0);}
#include "../../src/migration_graphics_wait.c"
int main(int argc,char**argv){
 int ordered=argc==1;_putenv_s("UCRR_NATIVE_GRAPHICS_WAIT","1");_putenv_s("UCRR_ORDERED_GPU",ordered?"1":"");g_xbox_mem_offset=(ptrdiff_t)ram;
 for(unsigned wrap=0;wrap<2;wrap++)for(unsigned done=0;done<2;done++)for(unsigned delay=1;delay<=7;delay+=6){
 memset(ram,0,sizeof ram);polls=irqs=flushes=progress=0;limit=delay;target_value=wrap?0xfffffffe:100;
 MEM32(0x3f4238)=Q;MEM32(Q+48)=R;MEM32(Q+44)=target_value+3;MEM32(R)=done?target_value:target_value-1;MEM32(Q)=B;MEM32(Q+68)=0x10000;MEM32(F)=target_value;MEM32(F+4)=B+0x4000;MEM32(0x44b268)=0x123456;
 g_esp=S;MEM32(S)=0xabcdef;MEM32(S+4)=target_value;MEM32(S+8)=0;g_esi=11;g_edi=22;g_ebx=33;g_seh_ebp=44;
 assert(uc_migration_graphics_wait(0x3e9560));assert(g_esp==S+12&&g_esi==11&&g_edi==22&&g_ebx==33);assert(MEM32(S)==0xabcdef);
 if(done){assert(!polls&&!irqs&&!progress);}else if(ordered){assert(polls==delay&&!irqs&&!flushes&&progress==1);assert(MEM32(R)==target_value);}else{assert(irqs==1&&!polls&&flushes==1&&progress==2);}
 }
 puts("PASS: completed/delayed fences, wraparound, guest ABI, legacy event path");return 0;
}

