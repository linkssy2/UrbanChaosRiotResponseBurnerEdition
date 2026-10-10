#include "migration_graphics_gpu_status.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int gpu_status_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_GPU_STATUS");flag=v&&!strcmp(v,"1");}return flag;}
static int gpu_status_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_graphics_gpu_status(uint32_t entry){
 if(!gpu_status_enabled()||entry!=0x3e3a80)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!gpu_status_span(s-16,24)||!gpu_status_span(0x51c178,1)||MEM8(0x51c178)||!gpu_status_span(0x3f4238,4))return 0;
 uint32_t d=MEM32(0x3f4238);if(!gpu_status_span(d,0x938))return 0;uint32_t aperture=MEM32(d+0x934);
 if(aperture!=0xfd000000u&&!gpu_status_span(aperture,0x400704))return 0;
 g_eax=aperture;g_ecx=MEM32(g_eax+0x3240);
 if(g_ecx!=MEM32(g_eax+0x3244)){g_eax=1;g_recomp_zf=0;}
 else{g_eax=MEM32(g_eax+0x400700);g_ecx=g_eax!=0;g_recomp_zf=g_eax==0;g_eax=g_ecx;}
 g_esp+=4;return 1;
}
