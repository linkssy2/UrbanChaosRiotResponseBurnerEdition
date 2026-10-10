/* Particle descriptor cache: preserve original nodes, ordering and service lifetime. */
#include "hybrid_particle_tree_owner_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00134680(void),sub_0023DDB0(void),sub_0023DFD0(void),sub_0023E040(void),sub_0023E220(void);
extern void sub_0023DA60(void),sub_0023DAC0(void),sub_0023DB20(void),sub_0023DB80(void),sub_0023DC00(void);
extern void sub_00238EF0(void),sub_00238F10(void),sub_002D57FB(void),sub_002D5CCD(void),sub_003878F0(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_TREE_OWNER");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}

static void allocate_node(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1a0);PUSH32(g_esp,0x23dfdb);sub_002D57FB();g_esi=g_eax;g_esp+=4;
 int zero=g_esi==0;
 if(!zero){
  g_eax=MEM32(g_esp+0x18);g_ecx=MEM32(g_esp+0x14);g_edx=MEM32(g_esp+0x10);PUSH32(g_esp,g_eax);
  g_eax=MEM32(g_esp+0x10);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+0x10);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_esi;PUSH32(g_esp,0x23e004);sub_0023DDB0();
 }
 g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=zero;g_esp+=24;
}
static void construct_node(void){
 g_eax=MEM32(g_esp+4);g_edx=MEM32(g_esp+12);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esp+16);
 MEM32(g_esi)=g_eax;g_eax=MEM32(g_esp+24);MEM32(g_esi+4)=g_ecx;MEM32(g_esi+8)=g_edx;g_ecx=MEM32(g_eax);MEM32(g_esi+16)=g_ecx;
 PUSH32(g_esp,g_edi);g_eax+=16;g_edx=g_eax;g_ebx=MEM32(g_edx);g_ecx=g_esi+32;g_edi=g_ecx;MEM32(g_edi)=g_ebx;
 g_ebx=MEM32(g_edx+4);MEM32(g_edi+4)=g_ebx;g_ebx=MEM32(g_edx+8);MEM32(g_edi+8)=g_ebx;g_edx=MEM32(g_edx+12);g_eax+=16;
 PUSH32(g_esp,g_eax);g_ecx+=16;MEM32(g_edi+12)=g_edx;PUSH32(g_esp,0x23ddfe);sub_00134680();
 SET_LO8(g_eax,MEM8(g_esp+32));MEM8(g_esi+0x190)=LO8(g_eax);POP32(g_esp,g_edi);MEM8(g_esi+0x191)=0;g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=g_ecx==0;g_esp+=24;
}
static void insert_node(void){
 uint32_t frame=g_seh_ebp;
 g_eax=MEM32(0);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e3b8);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x44;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;
 g_edx=MEM32(g_esp+0x68);g_eax=MEM32(g_edi+4);g_esi=MEM32(g_esp+0x64);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x23e0ca);sub_0023DFD0();
 g_ebx=MEM32(g_edi+8);g_ecx=g_eax;g_eax=MEM32(g_edi+4);++g_ebx;MEM32(g_esp+0x6c)=g_ecx;MEM32(g_edi+8)=g_ebx;
 if(g_esi==g_eax){MEM32(g_eax+4)=g_ecx;g_eax=MEM32(g_edi+4);MEM32(g_eax)=g_ecx;g_edx=MEM32(g_edi+4);MEM32(g_edx+8)=g_ecx;}
 else{
  SET_LO8(g_eax,MEM8(g_esp+0x64));
  if(LO8(g_eax)){MEM32(g_esi)=g_ecx;g_eax=MEM32(g_edi+4);if(g_esi==MEM32(g_eax))MEM32(g_eax)=g_ecx;}
  else{MEM32(g_esi+8)=g_ecx;g_eax=MEM32(g_edi+4);if(g_esi==MEM32(g_eax+8))MEM32(g_eax+8)=g_ecx;}
 }
 g_edx=MEM32(g_ecx+4);g_eax=g_ecx+4;g_esi=g_ecx;SET_LO8(g_ebx,1);
 if(!MEM8(g_edx+0x190)){
  PUSH32(g_esp,frame);
  do{
   g_ecx=MEM32(g_eax);frame=MEM32(g_ecx+4);g_edx=MEM32(frame);
   if(g_ecx==g_edx){
    g_edx=MEM32(frame+8);
    if(!MEM8(g_edx+0x190))goto recolor;
    if(g_esi==MEM32(g_ecx+8)){g_esi=g_ecx;PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x23e178);sub_0023DA60();}
    g_eax=MEM32(g_esi+4);MEM8(g_eax+0x190)=LO8(g_ebx);g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx+4);MEM8(g_edx+0x190)=0;
    g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+4);PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x23e19c);sub_0023DAC0();
   }else{
    if(!MEM8(g_edx+0x190)){
recolor:
     g_ecx=MEM32(g_eax);MEM8(g_ecx+0x190)=LO8(g_ebx);MEM8(g_edx+0x190)=LO8(g_ebx);g_edx=MEM32(g_eax);g_ecx=MEM32(g_edx+4);MEM8(g_ecx+0x190)=0;g_edx=MEM32(g_eax);g_esi=MEM32(g_edx+4);
    }else{
     if(g_esi==MEM32(g_ecx)){g_esi=g_ecx;PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x23e1b5);sub_0023DAC0();}
     g_eax=MEM32(g_esi+4);MEM8(g_eax+0x190)=LO8(g_ebx);g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx+4);MEM8(g_edx+0x190)=0;
     g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+4);PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x23e1d9);sub_0023DA60();
    }
   }
   g_edx=MEM32(g_esi+4);SET_LO8(g_ecx,MEM8(g_edx+0x190));g_eax=g_esi+4;
  }while(!LO8(g_ecx));
  g_ecx=MEM32(g_esp+0x70);POP32(g_esp,frame);
 }
 g_eax=MEM32(g_edi+4);g_edx=MEM32(g_eax+4);g_eax=MEM32(g_esp+0x60);MEM8(g_edx+0x190)=LO8(g_ebx);POP32(g_esp,g_ebx);MEM32(g_eax)=g_ecx;
 g_ecx=MEM32(g_esp+0x4c);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=0x50;g_recomp_zf=g_esp==0;g_esp+=20;
}
static void erase_node(void){
 uint32_t frame=g_seh_ebp;
 g_eax=MEM32(0);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e3b8);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x14);MEM32(0)=g_esp;g_esp-=0x48;PUSH32(g_esp,frame);frame=g_ecx;SET_LO8(g_ecx,MEM8(g_eax+0x191));PUSH32(g_esp,g_esi);
 PUSH32(g_esp,g_ebx);g_ebx=g_eax;PUSH32(g_esp,g_edi);g_ecx=g_esp+0x6c;MEM32(g_esp+0x10)=g_ebx;PUSH32(g_esp,0x23e2a7);sub_0023DB20();
 g_eax=MEM32(g_ebx);SET_LO8(g_ecx,MEM8(g_eax+0x191));
 if(LO8(g_ecx))g_edi=MEM32(g_ebx+8);
 else{
  g_edx=MEM32(g_ebx+8);SET_LO8(g_ecx,MEM8(g_edx+0x191));
  if(LO8(g_ecx))g_edi=g_eax;
  else{g_ecx=MEM32(g_esp+0x6c);g_edi=MEM32(g_ecx+8);if(g_ecx!=g_ebx)goto transplant;}
 }
 SET_LO8(g_eax,MEM8(g_edi+0x191));g_esi=MEM32(g_ebx+4);if(!LO8(g_eax))MEM32(g_edi+4)=g_esi;
 g_eax=MEM32(frame+4);if(MEM32(g_eax+4)==g_ebx)MEM32(g_eax+4)=g_edi;else MEM32(g_esi+(MEM32(g_esi)==g_ebx?0:8))=g_edi;
 g_ebx=MEM32(frame+4);g_eax=MEM32(g_ebx);
 if(g_eax==MEM32(g_esp+0x10)){
  SET_LO8(g_eax,MEM8(g_edi+0x191));if(LO8(g_eax))g_eax=g_esi;else{PUSH32(g_esp,g_edi);PUSH32(g_esp,0x23e31b);sub_00238EF0();g_esp+=4;}MEM32(g_ebx)=g_eax;
 }
 g_ebx=MEM32(frame+4);g_ecx=MEM32(g_esp+0x10);
 if(MEM32(g_ebx+8)==g_ecx){
  SET_LO8(g_eax,MEM8(g_edi+0x191));if(LO8(g_eax))g_eax=g_esi;else{PUSH32(g_esp,g_edi);PUSH32(g_esp,0x23e347);sub_00238F10();g_esp+=4;}MEM32(g_ebx+8)=g_eax;
 }
 goto fix_color;
