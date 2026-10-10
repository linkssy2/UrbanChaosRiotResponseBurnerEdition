#include "migration_animation_contribution_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F0D70(void);
static int contribution_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int contribution_separate(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int contribution_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_CONTRIBUTION");flag=v&&!strcmp(v,"1");}return flag;}
static int contribution_pool(uint32_t pool,uint32_t s,unsigned below,uint32_t rotation,uint32_t translation,uint32_t record,unsigned record_bytes){
 if((s&3)||s<0x10000u+below||!contribution_span(s-below,below+20)||!contribution_span(pool,12)||!contribution_separate(pool,12,s-below,below+20)||!contribution_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t cursor=MEM32(pool+4),base=MEM32(pool+8);
 if(cursor<=base)return 1;
 if(cursor-base<48||cursor<0x10030||!contribution_span(cursor-48,48)||!contribution_separate(cursor-48,48,s-below,below+20))return 0;
 const uint32_t reads[][2]={{rotation,16},{translation,12},{pool,12},{0x51c178,1},{0x44c174,4}};
 for(unsigned i=0;i<5;i++)if(!contribution_span(reads[i][0],reads[i][1])||!contribution_separate(cursor-48,48,reads[i][0],reads[i][1])||!contribution_separate(s-below,below+20,reads[i][0],reads[i][1]))return 0;
 if(record_bytes&&!contribution_separate(cursor-48,48,record,record_bytes))return 0;
 return contribution_separate(pool,12,rotation,16)&&contribution_separate(pool,12,translation,12);
}
/* Own a 48-byte contribution from the existing descending pool. Payload and
 * link writes keep source order; the final eight reserved bytes remain stale. */
int uc_migration_animation_contribution_node(void){
 if(!contribution_enabled()||!contribution_span(g_esp,20)||!contribution_pool(g_ecx,g_esp,8,MEM32(g_esp+4),MEM32(g_esp+8),0,0))return 0;
 g_eax=MEM32(g_ecx+4);
 if(g_eax>MEM32(g_ecx+8)){
  g_edx=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_eax-=48;MEM32(g_ecx+4)=g_eax;PUSH32(g_esp,g_edi);
  g_edi=MEM32(g_edx);g_esi=g_eax;MEM32(g_esi)=g_edi;g_edi=MEM32(g_edx+4);MEM32(g_esi+4)=g_edi;g_edi=MEM32(g_edx+8);MEM32(g_esi+8)=g_edi;g_edx=MEM32(g_edx+12);MEM32(g_esi+12)=g_edx;
  g_esi=MEM32(g_esp+16);g_edi=MEM32(g_esi);g_edx=g_eax+16;MEM32(g_edx)=g_edi;g_edi=MEM32(g_esi+4);MEM32(g_edx+4)=g_edi;g_esi=MEM32(g_esi+8);MEMF(g_eax+28)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+24));MEM32(g_edx+8)=g_esi;
  POP32(g_esp,g_edi);MEMF(g_eax+32)=g_xmm0.f[0];MEM32(g_eax+36)=0;g_eax=MEM32(g_ecx+4);POP32(g_esp,g_esi);
 }else g_eax=0;
 g_recomp_zf=g_eax==0;g_esp+=20;return 1;
}
static unsigned contribution_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
/* Append the source-clamped weighted sample, preserving unordered/negative
 * weights, empty-pool behavior, high EAX bytes and the live record head. */
int uc_migration_animation_contribution_add(void){
 if(!contribution_enabled()||!contribution_span(g_esp,20)||!contribution_span(g_ecx,48)||!contribution_separate(g_ecx,48,g_esp-32,52)||!contribution_span(0x44c174,4)||!contribution_separate(g_ecx,48,0x44c174,4)||!contribution_pool(0x51c158,g_esp,32,MEM32(g_esp+4),MEM32(g_esp+8),g_ecx,48))return 0;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_esi+40));g_xmm0.f[0]*=MEMF(g_esp+16);g_xmm1=XMM_SCALAR(MEMF(g_esi+36));g_xmm1.f[0]-=MEMF(g_esi+32);
 unsigned flags=contribution_compare(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_esp+16)=g_xmm1.f[0];if(flags&0x41)MEMF(g_esp+16)=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(g_esp+16));flags=contribution_compare(g_xmm0.f[0],MEMF(0x44c174));SET_HI8(g_eax,flags);
 if(RECOMP_PARITY8(HI8(g_eax)&0x44)){
  g_eax=MEM32(g_esp+20);g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_esp+12);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+12);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_ecx=0x51c158;PUSH32(g_esp,0x1f1a9a);sub_001F0D70();
  if(g_eax){g_xmm0=XMM_SCALAR(MEMF(g_esi+32));g_ecx=MEM32(g_esi+44);g_xmm0.f[0]+=MEMF(g_esp+16);MEMF(g_esi+32)=g_xmm0.f[0];MEM32(g_eax+36)=g_ecx;MEM32(g_esi+44)=g_eax;}
 }
 POP32(g_esp,g_esi);RECOMP_TRACE_EXIT("sub_001F1A40",0x1f1a40);g_esp+=20;return 1;
}
