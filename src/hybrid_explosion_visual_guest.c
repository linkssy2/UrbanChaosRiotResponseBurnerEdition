#include "hybrid_explosion_visual_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00218A60(void),sub_001F92B0(void),sub_001F8DC0(void),sub_001F91A0(void);
extern void sub_000EE000(void),sub_000EE2E0(void),sub_000EE0A0(void),sub_002D6380(void);
extern void sub_0021FC20(void),sub_00208C50(void),sub_00219530(void),sub_0023EA50(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_EXPLOSION_VISUAL");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_hybrid_explosion_visual(unsigned operation){
 if(!enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 unsigned size=operation>=6?0x230:operation==4?48:0xdc;
 if(operation>7||(s&3)||s<0x10400u||!span(s-1024,1048)||!span(p,size)||!apart(p,size,s-1024,1048)||!span(0x51c4dc,4)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(operation==2||operation==3){if(!span(MEM32(p),40))return 0;}
 if(operation==3&&!span(MEM32(0x51c4dc),24))return 0;
 if(operation==2){g_eax=MEM32(g_ecx);RECOMP_ITAIL(MEM32(g_eax+36));return 1;}
 if(operation==3){
  g_xmm0=XMM_ZERO();PUSH32(g_esp,g_esi);g_esi=g_ecx;double old=MEMF(g_esi+0xd8);
  unsigned flags=(old!=old)?0x47u:0<old?3u:0>old?2u:0x42u;
  if(!(flags&0x41u)){g_eax=MEM32(g_esi);uint32_t saved=g_esp,target=MEM32(g_eax+36);PUSH32(g_esp,0xee2b4);RECOMP_ICALL_SAFE(target,saved);}
  g_xmm0=XMM_SCALAR(MEMF(g_esi+0xd8));g_ecx=MEM32(0x51c4dc);g_xmm0.f[0]-=MEMF(g_ecx+20);MEMF(g_esi+0xd8)=g_xmm0.f[0];
  POP32(g_esp,g_esi);g_recomp_zf=(flags&0x40u)!=0;g_esp+=4;return 1;
 }
 if(operation==4){
  g_esp-=12;g_xmm0=XMM_ZERO();g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_eax=g_ecx;
  MEMF(g_esp)=g_xmm0.f[0];g_edx=MEM32(g_esp);g_ecx=g_eax+4;MEM32(g_ecx)=g_edx;
  MEMF(g_esp+4)=g_xmm0.f[0];g_edx=MEM32(g_esp+4);MEM32(g_ecx+4)=g_edx;
  MEMF(g_esp+8)=g_xmm0.f[0];g_edx=MEM32(g_esp+8);MEM32(g_ecx+8)=g_edx;
  MEMF(g_esp)=g_xmm0.f[0];g_edx=MEM32(g_esp);g_ecx=g_eax+16;MEM32(g_ecx)=g_edx;
  MEMF(g_esp+4)=g_xmm0.f[0];g_edx=MEM32(g_esp+4);MEMF(g_eax+32)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44c178));MEM32(g_ecx+4)=g_edx;
  MEMF(g_esp+8)=g_xmm0.f[0];g_edx=MEM32(g_esp+8);MEMF(g_eax+36)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44c5e4));
  MEM32(g_eax)=0;MEMF(g_eax+28)=g_xmm0.f[0];MEMF(g_eax+40)=g_xmm1.f[0];MEM32(g_eax+44)=0;MEM32(g_ecx+8)=g_edx;
  g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(operation==5){
  uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-28;
  g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_eax=g_ecx;g_xmm0=XMM_SCALAR(MEMF(0x4600d8));g_xmm3=XMM_SCALAR(MEMF(0x44c178));g_xmm2=XMM_SCALAR(MEMF(0x44bf80));
  MEMF(g_eax+156)=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEMF(g_esp+12)=g_xmm0.f[0];g_edx=MEM32(g_esp+12);MEM32(g_eax+176)=g_edx;
  MEMF(g_esp+16)=g_xmm1.f[0];g_edx=MEM32(g_esp+16);MEM32(g_eax+180)=g_edx;PUSH32(g_esp,g_esi);
  for(unsigned base=0;base<24;base+=12){
   MEMF(g_esp+16)=g_xmm0.f[0];g_esi=MEM32(g_esp+16);MEMF(g_esp+20)=g_xmm0.f[0];g_edx=g_eax+base;MEM32(g_edx)=g_esi;
   g_esi=MEM32(g_esp+20);MEM32(g_edx+4)=g_esi;MEMF(g_esp+24)=g_xmm0.f[0];g_esi=MEM32(g_esp+24);MEM32(g_edx+8)=g_esi;
  }
  MEMF(g_eax+192)=g_xmm3.f[0];g_ecx=0;MEMF(g_esp+16)=g_xmm1.f[0];g_edx=g_esp+16;g_xmm3=XMM_SCALAR(MEMF(g_edx));
  MEMF(g_eax+160)=g_xmm2.f[0];MEM32(g_eax+100)=g_ecx;MEM32(g_eax+144)=g_ecx;
  MEMF(g_eax+164)=g_xmm1.f[0];MEMF(g_eax+168)=g_xmm0.f[0];MEMF(g_eax+172)=g_xmm1.f[0];
  MEMF(g_eax+184)=g_xmm0.f[0];MEMF(g_eax+188)=g_xmm0.f[0];MEMF(g_eax+196)=g_xmm0.f[0];MEMF(g_eax+200)=g_xmm0.f[0];MEMF(g_eax+204)=g_xmm1.f[0];MEMF(g_eax+24)=g_xmm0.f[0];
  g_xmm4=g_xmm3;g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm3,0);XMM_STORE(g_eax+32,g_xmm4);
  MEMF(g_esp+16)=g_xmm1.f[0];g_edx=g_esp+16;g_xmm3=XMM_SCALAR(MEMF(g_edx));g_xmm4=g_xmm3;g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm3,0);XMM_STORE(g_eax+48,g_xmm4);
  MEMF(g_eax+80)=g_xmm2.f[0];MEMF(g_eax+84)=g_xmm2.f[0];MEMF(g_esp+16)=g_xmm1.f[0];MEMF(g_esp+20)=g_xmm1.f[0];MEMF(g_esp+24)=g_xmm1.f[0];MEMF(g_esp+28)=g_xmm0.f[0];
  g_xmm2=XMM_MEM(g_esp+16);XMM_STORE(g_eax+64,g_xmm2);g_xmm2=XMM_SCALAR(MEMF(0x44bf18));SET_LO8(g_edx,1);
  MEMF(g_eax+88)=g_xmm2.f[0];MEMF(g_eax+92)=g_xmm2.f[0];MEMF(g_eax+96)=g_xmm1.f[0];MEM32(g_eax+104)=4;MEM32(g_eax+148)=g_ecx;
  MEM8(g_eax+108)=LO8(g_ecx);MEMF(g_eax+208)=g_xmm1.f[0];MEMF(g_eax+112)=g_xmm0.f[0];MEM8(g_eax+116)=LO8(g_edx);MEM8(g_eax+117)=LO8(g_edx);MEM8(g_eax+118)=LO8(g_ecx);
  g_edx=g_esp+16;MEMF(g_esp+16)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_edx));g_xmm1=g_xmm0;g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm0,0);XMM_STORE(g_eax+128,g_xmm1);MEM8(g_eax+152)=LO8(g_ecx);
  POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_ecx==0;g_esp+=4;return 1;
 }
 if(operation==6){
  uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-20;g_xmm0=XMM_ZERO();PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;
  MEMF(g_esp+12)=g_xmm0.f[0];g_ecx=MEM32(g_esp+12);g_eax=g_esi+228;MEM32(g_eax)=g_ecx;
  MEMF(g_esp+16)=g_xmm0.f[0];g_edx=MEM32(g_esp+16);MEM32(g_eax+4)=g_edx;PUSH32(g_esp,g_edi);
  MEMF(g_esp+24)=g_xmm0.f[0];g_ecx=MEM32(g_esp+24);g_edx=g_esp+12;g_edi=g_esi+224;g_ebx=0;PUSH32(g_esp,g_edx);MEM32(g_edi)=1;MEM32(g_eax+8)=g_ecx;
  for(unsigned offset=252;offset<=264;offset+=4)MEMF(g_esi+offset)=g_xmm0.f[0];MEM32(g_esi+268)=g_ebx;MEM32(g_esp+16)=0x4600bc;
  g_ebp=frame;PUSH32(g_esp,0xee123);sub_0021FC20();g_xmm0=XMM_ZERO();MEM32(g_esi+372)=g_eax;
  MEMF(g_esp+20)=g_xmm0.f[0];g_ecx=MEM32(g_esp+20);MEMF(g_esp+24)=g_xmm0.f[0];g_edx=MEM32(g_esp+24);g_eax=g_esi+272;MEM32(g_eax)=g_ecx;MEM32(g_eax+4)=g_edx;
  MEMF(g_esp+28)=g_xmm0.f[0];g_ecx=MEM32(g_esp+28);MEM32(g_eax+8)=g_ecx;
  MEMF(g_esp+20)=g_xmm0.f[0];g_eax=MEM32(g_esp+20);MEMF(g_esp+24)=g_xmm0.f[0];g_ecx=MEM32(g_esp+24);g_edx=g_esi+284;MEM32(g_edx)=g_eax;MEM32(g_edx+4)=g_ecx;
  MEMF(g_esp+28)=g_xmm0.f[0];g_eax=MEM32(g_esp+28);MEM32(g_edx+8)=g_eax;
  for(unsigned offset=20;offset<=32;offset+=4)MEMF(g_esp+offset)=g_xmm0.f[0];g_xmm1=XMM_MEM(g_esp+20);MEM32(g_esi+416)=g_ebx;MEMF(g_esi+296)=g_xmm0.f[0];XMM_STORE(g_esi+304,g_xmm1);
  g_xmm1=XMM_SCALAR(MEMF(g_esi+220));MEMF(g_esp+16)=g_xmm1.f[0];g_ecx=g_esp+16;g_xmm1=XMM_SCALAR(MEMF(g_ecx));g_xmm2=g_xmm1;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm1,0);XMM_STORE(g_esi+320,g_xmm2);
  for(unsigned offset=20;offset<=32;offset+=4)MEMF(g_esp+offset)=g_xmm0.f[0];g_xmm1=XMM_MEM(g_esp+20);XMM_STORE(g_esi+336,g_xmm1);
  g_edx=MEM32(g_esi+208);g_eax=MEM32(g_esi+212);g_ecx=MEM32(g_esi+216);g_xmm1=XMM_SCALAR(MEMF(0x44c47c));g_esp+=4;
  MEMF(g_esi+352)=g_xmm0.f[0];MEMF(g_esi+356)=g_xmm1.f[0];MEM32(g_esi+360)=g_edx;MEM32(g_esi+364)=g_eax;MEM32(g_esi+368)=g_ecx;MEM32(g_esi+376)=4;MEM32(g_esi+420)=1;MEM8(g_esi+380)=LO8(g_ebx);MEMF(g_esi+384)=g_xmm0.f[0];
  g_ecx=MEM32(0x51c4dc);g_ebp=frame;PUSH32(g_esp,0xee26a);sub_00208C50();g_esi=MEM32(g_esi+84);PUSH32(g_esp,g_edi);g_ebx=g_eax;g_eax=MEM32(g_ebx);PUSH32(g_esp,0);PUSH32(g_esp,g_esi);MEM32(g_esp+24)=g_eax;
  g_ebp=frame;PUSH32(g_esp,0xee27e);sub_00219530();g_esp+=8;PUSH32(g_esp,g_eax);PUSH32(g_esp,1);g_ebp=frame;PUSH32(g_esp,0xee289);sub_0023EA50();g_esp+=4;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+24);g_ecx=g_ebx;uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0xee296);RECOMP_ICALL_SAFE(target,saved);}
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(operation==7){
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38e498);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;
  PUSH32(g_esp,0xee4d3);sub_00218A60();g_edi=g_esi+224;g_ecx=g_edi;MEM32(g_esp+20)=0;MEM32(g_esi)=0x4600e8;PUSH32(g_esp,0xee4ee);sub_000EE000();
  g_ecx=g_edi+48;PUSH32(g_esp,0xee4f6);sub_000EE2E0();PUSH32(g_esp,63);MEM8(g_edi+272)=0;PUSH32(g_esp,0x4600dc);g_edi+=273;PUSH32(g_esp,g_edi);PUSH32(g_esp,0xee510);sub_002D6380();
  g_edi=MEM32(g_esp+40);g_esp+=12;g_eax=g_esp+28;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x45f300);PUSH32(g_esp,2);g_ecx=g_edi;PUSH32(g_esp,0xee52a);sub_001F8DC0();
  g_xmm0=XMM_SCALAR(MEMF(0x44c178));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+28));
  static const uint32_t keys[]={0x45f2f8,0x4588f0,0x45f2ec},defaults[]={0x44e550,0x44bf94,0x44bf94},returns[]={0xee557,0xee584,0xee5b1};
  for(unsigned i=0;i<3;i++){
   uint32_t temp=g_esp+28;if(i==0)g_ecx=temp;else if(i==1)g_edx=temp;else g_eax=temp;
   PUSH32(g_esp,temp);PUSH32(g_esp,keys[i]);PUSH32(g_esp,2);g_ecx=g_edi;MEMF(g_esi+208+4*i)=g_xmm0.f[0];PUSH32(g_esp,returns[i]);sub_001F8DC0();
   g_xmm0=XMM_SCALAR(MEMF(defaults[i]));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+28));
  }
  g_ecx=g_esi;MEMF(g_esi+220)=g_xmm0.f[0];PUSH32(g_esp,0xee5d2);sub_000EE0A0();g_ecx=MEM32(g_esp+12);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;
  g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
 }
 /* Both camera classes construct the same event payload. Only return PCs and
  * exception scope differ; keep the source's register and publication order. */
 uint32_t pc=operation?0x1cce00:0x1ccc00;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,operation?0x399de8:0x399da8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=44;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;MEM32(g_esp+8)=g_edi;PUSH32(g_esp,pc+0x25);sub_00218A60();
 g_eax=g_esp+12;PUSH32(g_esp,0x47c5cc);PUSH32(g_esp,g_eax);MEM32(g_esp+68)=0;MEM32(g_edi)=0x47c610;PUSH32(g_esp,pc+0x42);sub_001F92B0();
 g_esi=MEM32(g_esp+76);g_ecx=g_edi+64;g_edx=MEM32(g_ecx);g_eax=MEM32(g_ecx+4);g_ecx=MEM32(g_ecx+8);g_esp+=8;
 MEM32(g_esp+16)=g_edx;g_edx=g_esp+68;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x4588fc);MEM32(g_esp+32)=g_ecx;PUSH32(g_esp,2);g_ecx=g_esi;MEM32(g_esp+32)=g_eax;
 PUSH32(g_esp,pc+0x73);sub_001F8DC0();g_xmm0=XMM_SCALAR(MEMF(0x44bf94));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+68));
 static const uint32_t keys[]={0x4588f0,0x4588e0,0x4588d0,0x4588c0,0x4588ac};
 static const uint32_t returns[]={0x9e,0xc4,0xea,0x115,0x140};
 static const uint32_t defaults[]={0,0,0x44bf94,0x467798,0x46c150};
 for(unsigned i=0;i<5;i++){
  uint32_t temp=g_esp+68;
  if(i%3==0)g_eax=temp;else if(i%3==1)g_ecx=temp;else g_edx=temp;
  PUSH32(g_esp,temp);PUSH32(g_esp,keys[i]);PUSH32(g_esp,2);g_ecx=g_esi;MEMF(g_esp+40+4*i)=g_xmm0.f[0];
  PUSH32(g_esp,pc+returns[i]);sub_001F8DC0();g_xmm0=defaults[i]?XMM_SCALAR(MEMF(defaults[i])):XMM_ZERO();if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+68));
 }
 g_edx=MEM32(g_edi+84);g_eax=g_esp+68;PUSH32(g_esp,g_eax);g_eax=MEM32(0x51c398);PUSH32(g_esp,36);MEM32(g_esp+76)=g_edx;g_edx=MEM32(0x4f9d18);
 g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);MEMF(g_esp+60)=g_xmm0.f[0];g_ecx=MEM32(g_eax+8);PUSH32(g_esp,g_edx);PUSH32(g_esp,pc+0x17f);sub_001F91A0();
 g_ecx=MEM32(g_esp+52);g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=56;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
