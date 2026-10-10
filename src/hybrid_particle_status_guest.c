#include "hybrid_particle_status_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00239110(void);
static int status_enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_STATUS");value=p&&!strcmp(p,"1");}return value;}
static int status_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int status_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_particle_status(uint32_t entry){
 if(!status_enabled())return 0;
 if(entry!=0x2390c0&&entry!=0x23bd40&&entry!=0x23d180&&entry!=0x23d600)return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10800u||!status_span(s-2048,2056)||!status_span(p,0xb4)||
    !status_span(0x51c178,1)||MEM8(0x51c178)||!status_apart(p,0xb4,s-2048,2056)||
    !status_apart(p,0xb4,0x51c178,1)||!status_apart(s-2048,2056,0x51c178,1))return 0;
 uint32_t table=MEM32(p);
 int ordinary=table==0x482fac,line=table==0x48309c;
 int other=table==0x482fe0||table==0x483010||table==0x48303c||table==0x483070;
 if((entry==0x2390c0&&!ordinary)||(entry==0x23bd40&&!ordinary&&!other)||
    (entry==0x23d180&&!other&&!line)||(entry==0x23d600&&!line))return 0;
 /* All rejection is above. Original completing callbacks retain lifetime and
  * may replace live registers, fields or saved words; none are restored here. */
 if(entry==0x23bd40||entry==0x23d600){
  uint32_t frame=g_seh_ebp;
  if(entry==0x23d600){g_xmm0=XMM_SCALAR(MEMF(g_esp+4));MEMF(g_ecx+0xb0)=g_xmm0.f[0];}
  g_ecx+=16;g_seh_ebp=frame;sub_00239110();return 1;
 }
 unsigned adjust=entry==0x2390c0?16:0;
 if(adjust)g_ecx+=16;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0xa0-adjust);
 int finished=0;
 if(!g_eax){
  g_ecx=MEM32(g_esi+0x1c-adjust);g_eax=MEM32(g_ecx);
  {uint32_t saved=g_esp,target=MEM32(g_eax+12);PUSH32(g_esp,adjust?0x2390e5:0x23d195);RECOMP_ICALL_SAFE(target,saved);}
  finished=LO8(g_eax)!=0;
  if(!finished){
   g_ecx=MEM32(g_esi+0x20-adjust);g_edx=MEM32(g_ecx);
   {uint32_t saved=g_esp,target=MEM32(g_edx+4);PUSH32(g_esp,adjust?0x2390f1:0x23d1a1);RECOMP_ICALL_SAFE(target,saved);}
   finished=LO8(g_eax)!=0;
  }
 }
 g_eax=finished?1:0;POP32(g_esp,g_esi);g_recomp_zf=!finished;g_esp+=4;return 1;
}
