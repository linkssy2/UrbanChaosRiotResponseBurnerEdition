#include "migration_animation_tree_release_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D5CCD(void);
static int tree_release_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int tree_release_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* Right subtrees finish before each node is freed; left links are fetched
 * after recursion and before the free callback, as in the original owner. */
static void tree_release_run(void){
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);SET_LO8(g_eax,MEM8(g_edi+17));g_ebx=g_ecx;g_esi=g_edi;
 while(!LO8(g_eax)){
  g_eax=MEM32(g_esi+8);PUSH32(g_esp,g_eax);g_ecx=g_ebx;PUSH32(g_esp,0x1f410d);tree_release_run();
  g_esi=MEM32(g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x1f4115);sub_002D5CCD();SET_LO8(g_eax,MEM8(g_esi+17));g_esp+=4;g_edi=g_esi;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=!LO8(g_eax);g_esp+=8;
}
int uc_migration_animation_tree_release(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_TREE_RELEASE");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp;if(!enabled||entry!=0x1f40f0||(s&3)||!tree_release_span(s,8)||!tree_release_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t pending[1025],seen[1024];unsigned count=0,used=1;pending[0]=MEM32(s+4);
 while(used){uint32_t node=pending[--used];if(!tree_release_span(node,18))return 0;if(MEM8(node+17))continue;if(count==1024)return 0;
  /* ponytail: bounded quadratic duplicate check; use a set if the tree cap grows. */
  for(unsigned i=0;i<count;i++)if(seen[i]==node)return 0;seen[count++]=node;if(used+2>1025)return 0;pending[used++]=MEM32(node);pending[used++]=MEM32(node+8);
 }
 unsigned reserve=1024+(count+1)*20;if(s<0x10000u+reserve||!tree_release_span(s-reserve,reserve+8))return 0;
 if(!tree_release_apart(MEM32(s+4),18,s-reserve,reserve+8))return 0;
 for(unsigned i=0;i<count;i++){uint32_t node=seen[i];if(!tree_release_apart(node,18,s-reserve,reserve+8)||!tree_release_apart(MEM32(node),18,s-reserve,reserve+8)||!tree_release_apart(MEM32(node+8),18,s-reserve,reserve+8))return 0;}
 tree_release_run();return 1;
}
