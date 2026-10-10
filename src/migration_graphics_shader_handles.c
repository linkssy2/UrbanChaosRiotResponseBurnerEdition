#include "migration_graphics_shader_handles.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0009C660(void),sub_0009C740(void),sub_0009CFD0(void),sub_0041FCBA(void),sub_003EE8B0(void),sub_003E67F0(void);
#define HANDLE_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int handles_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_SHADER_HANDLES");flag=v&&!strcmp(v,"1");}return flag;}
static int handles_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void shader_crc_table(void){
 SET_LO8(g_eax,MEM8(0x51a7f0));if(!(g_eax&255)){
  MEM8(0x51a7f0)=1;g_edx=0;
  do{g_ecx=g_edx<<24;for(unsigned pair=0;pair<4;pair++){g_eax=(g_ecx<<1)^((g_ecx&0x80000000u)?0x4c11db7:0);g_ecx=(g_eax&0x80000000u)?0x4c11db7:0;g_eax+=g_eax;g_ecx^=g_eax;}MEM32(0x51a3e0+g_edx*4)=g_ecx;g_edx++;}while(g_edx<256);
 }g_esp+=4;
}
static void shader_hash_bytes(void){do{g_ebx=MEM8(g_esi);g_edx=g_eax>>24;g_eax<<=8;g_edx=(g_edx^g_ebx)&255;g_eax^=MEM32(0x51a3e0+g_edx*4);g_esi++;g_ecx--;}while(g_ecx);}
static void shader_hash(void){PUSH32(g_esp,g_esi);g_esi=g_eax;HANDLE_CALL(sub_0009C660,0x9c748);g_eax=0;if(g_edi){g_ecx=g_edi;PUSH32(g_esp,g_ebx);shader_hash_bytes();POP32(g_esp,g_ebx);}POP32(g_esp,g_esi);g_esp+=4;}
static void shader_device(uint32_t first,uint32_t second){g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp,target=MEM32(g_edx+0xa4);PUSH32(g_esp,first);RECOMP_ICALL_SAFE(target,saved);}g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp,target=MEM32(g_edx+28);PUSH32(g_esp,second);RECOMP_ICALL_SAFE(target,saved);}}
static void pixel_handle(void){
 g_esp-=16;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+28);g_esi=g_edi;HANDLE_CALL(sub_0009C660,0x9d0a0);g_eax=0;g_ecx=240;PUSH32(g_esp,g_ebx);shader_hash_bytes();MEM32(g_esp+12)=g_eax;g_eax=g_esp+12;PUSH32(g_esp,g_eax);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_ecx=0x51a880;MEM32(g_esp+24)=0;HANDLE_CALL(sub_0009CFD0,0x9d0e7);SET_LO8(g_ecx,MEM8(g_eax+4));g_esi=MEM32(g_eax);POP32(g_esp,g_ebx);
 if(g_ecx&255){shader_device(0x9d0ff,0x9d106);g_eax=g_esp+28;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);HANDLE_CALL(sub_003EE8B0,0x9d111);g_ecx=MEM32(g_esp+28);MEM32(g_esi+16)=g_ecx;g_eax=MEM32(g_esp+28);}else g_eax=MEM32(g_esi+16);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void vertex_handle(void){
 g_esp-=20;PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+32);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);HANDLE_CALL(sub_0041FCBA,0x9d13f);g_edi=(g_eax<<4)+4;g_eax=g_esi;HANDLE_CALL(sub_0009C740,0x9d14e);MEM32(g_esp+12)=g_eax;g_eax=g_esp+12;PUSH32(g_esp,g_eax);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_ecx=0x51a7f4;MEM32(g_esp+24)=0;HANDLE_CALL(sub_0009CFD0,0x9d16e);SET_LO8(g_ecx,MEM8(g_eax+4));g_edi=MEM32(g_eax);
 if(g_ecx&255){shader_device(0x9d185,0x9d18c);g_ecx=MEM32(g_esp+32);PUSH32(g_esp,0);g_eax=g_esp+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ecx);HANDLE_CALL(sub_003E67F0,0x9d19e);g_edx=MEM32(g_esp+8);MEM32(g_edi+16)=g_edx;g_eax=MEM32(g_esp+8);}else g_eax=MEM32(g_edi+16);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_graphics_shader_handles(uint32_t entry){
 if(!handles_enabled()||(entry!=0x9c660&&entry!=0x9c740&&entry!=0x9d090&&entry!=0x9d130&&entry!=0x41fcba))return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!handles_span(s-128,144)||!handles_span(0x51c178,1)||MEM8(0x51c178)||!handles_span(0x51a3e0,1040))return 0;
 if(entry==0x9c740&&g_edi&&!handles_span(g_eax,g_edi))return 0;
 if(entry==0x9d090&&!handles_span(MEM32(s+4),240))return 0;
 if(entry==0x41fcba){if(!handles_span(MEM32(s+4),4))return 0;g_eax=MEM32(g_esp+4);g_eax=MEM16(g_eax+2);g_esp+=8;return 1;}
 if(entry==0x9c660)shader_crc_table();else if(entry==0x9c740)shader_hash();else if(entry==0x9d090)pixel_handle();else vertex_handle();return 1;
}
