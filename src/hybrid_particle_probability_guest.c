#include "hybrid_particle_probability_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int probability_enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_PROBABILITY");value=p&&!strcmp(p,"1");}return value;}
static int probability_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int probability_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_particle_probability(void){
 if(!probability_enabled())return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10004u||!probability_span(s-4,12)||
    (MEM32(s)!=0x1756bcu&&MEM32(s)!=0x1756ecu)||
    !probability_span(0x51dd10,1004)||!probability_span(0x504670,4)||
    !probability_span(0x51c178,1)||MEM8(0x51c178)||!probability_span(0x492d20,4)||
    MEM32(0x51e0f8)>=250||MEM32(0x504670)>=250||
    !probability_apart(s-4,12,0x51dd10,1004)||!probability_apart(s-4,12,0x504670,4)||
    !probability_apart(s-4,12,0x51c178,1)||!probability_apart(s-4,12,0x492d20,4)||
    (_mm_getcsr()&0x1f80u)!=0x1f80u)return 0;
 /* Complete 2D37D0 plus its callback-free 2D3670 child, restricted to the two
  * pinned LightningBeam update calls. Original seed/reset timing is untouched.
  * The worker-disabled title mode qualifies borrowed globals; it is not a lock.
  * No callback or decline occurs after this first source-ordered publication. */
 PUSH32(g_esp,0x2d37d5u);
 g_eax=MEM32(0x51e0f8)+1;MEM32(0x51e0f8)=g_eax;
 if(g_eax>=250){g_eax=0;MEM32(0x51e0f8)=g_eax;}
 g_ecx=MEM32(0x504670)+1;MEM32(0x504670)=g_ecx;
 if(g_ecx>=250){g_ecx=0;MEM32(0x504670)=g_ecx;}
 g_edx=MEM32(0x51dd10+4*g_eax);g_ecx=MEM32(0x51dd10+4*g_ecx);
 g_edx^=g_ecx;MEM32(0x51dd10+4*g_eax)=g_edx;g_eax=g_edx;
 g_recomp_zf=g_edx==0;g_esp+=4;
 g_xmm1=XMM_SCALAR(MEMF(g_esp+4));
 g_xmm0.f[0]=(float)(int32_t)g_eax;
 g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x492d20);
 {double a=g_xmm1.f[0],b=g_xmm0.f[0];
  unsigned flags=(a!=a||b!=b)?0x47u:a<b?0x03u:a>b?0x02u:0x42u;
  if(flags&0x41u){g_eax=0;g_recomp_zf=1;}
  else{g_eax=1;g_recomp_zf=!!(flags&0x40u);}
 }
 g_esp+=4;return 1;
}
