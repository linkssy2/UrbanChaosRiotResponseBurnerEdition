#include "migration_particles_radius_bounds_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D6EB0(void);
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_RADIUS_BOUNDS");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int above(float a,float b){double x=a,y=b;return x!=x||y!=y?0:x>y;}
static __declspec(noinline) float ordered_sum(float first,float second){return first+second;}
static void fp_push_value(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void fp_pop_value(void){g_fp_top=(g_fp_top+1)&7;}
#define TOP g_fp_stack[g_fp_top]
#define ST(n) g_fp_stack[(g_fp_top+(n))&7]
int uc_migration_particles_radius_bounds(unsigned entry){
 if(entry!=0x23cde0||!enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10200||!span(s-512,528)||!span(p,132)||!span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
 uint32_t d=MEM32(p+32),out=MEM32(s+4);
 if(!span(d,256)||!span(out,48)||!span(0x44c1f8,4)||!apart(p,132,s-512,528)||!apart(d,256,s-512,528)||!apart(out,48,s-512,528))return 0;
 g_esp-=72;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+32);g_xmm0=XMM_SCALAR(MEMF(g_eax+140));g_xmm1=XMM_SCALAR(MEMF(g_eax+136));
 MEMF(g_esp+8)=above(g_xmm0.f[0],g_xmm1.f[0])?g_xmm0.f[0]:g_xmm1.f[0];
 g_eax=MEM32(g_esi+32);g_xmm0=XMM_SCALAR(MEMF(g_eax+240));g_xmm1=XMM_SCALAR(MEMF(g_eax+232));g_eax+=224;
 if(!above(g_xmm0.f[0],g_xmm1.f[0]))g_xmm0=g_xmm1;g_xmm1=XMM_SCALAR(MEMF(g_eax+12));
 MEMF(g_esp+4)=above(g_xmm0.f[0],g_xmm1.f[0])?g_xmm0.f[0]:g_xmm1.f[0];
 /* Preserve all retired x87 slots while deriving the descriptor's vector length. */
 fp_push_value(MEMF(g_eax+4));fp_push_value(MEMF(g_eax));fp_push_value(TOP);TOP*=ST(1);fp_push_value(ST(2));TOP*=ST(3);ST(1)+=TOP;fp_pop_value();TOP=sqrt(TOP);
 MEMF(g_esp+12)=(float)TOP;fp_pop_value();fp_pop_value();fp_pop_value();
 fp_push_value(MEMF(g_esp+12));fp_push_value(MEMF(g_esp+4));fp_push_value(MEMF(g_eax+28));PUSH32(g_esp,0x23ce69);sub_002D6EB0();
 MEMF(g_esp+12)=(float)TOP;g_xmm0=XMM_SCALAR(MEMF(0x44c1f8));g_xmm1=XMM_SCALAR(MEMF(g_esp+8));
 float low[3],high[3],center[3],extent[3];for(unsigned i=0;i<3;i++){low[i]=MEMF(g_esi+108+4*i);high[i]=MEMF(g_esi+120+4*i);}
 g_xmm1.f[0]*=g_xmm0.f[0];ST(1)*=TOP;fp_pop_value();MEMF(g_esp+4)=(float)TOP;fp_pop_value();g_xmm1.f[0]+=MEMF(g_esp+4);
 float radius=g_xmm1.f[0],half=g_xmm0.f[0];MEMF(g_esp+12)=radius;
 /* Preserve the qualified original's axis-specific NaN payload selection. */
 for(unsigned i=0;i<3;i++){low[i]-=radius;high[i]+=radius;center[i]=(i==2?ordered_sum(high[i],low[i]):ordered_sum(low[i],high[i]))*half;extent[i]=(high[i]-low[i])*half;}
 MEMF(g_esp+28)=low[0];MEMF(g_esp+32)=low[1];
 for(unsigned i=0;i<3;i++){MEMF(g_esp+16+4*i)=extent[i];MEMF(g_esp+52+4*i)=center[i];MEMF(g_esp+64+4*i)=extent[i];}
 g_eax=MEM32(g_esp+80);for(unsigned i=0;i<3;i++){MEMF(g_eax+4*i)=low[i];MEMF(g_eax+12+4*i)=high[i];MEMF(g_eax+24+4*i)=center[i];MEMF(g_eax+36+4*i)=extent[i];}
 g_xmm0=XMM_SCALAR(extent[2]);g_xmm1=XMM_SCALAR(extent[0]);g_xmm2=XMM_SCALAR(extent[1]);g_xmm3=XMM_SCALAR(extent[2]);g_xmm4=XMM_SCALAR(low[2]);g_xmm5=XMM_SCALAR(high[0]);g_xmm6=XMM_SCALAR(high[1]);g_xmm7=XMM_SCALAR(high[2]);
 g_ecx=MEM32(g_esp+20);g_edx=MEM32(g_esp+24);POP32(g_esp,g_esi);g_esp+=72;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
#undef TOP
#undef ST
