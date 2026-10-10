#include "migration_animation_pose_conversion_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
extern void sub_002D5640(void);
static int pose_conversion_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int pose_conversion_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_migration_animation_pose_conversion(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_POSE_CONVERSION");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,out=g_ecx;if(!enabled||entry!=0x1f4c00||(s&3)||s<0x10100||!pose_conversion_span(s-256,268)||!pose_conversion_span(out,28)||!pose_conversion_span(0x51c178,1)||MEM8(0x51c178)||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
 uint32_t matrix=MEM32(s+4);if(!pose_conversion_span(matrix,64)||!pose_conversion_apart(out,28,s-256,268)||!pose_conversion_apart(matrix,64,s-256,268)||!pose_conversion_span(0x44bf94,4))return 0;
 /* The shared matrix-to-quaternion service is a retained dependency. Its
  * actual writes are consumed before the ordered translation stores. */
 g_esp-=28;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=matrix;g_esi=out;PUSH32(g_esp,g_edi);g_ecx=g_esp+24;PUSH32(g_esp,0x1f4c15);sub_002D5640();
 g_xmm2=XMM_SCALAR(MEMF(g_edi+48));g_xmm3=XMM_SCALAR(MEMF(g_edi+52));g_xmm1=XMM_SCALAR(MEMF(g_edi+56));g_ecx=MEM32(g_esp+20);g_edx=MEM32(g_esp+24);g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm0.f[0]/=MEMF(g_esp+44);
 g_eax=g_esi;MEM32(g_eax)=g_ecx;g_ecx=MEM32(g_esp+28);MEM32(g_eax+4)=g_edx;g_edx=MEM32(g_esp+32);MEM32(g_eax+8)=g_ecx;MEM32(g_eax+12)=g_edx;
 g_xmm4=g_xmm0;g_xmm4.f[0]*=g_xmm2.f[0];g_eax=g_esi+16;MEMF(g_esp+8)=g_xmm4.f[0];g_ecx=MEM32(g_esp+8);g_xmm2=g_xmm0;MEM32(g_eax)=g_ecx;g_xmm2.f[0]*=g_xmm3.f[0];MEMF(g_esp+12)=g_xmm2.f[0];g_edx=MEM32(g_esp+12);g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+16)=g_xmm1.f[0];g_ecx=MEM32(g_esp+16);MEM32(g_eax+4)=g_edx;MEM32(g_eax+8)=g_ecx;
 POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
