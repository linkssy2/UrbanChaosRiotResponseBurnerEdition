/* Original control480D40 and manager/resource lifetime. No alternate graph. */
#include "hybrid_particle_control_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00238EB0(void),sub_00238D80(void),sub_00238DA0(void),sub_00238DC0(void),sub_00238E30(void),sub_00238E90(void);
extern void sub_0023DEE0(void),sub_0023DE80(void),sub_0023DC80(void),sub_0023DF00(void),sub_0023DF30(void),sub_0023DF90(void),sub_0023DFB0(void),sub_0023E010(void);
extern void sub_00214E70(void),sub_002AB090(void),sub_00011030(void),sub_00219210(void),sub_002D5CCD(void),sub_002D73A4(void),sub_0023DB20(void),sub_0023DC00(void);
extern void sub_0023E7C0(void),sub_002D589F(void),sub_002D57FB(void),sub_00134680(void),sub_0023D760(void),sub_0023DE20(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_CONTROL");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_particle_control(unsigned op){
 if(!enabled())return 0;
 uint32_t s=g_esp,frame=g_seh_ebp;
 if(op>24||(s&3)||s<0x11000||!span(s-4096,4120)||!span(0x51cb58,0x30)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if((op==0||op==15||op==16||op==17||op==18||op>=20)&&!span(g_ecx,0x30))return 0;
 if((op==6||op==7)&&!span(MEM32(s+8),4))return 0;
 if((op==8||op==12||op==13)&&!span(MEM32(s+4),4))return 0;
 if(op==0||op==16){
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39d358);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;
  if(op==0){MEM32(g_esp+16)=0;MEM32(g_esi)=0x480d40;PUSH32(g_esp,0x219050);sub_00238EB0();g_ecx=g_eax;PUSH32(g_esp,0x219057);sub_00238D80();g_ecx=MEM32(g_esp+8);g_eax=g_esi;}
  else{MEM32(g_esi)=0x480d40;MEM32(g_esp+16)=0;PUSH32(g_esp,0x219240);sub_00238EB0();g_ecx=g_eax;PUSH32(g_esp,0x219247);sub_0023DEE0();PUSH32(g_esp,0x21924c);sub_00238EB0();g_ecx=g_eax;PUSH32(g_esp,0x219253);sub_00238DA0();g_ecx=MEM32(g_esp+8);MEM32(g_esi)=0x480ca8;}
  POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(op==1||op==2||op==10||op==14){
  static const uint32_t returns[]={0x219075,0x219085,0x219175,0x2191e5};unsigned i=op==1?0:op==2?1:op==10?2:3;
  PUSH32(g_esp,returns[i]);sub_00238EB0();g_ecx=g_eax;g_seh_ebp=frame;
  if(op==1)sub_00238DC0();else if(op==2)sub_0023DEE0();else if(op==10)sub_00214E70();else sub_00011030();return 1;
 }
 if(op==3||op==9||op==11||op==12||op==13){
  g_eax=MEM32(g_esp+4);if(op==12||op==13)g_eax=MEM32(g_eax);PUSH32(g_esp,g_eax);
  uint32_t ret=op==3?0x21909a:op==9?0x21915a:op==11?0x21918a:op==12?0x2191ac:0x2191cc;
  PUSH32(g_esp,ret);sub_00238EB0();g_ecx=g_eax;PUSH32(g_esp,ret+7);
  if(op==3)sub_00238E30();else if(op==9)sub_00238E90();else if(op==11)sub_002AB090();else if(op==12)sub_0023DF90();else sub_0023DFB0();g_esp+=8;return 1;
 }
 if(op==4||op==5){
  g_eax=MEM32(g_esp+12);g_ecx=MEM32(g_esp+8);g_edx=MEM32(g_esp+4);PUSH32(g_esp,op==4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,op==4?0x2190c6:0x2190e6);sub_00238EB0();g_ecx=g_eax;PUSH32(g_esp,op==4?0x2190cd:0x2190ed);sub_0023DC80();g_esp+=16;return 1;
 }
 if(op==6){g_eax=MEM32(g_esp+8);g_eax=MEM32(g_eax);g_ecx=MEM32(g_esp+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x219101);sub_00238EB0();g_ecx=g_eax;PUSH32(g_esp,0x219108);sub_0023E010();g_esp+=12;return 1;}
 if(op==7){g_eax=MEM32(g_esp+8);g_eax=MEM32(g_eax);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x219122);sub_00238EB0();g_ecx=g_eax;PUSH32(g_esp,0x219129);sub_0023DF00();g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=12;return 1;}
 if(op==8){g_eax=MEM32(g_esp+4);g_eax=MEM32(g_eax);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x21913c);sub_00238EB0();g_ecx=g_eax;PUSH32(g_esp,0x219143);sub_0023DF30();g_esp+=8;return 1;}
 if(op==15){PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x2191f8);sub_00219210();if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x219205);sub_002D5CCD();g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 if(op==17||op==18){
  uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x5150b0);g_eax=MEM32(g_ecx);uint32_t target=MEM32(g_eax+100);PUSH32(g_esp,op==17?0x238d8e:0x238dae);RECOMP_ICALL_SAFE(target,saved);
  g_edx=MEM32(g_eax);g_esi+=32;saved=g_esp;PUSH32(g_esp,g_esi);g_ecx=g_eax;target=MEM32(g_edx+(op==17?8:12));PUSH32(g_esp,op==17?0x238d99:0x238db9);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);g_recomp_zf=g_esi==0;g_esp+=4;return 1;
 }
 if(op==19){int empty=MEM32(0x51cb58)==0;g_eax=0x51cb58;if(empty)MEM32(0x51cb58)=1;g_recomp_zf=empty;g_esp+=4;return 1;}
 if(op==20){PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x23dee8);sub_00238DC0();g_ecx=g_esi+32;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_0023DE80();return 1;}
 if(op==21){
  if(!span(MEM32(g_ecx+8),12))return 0;
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);MEM32(g_esp+4)=g_eax;
  if(g_eax!=g_ecx)do{g_eax=MEM32(g_eax+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23de9a);sub_002D73A4();g_esp+=4;g_ecx=g_esp+4;PUSH32(g_esp,0x23dea6);sub_0023DB20();g_eax=MEM32(g_esp+4);}while(g_eax!=MEM32(g_esi+8));
  g_ecx=MEM32(g_esi+8);g_edx=MEM32(g_ecx+4);g_esi+=4;PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,0x23dec0);sub_0023DC00();
  g_eax=MEM32(g_esi+4);MEM32(g_eax+4)=g_eax;g_eax=MEM32(g_esi+4);MEM32(g_esi+8)=0;MEM32(g_eax)=g_eax;g_esi=MEM32(g_esi+4);MEM32(g_esi+8)=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=g_esi==0;g_esp+=4;return 1;
 }
 if(op==22){
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e43b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=16;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=g_esi+32;MEM32(g_esp+4)=g_esi;PUSH32(g_esp,0x23e967);sub_0023E7C0();
  g_xmm0=XMM_ZERO();MEMF(g_esp+8)=g_xmm0.f[0];g_ecx=MEM32(g_esp+8);g_eax=g_esi+16;MEM32(g_eax)=g_ecx;MEMF(g_esp+12)=g_xmm0.f[0];g_edx=MEM32(g_esp+12);MEMF(g_esp+16)=g_xmm0.f[0];g_ecx=MEM32(g_esp+16);MEM32(g_eax+4)=g_edx;
  PUSH32(g_esp,0x400);MEM32(g_esp+32)=0;MEM32(g_eax+8)=g_ecx;MEM8(g_esi+28)=0;MEM32(g_esi+8)=0;MEM32(g_esi+12)=256;PUSH32(g_esp,0x23e9b7);sub_002D589F();
  g_ecx=MEM32(g_esp+24);MEM32(g_esi+4)=g_eax;g_esp+=4;MEM32(g_esi)=0;g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(op==23){
  SET_LO8(g_eax,MEM8(g_esp+16));int copy=LO8(g_eax)!=0;PUSH32(g_esp,g_esi);g_esi=g_ecx;
  if(copy){
   PUSH32(g_esp,0x160);PUSH32(g_esp,0x23dc95);sub_002D57FB();g_esp+=4;int allocated=g_eax!=0;
   if(allocated){g_ecx=MEM32(g_esp+16);PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x23dca8);sub_00134680();g_edx=MEM32(g_esp+12);PUSH32(g_esp,g_eax);MEM8(g_eax+0x110)=1;g_eax=MEM32(g_esp+12);PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23dcc1);sub_0023D760();POP32(g_esp,g_esi);g_recomp_zf=!allocated;}
   else{g_edx=MEM32(g_esp+12);g_eax=0;PUSH32(g_esp,g_eax);MEM8(g_eax+0x110)=1;g_eax=MEM32(g_esp+12);PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23dce0);sub_0023D760();POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;}
  }else{g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_esp+12);g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23dcfa);sub_0023D760();POP32(g_esp,g_esi);g_recomp_zf=!copy;}
  g_esp+=20;return 1;
 }
 g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_ecx+=32;PUSH32(g_esp,0x23df0e);sub_0023DE20();g_esi=MEM32(g_esp+8);g_eax+=16;PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x23df1d);sub_00134680();g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=12;return 1;
}
