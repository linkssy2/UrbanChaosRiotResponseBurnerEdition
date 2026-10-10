#include "migration_graphics_model_accessor.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int accessor_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MODEL_ACCESSOR");flag=v&&!strcmp(v,"1");}return flag;}
static int accessor_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_graphics_model_accessor(uint32_t entry){
 if(entry!=0xa24a0||!accessor_enabled())return 0;
 uint32_t s=esp;if((s&3)||s<0x10100||!accessor_span(s-32,40)||!accessor_span(0x51c178,1)||MEM8(0x51c178)||!accessor_span(ecx,4)||!accessor_span(MEM32(ecx),20))return 0;
 edx=MEM32(esp+4);eax=MEM32(ecx);esp-=8;uint32_t saved=esp;PUSH32(esp,edx);edx=esp+4;PUSH32(esp,edx);uint32_t target=MEM32(eax+16);PUSH32(esp,0xa24b2);RECOMP_ICALL_SAFE(target,saved);
 eax=MEM32(eax);esp+=8;g_recomp_zf=esp==0;esp+=8;return 1;
}
