#include "hybrid_particle_factory_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023DE20(void),sub_0023EA50(void),sub_0023EA70(void),sub_0023EB60(void);
extern void sub_0023DF30(void),sub_0023D760(void),sub_002D83F8(void),sub_002D5CCD(void);
extern void sub_00239020(void),sub_0023A270(void),sub_0023AC40(void),sub_0023B4C0(void),sub_0023D0D0(void),sub_0023BBD0(void);
static int factory_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_FACTORY");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_particle_factory(unsigned operation){
 if(!factory_enabled())return 0;
 uint32_t stack=g_esp;
 if(operation>2||(stack&3)||stack<0x10400||!span(stack-1024,1048)||!span(g_ecx,0x24)||!span(0x51cb88,4)||MEM8(0x51c178))return 0;
 if(operation==0){
  g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_eax);g_ecx+=32;PUSH32(g_esp,0x23df3d);sub_0023DE20();
  g_ecx=g_eax;g_edx=MEM32(g_ecx);g_eax=0;
  if(g_edx){
   if(--g_edx){g_esp+=8;return 1;}
   g_ecx=MEM32(g_ecx+12);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23df53);sub_0023EA50();g_esp+=4;g_recomp_zf=g_esp==0;
  }else{
   g_xmm0=XMM_SCALAR(MEMF(g_ecx+8));double a=g_xmm0.f[0],b=MEMF(0x44c174);
   unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;SET_HI8(g_eax,flags);
   if(RECOMP_PARITY8(flags&0x44u)){
    g_eax=MEM32(g_ecx+8);g_ecx=MEM32(g_ecx+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23df87);sub_0023EB60();g_esp+=8;
   }else{
    g_edx=MEM32(g_ecx+4);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x23df74);sub_0023EA70();g_esp+=4;g_recomp_zf=g_esp==0;
   }
  }
  g_esp+=8;return 1;
 }
 if(operation==1){
  PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_ecx=g_esi+32;PUSH32(g_esp,0x23e021);sub_0023DE20();
  g_eax+=16;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x23e032);sub_0023DF30();
  PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x23e03a);sub_0023D760();POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=12;return 1;
 }
 if(!span(MEM32(g_esp+4),4)||!span(MEM32(MEM32(g_esp+4)),20)||!span(MEM32(g_esp+12),0x114))return 0;
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e3a2);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+36);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+32);g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;
 g_ecx=MEM32(frame+144);PUSH32(g_esp,g_ecx);g_ecx=g_esi;MEM32(g_esp+20)=g_edi;
 {uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x23d796);RECOMP_ICALL_SAFE(target,stack);}
 g_edx=MEM32(g_edi+8);g_ebx=g_eax;
 if(g_edx==MEM32(g_edi+12))goto reject;
 if((int32_t)g_ebx<10){
  g_eax=MEM32(g_esi);g_ecx=g_esi;{uint32_t saved=g_esp,target=MEM32(g_eax+16);PUSH32(g_esp,0x23d7b0);RECOMP_ICALL_SAFE(target,saved);}
  if(LO8(g_eax)){
   g_ecx=MEM32(0x51cb88);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp,target=MEM32(g_edx+12);PUSH32(g_esp,0x23d7bf);RECOMP_ICALL_SAFE(target,saved);}
   if(LO8(g_eax))goto reject;
   g_ecx=MEM32(0x51cb88);g_eax=MEM32(g_ecx);{uint32_t saved=g_esp,target=MEM32(g_eax+4);PUSH32(g_esp,0x23d7d2);RECOMP_ICALL_SAFE(target,saved);}
   frame=0x238c50;goto allocated;
  }
 }
 g_ecx=g_ebx<<5;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23d7e4);sub_002D83F8();g_esp+=4;frame=0x2d73a4;
allocated:
 g_edi=g_eax;
 if(!g_edi){frame=MEM32(g_esp+44);goto reject;}
 g_edx=MEM32(g_esp+44);g_eax=MEM32(g_edx+192);
 if(g_eax>5)goto finish;
 {
  static const uint32_t labels[]={0x23d810,0x23d851,0x23d892,0x23d8d3,0x23d911,0x23d94b};
  static const uint32_t allocated_pc[]={0x23d81b,0x23d85c,0x23d89d,0x23d8de,0x23d91c,0x23d956};
  static const uint32_t constructed_pc[]={0x23d84c,0x23d88d,0x23d8ce,0x23d90f,0x23d949,0x23d983};
  uint32_t branch=MEM32(0x23d9f8+g_eax*4);unsigned kind=0;while(kind<6&&labels[kind]!=branch)kind++;
  if(kind==6){g_seh_ebp=frame;RECOMP_ITAIL(branch);return 1;}
  g_ecx=MEM32(0x51ca90);uint32_t target;
  if(kind==0){g_eax=MEM32(g_ecx);target=MEM32(g_eax+4);}else{g_edx=MEM32(g_ecx);target=MEM32(g_edx+4);}
  {uint32_t saved=g_esp;PUSH32(g_esp,allocated_pc[kind]);RECOMP_ICALL_SAFE(target,saved);}
  MEM32(g_esp+36)=g_eax;MEM32(g_esp+28)=kind;
  if(g_eax){
   g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_esp+44);g_ecx+=16;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+44);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);
   PUSH32(g_esp,g_esi);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);g_ecx=g_eax;PUSH32(g_esp,constructed_pc[kind]);
   switch(kind){case 0:sub_00239020();break;case 1:sub_0023A270();break;case 2:sub_0023AC40();break;case 3:sub_0023B4C0();break;case 4:sub_0023D0D0();break;case 5:sub_0023BBD0();break;}
  }else g_eax=0;
 }
 if(!g_eax)goto finish;
 g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_ecx+8);g_esi=MEM32(g_ecx+4);MEM32(g_esi+g_edx*4)=g_eax;g_eax=MEM32(g_ecx+8);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax++;POP32(g_esp,frame);MEM32(g_ecx+8)=g_eax;POP32(g_esp,g_ebx);g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=16;return 1;
reject:
 g_eax=MEM32(g_esi);{uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x23d9c0);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=MEM32(g_esp+40);
 if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x23d9ce);RECOMP_ICALL_SAFE(target,saved);}
 SET_LO8(g_eax,MEM8(frame+272));if(LO8(g_eax)){PUSH32(g_esp,frame);PUSH32(g_esp,0x23d9de);sub_002D5CCD();g_esp+=4;}
finish:
 g_ecx=MEM32(g_esp+20);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=16;return 1;
}