transplant:
 MEM32(g_eax+4)=g_ecx;g_edx=MEM32(g_ebx);MEM32(g_ecx)=g_edx;
 if(g_ecx==MEM32(g_ebx+8))g_esi=g_ecx;
 else{SET_LO8(g_eax,MEM8(g_edi+0x191));g_esi=MEM32(g_ecx+4);if(!LO8(g_eax))MEM32(g_edi+4)=g_esi;MEM32(g_esi)=g_edi;g_eax=MEM32(g_ebx+8);MEM32(g_ecx+8)=g_eax;g_edx=MEM32(g_ebx+8);MEM32(g_edx+4)=g_ecx;}
 g_eax=MEM32(frame+4);if(MEM32(g_eax+4)==g_ebx)MEM32(g_eax+4)=g_ecx;else{g_eax=MEM32(g_ebx+4);MEM32(g_eax+(MEM32(g_eax)==g_ebx?0:8))=g_ecx;}
 g_eax=MEM32(g_ebx+4);MEM32(g_ecx+4)=g_eax;SET_LO8(g_edx,MEM8(g_ebx+0x190));SET_LO8(g_eax,MEM8(g_ecx+0x190));MEM8(g_ecx+0x190)=LO8(g_edx);MEM8(g_ebx+0x190)=LO8(g_eax);
