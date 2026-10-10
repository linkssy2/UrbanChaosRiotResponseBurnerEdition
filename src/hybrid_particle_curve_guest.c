#include "hybrid_particle_curve_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int particle_curve_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_CURVE");enabled=v&&!strcmp(v,"1");}return enabled;}
static int particle_curve_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Keep the source comparison projection, including unordered input and its
 * float-to-double conversions. The typed finite/disjoint curve API remains
 * unchanged; these full ABI entries also preserve source aliases and scratch. */
#define CURVE_COMPARE(a,b) do{double x=(a);double y=(b);flags=(x!=x||y!=y)?0x47u:x<y?0x03u:x>y?0x02u:0x42u;}while(0)
int uc_hybrid_particle_curve(unsigned operation){
 if(!particle_curve_enabled())return 0;
 static const unsigned bytes[]={0x58,0x58,0x58,0x14,0x1c};uint32_t start=g_esp;
 if(operation>4||(start&3)||start<0x10040u||!particle_curve_span(start-64,88)||
    !particle_curve_span(g_ecx,bytes[operation])||!particle_curve_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(operation==2){
  g_eax=g_ecx;g_ecx=MEM32(g_esp+4);
  for(unsigned i=0;i<0x50;i+=16){g_xmm0=XMM_MEM(g_ecx+i);XMM_STORE(g_eax+i,g_xmm0);}
  g_edx=MEM32(g_ecx+0x50);MEM32(g_eax+0x50)=g_edx;g_ecx=MEM32(g_ecx+0x54);MEM32(g_eax+0x54)=g_ecx;g_esp+=8;return 1;
 }
 if(operation==3){
  g_edx=MEM32(g_esp+4);g_eax=MEM32(g_edx+0x14);g_eax+=0xd0;PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax);MEM32(g_ecx)=g_esi;
  g_xmm0=XMM_SCALAR(MEMF(g_eax+4));g_xmm0.f[0]-=MEMF(g_eax);MEMF(g_ecx+4)=g_xmm0.f[0];g_esi=MEM32(g_eax+8);MEM32(g_ecx+8)=g_esi;
  g_xmm0=XMM_SCALAR(MEMF(g_eax+12));g_xmm0.f[0]-=MEMF(g_eax+8);MEMF(g_ecx+12)=g_xmm0.f[0];g_eax=MEM32(g_edx+0x14);
  g_xmm0=XMM_SCALAR(MEMF(g_eax+0x90));g_xmm0.f[0]*=MEMF(0x44d95c);MEMF(g_ecx+16)=g_xmm0.f[0];POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;return 1;
 }
 uint32_t flags=2;
 if(operation==4){
  g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_xmm1=XMM_ZERO();CURVE_COMPARE(g_xmm1.f[0],g_xmm0.f[0]);g_xmm3=XMM_SCALAR(MEMF(g_esp+20));
  g_eax=g_ecx;g_ecx=MEM32(g_esp+4);g_edx=MEM32(g_ecx);g_ecx=MEM32(g_esp+8);MEM32(g_eax)=g_edx;g_edx=MEM32(g_ecx);g_ecx=MEM32(g_esp+12);MEM32(g_eax+4)=g_edx;g_edx=MEM32(g_ecx);MEM32(g_eax+8)=g_edx;
  MEMF(g_eax+0x14)=g_xmm0.f[0];MEMF(g_eax+0x18)=g_xmm3.f[0];
  if(!(flags&1)){MEMF(g_eax+0x14)=g_xmm1.f[0];MEMF(g_eax+12)=g_xmm1.f[0];}
  else{g_xmm2=XMM_SCALAR(MEMF(g_eax+4));g_xmm2.f[0]-=MEMF(g_eax);g_xmm2.f[0]/=g_xmm0.f[0];MEMF(g_eax+12)=g_xmm2.f[0];}
  g_xmm2=XMM_SCALAR(MEMF(0x44bf94));g_xmm0=g_xmm2;g_xmm0.f[0]-=g_xmm3.f[0];CURVE_COMPARE(g_xmm1.f[0],g_xmm0.f[0]);
  if(!(flags&1)){MEMF(g_eax+0x18)=g_xmm2.f[0];MEMF(g_eax+16)=g_xmm1.f[0];}
  else{g_xmm1=XMM_SCALAR(MEMF(g_eax+8));g_xmm1.f[0]-=MEMF(g_eax+4);g_xmm1.f[0]/=g_xmm0.f[0];MEMF(g_eax+16)=g_xmm1.f[0];}
  g_recomp_zf=!!(flags&0x40);g_esp+=24;return 1;
 }
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;
 if(operation==0){
  g_esp=(g_esp&0xfffffff0u)-16;g_edx=MEM32(frame+12);g_xmm1=XMM_SCALAR(MEMF(frame+24));g_eax=g_ecx;g_ecx=MEM32(frame+8);
  g_xmm0=XMM_MEM(g_ecx);g_ecx=MEM32(frame+16);XMM_STORE(g_eax,g_xmm0);g_xmm0=XMM_MEM(g_edx);XMM_STORE(g_eax+16,g_xmm0);g_xmm0=XMM_MEM(g_ecx);XMM_STORE(g_eax+32,g_xmm0);
  g_xmm0=XMM_SCALAR(MEMF(frame+20));MEMF(g_eax+0x54)=g_xmm1.f[0];g_xmm1=XMM_ZERO();CURVE_COMPARE(g_xmm1.f[0],g_xmm0.f[0]);MEMF(g_eax+0x50)=g_xmm0.f[0];
  if(!(flags&1)){g_edx=g_esp+12;MEMF(g_esp+12)=g_xmm1.f[0];g_xmm0=XMM_SCALAR(MEMF(g_edx));g_xmm2=g_xmm0;MEMF(g_eax+0x50)=g_xmm1.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0);XMM_STORE(g_eax+0x30,g_xmm2);}
  else{g_xmm2=XMM_MEM(g_eax);g_xmm3=XMM_MEM(g_eax+16);g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm3=XMM_SUB(g_xmm3,g_xmm2);g_xmm3=XMM_DIV(g_xmm3,g_xmm0);XMM_STORE(g_eax+0x30,g_xmm3);}
  g_xmm2=XMM_SCALAR(MEMF(0x44bf94));g_xmm0=g_xmm2;g_xmm0.f[0]-=MEMF(g_eax+0x54);CURVE_COMPARE(g_xmm1.f[0],g_xmm0.f[0]);
  if(!(flags&1)){MEMF(g_esp+12)=g_xmm1.f[0];g_ecx=g_esp+12;g_xmm0=XMM_SCALAR(MEMF(g_ecx));g_xmm1=g_xmm0;MEMF(g_eax+0x54)=g_xmm2.f[0];g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm0,0);XMM_STORE(g_eax+0x40,g_xmm1);}
  else{g_xmm1=XMM_MEM(g_eax+16);g_xmm2=XMM_MEM(g_eax+32);g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm2=XMM_SUB(g_xmm2,g_xmm1);g_xmm2=XMM_DIV(g_xmm2,g_xmm0);XMM_STORE(g_eax+0x40,g_xmm2);}
  g_esp=frame;POP32(g_esp,frame);g_recomp_zf=!!(flags&0x40);g_esp+=24;return 1;
 }
 g_xmm1=XMM_ZERO();g_xmm0=XMM_SCALAR(MEMF(frame+12));g_esp&=0xfffffff0u;CURVE_COMPARE(g_xmm1.f[0],g_xmm0.f[0]);
 if(!(flags&1)){g_eax=MEM32(frame+8);g_xmm0=XMM_MEM(g_ecx);}
 else{
  CURVE_COMPARE(g_xmm0.f[0],MEMF(0x44bf94));
  if(!(flags&1)){g_eax=MEM32(frame+8);g_xmm0=XMM_MEM(g_ecx+32);}
  else{
   g_xmm1=XMM_SCALAR(MEMF(g_ecx+0x50));CURVE_COMPARE(g_xmm1.f[0],g_xmm0.f[0]);
   if(!(flags&0x41)){g_xmm1=XMM_MEM(g_ecx+0x30);g_eax=MEM32(frame+8);g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm1=XMM_MUL(g_xmm1,g_xmm0);g_xmm0=XMM_MEM(g_ecx);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);}
   else{
    CURVE_COMPARE(g_xmm0.f[0],MEMF(g_ecx+0x54));
    if(!(flags&0x41)){g_xmm0.f[0]-=MEMF(g_ecx+0x54);g_xmm1=XMM_MEM(g_ecx+0x40);g_eax=MEM32(frame+8);g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm1=XMM_MUL(g_xmm1,g_xmm0);g_xmm0=XMM_MEM(g_ecx+16);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);}
    else{g_eax=MEM32(frame+8);g_xmm0=XMM_MEM(g_ecx+16);}
   }
  }
 }
 XMM_STORE(g_eax,g_xmm0);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=!!(flags&0x40);g_esp+=12;return 1;
}
#undef CURVE_COMPARE
