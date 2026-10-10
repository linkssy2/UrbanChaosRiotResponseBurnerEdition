#include "hybrid_particle_emitter_extra_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002084E0(void),sub_00210760(void),sub_002D57FB(void),sub_002D5CCD(void);
extern void sub_0018F200(void),sub_00219B10(void),sub_002D72D1(void),sub_002D6780(void),sub_002D8121(void);
static int extended_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_EMITTER_EXTRA");flag=v&&!strcmp(v,"1");}return flag;}
/* Remaining conditional/fire emitter implementations. Shared entity queries,
 * CRT and allocator services keep the original guest ownership and ABI. */
static int extended_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static unsigned extended_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
static void extended_push(double value){g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value;}
static void extended_pop(void){g_fp_top=(g_fp_top+1u)&7u;}
#define EXTENDED_TOP g_fp_stack[g_fp_top]
int uc_hybrid_particle_emitter_extra(uint32_t address){
 if(!extended_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx,frame=g_seh_ebp;
 if((s&3)||s<0x10400u||!extended_span(s-1024,1040)||!extended_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7)return 0;
 if(address!=0x219b60){
  if(!extended_span(p,28)||!((uint64_t)p+28<=s-1024||(uint64_t)s+16<=p))return 0;
  uint32_t table=MEM32(p);
  if(address>=0x18f130&&address<=0x18f200){if(table!=0x474270)return 0;}
  else if(address==0x276e0){if(table!=0x474270)return 0;}
  else if(table!=0x480d88)return 0;
 }
 switch(address){
 case 0x276e0:SET_LO8(g_eax,0);g_recomp_zf=1;g_esp+=4;return 1;
 case 0x218f70:case 0xe40c0:case 0xe40d0:
  g_ecx=MEM32(g_ecx+8);g_eax=MEM32(g_ecx);g_seh_ebp=frame;
  RECOMP_ITAIL(MEM32(g_eax+(address==0x218f70?4:address==0xe40c0?12:16)));return 1;
 case 0x219ac0:
  PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);g_ecx=MEM32(0x51c4dc);
  PUSH32(g_esp,g_eax);PUSH32(g_esp,0x219ad2);sub_002084E0();g_ecx=g_eax;PUSH32(g_esp,0x219ad9);sub_00210760();
  if(LO8(g_eax)){g_ecx=MEM32(g_esi+8);g_edx=MEM32(g_ecx);POP32(g_esp,g_esi);g_seh_ebp=frame;RECOMP_ITAIL(MEM32(g_edx+8));return 1;}
  g_eax=0;POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;return 1;
 case 0x219b60:{
  PUSH32(g_esp,12);PUSH32(g_esp,0x219b67);sub_002D57FB();g_esp+=4;int zero=g_eax==0;
  if(!zero){g_ecx=MEM32(g_esp+4);g_edx=MEM32(g_esp+8);MEM32(g_eax)=0x480d88;MEM32(g_eax+4)=g_ecx;MEM32(g_eax+8)=g_edx;}
  else g_eax=0;g_recomp_zf=zero;g_esp+=4;return 1;
 }
 case 0x219b10:case 0x18f200:{
  int fire=address==0x18f200;
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,fire?0x3965b8:0x39d398);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=fire?0x474270:0x480d88;
  g_ecx=MEM32(g_esi+8);int child=g_ecx!=0;MEM32(g_esp+16)=0;
  if(child){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,fire?0x18f238:0x219b48);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_esp+8);MEM32(g_esi)=0x45c298;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 case 0x219af0:case 0x18f1e0:{
  int fire=address==0x18f1e0;PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,fire?0x18f1e8:0x219af8);
  if(fire)sub_0018F200();else sub_00219B10();
  if(MEM8(g_esp+8)&1u){PUSH32(g_esp,g_esi);PUSH32(g_esp,fire?0x18f1f5:0x219b05);sub_002D5CCD();g_esp+=4;}
  g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
 }
 case 0x18f130:
  extended_push(MEMF(g_esp+4));g_esp-=8;EXTENDED_TOP*=MEMF(g_ecx+12);MEMD(g_esp)=EXTENDED_TOP;extended_pop();
  PUSH32(g_esp,0x18f142);sub_002D72D1();g_esp+=8;PUSH32(g_esp,0x18f14a);sub_002D6780();g_recomp_zf=g_esp==0;g_esp+=8;return 1;
 case 0x18f1c0:
  SET_LO8(g_eax,MEM8(g_ecx+20));
  if(!LO8(g_eax)){SET_LO8(g_eax,0);g_recomp_zf=1;g_esp+=4;return 1;}
  g_xmm0=XMM_ZERO();{unsigned flags=extended_compare(g_xmm0.f[0],MEMF(g_ecx+16));if(flags&1u){g_eax=0;g_recomp_zf=1;}else{g_eax=1;g_recomp_zf=!!(flags&0x40u);}}g_esp+=4;return 1;
 case 0x18f150:{
  g_esp-=8;g_xmm0=XMM_ZERO();uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);MEMF(g_esp+4)=g_xmm0.f[0];
  {uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x18f164);RECOMP_ICALL_SAFE(target,saved);}
  if(!LO8(g_eax)){
   extended_push(MEMF(g_esp+16));g_ecx=g_esp+4;EXTENDED_TOP*=MEMF(g_esi+12);PUSH32(g_esp,g_ecx);g_esp-=8;EXTENDED_TOP+=MEMF(g_esi+24);
   MEMD(g_esp)=EXTENDED_TOP;extended_pop();PUSH32(g_esp,0x18f182);sub_002D8121();extended_push(MEMD(g_esp+16));g_esp+=12;
   MEMF(g_esp+4)=(float)EXTENDED_TOP;extended_pop();MEMF(g_esi+24)=(float)EXTENDED_TOP;extended_pop();
  }
  SET_LO8(g_eax,MEM8(g_esi+20));int finite=LO8(g_eax)!=0;g_eax=(int32_t)MEMF(g_esp+4);
  if(finite){g_xmm0=XMM_SCALAR(MEMF(g_esi+16));g_xmm0.f[0]-=MEMF(g_esp+16);MEMF(g_esi+16)=g_xmm0.f[0];}
  POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
 }
 default:return 0;
 }
}
#undef EXTENDED_TOP
