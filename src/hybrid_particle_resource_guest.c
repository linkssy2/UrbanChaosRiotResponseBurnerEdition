#include "hybrid_particle_resource_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023DC40(void),sub_0023E640(void),sub_002D5CCD(void),sub_0023E830(void);
extern void sub_002D7960(void),sub_0023DD00(void),sub_0023D700(void),sub_0023F850(void);
extern void sub_00238D20(void),sub_002D6380(void),sub_00387C82(void),sub_0023DD60(void),sub_0023E570(void);
extern void sub_0023DA10(void),sub_003878F0(void);
static int enabled(void){static int value=-1;if(value<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_RESOURCE");value=v&&!strcmp(v,"1");}return value;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_particle_resource(unsigned operation){
 if(!enabled())return 0;
 uint32_t s=g_esp;
 if(operation>4||(s&3)||s<0x11000||!span(s-4096,4120)||!span(g_ecx,16)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(operation==0){
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e3d8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;MEM32(g_esp+8)=g_edi;g_esi=g_edi+4;g_ecx=g_esi;MEM32(g_esp+20)=0;MEM32(g_edi)=0x4830cc;
  PUSH32(g_esp,0x23e7f6);sub_0023DC40();g_ecx=MEM32(g_esp+12);MEM32(g_esi+4)=g_eax;MEM8(g_eax+0x191)=1;
  g_eax=MEM32(g_esi+4);MEM32(g_eax+4)=g_eax;g_eax=MEM32(g_esi+4);MEM32(g_eax)=g_eax;g_eax=MEM32(g_esi+4);MEM32(g_eax+8)=g_eax;
  g_eax=g_edi;MEM32(g_esi+8)=0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(operation==1){
  if(!span(MEM32(g_ecx+8),12))return 0;
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e3f8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=8;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
  g_edi=g_ecx;MEM32(g_esp+8)=g_edi;g_eax=MEM32(g_edi+8);g_ecx=MEM32(g_eax);g_esi=g_edi+4;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_eax=g_esp+20;PUSH32(g_esp,g_eax);g_ecx=g_esi;MEM32(g_esp+36)=0;
  PUSH32(g_esp,0x23e86e);sub_0023E640();g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23e877);sub_002D5CCD();
  g_ecx=MEM32(g_esp+20);MEM32(g_esi+4)=0;MEM32(g_esi+8)=0;g_esp+=4;MEM32(g_edi)=0x452078;POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(operation==2){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x23e8a8);sub_0023E830();
  if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x23e8b5);sub_002D5CCD();g_esp+=4;}
  g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
 }
 if(operation==4){
  if(!span(MEM32(s+4),4)||!span(MEM32(s+8),4)||!span(MEM32(g_ecx+4),0x192))return 0;
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+20);PUSH32(g_esp,g_edi);g_esi=g_ecx;PUSH32(g_esp,0x23dd0f);sub_0023DA10();
  int missing=g_eax==MEM32(g_esi+4);MEM32(g_esp+20)=g_eax;
  if(!missing){g_eax=MEM32(g_eax+16);g_ecx=MEM32(g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23dd24);sub_003878F0();g_esp+=8;missing=(int32_t)g_eax<0;}
  if(!missing){int zero=g_eax==0;g_eax=g_esp+20;g_ecx=MEM32(g_eax);g_eax=MEM32(g_esp+16);POP32(g_esp,g_edi);MEM32(g_eax)=g_ecx;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=zero;}
  else{g_eax=MEM32(g_esi+4);MEM32(g_esp+8)=g_eax;g_eax=g_esp+8;g_ecx=MEM32(g_eax);g_eax=MEM32(g_esp+16);POP32(g_esp,g_edi);MEM32(g_eax)=g_ecx;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);}
  g_esp+=12;return 1;
 }
 uint32_t frame=g_ebp;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_esp-=0x304;g_eax=MEM32(frame+16);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);
 g_esi=g_ecx;g_ecx=MEM32(frame+12);PUSH32(g_esp,0x4830b4);PUSH32(g_esp,g_ecx);g_ebp=frame;PUSH32(g_esp,0x23e6e3);sub_002D7960();g_esp+=12;
 if(g_eax){SET_LO8(g_eax,0);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=16;return 1;}
 g_edi=MEM32(g_esi+8);g_edx=frame+8;PUSH32(g_esp,g_edx);g_eax=g_esp+28;g_ebx=g_esi+4;PUSH32(g_esp,g_eax);g_ecx=g_ebx;g_ebp=frame;PUSH32(g_esp,0x23e70b);sub_0023DD00();
 if(MEM32(g_eax)==g_edi){
  g_ecx=MEM32(0x5150b0);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp,target=MEM32(g_edx+60);PUSH32(g_esp,0x23e71e);RECOMP_ICALL_SAFE(target,saved);}
  g_edi=MEM32(frame+8);g_edx=MEM32(g_eax);{uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_edi);g_ecx=g_eax;uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x23e72b);RECOMP_ICALL_SAFE(target,saved);}
  g_esi=g_eax;
  if(g_esi){
   g_ecx=g_esp+32;g_ebp=frame;PUSH32(g_esp,0x23e73a);sub_0023D700();g_eax=g_esp+32;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x23e745);sub_0023F850();g_esp+=8;
   if(LO8(g_eax)){
    PUSH32(g_esp,g_edi);g_eax=0x4830ac;g_ebp=frame;PUSH32(g_esp,0x23e757);sub_00238D20();g_esp+=4;if(g_eax)MEM8(g_esp+0xf8)=1;
    PUSH32(g_esp,63);g_ecx=g_esp+0x145;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);g_ebp=frame;PUSH32(g_esp,0x23e776);sub_002D6380();PUSH32(g_esp,g_edi);g_ebp=frame;PUSH32(g_esp,0x23e77c);sub_00387C82();g_esp+=16;
    g_edx=g_esp+32;MEM32(g_esp+24)=g_eax;PUSH32(g_esp,g_edx);g_eax=g_esp+28;PUSH32(g_esp,g_eax);g_ecx=g_esp+0x198;g_ebp=frame;PUSH32(g_esp,0x23e799);sub_0023DD60();
    PUSH32(g_esp,g_eax);g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);g_ecx=g_ebx;g_ebp=frame;PUSH32(g_esp,0x23e7a6);sub_0023E570();
   }
   g_edx=MEM32(g_esi);{uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x23e7ae);RECOMP_ICALL_SAFE(target,saved);}
  }
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=16;return 1;
}
