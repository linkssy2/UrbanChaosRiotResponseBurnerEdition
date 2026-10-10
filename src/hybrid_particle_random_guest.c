#include "hybrid_particle_random_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int random_enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_RANDOM");value=p&&!strcmp(p,"1");}return value;}
static int random_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int random_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int random_caller(unsigned entry,uint32_t pc){
 if(entry==0x2d3670)return pc==0x239a9b;
 if(entry==0x2d3760)switch(pc){case 0x23953d:case 0x239826:case 0x2398a6:case 0x2399c1:case 0x239a4c:return 1;default:return 0;}
 if(entry==0x2d3780)switch(pc){case 0x239550:case 0x239563:case 0x2395ca:case 0x239603:case 0x239616:case 0x239629:case 0x2396b4:case 0x2396d5:case 0x2396e8:case 0x2396fb:case 0x239928:case 0x219804:case 0x187912:return 1;default:return 0;}
 return 0;
}
/* Same recurrence as native/game_random, operating on the original noncontiguous
 * guest globals. There is no new stream, copied table, or seed/reset operation. */
static void random_next(void){
 g_eax=MEM32(0x51e0f8)+1;MEM32(0x51e0f8)=g_eax;
 if(g_eax>=250){g_eax=0;MEM32(0x51e0f8)=g_eax;}
 g_ecx=MEM32(0x504670)+1;MEM32(0x504670)=g_ecx;
 if(g_ecx>=250){g_ecx=0;MEM32(0x504670)=g_ecx;}
 g_edx=MEM32(0x51dd10+4*g_eax);g_ecx=MEM32(0x51dd10+4*g_ecx);
 g_edx^=g_ecx;MEM32(0x51dd10+4*g_eax)=g_edx;g_eax=g_edx;
 g_recomp_zf=g_edx==0;g_esp+=4;
}
int uc_hybrid_particle_random(unsigned entry){
 if(!random_enabled())return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10008u||!random_span(s-8,20)||!random_caller(entry,MEM32(s))||
    !random_span(0x51dd10,1004)||!random_span(0x504670,4)||
    !random_span(0x51c178,1)||MEM8(0x51c178)||!random_span(0x492d20,4)||
    MEM32(0x51e0f8)>=250||MEM32(0x504670)>=250||
    !random_apart(s-8,20,0x51dd10,1004)||!random_apart(s-8,20,0x504670,4)||
    !random_apart(s-8,20,0x51c178,1)||!random_apart(s-8,20,0x492d20,4))return 0;
 if(entry!=0x2d3670&&((unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80u)!=0x1f80u))return 0;
 /* No callback, reentry or decline after publication. The pinned worker-disabled
  * title configuration qualifies the borrowed globals; this is not a lock. */
 if(entry==0x2d3670){random_next();return 1;}
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,entry==0x2d3760?0x2d3766:0x2d3786);random_next();
 MEM32(g_esp)=g_eax;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=(double)SMEM32(g_esp);
 if(entry==0x2d3780){
  g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_esp+12);
  g_fp_stack[g_fp_top]-=MEMF(g_esp+8);
  g_fp_stack[(g_fp_top+1)&7]*=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
 }
 g_fp_stack[g_fp_top]*=MEMF(0x492d20);
 if(entry==0x2d3780)g_fp_stack[g_fp_top]+=MEMF(g_esp+8);
 POP32(g_esp,g_ecx);g_esp+=4;return 1;
}
