#include "migration_animation_control_state_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
extern void sub_001F2CD0(void);
static int control_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static unsigned control_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
int uc_migration_animation_control_state(uint32_t entry){
 unsigned bytes,args=4;
 switch(entry){case 0x1f0de0:bytes=44;args=8;break;case 0x1f0e30:bytes=48;break;case 0x1f1f90:bytes=16;break;case 0x1f1fb0:case 0x1f23b0:bytes=4;break;case 0x1f2330:case 0x1f2340:bytes=52;break;case 0x1f2350:bytes=52;args=16;break;default:return 0;}
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_CONTROL_STATE");enabled=v&&!strcmp(v,"1");}
 if(!enabled||(g_esp&3)||!control_span(g_esp,args)||!control_span(g_ecx,bytes)||!((uint64_t)g_ecx+bytes<=g_esp||(uint64_t)g_esp+args<=g_ecx)||!control_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x1f0de0&&(!control_span(0x44bf94,4)||(_mm_getcsr()&0x1f80)!=0x1f80))return 0;
 if(entry==0x1f2330&&(!control_span(MEM32(g_ecx+48),56)||g_fp_top<0||g_fp_top>7))return 0;
 switch(entry){
 case 0x1f0de0:{
  g_xmm1=XMM_SCALAR(MEMF(g_esp+4));g_xmm2=XMM_SCALAR(MEMF(0x44bf94));g_xmm0=g_xmm1;g_xmm0.f[0]+=MEMF(g_ecx+32);unsigned flags=control_compare(g_xmm2.f[0],g_xmm0.f[0]);if(flags&0x41)g_xmm0=g_xmm2;
  g_xmm2=XMM_ZERO();flags=control_compare(g_xmm1.f[0],g_xmm2.f[0]);MEMF(g_ecx+36)=g_xmm0.f[0];if(!(flags&0x41)){g_xmm0.f[0]-=MEMF(g_ecx+32);g_xmm0.f[0]/=g_xmm1.f[0];MEMF(g_ecx+40)=g_xmm0.f[0];}else MEMF(g_ecx+40)=g_xmm2.f[0];g_recomp_zf=!!(flags&0x40);g_esp+=8;break;
 }
 case 0x1f0e30:g_xmm0=XMM_ZERO();g_eax=g_ecx;MEMF(g_eax+32)=g_xmm0.f[0];MEM32(g_eax+44)=0;g_esp+=4;break;
 case 0x1f1f90:g_eax=g_ecx;g_ecx=0;MEM32(g_eax)=0x47fa80;for(unsigned i=1;i<4;i++)MEM32(g_eax+i*4)=g_ecx;g_recomp_zf=1;g_esp+=4;break;
 case 0x1f1fb0:MEM32(g_ecx)=0x47fa80;g_esp+=4;break;
 case 0x1f2330:{g_eax=MEM32(g_ecx+48);double value=MEMF(g_eax+52);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value;g_esp+=4;break;}
 case 0x1f2340:g_eax=MEM32(g_ecx+48)+76;g_recomp_zf=g_eax==0;g_esp+=4;break;
 case 0x1f2350:g_ecx=MEM32(g_ecx+48);sub_001F2CD0();break;
 case 0x1f23b0:g_eax=g_ecx;MEM32(g_eax)=0x47fabc;g_esp+=4;break;
 }
 return 1;
}
