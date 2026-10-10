#include "migration_animation_controller_pose_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F1F20(void);
extern void sub_001F3BA0(void);
static int pose_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static unsigned pose_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
/* The active-state callback runs before weight, rate and clip are read. Its
 * normal lifetime/ABI contract is inherited; committed work never replays. */
int uc_migration_animation_controller_pose(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_CONTROLLER_POSE");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;
 if(!enabled||(s&3)||s<0x10400u||!pose_span(s-1024,1048)||!pose_span(p,56)||
    !((uint64_t)p+56<=s-1024||(uint64_t)s+24<=p)||!pose_span(0x51c178,1)||MEM8(0x51c178)||!pose_span(0x44c174,4)||!pose_span(0x44bf94,4))return 0;
 uint32_t table=MEM32(p);if(!pose_span(table,44))return 0;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);uint32_t target=MEM32(g_eax+40);PUSH32(g_esp,0x1f22b8);RECOMP_ICALL_SAFE(target,saved);}
 if(LO8(g_eax)){
  g_xmm0=XMM_SCALAR(MEMF(g_esi+24));
  if(!(pose_compare(g_xmm0.f[0],MEMF(0x44c174))&0x41)){
   g_ecx=MEM32(g_esi+24);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x1f22da);sub_001F1F20();
   unsigned waiting=MEM8(g_esi+12)&32;g_xmm0=XMM_SCALAR(MEMF(g_esi+32));MEMF(g_esp+12)=g_xmm0.f[0];
   if(waiting){g_xmm0=XMM_SCALAR(MEMF(0x44bf94));if(!(pose_compare(g_xmm0.f[0],MEMF(g_esi+24))&0x41)){g_xmm0=XMM_ZERO();MEMF(g_esp+12)=g_xmm0.f[0];}}
   g_edx=MEM32(g_esi+52);g_eax=MEM32(g_esi+20);g_ecx=MEM32(g_esp+12);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+24);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+16);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+48);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x1f2320);sub_001F3BA0();POP32(g_esp,g_edi);
  }
 }
 POP32(g_esp,g_esi);g_esp+=8;return 1;
}
