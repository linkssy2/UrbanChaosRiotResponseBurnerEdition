#include "migration_animation_weight_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int weight_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int weight_separate(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static unsigned weight_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
/* Complete contribution-limit/scale preparation. Keep source-ordered scalar
 * operations, including unordered comparisons and the final SIMD register state. */
int uc_migration_animation_weight(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_WEIGHT");enabled=v&&!strcmp(v,"1");}
 uint32_t p=g_ecx,s=g_esp;
 if(!enabled||(s&3)||s<0x10004u||!weight_span(s-4,12)||!weight_span(p,40)||!weight_separate(p,40,s-4,12)||!weight_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t count=MEM32(p+36);
 if((int32_t)count>0){
  /* ponytail: bounded guest projection; raise the cap with larger-asset evidence. */
  uint32_t records=MEM32(p+32);
  if(count>4096||!weight_span(records,count*48)||!weight_span(0x44bf94,4)||
   !weight_separate(records,count*48,p,40)||!weight_separate(records,count*48,s-4,12)||!weight_separate(records,count*48,0x44bf94,4)||!weight_separate(s-4,12,0x44bf94,4))return 0;
 }
 g_eax=MEM32(g_ecx+36);g_edx=0;
 if((int32_t)g_eax>0){
  g_xmm1=XMM_SCALAR(MEMF(g_esp+4));g_xmm2=XMM_SCALAR(MEMF(0x44bf94));g_xmm3=XMM_ZERO();PUSH32(g_esp,g_esi);g_esi=0;
  do{
   g_eax=MEM32(g_ecx+32);g_xmm0=XMM_SCALAR(MEMF(g_eax+g_esi+32));g_eax+=g_esi;g_xmm0.f[0]+=g_xmm1.f[0];
   if(weight_compare(g_xmm2.f[0],g_xmm0.f[0])&0x41)g_xmm0=g_xmm2;
   unsigned flags=weight_compare(g_xmm1.f[0],g_xmm3.f[0]);MEMF(g_eax+36)=g_xmm0.f[0];
   if(!(flags&0x41)){g_xmm0.f[0]-=MEMF(g_eax+32);g_xmm0.f[0]/=g_xmm1.f[0];}else g_xmm0=g_xmm3;
   MEMF(g_eax+40)=g_xmm0.f[0];g_eax=MEM32(g_ecx+36);++g_edx;g_esi+=48;
  }while((int32_t)g_edx<(int32_t)g_eax);
  POP32(g_esp,g_esi);
 }
 g_esp+=8;return 1;
}
