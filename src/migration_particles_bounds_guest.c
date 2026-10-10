#include "migration_particles_bounds_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0004EEC0(void),sub_000A5FD0(void);
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_BOUNDS");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* Match the original unordered comparison without adding FE_INVALID for qNaN. */
static int above(float a,float b){double x=a,y=b;return x!=x||y!=y?0:x>y;}
/* MSVC's qualified original selects the lower endpoint's NaN payload. Keep
 * this scalar addition ordered across optimization of the surrounding loop. */
static __declspec(noinline) float center_sum(float low,float high){return low+high;}
static void expanded_box(void){
 g_eax=MEM32(g_ecx+32);g_xmm0=XMM_SCALAR(MEMF(g_eax+140));g_xmm7=XMM_SCALAR(MEMF(g_eax+136));g_esp-=64;
 if(above(g_xmm0.f[0],g_xmm7.f[0]))g_xmm7=g_xmm0;
 float low[3],high[3],center[3],extent[3];
 for(unsigned i=0;i<3;i++){low[i]=MEMF(g_ecx+108+4*i);high[i]=MEMF(g_ecx+120+4*i);}
 float half=MEMF(0x44c1f8);g_xmm7.f[0]*=half;MEMF(g_esp)=g_xmm7.f[0];
 for(unsigned i=0;i<3;i++){low[i]-=g_xmm7.f[0];high[i]+=g_xmm7.f[0];center[i]=center_sum(low[i],high[i])*half;extent[i]=(high[i]-low[i])*half;}
 MEMF(g_esp+16)=low[0];MEMF(g_esp+20)=low[1];
 for(unsigned i=0;i<3;i++){MEMF(g_esp+4+4*i)=extent[i];MEMF(g_esp+40+4*i)=center[i];MEMF(g_esp+52+4*i)=extent[i];}
 g_eax=MEM32(g_esp+68);
 for(unsigned i=0;i<3;i++){MEMF(g_eax+4*i)=low[i];MEMF(g_eax+12+4*i)=high[i];MEMF(g_eax+24+4*i)=center[i];MEMF(g_eax+36+4*i)=extent[i];}
 g_xmm0=XMM_SCALAR(extent[2]);g_xmm1=XMM_SCALAR(extent[0]);g_xmm2=XMM_SCALAR(extent[1]);g_xmm3=XMM_SCALAR(extent[2]);
 g_xmm4=XMM_SCALAR(high[0]);g_xmm5=XMM_SCALAR(high[1]);g_xmm6=XMM_SCALAR(high[2]);g_xmm7=XMM_SCALAR(low[2]);
 g_ecx=MEM32(g_esp+8);g_edx=MEM32(g_esp+12);g_esp+=64;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void copied_box(void){
 g_esp-=52;g_eax=MEM32(g_ecx+32);g_xmm0=XMM_SCALAR(MEMF(g_eax+140));g_xmm1=XMM_SCALAR(MEMF(g_eax+136));
 MEMF(g_esp)=above(g_xmm0.f[0],g_xmm1.f[0])?g_xmm0.f[0]:g_xmm1.f[0];g_ecx+=108;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ecx);g_ecx=g_esp+12;PUSH32(g_esp,0x23a0f3);sub_0004EEC0();
 g_xmm0=XMM_SCALAR(MEMF(g_esp+4));g_xmm0.f[0]*=MEMF(0x44c1f8);
 for(unsigned i=0;i<3;i++){g_xmm1=XMM_SCALAR(MEMF(g_esp+8+4*i));g_xmm1.f[0]-=g_xmm0.f[0];MEMF(g_esp+8+4*i)=g_xmm1.f[0];}
 g_xmm1=g_xmm0;g_xmm1.f[0]+=MEMF(g_esp+20);MEMF(g_esp+20)=g_xmm1.f[0];
 for(unsigned i=0;i<2;i++){g_xmm1=XMM_SCALAR(MEMF(g_esp+24+4*i));g_xmm1.f[0]+=g_xmm0.f[0];MEMF(g_esp+24+4*i)=g_xmm1.f[0];}
 g_ecx=g_esp+8;PUSH32(g_esp,0x23a169);sub_000A5FD0();
 g_esi=MEM32(g_esp+60);g_eax=g_esp+8;PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x23a179);sub_0004EEC0();
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=52;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_particles_bounds(unsigned entry){
 if((entry!=0x23a0c0&&entry!=0x23b250)||!enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10100||!span(s-256,272)||!span(p,156)||!span(0x51c178,1)||MEM8(0x51c178)||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
 uint32_t descriptor=MEM32(p+32),out=MEM32(s+4);
 if(!span(descriptor,144)||!span(out,48)||!span(0x44c1f8,4)||!apart(p,156,s-256,272)||!apart(descriptor,144,s-256,272)||!apart(out,48,s-256,272))return 0;
 if(entry==0x23b250)expanded_box();else copied_box();return 1;
}
