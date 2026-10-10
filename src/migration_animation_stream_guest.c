#include "migration_animation_stream_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int stream_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int stream_read(unsigned width,unsigned array,uint32_t return_pc){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_STREAM");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp;
 if(!enabled||(s&3)||s<0x10020||!stream_span(s-32,44)||!stream_span(g_ecx,4)||!stream_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t holder=MEM32(g_ecx);if(!stream_span(holder,4))return 0;uint32_t object=MEM32(holder);if(!stream_span(object,4)||!stream_span(MEM32(object),8))return 0;
 uint32_t bytes=array?MEM32(s+8)*width:width,dest=MEM32(s+4);
 /* ponytail: bounded normal asset reads; larger requests retain fallback. */
 if(bytes>0x1000000||(bytes&&(!stream_span(dest,bytes)||!((uint64_t)dest+bytes<=s-32||(uint64_t)s+12<=dest))))return 0;
 g_eax=MEM32(g_ecx);g_ecx=MEM32(g_eax);g_eax=MEM32(g_esp+4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;
 if(array){PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);if(width==4)g_esi<<=2;PUSH32(g_esp,g_esi);}else PUSH32(g_esp,width);
 PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,return_pc);RECOMP_ICALL_SAFE(target,saved);
 if(array){g_eax=g_esi;POP32(g_esp,g_esi);if(width==4)g_recomp_zf=g_esi==0;g_esp+=12;}
 else{SET_LO8(g_eax,1);g_esp+=8;}
 return 1;
}
int uc_migration_animation_stream_words(void){return stream_read(4,1,0x1f0b37);}
int uc_migration_animation_stream_bytes(void){return stream_read(1,1,0x1f0b54);}
int uc_migration_animation_stream_u32(void){return stream_read(4,0,0x1f0b70);}
int uc_migration_animation_stream_u16(void){return stream_read(2,0,0x1f0b90);}
int uc_migration_animation_stream_u8(void){return stream_read(1,0,0x1f0bb0);}
