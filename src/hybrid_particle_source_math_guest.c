#include "hybrid_particle_source_math_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int source_math_enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_SOURCE_MATH");value=p&&!strcmp(p,"1");}return value;}
static int source_math_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int source_math_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_particle_source_math(unsigned entry){
 if(!source_math_enabled())return 0;
 uint32_t s=g_esp;
 if((s&3)||!source_math_span(s,20)||!source_math_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x11000){
  if(MEM32(s)!=0x2193f3||MEM32(s+8)!=16||MEM32(s+12)!=4||MEM32(s+16)!=0x1f4b10||
     s<0x10014u||!source_math_span(s-20,40)||!source_math_apart(s-20,20,0x51c178,1))return 0;
  /* The pinned source calls four proven 1F4B10 constructors: EAX=ECX, RET.
   * Preserve the full retired frame while closing only those pure callbacks.
   * No pointer dereference occurs in the constructors, even on wraparound. */
  MEM32(s-4)=g_ebx;MEM32(s-8)=g_seh_ebp;MEM32(s-12)=g_esi;MEM32(s-16)=g_edi;
  MEM32(s-20)=0x11024;g_eax=g_ecx=MEM32(s+4)+48u;g_esp=s+20;return 1;
 }
 if(entry!=0xbe360||MEM32(s)!=0x2199a3||
    !source_math_span(MEM32(s+4),12)||!source_math_span(MEM32(s+8),12)||
    !source_math_span(MEM32(s+12),12)||!source_math_span(MEM32(s+16),4)||
    !source_math_apart(MEM32(s+4),12,0x51c178,1)||(_mm_getcsr()&0x1f80u)!=0x1f80u)return 0;
 /* Original scalar order and all XMM residuals, including zero high lanes.
  * Every input read completes before the first store; valid aliases stay valid.
  * The pinned worker-disabled title mode is qualification, not synchronization. */
 g_ecx=MEM32(g_esp+12);g_xmm0=XMM_SCALAR(MEMF(g_ecx));g_eax=MEM32(g_esp+8);
 g_xmm3=XMM_SCALAR(MEMF(g_ecx+4));g_xmm1=XMM_SCALAR(MEMF(g_ecx+8));
 g_xmm3.f[0]=g_xmm3.f[0]-MEMF(g_eax+4);g_xmm1.f[0]=g_xmm1.f[0]-MEMF(g_eax+8);
 g_ecx=MEM32(g_esp+16);g_xmm4=XMM_SCALAR(MEMF(g_ecx));g_xmm0.f[0]=g_xmm0.f[0]-MEMF(g_eax);
 g_xmm2=g_xmm4;g_xmm2.f[0]=g_xmm2.f[0]*g_xmm3.f[0];g_xmm3=XMM_SCALAR(MEMF(g_eax+8));
 g_xmm2.f[0]=g_xmm2.f[0]+MEMF(g_eax+4);g_xmm1.f[0]=g_xmm1.f[0]*g_xmm4.f[0];
 g_xmm3.f[0]=g_xmm3.f[0]+g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_eax));g_eax=MEM32(g_esp+4);
 g_xmm0.f[0]=g_xmm0.f[0]*g_xmm4.f[0];g_xmm1.f[0]=g_xmm1.f[0]+g_xmm0.f[0];
 MEMF(g_eax)=g_xmm1.f[0];MEMF(g_eax+4)=g_xmm2.f[0];MEMF(g_eax+8)=g_xmm3.f[0];g_esp+=4;return 1;
}
