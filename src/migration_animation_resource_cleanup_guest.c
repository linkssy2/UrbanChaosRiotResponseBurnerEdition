#include "migration_animation_resource_cleanup_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F40F0(void),sub_001F46E0(void),sub_002D5CCD(void);
static int cleanup_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int cleanup_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_migration_animation_resource_cleanup(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_RESOURCE_CLEANUP");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,o=g_ecx;unsigned args=entry==0x1f46e0?16:4;
 if(!enabled||(entry!=0x1f46e0&&entry!=0x1f48f0)||(s&3)||!cleanup_span(s,args)||!cleanup_span(o,12)||!cleanup_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t head=MEM32(o+4);if(!cleanup_span(head,20)||!MEM8(head+17))return 0;
 /* Only whole-tree erasure is admitted here; partial erase remains original. */
 uint32_t output=entry==0x1f46e0?MEM32(s+4):s-4;
 if(!cleanup_span(output,4)||(entry==0x1f46e0&&(MEM32(s+8)!=MEM32(head)||MEM32(s+12)!=head)))return 0;
 uint32_t pending[1025],seen[1024];unsigned used=1,count=0;pending[0]=MEM32(head+4);
 while(used){uint32_t node=pending[--used];if(!cleanup_span(node,20))return 0;if(MEM8(node+17))continue;if(count==1024)return 0;
  /* ponytail: bounded quadratic duplicate check; use a set if the node cap grows. */
  for(unsigned i=0;i<count;i++)if(seen[i]==node)return 0;seen[count++]=node;if(used+2>1025)return 0;pending[used++]=MEM32(node);pending[used++]=MEM32(node+8);
 }
 unsigned reserve=2048+(count+1)*20;if(s<0x10000u+reserve||!cleanup_span(s-reserve,reserve+args)||!cleanup_apart(o,12,s-reserve,reserve+args)||!cleanup_apart(head,20,s-reserve,reserve+args))return 0;
 for(unsigned i=0;i<count;i++){uint32_t n=seen[i];if(!cleanup_apart(n,20,s-reserve,reserve+args)||!cleanup_apart(MEM32(n),20,s-reserve,reserve+args)||!cleanup_apart(MEM32(n+8),20,s-reserve,reserve+args))return 0;}
 if(entry==0x1f46e0){
  PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+16);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+16);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+4);g_eax=MEM32(g_eax+4);PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x1f4703);sub_001F40F0();
  g_eax=MEM32(g_edi+4);MEM32(g_eax+4)=g_eax;g_eax=MEM32(g_edi+4);MEM32(g_edi+8)=0;MEM32(g_eax)=g_eax;g_eax=MEM32(g_edi+4);MEM32(g_eax+8)=g_eax;g_ecx=MEM32(g_edi+4);g_edx=MEM32(g_ecx);g_eax=MEM32(g_esp+16);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(g_eax)=g_edx;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=16;
 }else{
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_eax=g_esp+12;PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x1f4907);sub_001F46E0();
  g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f4910);sub_002D5CCD();g_esp+=4;g_eax=0;MEM32(g_esi+4)=0;MEM32(g_esi+8)=0;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=4;
 }
 return 1;
}