fix_color:
 g_eax=MEM32(g_esp+0x10);SET_LO8(g_ecx,MEM8(g_eax+0x190));SET_LO8(g_ebx,1);
 if(LO8(g_ecx)==LO8(g_ebx)){
  g_ecx=MEM32(frame+4);if(g_edi==MEM32(g_ecx+4))goto blacken;
  while(MEM8(g_edi+0x190)==LO8(g_ebx)){
   g_eax=MEM32(g_esi);
   if(g_edi==g_eax){
    g_eax=MEM32(g_esi+8);SET_LO8(g_ecx,MEM8(g_eax+0x190));
    if(!LO8(g_ecx)){MEM8(g_eax+0x190)=LO8(g_ebx);PUSH32(g_esp,g_esi);g_ecx=frame;MEM8(g_esi+0x190)=0;PUSH32(g_esp,0x23e418);sub_0023DA60();g_eax=MEM32(g_esi+8);}
    SET_LO8(g_ecx,MEM8(g_eax+0x191));if(LO8(g_ecx))goto ascend;
    g_edx=MEM32(g_eax);if(MEM8(g_edx+0x190)==LO8(g_ebx)){g_ecx=MEM32(g_eax+8);if(MEM8(g_ecx+0x190)==LO8(g_ebx))goto red_sibling;}
    g_ecx=MEM32(g_eax+8);
    if(MEM8(g_ecx+0x190)==LO8(g_ebx)){g_edx=MEM32(g_eax);MEM8(g_edx+0x190)=LO8(g_ebx);PUSH32(g_esp,g_eax);g_ecx=frame;MEM8(g_eax+0x190)=0;PUSH32(g_esp,0x23e464);sub_0023DAC0();g_eax=MEM32(g_esi+8);}
    SET_LO8(g_ecx,MEM8(g_esi+0x190));MEM8(g_eax+0x190)=LO8(g_ecx);MEM8(g_esi+0x190)=LO8(g_ebx);g_edx=MEM32(g_eax+8);PUSH32(g_esp,g_esi);g_ecx=frame;MEM8(g_edx+0x190)=LO8(g_ebx);PUSH32(g_esp,0x23e48a);sub_0023DA60();break;
   }else{
    SET_LO8(g_ecx,MEM8(g_eax+0x190));
    if(!LO8(g_ecx)){MEM8(g_eax+0x190)=LO8(g_ebx);PUSH32(g_esp,g_esi);g_ecx=frame;MEM8(g_esi+0x190)=0;PUSH32(g_esp,0x23e4ae);sub_0023DAC0();g_eax=MEM32(g_esi);}
    SET_LO8(g_ecx,MEM8(g_eax+0x191));if(LO8(g_ecx))goto ascend;
    g_edx=MEM32(g_eax+8);if(MEM8(g_edx+0x190)==LO8(g_ebx)){g_ecx=MEM32(g_eax);if(MEM8(g_ecx+0x190)==LO8(g_ebx))goto red_sibling;}
    g_ecx=MEM32(g_eax);
    if(MEM8(g_ecx+0x190)==LO8(g_ebx)){g_edx=MEM32(g_eax+8);MEM8(g_edx+0x190)=LO8(g_ebx);PUSH32(g_esp,g_eax);g_ecx=frame;MEM8(g_eax+0x190)=0;PUSH32(g_esp,0x23e50d);sub_0023DA60();g_eax=MEM32(g_esi);}
    SET_LO8(g_ecx,MEM8(g_esi+0x190));MEM8(g_eax+0x190)=LO8(g_ecx);MEM8(g_esi+0x190)=LO8(g_ebx);g_edx=MEM32(g_eax);PUSH32(g_esp,g_esi);g_ecx=frame;MEM8(g_edx+0x190)=LO8(g_ebx);PUSH32(g_esp,0x23e531);sub_0023DAC0();break;
   }
red_sibling:
   MEM8(g_eax+0x190)=0;
ascend:
   g_edx=MEM32(frame+4);g_eax=MEM32(g_edx+4);g_edi=g_esi;g_esi=MEM32(g_esi+4);if(g_edi==g_eax)break;
  }
blacken:
  MEM8(g_edi+0x190)=LO8(g_ebx);
 }
 g_eax=MEM32(g_esp+0x10);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23e541);sub_002D5CCD();g_eax=MEM32(frame+8);g_esp+=4;POP32(g_esp,g_edi);POP32(g_esp,g_ebx);if(g_eax){--g_eax;MEM32(frame+8)=g_eax;}
 g_ecx=MEM32(g_esp+0x64);g_eax=MEM32(g_esp+0x60);MEM32(g_eax)=g_ecx;g_ecx=MEM32(g_esp+0x50);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(0)=g_ecx;g_esp+=0x54;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void find_insert(void){
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+0x14);PUSH32(g_esp,g_esi);g_ebx=g_ecx;PUSH32(g_esp,g_edi);g_edi=MEM32(g_ebx+4);g_esi=MEM32(g_edi+4);SET_LO8(g_ecx,MEM8(g_esi+0x191));SET_LO8(g_eax,1);MEM8(g_esp+0x10)=LO8(g_eax);
 while(!LO8(g_ecx)){
  g_eax=MEM32(g_esi+16);g_ecx=MEM32(frame);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_edi=g_esi;PUSH32(g_esp,0x23e5a0);sub_003878F0();g_esp+=8;
  SET_LO8(g_eax,(int32_t)g_eax<0);MEM8(g_esp+0x10)=LO8(g_eax);g_esi=MEM32(g_esi+(LO8(g_eax)?0:8));SET_LO8(g_ecx,MEM8(g_esi+0x191));
 }
 g_esi=g_edi;MEM32(g_esp+0x1c)=g_esi;
 if(LO8(g_eax)){
  g_eax=MEM32(g_ebx+4);int first=g_edi==MEM32(g_eax);g_ecx=g_esp+0x1c;
  if(first){PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);PUSH32(g_esp,1);goto insert;}
  PUSH32(g_esp,0x23e5fb);sub_0023DB80();g_esi=MEM32(g_esp+0x1c);
 }
 g_eax=MEM32(frame);g_ecx=MEM32(g_esi+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23e60c);sub_003878F0();g_esp+=8;
 if((int32_t)g_eax<0){
  g_eax=MEM32(g_esp+0x10);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);g_ecx=g_esp+0x28;
