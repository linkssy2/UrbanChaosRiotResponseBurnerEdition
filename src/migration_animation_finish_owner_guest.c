#include "migration_animation_finish_owner_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F1A40(void);
extern void sub_001F0E50(void);
static int finish_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int finish_read(uint32_t p,unsigned n,uint32_t s){return finish_span(p,n)&&((uint64_t)p+n<=s-1024||(uint64_t)s+4<=p);}
static unsigned finish_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
/* Complete controller traversal, root contribution and finalization. Callback
 * lifetime/ABI is inherited after admission, with live links/counts reread. */
int uc_migration_animation_finish_owner(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_FINISH_OWNER");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;
 if(!enabled||(s&3)||s<0x10400||!finish_span(s-1024,1028)||!finish_read(p,44,s)||!finish_read(0x51c118,72,s)||!finish_read(0x51c178,1,s)||MEM8(0x51c178)||!finish_read(0x453190,8,s)||!finish_read(0x44bf80,24,s)||g_fp_top<0||g_fp_top>7)return 0;
 uint32_t n=MEM32(p+36),desc=MEM32(p+4);if(n>4096||!finish_read(desc,28,s))return 0;
 if(n&&(!finish_read(MEM32(p+32),n*48,s)||!finish_read(MEM32(p+12),n,s)))return 0;
 uint32_t root=MEM32(desc+24);if(root!=0xffffffffu&&root>=n)return 0;
 /* ponytail: bounded source list scan; larger lists retain original fallback. */
 for(uint32_t node=MEM32(p+40),seen=0;node;node=MEM32(node+4))if(seen++==2048||!finish_read(node,8,s)||!finish_read(MEM32(node),8,s))return 0;
 PUSH32(g_esp,g_ecx);g_eax=MEM32(0x51c134);g_xmm1=XMM_ZERO();PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;
 if(!(g_eax&1)){
  g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMD(0x453190);g_eax|=1;g_fp_stack[g_fp_top]=sqrt(g_fp_stack[g_fp_top]);MEM32(0x51c134)=g_eax;g_fp_stack[g_fp_top]=MEMF(0x44bf94)/g_fp_stack[g_fp_top];MEMF(g_esp+8)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
  g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_esp+8);g_fp_stack[g_fp_top]*=MEMF(0x44bf80);g_xmm0=XMM_SCALAR(MEMF(g_esp+8));g_xmm0.f[0]*=g_xmm1.f[0];MEMF(0x51c124)=g_xmm0.f[0];MEMF(0x51c128)=g_xmm0.f[0];MEMF(0x51c130)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;MEMF(0x51c12c)=g_xmm0.f[0];
 }
 if(!(g_eax&2)){g_eax|=2;MEM32(0x51c134)=g_eax;MEMF(0x51c118)=g_xmm1.f[0];MEMF(0x51c11c)=g_xmm1.f[0];MEMF(0x51c120)=g_xmm1.f[0];}
 g_edi=MEM32(g_esi+40);
 while(g_edi){g_eax=MEM32(g_edi);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_ecx=g_edi;uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x1f1c10);RECOMP_ICALL_SAFE(target,saved);g_edi=MEM32(g_edi+4);}
 g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx+24);
 if(g_eax!=0xffffffffu){
  g_edx=MEM32(g_esi+12);
  if(MEM8(g_eax+g_edx)){
   g_ecx=MEM32(g_esi+32);g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_eax=(g_eax+g_eax*2)<<4;g_xmm0=XMM_SCALAR(MEMF(g_eax+g_ecx+32));g_eax+=g_ecx;g_xmm0.f[0]+=g_xmm1.f[0];if(finish_compare(g_xmm1.f[0],g_xmm0.f[0])&0x41)g_xmm0=g_xmm1;
   MEMF(g_eax+36)=g_xmm0.f[0];g_xmm0.f[0]-=MEMF(g_eax+32);MEMF(g_eax+40)=g_xmm0.f[0];g_eax=MEM32(g_esi+4);g_eax=MEM32(g_eax+24);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3f800000);g_ecx=g_eax+g_eax*2;g_eax=MEM32(g_esi+32);PUSH32(g_esp,0x51c118);g_ecx<<=4;PUSH32(g_esp,0x51c124);g_ecx+=g_eax;PUSH32(g_esp,0x1f1c89);sub_001F1A40();
  }
 }
 g_eax=MEM32(g_esi+36);g_edi=0;
 if((int32_t)g_eax>0){
  PUSH32(g_esp,g_ebx);g_ebx=0;
  do{g_ecx=MEM32(g_esi+32)+g_ebx;PUSH32(g_esp,0x1f1c9f);sub_001F0E50();g_eax=MEM32(g_esi+36);++g_edi;g_ebx+=48;}while((int32_t)g_edi<(int32_t)g_eax);
  g_ecx=MEM32(0x51c160);POP32(g_esp,g_ebx);g_ecx+=0x18000;POP32(g_esp,g_edi);MEM32(0x51c15c)=g_ecx;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=g_ecx==0;
 }else{g_edx=MEM32(0x51c160)+0x18000;POP32(g_esp,g_edi);MEM32(0x51c15c)=g_edx;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=g_edx==0;}
 g_esp+=4;return 1;
}
