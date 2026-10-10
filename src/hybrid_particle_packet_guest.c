#include "hybrid_particle_packet_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003EE5D0(void),sub_003E9820(void),sub_003E96D0(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_PACKET");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Original device reservations. Flush, growth, synchronization and GPU execution
 * remain original services; a reservation does not commit its returned cursor. */
int uc_hybrid_particle_packet(unsigned op){
 if(!enabled())return 0;
 uint32_t s=g_esp;
 if(op>3||(s&3)||s<0x11000||!span(s-4096,4108)||!span(0x3f69d0,4)||!span(0x3f4238,4)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(op!=2&&!span(MEM32(0x3f4238),op==0?0x798:8))return 0;
 if(op==0){
  uint32_t frame=g_seh_ebp;
  g_ecx=MEM32(0x3f4238);PUSH32(g_esp,0);PUSH32(g_esp,0x3e935d);sub_003EE5D0();
  g_eax=MEM32(g_esp+4);g_eax++;MEM32(g_esp+4)=g_eax;g_seh_ebp=frame;sub_003E9820();return 1;
 }
 if(op==1){g_eax=MEM32(g_esp+4);g_ecx=MEM32(0x3f4238);MEM32(g_ecx)=g_eax;g_esp+=8;return 1;}
 if(op==2){
  g_eax=MEM32(0x3f69d0);PUSH32(g_esp,g_eax);g_eax>>=1;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x3e981e);sub_003E96D0();g_recomp_zf=g_eax==0;g_esp+=4;return 1;
 }
 g_ecx=MEM32(0x3f4238);g_eax=MEM32(g_ecx);g_ecx=MEM32(g_ecx+4);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_edx=g_eax+g_esi*4;g_ecx+=0x200;
 if(g_edx>=g_ecx){
  g_eax=MEM32(0x3f69d0);g_edx=g_eax;g_eax>>=1;g_ecx=g_esi*4+0x204;
  if(g_ecx>g_eax)g_eax=g_ecx;if(g_ecx>g_edx)g_edx=g_ecx;
  PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x3e9860);sub_003E96D0();
 }
 POP32(g_esp,g_esi);g_esp+=8;return 1;
}
