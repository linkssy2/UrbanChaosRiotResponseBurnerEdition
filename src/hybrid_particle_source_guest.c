#include "hybrid_particle_source_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00011000(void);
extern void sub_000BE360(void);
extern void sub_00207D70(void);
extern void sub_00218EF0(void);
extern void sub_00218FA0(void);
extern void sub_002192D0(void);
extern void sub_002D3780(void);
extern void sub_002D57FB(void);
extern void sub_002D5CCD(void);
static int source_enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_SOURCE");value=p&&!strcmp(p,"1");}return value;}
/* Existing source objects and entity registry remain authoritative. The
 * child/entity virtual calls are completing original services, never replayed. */
static int source_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int source_away(uint32_t p,unsigned n,uint32_t q,unsigned m){return (uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static void source_rows(uint32_t out,uint32_t input){for(unsigned i=0;i<4;i++){g_xmm0=XMM_MEM(input+i*16);XMM_STORE(out+i*16,g_xmm0);}}
static void source_point(void){
 g_xmm1=XMM_MEM(g_eax+48);g_xmm3=XMM_MEM(g_eax+32);g_xmm0=XMM_MEM(g_ecx);
 g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,170);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=XMM_MEM(g_eax+16);g_xmm3=XMM_ADD(g_xmm3,g_xmm1);
 g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,85);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=XMM_MEM(g_eax);
 g_xmm0=XMM_MUL(g_xmm0,g_xmm1);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);XMM_STORE(g_esp+48,g_xmm0);
}
int uc_hybrid_particle_source(uint32_t address){
 if(!source_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx,table=0;
 if((s&3)||s<0x10400u||!source_span(s-1024,1040)||!source_span(0x51c178,1)||!source_away(s-1024,1040,0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7)return 0;
 int allocate=address==0x219450||address==0x219530||address==0x219740||address==0x219790||address==0x218e10||address==0x218f40||address==0x218ff0;
 if(!allocate){
  if(!source_span(p,52)||!source_away(p,52,s-1024,1040)||!source_away(p,52,0x51c178,1))return 0;
  table=MEM32(p);
  if(address!=0x2192d0&&table!=0x480d04&&table!=0x480d10&&table!=0x480d1c&&table!=0x480d28&&table!=0x480d34&&table!=0x480d7c)return 0;
  if(address==0x276e0&&table!=0x480d7c)return 0;
  if((address==0x218dc0)&&(table!=0x480d04&&table!=0x480d10))return 0;
  if(address==0x218df0&&table!=0x480d1c)return 0;
  if(address==0x2194b0&&table!=0x480d04)return 0;
  if(address==0x219330&&table!=0x480d7c)return 0;
 }
 int matrix=address==0x219330||address==0x2194b0||address==0x219560||address==0x2197c0||address==0x218eb0||address==0x219a50;
 if(matrix&&(!source_span(MEM32(s+4),64)||!source_away(MEM32(s+4),64,0x51c178,1)||!source_span(0x51e110,64)||!source_span(0x44bf94,4)))return 0;
 int lookup=address==0x218dc0||address==0x218df0||address==0x2194b0||address==0x219560||address==0x2197c0||address==0x218e80||address==0x219740||address==0x218e10;
 if(lookup){
  if(!source_span(0x51c4dc,4)||!source_span(MEM32(0x51c4dc),24))return 0;
  uint32_t registry=MEM32(MEM32(0x51c4dc)+4),key=allocate?MEM32(s+4):MEM32(p+4);
  uint64_t slot=(uint64_t)registry+60+(key&1023u)*8;
  if(key!=0xffffffffu&&(slot>0xffffffffu||!source_span((uint32_t)slot,8)))return 0;
 }
 if(address==0x219420&&(MEM8(s+4)&1u)){
  if(!source_span(0x51c604,4)||!source_span(MEM32(0x51c604),4)||!source_span(MEM32(MEM32(0x51c604)),12))return 0;
 }
 switch(address){
 case 0x276e0:SET_LO8(g_eax,0);g_recomp_zf=1;g_esp+=4;return 1;
 case 0x2192d0:{
  if(!source_span(MEM32(s+4),64))return 0;
  static const unsigned offsets[]={0,4,8,16,20,24,32,36,40,48,52};
  g_eax=g_ecx;g_ecx=MEM32(g_esp+4);MEM32(g_eax)=0x480d7c;
  for(unsigned i=0;i<11;i++){g_edx=MEM32(g_ecx+offsets[i]);MEM32(g_eax+4+4*i)=g_edx;}
  g_ecx=MEM32(g_ecx+56);MEM32(g_eax+48)=g_ecx;g_esp+=8;return 1;
 }
 case 0x219450:case 0x219530:case 0x219790:{
  if(!source_span(0x51c604,4)||!source_span(MEM32(0x51c604),4)||!source_span(MEM32(MEM32(0x51c604)),12))return 0;
  g_ecx=MEM32(0x51c604);g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+4);
  PUSH32(g_esp,address==0x219450?0x21945b:address==0x219530?0x21953b:0x21979b);RECOMP_ICALL_SAFE(target,saved);
  int zero=g_eax==0;
  if(!zero){
   g_ecx=MEM32(g_esp+4);
   if(address==0x219450){PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x21946b);sub_002192D0();}
   else if(address==0x219530){SET_LO8(g_edx,MEM8(g_esp+8));MEM32(g_eax+4)=g_ecx;MEM8(g_eax+8)=LO8(g_edx);MEM32(g_eax)=0x480d04;}
   else{SET_LO8(g_edx,MEM8(g_esp+12));MEM32(g_eax+4)=g_ecx;g_ecx=MEM32(g_esp+8);MEM8(g_eax+8)=LO8(g_edx);MEM32(g_eax)=0x480d10;MEM32(g_eax+12)=g_ecx;}
  }else g_eax=0;
  g_recomp_zf=zero;g_esp+=4;return 1;
 }
 case 0x219420:case 0x1d52d0:{
  if(address==0x219420&&(table!=0x480d04&&table!=0x480d10&&table!=0x480d7c))return 0;
  if(address==0x1d52d0&&table!=0x480d1c)return 0;
  PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,address==0x219420?0x219428:0x1d52d8);MEM32(g_ecx)=0x477880;g_esp+=4;
  int deleting=MEM8(g_esp+8)&1u;
  if(deleting){
   if(address==0x219420){g_ecx=MEM32(0x51c604);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x21943b);RECOMP_ICALL_SAFE(target,saved);}
   else{PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1d52e5);sub_002D5CCD();g_esp+=4;}
  }
  g_eax=g_esi;POP32(g_esp,g_esi);if(address==0x219420)g_recomp_zf=!deleting;g_esp+=8;return 1;
 }
 case 0x218dc0:{
  PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x218dd2);sub_00207D70();
  int dead=g_eax==0;
  if(!dead){SET_LO8(g_ecx,MEM8(g_esi+8));dead=LO8(g_ecx)&&(MEM8(g_eax+0x5c)&0x20);}
  if(dead){SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_esp+=4;return 1;}
  SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=4;return 1;
 }
 case 0x218df0:
  g_eax=MEM32(g_ecx+4);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x218dff);sub_00207D70();g_eax=g_eax?0:1;g_recomp_zf=g_eax==0;g_esp+=4;return 1;
 case 0x2194b0:{
  g_eax=MEM32(g_ecx+4);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2194bf);sub_00207D70();int missing=g_eax==0;
  if(missing){g_eax=MEM32(g_esp+4);source_rows(g_eax,0x51e110);}
  else{g_edx=MEM32(g_eax);g_ecx=g_eax;uint32_t saved=g_esp,target=MEM32(g_edx+44);PUSH32(g_esp,0x2194fc);RECOMP_ICALL_SAFE(target,saved);
   g_xmm0=XMM_MEM(g_eax);g_ecx=MEM32(g_esp+4);XMM_STORE(g_ecx,g_xmm0);
   for(unsigned i=1;i<4;i++){g_xmm0=XMM_MEM(g_eax+i*16);XMM_STORE(g_ecx+i*16,g_xmm0);}g_eax=g_ecx;}
  g_recomp_zf=missing;g_esp+=12;return 1;
 }
 case 0x218f70:
  if(table!=0x480d34)return 0;
  g_ecx=MEM32(g_ecx+8);g_eax=MEM32(g_ecx);RECOMP_ITAIL(MEM32(g_eax+4));return 1;
 case 0x218e80:{
  if(table!=0x480d28)return 0;
  PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x218e92);sub_00207D70();
  int dead=g_eax==0;
  if(!dead){g_ecx=MEM32(g_esi+8);g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+4);PUSH32(g_esp,0x218e9e);RECOMP_ICALL_SAFE(target,saved);dead=LO8(g_eax)!=0;}
  g_eax=dead?1:0;POP32(g_esp,g_esi);if(!dead)g_recomp_zf=1;g_esp+=4;return 1;
 }
 case 0x218eb0:{
  if(table!=0x480d28)return 0;
  g_edx=MEM32(g_esp+8);g_ecx=MEM32(g_ecx+8);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x218ec3);RECOMP_ICALL_SAFE(target,saved);
  g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=12;return 1;
 }
 case 0x219a50:{
  if(table!=0x480d34)return 0;
  uint32_t frame=g_ebp,saved=g_esp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-76;
  g_edx=MEM32(frame+12);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);
  PUSH32(g_esp,g_edx);g_edx=g_esp+20;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x219a6d);RECOMP_ICALL_SAFE(target,saved);
  g_eax=MEM32(g_esi+4);g_xmm0=XMM_MEM(g_esp+16);g_eax<<=4;g_xmm1=XMM_MEM(g_esp+g_eax+16);g_eax=g_esp+g_eax+16;
  XMM_STORE(g_esp+16,g_xmm1);XMM_STORE(g_eax,g_xmm0);g_eax=MEM32(frame+8);source_rows(g_eax,g_esp+16);
  POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_eax==0;g_esp+=12;return 1;
 }
 case 0x218f40:case 0x218ff0:{
  PUSH32(g_esp,12);PUSH32(g_esp,address==0x218f40?0x218f47:0x218ff7);sub_002D57FB();g_esp+=4;int zero=g_eax==0;
  if(!zero){g_ecx=MEM32(g_esp+4);g_edx=MEM32(g_esp+8);MEM32(g_eax)=address==0x218f40?0x480d28:0x480d34;MEM32(g_eax+4)=g_ecx;MEM32(g_eax+8)=g_edx;}
  else g_eax=0;g_recomp_zf=zero;g_esp+=4;return 1;
 }
 case 0x218ef0:case 0x218fa0:{
  if(table!=(address==0x218ef0?0x480d28u:0x480d34u))return 0;
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39d338);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=address==0x218ef0?0x480d28:0x480d34;
  g_ecx=MEM32(g_esi+8);int child=g_ecx!=0;MEM32(g_esp+16)=0;
  if(child){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,address==0x218ef0?0x218f28:0x218fd8);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_esp+8);MEM32(g_esi)=0x477880;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 case 0x218ed0:case 0x218f80:{
  if(table!=(address==0x218ed0?0x480d28u:0x480d34u))return 0;
  PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,address==0x218ed0?0x218ed8:0x218f88);
  if(address==0x218ed0)sub_00218EF0();else sub_00218FA0();
  if(MEM8(g_esp+8)&1u){PUSH32(g_esp,g_esi);PUSH32(g_esp,address==0x218ed0?0x218ee5:0x218f95);sub_002D5CCD();g_esp+=4;}
  g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
 }
 case 0x219740:{
  g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x219752);sub_00207D70();
  g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_eax);{uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_eax;uint32_t target=MEM32(g_edx+28);PUSH32(g_esp,0x21975e);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(0x51c604);g_edx=MEM32(g_ecx);g_edi=g_eax;{uint32_t saved=g_esp,target=MEM32(g_edx+4);PUSH32(g_esp,0x21976b);RECOMP_ICALL_SAFE(target,saved);}
  int zero=g_eax==0;
  if(!zero){SET_LO8(g_ecx,MEM8(g_esp+20));MEM32(g_eax+12)=g_edi;POP32(g_esp,g_edi);MEM32(g_eax+4)=g_esi;MEM8(g_eax+8)=LO8(g_ecx);MEM32(g_eax)=0x480d10;POP32(g_esp,g_esi);}
  else{POP32(g_esp,g_edi);g_eax=0;POP32(g_esp,g_esi);}g_recomp_zf=zero;g_esp+=4;return 1;
 }
 case 0x218e10:{
  g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);
  PUSH32(g_esp,g_edi);PUSH32(g_esp,0x218e23);sub_00207D70();g_ecx=MEM32(g_esp+20);g_esi=g_eax;g_eax=MEM32(g_esi);
  {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_esi;uint32_t target=MEM32(g_eax+28);PUSH32(g_esp,0x218e31);RECOMP_ICALL_SAFE(target,saved);}
  g_edx=MEM32(g_esi);g_ebx=g_eax;g_eax=MEM32(g_esp+24);
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;uint32_t target=MEM32(g_edx+28);PUSH32(g_esp,0x218e3f);RECOMP_ICALL_SAFE(target,saved);}
  PUSH32(g_esp,24);g_esi=g_eax;PUSH32(g_esp,0x218e48);sub_002D57FB();g_esp+=4;int zero=g_eax==0;
  if(!zero){g_xmm0=XMM_SCALAR(MEMF(0x44bf94));MEM32(g_eax+4)=g_edi;POP32(g_esp,g_edi);MEM32(g_eax+12)=g_esi;MEMF(g_eax+16)=g_xmm0.f[0];g_xmm0=XMM_ZERO();POP32(g_esp,g_esi);MEM32(g_eax+8)=g_ebx;MEM32(g_eax)=0x480d1c;MEMF(g_eax+20)=g_xmm0.f[0];POP32(g_esp,g_ebx);}
  else{POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=0;POP32(g_esp,g_ebx);}g_recomp_zf=zero;g_esp+=4;return 1;
 }
 case 0x219330:{
  if(!source_span(MEM32(s+4),64))return 0;
  uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-76;
  g_xmm0=XMM_SCALAR(MEMF(g_ecx+40));g_xmm1=XMM_SCALAR(MEMF(g_ecx+16));MEMF(g_esp+60)=g_xmm0.f[0];
  g_xmm0=XMM_SCALAR(MEMF(g_ecx+44));MEMF(g_esp+64)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ecx+48));MEMF(g_esp+68)=g_xmm0.f[0];
  g_xmm0=XMM_SCALAR(MEMF(0x44bf94));MEMF(g_esp+28)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_ecx+20));
  PUSH32(g_esp,g_esi);g_esi=MEM32(frame+8);MEMF(g_esp+76)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ecx+28));MEMF(g_esp+36)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_ecx+24));
  PUSH32(g_esp,0x1f4b10);MEMF(g_esp+52)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ecx+32));MEMF(g_esp+44)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_ecx+4));
  PUSH32(g_esp,4);MEMF(g_esp+60)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ecx+36));MEMF(g_esp+24)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_ecx+8));
  PUSH32(g_esp,16);MEMF(g_esp+68)=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEMF(g_esp+32)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_ecx+12));
  PUSH32(g_esp,g_esi);MEMF(g_esp+76)=g_xmm0.f[0];MEMF(g_esp+60)=g_xmm0.f[0];MEMF(g_esp+40)=g_xmm1.f[0];MEMF(g_esp+44)=g_xmm0.f[0];
  g_ebp=frame;PUSH32(g_esp,0x2193f3);sub_00011000();source_rows(g_esi,g_esp+16);g_eax=g_esi;
  POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=12;return 1;
 }
 case 0x219560:{
  if(table!=0x480d10||!source_span(MEM32(s+4),64))return 0;
  uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-136;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
  g_edi=g_ecx;g_eax=MEM32(g_edi+4);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x21957f);sub_00207D70();g_esi=g_eax;
  int missing=!g_esi;
  if(!missing){g_eax=MEM32(g_edi+12);missing=g_eax==0xffffffffu;}
  if(!missing){g_edx=MEM32(g_esi);g_ecx=g_esp+16;uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_ecx=g_esi;uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x2195a2);RECOMP_ICALL_SAFE(target,saved);missing=!LO8(g_eax);}
  if(missing){
   g_xmm0=XMM_MEM(0x51e110);g_eax=MEM32(frame+8);XMM_STORE(g_eax,g_xmm0);g_xmm0=XMM_MEM(0x51e120);XMM_STORE(g_eax+16,g_xmm0);g_xmm0=XMM_MEM(0x51e130);XMM_STORE(g_eax+32,g_xmm0);
   g_xmm0=XMM_MEM(0x51e140);POP32(g_esp,g_edi);XMM_STORE(g_eax+48,g_xmm0);POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_esp+=12;return 1;
  }
  g_xmm0=XMM_MEM(g_esp+32);g_xmm1=XMM_MEM(g_esp+48);g_edx=MEM32(g_esi);g_ecx=g_esi;XMM_STORE(g_esp+32,g_xmm1);XMM_STORE(g_esp+48,g_xmm0);
  {uint32_t saved=g_esp,target=MEM32(g_edx+44);PUSH32(g_esp,0x2195c5);RECOMP_ICALL_SAFE(target,saved);}
  g_xmm0=XMM_MEM(g_esp+16);g_xmm1=XMM_MEM(g_eax+48);g_xmm2=XMM_MEM(g_eax+32);g_xmm3=XMM_MEM(g_eax+16);g_xmm4=XMM_MEM(g_eax);
  for(unsigned row=0;row<3;row++){
   if(row)g_xmm0=XMM_MEM(g_esp+16+row*16);
   g_xmm5=XMM_SHUFFLE(g_xmm0,g_xmm0,255);g_xmm6=XMM_MUL(g_xmm1,g_xmm5);
   g_xmm5=XMM_SHUFFLE(g_xmm0,g_xmm0,170);g_xmm7=XMM_MUL(g_xmm2,g_xmm5);g_xmm7=XMM_ADD(g_xmm7,g_xmm6);
   g_xmm5=XMM_SHUFFLE(g_xmm0,g_xmm0,85);g_xmm6=XMM_MUL(g_xmm3,g_xmm5);g_xmm5=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=XMM_MUL(g_xmm4,g_xmm5);
   if(row<2){g_xmm0=XMM_ADD(g_xmm0,g_xmm6);g_xmm0=XMM_ADD(g_xmm0,g_xmm7);XMM_STORE(g_esp+80+row*16,g_xmm0);}
  }
  g_xmm5=g_xmm0;g_xmm0=XMM_MEM(g_esp+64);g_xmm5=XMM_ADD(g_xmm5,g_xmm6);g_xmm6=XMM_SHUFFLE(g_xmm0,g_xmm0,255);g_xmm1=XMM_MUL(g_xmm1,g_xmm6);
  g_xmm6=XMM_SHUFFLE(g_xmm0,g_xmm0,170);g_xmm5=XMM_ADD(g_xmm5,g_xmm7);g_xmm2=XMM_MUL(g_xmm2,g_xmm6);g_eax=MEM32(frame+8);g_xmm2=XMM_ADD(g_xmm2,g_xmm1);
  g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,85);g_xmm3=XMM_MUL(g_xmm3,g_xmm1);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm4=XMM_MUL(g_xmm4,g_xmm1);
  g_xmm1=XMM_MEM(g_esp+80);g_xmm4=XMM_ADD(g_xmm4,g_xmm3);g_xmm0=g_xmm4;XMM_STORE(g_eax,g_xmm1);g_xmm1=XMM_MEM(g_esp+96);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);
  XMM_STORE(g_eax+16,g_xmm1);XMM_STORE(g_eax+32,g_xmm5);XMM_STORE(g_eax+48,g_xmm0);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=0;g_esp+=12;return 1;
 }
 case 0x2197c0:{
  if(table!=0x480d1c||!source_span(MEM32(s+4),64))return 0;
  uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-148;
  g_eax=MEM32(0x51c4dc);g_xmm0=XMM_SCALAR(MEMF(g_eax+20));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;
  g_xmm0.f[0]*=MEMF(g_edi+20);g_xmm0.f[0]+=MEMF(g_edi+16);
  double a=g_xmm0.f[0],b=MEMF(0x44bf94);unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;
  g_ebx=g_edi+16;MEMF(g_ebx)=g_xmm0.f[0];
  if(!(flags&1u)){
   PUSH32(g_esp,0x40400000);PUSH32(g_esp,0x3fc00000);g_ebp=frame;PUSH32(g_esp,0x219804);sub_002D3780();
   MEMF(g_edi+20)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;g_xmm0=XMM_ZERO();g_esp+=8;MEMF(g_ebx)=g_xmm0.f[0];
  }
  g_ecx=MEM32(g_edi+4);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x51c4dc);g_ebp=frame;PUSH32(g_esp,0x219820);sub_00207D70();g_esi=g_eax;
  int missing=!g_esi;
  if(!missing){g_eax=MEM32(g_edi+8);missing=g_eax==0xffffffffu;}
  if(!missing)missing=MEM32(g_edi+12)==0xffffffffu;
  if(missing){
   g_xmm0=XMM_MEM(0x51e110);g_eax=MEM32(frame+8);XMM_STORE(g_eax,g_xmm0);g_xmm0=XMM_MEM(0x51e120);XMM_STORE(g_eax+16,g_xmm0);
   g_xmm0=XMM_MEM(0x51e130);POP32(g_esp,g_edi);XMM_STORE(g_eax+32,g_xmm0);g_xmm0=XMM_MEM(0x51e140);POP32(g_esp,g_esi);XMM_STORE(g_eax+48,g_xmm0);
   POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=12;return 1;
  }
  g_xmm0=XMM_MEM(0x51e110);g_edx=MEM32(g_esi);XMM_STORE(g_esp+96,g_xmm0);g_xmm0=XMM_MEM(0x51e120);g_ecx=g_esp+32;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);XMM_STORE(g_esp+116,g_xmm0);g_xmm0=XMM_MEM(0x51e130);PUSH32(g_esp,g_eax);g_ecx=g_esi;XMM_STORE(g_esp+136,g_xmm0);
   uint32_t target=MEM32(g_edx+24);PUSH32(g_esp,0x219874);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_edi+12);g_edx=MEM32(g_esi);g_eax=g_esp+72;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_esi;uint32_t target=MEM32(g_edx+24);PUSH32(g_esp,0x219884);RECOMP_ICALL_SAFE(target,saved);}
  g_edx=MEM32(g_esi);g_ecx=g_esi;
  {uint32_t saved=g_esp,target=MEM32(g_edx+44);PUSH32(g_esp,0x21988b);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=g_esp+32;source_point();
  g_xmm0=XMM_SCALAR(MEMF(g_esp+48));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+52));g_edx=MEM32(g_esp+20);
  MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+56));g_eax=MEM32(g_esp+24);MEMF(g_esp+28)=g_xmm0.f[0];g_ecx=MEM32(g_esp+28);
  MEM32(g_esp+32)=g_edx;g_edx=MEM32(g_esi);MEM32(g_esp+40)=g_ecx;g_ecx=g_esi;MEM32(g_esp+36)=g_eax;
  {uint32_t saved=g_esp,target=MEM32(g_edx+44);PUSH32(g_esp,0x219910);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=g_esp+72;source_point();
  g_xmm0=XMM_SCALAR(MEMF(g_esp+48));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+52));MEMF(g_esp+24)=g_xmm0.f[0];
  g_xmm0=XMM_SCALAR(MEMF(g_esp+56));MEMF(g_esp+28)=g_xmm0.f[0];g_edx=MEM32(g_esp+20);g_eax=MEM32(g_esp+24);g_ecx=MEM32(g_esp+28);
  MEM32(g_esp+72)=g_edx;PUSH32(g_esp,g_ebx);g_edx=g_esp+76;MEM32(g_esp+80)=g_eax;PUSH32(g_esp,g_edx);g_eax=g_esp+40;MEM32(g_esp+88)=g_ecx;
  PUSH32(g_esp,g_eax);g_ecx=g_esp+96;PUSH32(g_esp,g_ecx);g_ebp=frame;PUSH32(g_esp,0x2199a3);sub_000BE360();
  g_xmm0=XMM_SCALAR(MEMF(g_esp+100));g_eax=MEM32(frame+8);g_xmm1=XMM_MEM(g_esp+112);MEMF(g_esp+48)=g_xmm0.f[0];
  g_xmm0=XMM_SCALAR(MEMF(g_esp+104));MEMF(g_esp+52)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+108));MEMF(g_esp+56)=g_xmm0.f[0];
  g_xmm0=XMM_SCALAR(MEMF(0x44bf94));XMM_STORE(g_eax,g_xmm1);g_xmm1=XMM_MEM(g_esp+128);MEMF(g_esp+60)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+48);
  XMM_STORE(g_eax+16,g_xmm1);g_xmm1=XMM_MEM(g_esp+144);g_esp+=16;XMM_STORE(g_eax+32,g_xmm1);XMM_STORE(g_eax+48,g_xmm0);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=12;return 1;
 }
 default:return 0;
 }
}
