#include "hybrid_particle_emitter_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D72D1(void);
extern void sub_002D8121(void);
extern void sub_002D6780(void);
static int emitter_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_EMITTER");flag=v&&!strcmp(v,"1");}return flag;}
/* Complete concrete emitter implementations on the existing title objects.
 * CRT ceil/modf/conversion are explicit retained primitive services. No
 * particle state, emitter clock, pool or random stream is duplicated. */
static int emitter_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static unsigned emitter_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
static void emitter_push(double value){g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value;}
static void emitter_pop(void){g_fp_top=(g_fp_top+1u)&7u;}
#define EMITTER_TOP g_fp_stack[g_fp_top]
int uc_hybrid_particle_emitter(uint32_t address){
 if(!emitter_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx,table=0;
 if((s&3)||s<0x10400u||!emitter_span(s-1024,1040)||!emitter_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7)return 0;
 int constructor=address==0x23ea50||address==0x23ea70||address==0x23eb60;
 if(constructor){
  if(!emitter_span(0x51cb8c,4))return 0;uint32_t pool=MEM32(0x51cb8c);
  if(!emitter_span(pool,4)||!emitter_span(MEM32(pool),12))return 0;
 }else{
  if(!emitter_span(p,20)||!((uint64_t)p+20<=s-1024||(uint64_t)s+16<=p))return 0;
  table=MEM32(p);
  if(table!=0x483118&&table!=0x48312c&&table!=0x483140)return 0;
  if((address==0x1a89c0||address==0x1a89e0||address==0x276e0)&&table!=0x483118)return 0;
  if((address==0x23eaa0||address==0x23eae0||address==0x23e9e0||address==0x486a0)&&table!=0x48312c)return 0;
  if((address==0x23ea00||address==0x23ea10||address==0x2729b0||address==0x2c3360)&&table!=0x483140)return 0;
  if(address==0x23ea20&&(!emitter_span(0x51cb8c,4)||!emitter_span(MEM32(0x51cb8c),4)))return 0;
 }
 switch(address){
 case 0x276e0:SET_LO8(g_eax,0);g_recomp_zf=1;g_esp+=4;return 1;
 case 0x2c3360:SET_LO8(g_eax,1);g_esp+=4;return 1;
 case 0x2729b0:g_edx=MEM32(g_ecx+4);g_eax=0;SET_LO8(g_eax,g_edx==0);g_recomp_zf=g_edx==0;g_esp+=4;return 1;
 case 0x23ea00:g_eax=MEM32(g_ecx+4);g_esp+=8;return 1;
 case 0x23ea10:g_eax=MEM32(g_ecx+4);MEM32(g_ecx+4)=0;g_esp+=8;return 1;
 case 0x486a0:case 0x23e9e0:{
  unsigned flags;
  if(address==0x486a0){g_xmm0=XMM_ZERO();flags=emitter_compare(g_xmm0.f[0],MEMF(g_ecx+12));}
  else{g_xmm0=XMM_SCALAR(MEMF(0x44c530));flags=emitter_compare(g_xmm0.f[0],MEMF(g_ecx+16));}
  if(!(flags&(address==0x486a0?1u:0x41u))){g_eax=1;g_recomp_zf=!!(flags&0x40u);}else{g_eax=0;g_recomp_zf=1;}
  g_esp+=4;return 1;
 }
 case 0x23ea50:case 0x23ea70:case 0x23eb60:{
  g_ecx=MEM32(0x51cb8c);g_eax=MEM32(g_ecx);
  {uint32_t saved=g_esp,target=MEM32(g_eax+4);PUSH32(g_esp,address==0x23ea50?0x23ea5b:address==0x23ea70?0x23ea7b:0x23eb6b);RECOMP_ICALL_SAFE(target,saved);}
  int zero=g_eax==0;
  if(!zero){
   if(address==0x23ea50){g_ecx=MEM32(g_esp+4);MEM32(g_eax)=0x483140;MEM32(g_eax+4)=g_ecx;}
   else{
    g_xmm0=XMM_SCALAR(MEMF(g_esp+4));MEMF(g_eax+4)=g_xmm0.f[0];g_xmm0=XMM_ZERO();
    if(address==0x23ea70){MEM32(g_eax)=0x483118;MEMF(g_eax+8)=g_xmm0.f[0];}
    else{MEMF(g_eax+8)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+8));MEM32(g_eax)=0x48312c;MEMF(g_eax+12)=g_xmm0.f[0];MEMF(g_eax+16)=g_xmm0.f[0];}
   }
  }else g_eax=0;
  g_recomp_zf=zero;g_esp+=4;return 1;
 }
 case 0x23ea20:{
  PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x23ea28);
  /* Entire DA5E0 base destructor, including its retired return word. */
  MEM32(g_ecx)=0x45c298;g_esp+=4;int deleting=MEM8(g_esp+8)&1u;
  if(deleting){g_ecx=MEM32(0x51cb8c);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x23ea3b);RECOMP_ICALL_SAFE(target,saved);}
  g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=!deleting;g_esp+=8;return 1;
 }
 case 0x1a89c0:case 0x23eaa0:{
  if(address==0x23eaa0){g_xmm0=XMM_SCALAR(MEMF(g_ecx+16));g_xmm1=XMM_SCALAR(MEMF(g_esp+4));unsigned flags=emitter_compare(g_xmm1.f[0],g_xmm0.f[0]);MEMF(g_esp+4)=g_xmm0.f[0];if(flags&0x41u)MEMF(g_esp+4)=g_xmm1.f[0];}
  emitter_push(MEMF(g_esp+4));g_esp-=8;EMITTER_TOP*=MEMF(g_ecx+4);MEMD(g_esp)=EMITTER_TOP;emitter_pop();
  PUSH32(g_esp,address==0x1a89c0?0x1a89d2:0x23eace);sub_002D72D1();g_esp+=8;
  PUSH32(g_esp,address==0x1a89c0?0x1a89da:0x23ead6);sub_002D6780();
  g_recomp_zf=g_esp==0;g_esp+=8;return 1;
 }
 case 0x1a89e0:{
  emitter_push(MEMF(g_esp+4));g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;EMITTER_TOP*=MEMF(g_esi+4);g_eax=g_esp+4;
  PUSH32(g_esp,g_eax);g_esp-=8;EMITTER_TOP+=MEMF(g_esi+8);MEMD(g_esp)=EMITTER_TOP;emitter_pop();
  PUSH32(g_esp,0x1a8a00);sub_002D8121();MEMF(g_esi+8)=(float)EMITTER_TOP;emitter_pop();emitter_push(MEMD(g_esp+16));g_esp+=12;
  PUSH32(g_esp,0x1a8a0f);sub_002D6780();POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
 }
 case 0x23eae0:{
  g_xmm0=XMM_ZERO();g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;
  unsigned flags=emitter_compare(g_xmm0.f[0],MEMF(g_esi+12));
  if(!(flags&1u)){g_eax=0;POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;return 1;}
  g_xmm1=XMM_SCALAR(MEMF(g_esp+16));g_xmm0=XMM_SCALAR(MEMF(g_esi+12));flags=emitter_compare(g_xmm0.f[0],g_xmm1.f[0]);
  MEMF(g_esp+4)=g_xmm1.f[0];if(flags&0x41u)MEMF(g_esp+4)=g_xmm0.f[0];
  emitter_push(MEMF(g_esp+4));g_eax=g_esp+4;EMITTER_TOP*=MEMF(g_esi+4);PUSH32(g_esp,g_eax);g_esp-=8;EMITTER_TOP+=MEMF(g_esi+8);
  MEMD(g_esp)=EMITTER_TOP;emitter_pop();PUSH32(g_esp,0x23eb2e);sub_002D8121();
  g_xmm0=XMM_SCALAR(MEMF(g_esi+12));g_xmm0.f[0]-=MEMF(g_esp+28);MEMF(g_esi+8)=(float)EMITTER_TOP;emitter_pop();
  emitter_push(MEMD(g_esp+16));g_esp+=12;MEMF(g_esi+12)=g_xmm0.f[0];PUSH32(g_esp,0x23eb4d);sub_002D6780();
  POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
 }
 default:return 0;
 }
}
#undef EMITTER_TOP