insert:
  PUSH32(g_esp,g_ecx);g_ecx=g_ebx;PUSH32(g_esp,0x23e5e2);sub_0023E040();g_edx=MEM32(g_eax);g_eax=MEM32(g_esp+0x18);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(g_eax)=g_edx;MEM8(g_eax+4)=1;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_esp+=12;return;
 }
 int zero=g_eax==0;g_eax=MEM32(g_esp+0x18);POP32(g_esp,g_edi);MEM32(g_eax)=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);MEM8(g_eax+4)=0;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=zero;g_esp+=12;
}
static void erase_range(void){
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+16);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esp+16);
 if(g_ecx==MEM32(g_eax)&&g_ebx==g_eax){
  g_eax=MEM32(g_eax+4);PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x23e662);sub_0023DC00();g_eax=MEM32(g_esi+4);MEM32(g_eax+4)=g_eax;g_eax=MEM32(g_esi+4);MEM32(g_esi+8)=0;MEM32(g_eax)=g_eax;g_eax=MEM32(g_esi+4);MEM32(g_eax+8)=g_eax;g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);g_eax=MEM32(g_esp+12);POP32(g_esp,g_esi);MEM32(g_eax)=g_edx;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=16;return;
 }
 if(g_ecx!=g_ebx){
  PUSH32(g_esp,g_edi);
  do{g_edi=g_ecx;g_ecx=g_esp+0x14;PUSH32(g_esp,0x23e69b);sub_0023DB20();PUSH32(g_esp,g_edi);g_eax=g_esp+0x1c;PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x23e6a8);sub_0023E220();g_ecx=MEM32(g_esp+0x14);}while(g_ecx!=g_ebx);
  POP32(g_esp,g_edi);
 }
 g_eax=MEM32(g_esp+12);POP32(g_esp,g_esi);MEM32(g_eax)=g_ecx;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=16;
}
int uc_hybrid_particle_tree_owner(unsigned op){
 if(!enabled())return 0;
 uint32_t s=g_esp;
 if(op>5||(s&3)||s<0x11000||!span(s-4096,4124)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(op==0){if(!span(MEM32(s+16),0x171))return 0;allocate_node();return 1;}
 if(op==5){if(!span(g_ecx,0x192)||!span(MEM32(s+16),0x171))return 0;construct_node();return 1;}
 if(!span(g_ecx,12)||!span(MEM32(g_ecx+4),0x192)||!span(MEM32(s+4),op==3?5:4))return 0;
 /* The skipped exception predicates must survive the original SEH/frame stores. */
 if(op==1){if((g_ecx+8<s&&g_ecx+12>s-88)||MEM32(g_ecx+8)>=0xaaaaa9u||!span(MEM32(s+12),0x192)||!span(MEM32(s+16),0x171))return 0;insert_node();}
 else if(op==2){uint32_t node=MEM32(s+8);if(!span(node,0x192)||(node+0x191>=s-88&&node+0x191<s)||MEM8(node+0x191))return 0;erase_node();}
 else if(op==3){if(!span(MEM32(s+8),0x171))return 0;find_insert();}
 else{if(!span(MEM32(s+8),0x192)||!span(MEM32(s+12),0x192))return 0;erase_range();}
 return 1;
}
