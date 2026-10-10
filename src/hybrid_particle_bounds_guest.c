#include "hybrid_particle_bounds_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int bounds_enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_BOUNDS");value=p&&!strcmp(p,"1");}return value;}
static int bounds_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int bounds_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static unsigned bounds_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
int uc_hybrid_particle_bounds(void){
 if(!bounds_enabled())return 0;
 uint32_t s=g_esp,o=g_ecx;
 if((s&3)||s<0x10010u||!bounds_span(s-16,28)||MEM32(s)!=0x2394f6||
    !bounds_span(0x44c1f8,4)||!bounds_span(0x51c178,1)||MEM8(0x51c178)||
    (_mm_getcsr()&0x1f80u)!=0x1f80u||!bounds_span(o,48)||
    !bounds_apart(o,48,s-16,28)||!bounds_apart(o,48,0x51c178,1)||
    !bounds_apart(s-16,28,0x51c178,1))return 0;
 uint32_t a=MEM32(s+4),b=MEM32(s+8);
 if(!bounds_span(a,12)||!bounds_span(b,12)||!bounds_apart(a,12,s-16,28)||!bounds_apart(b,12,s-16,28))return 0;
 /* No rejection after publication. Borrowed vectors may overlap each other
  * or the result; preserve each source reload and store in original order. */
 g_eax=g_ecx;g_ecx=MEM32(g_esp+8);g_edx=MEM32(g_ecx);MEM32(g_eax)=g_edx;
 g_edx=MEM32(g_esp+4);g_esp-=12;PUSH32(g_esp,g_esi);g_esi=MEM32(g_edx);MEM32(g_eax+12)=g_esi;
 for(unsigned axis=0;axis<3;axis++){
  unsigned off=axis*4;
  if(axis){g_esi=MEM32(g_ecx+off);MEM32(g_eax+off)=g_esi;g_esi=MEM32(g_edx+off);MEM32(g_eax+12+off)=g_esi;}
  g_xmm0=XMM_SCALAR(MEMF(g_ecx+off));unsigned flags=bounds_compare(g_xmm0.f[0],MEMF(g_edx+off));
  if(axis==2)POP32(g_esp,g_esi);
  if(!(flags&1u)){
   if(axis==2){g_edx=MEM32(g_edx+8);MEM32(g_eax+8)=g_edx;g_ecx=MEM32(g_ecx+8);MEM32(g_eax+20)=g_ecx;}
   else{g_esi=MEM32(g_edx+off);MEM32(g_eax+off)=g_esi;g_esi=MEM32(g_ecx+off);MEM32(g_eax+12+off)=g_esi;}
  }
 }
 g_xmm0=XMM_SCALAR(MEMF(g_eax+12));g_xmm3=XMM_SCALAR(MEMF(0x44c1f8));g_xmm1=XMM_SCALAR(MEMF(g_eax+16));
 g_xmm0.f[0]+=MEMF(g_eax);g_xmm2=XMM_SCALAR(MEMF(g_eax+20));g_xmm1.f[0]+=MEMF(g_eax+4);g_xmm2.f[0]+=MEMF(g_eax+8);
 g_xmm0.f[0]*=g_xmm3.f[0];MEMF(g_esp)=g_xmm0.f[0];g_ecx=MEM32(g_esp);g_xmm1.f[0]*=g_xmm3.f[0];MEMF(g_esp+4)=g_xmm1.f[0];
 g_edx=g_eax+24;MEM32(g_edx)=g_ecx;g_ecx=MEM32(g_esp+4);MEM32(g_edx+4)=g_ecx;g_xmm2.f[0]*=g_xmm3.f[0];MEMF(g_esp+8)=g_xmm2.f[0];g_ecx=MEM32(g_esp+8);MEM32(g_edx+8)=g_ecx;
 g_xmm0=XMM_SCALAR(MEMF(g_eax+12));g_xmm1=XMM_SCALAR(MEMF(g_eax+16));g_xmm0.f[0]-=MEMF(g_eax);g_xmm2=XMM_SCALAR(MEMF(g_eax+20));
 g_xmm1.f[0]-=MEMF(g_eax+4);g_xmm2.f[0]-=MEMF(g_eax+8);g_xmm0.f[0]*=g_xmm3.f[0];MEMF(g_esp)=g_xmm0.f[0];g_ecx=MEM32(g_esp);
 g_edx=g_eax+36;MEM32(g_edx)=g_ecx;g_xmm1.f[0]*=g_xmm3.f[0];MEMF(g_esp+4)=g_xmm1.f[0];g_ecx=MEM32(g_esp+4);g_xmm2.f[0]*=g_xmm3.f[0];
 MEM32(g_edx+4)=g_ecx;MEMF(g_esp+8)=g_xmm2.f[0];g_ecx=MEM32(g_esp+8);MEM32(g_edx+8)=g_ecx;
 g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
