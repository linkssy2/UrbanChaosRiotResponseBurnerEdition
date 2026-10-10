#include "hybrid_light_flash_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00218A60(void),sub_001FB780(void),sub_001F8DC0(void),sub_001FB9D0(void),sub_00087270(void);
extern void sub_002D57FB(void),sub_002D5CCD(void),sub_00175D60(void),sub_00176170(void),sub_001761C0(void);
extern void sub_001FBA40(void),sub_00218CE0(void),sub_00218B30(void),sub_001FB910(void),sub_00218C40(void);
extern void sub_001F9010(void),sub_002D6EB0(void),sub_001FB930(void);
static int enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_LIGHT_FLASH");value=p&&!strcmp(p,"1");}return value;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define VCALL(target,pc,saved) do{uint32_t t_=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(t_,saved);}while(0)
#define TOP g_fp_stack[g_fp_top]
#define ST(i) g_fp_stack[(g_fp_top+(i))&7u]
#define FPUSH(v) do{double v_=(v);g_fp_top=(g_fp_top+7u)&7u;TOP=v_;}while(0)
#define FPOP() (g_fp_top=(g_fp_top+1u)&7u)
static void construct(void){
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x394ef6);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=28;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+16)=g_esi;CALL(sub_00218A60,0x175d87);
 g_ebx=g_esi+0xd0;g_ecx=g_ebx;MEM32(g_esp+52)=0;MEM32(g_esi)=0x4700c8;CALL(sub_001FB780,0x175da2);
 g_edi=MEM32(g_esp+60);g_eax=g_esp+60;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x45882c);PUSH32(g_esp,2);g_ecx=g_edi;MEM8(g_esp+64)=1;CALL(sub_001F8DC0,0x175dbe);
 g_xmm0=XMM_SCALAR(MEMF(0x44bf94));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+60));
 static const uint32_t keys[]={0x45888c,0x45887c,0x45886c,0x45885c};
 static const uint32_t returns[]={0x175deb,0x175e18,0x175e45,0x175e72};
 static const uint32_t defaults[]={0x44c178,0x44c178,0x44bf94,0};
 for(unsigned i=0;i<4;i++){
  uint32_t temp=g_esp+60;if(i%3==0)g_ecx=temp;else if(i%3==1)g_edx=temp;else g_eax=temp;
  PUSH32(g_esp,temp);PUSH32(g_esp,keys[i]);PUSH32(g_esp,2);g_ecx=g_edi;MEMF(g_esi+0xd4+4*i)=g_xmm0.f[0];CALL(sub_001F8DC0,returns[i]);
  g_xmm0=defaults[i]?XMM_SCALAR(MEMF(defaults[i])):XMM_ZERO();if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+60));
 }
 g_edx=g_esp+32;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x458820);PUSH32(g_esp,4);g_ecx=g_edi;MEMF(g_esi+0xe4)=g_xmm0.f[0];CALL(sub_001F8DC0,0x175e9a);
 for(unsigned i=0;i<2;i++){
  if(!LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(0x44bf94));MEMF(g_esp+20)=g_xmm0.f[0];MEMF(g_esp+24)=g_xmm0.f[0];}
  else{g_xmm0=XMM_SCALAR(MEMF(g_esp+32));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+36));MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+40));}
  g_ecx=MEM32(g_esp+20);g_edx=MEM32(g_esp+24);if(!i){frame=g_esi+0xe8;g_eax=frame;}else g_eax=g_esi+0xf4;
  MEM32(g_eax)=g_ecx;MEM32(g_eax+4)=g_edx;g_edx=g_esp+(i?60:32);PUSH32(g_esp,g_edx);MEMF(g_esp+32)=g_xmm0.f[0];g_ecx=MEM32(g_esp+32);
  PUSH32(g_esp,i?0x458850:0x458810);MEM32(g_eax+8)=g_ecx;PUSH32(g_esp,i?3:4);g_ecx=g_edi;CALL(sub_001F8DC0,i?0x175f72:0x175f07);
 }
 if(LO8(g_eax)){g_eax=MEM32(g_esp+60)&0xff;SET_LO8(g_eax,g_eax!=0);}
 g_ecx=g_esp+60;PUSH32(g_esp,g_ecx);g_eax=ZX8(LO8(g_eax));PUSH32(g_esp,0x458844);PUSH32(g_esp,3);g_ecx=g_edi;MEM32(g_esi+0x100)=g_eax;CALL(sub_001F8DC0,0x175f9e);
 if(LO8(g_eax)){g_eax=MEM32(g_esp+60)&0xff;SET_LO8(g_eax,g_eax!=0);}
 g_edx=ZX8(LO8(g_eax));g_eax=g_esp+60;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x458838);PUSH32(g_esp,2);g_ecx=g_edi;MEM32(g_esi+0x104)=g_edx;CALL(sub_001F8DC0,0x175fca);
 g_xmm0=XMM_SCALAR(MEMF(0x44bf94));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+60));
 PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ecx=g_ebx;MEMF(g_esi+0x108)=g_xmm0.f[0];CALL(sub_001FB9D0,0x175fee);
 g_xmm0=XMM_ZERO();MEMF(g_esi+0x10c)=g_xmm0.f[0];g_ecx=MEM32(0x5150b0);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;VCALL(MEM32(g_edx+0x30),0x176004,saved);}
 g_edx=MEM32(g_eax);{uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_esi+64;PUSH32(g_esp,g_ecx);g_ecx=g_eax;VCALL(MEM32(g_edx+4),0x176013,saved);}
 MEM32(g_esi+0x110)=g_eax;
 if(g_eax){
  g_edx=MEM32(g_eax);g_edi=1;{uint32_t saved=g_esp;PUSH32(g_esp,g_edi);g_ecx=g_eax;VCALL(MEM32(g_edx+12),0x17602a,saved);}
  g_edx=MEM32(g_esi+0x104);g_ecx=MEM32(g_esi+0x110);g_eax=MEM32(g_ecx);SET_LO8(g_edx,g_edx==g_edi);{uint32_t saved=g_esp;PUSH32(g_esp,g_edx);VCALL(MEM32(g_eax+20),0x176041,saved);}
  g_eax=MEM32(g_esi+0x110);g_ebx=MEM32(g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_esp;MEM32(g_esp+64)=g_esp;PUSH32(g_esp,frame);CALL(sub_00087270,0x176056);
  g_ecx=MEM32(g_esi+0x110);{uint32_t saved=g_esp;VCALL(MEM32(g_ebx+28),0x17605f,saved);}
  g_ecx=MEM32(g_esi+0x110);g_eax=MEM32(g_esi+0xe0);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);VCALL(MEM32(g_edx+36),0x176071,saved);}
  g_ecx=MEM32(g_esi+0x110);g_eax=MEM32(g_esi+0xd8);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);VCALL(MEM32(g_edx+44),0x176083,saved);}
  g_ebx=MEM32(g_esi+0x100);g_ecx=MEM32(g_esi+0x110);g_edx=MEM32(g_ecx);SET_LO8(g_eax,g_ebx==g_edi);{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);VCALL(MEM32(g_edx+52),0x17609a,saved);}
 }
 g_ecx=MEM32(g_esp+44);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp+=40;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void update(void){
 g_esp-=24;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;CALL(sub_001F9010,0x1761cc);
 TOP+=MEMF(g_esi+0x10c);MEMF(g_esp+8)=(float)TOP;FPOP();FPUSH(MEMF(0x44c174));g_eax=MEM32(g_esp+8);FPUSH(MEMF(g_esp+8));MEM32(g_esi+0x10c)=g_eax;
 g_fp_cmp=RECOMP_FCMP(TOP,ST(1));FPOP();FPOP();SET_HI8(g_eax,g_fp_cmp==2?0x47:g_fp_cmp<0?3:g_fp_cmp>0?2:0x42);
 if(RECOMP_PARITY8(HI8(g_eax)&0x44)){
  g_edi=MEM32(g_esi+0x110);
  if(g_edi){
   FPUSH(MEMF(g_esp+8));TOP/=MEMF(g_esi+0xd4);FPUSH(MEMF(g_esi+0x108));CALL(sub_002D6EB0,0x17621b);
   g_xmm0=XMM_SCALAR(MEMF(g_esi+0xf4));g_xmm0.f[0]-=MEMF(g_esi+0xe8);g_xmm1=XMM_SCALAR(MEMF(g_esi+0xf8));g_xmm2=XMM_SCALAR(MEMF(g_esi+0xfc));g_xmm2.f[0]-=MEMF(g_esi+0xf0);g_xmm1.f[0]-=MEMF(g_esi+0xec);
   MEMF(g_esp+8)=(float)TOP;FPOP();FPUSH(MEMF(g_esi+0xdc));g_xmm3=XMM_SCALAR(MEMF(g_esp+8));TOP-=MEMF(g_esi+0xd8);
   g_xmm0.f[0]*=g_xmm3.f[0];g_xmm1.f[0]*=g_xmm3.f[0];g_xmm2.f[0]*=g_xmm3.f[0];g_xmm3=XMM_SCALAR(MEMF(g_esi+0xe8));g_xmm2.f[0]+=MEMF(g_esi+0xf0);TOP*=MEMF(g_esp+8);g_xmm3.f[0]+=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0xec));TOP+=MEMF(g_esi+0xd8);
   PUSH32(g_esp,g_ecx);g_edx=g_esp+24;MEMF(g_esp+20)=(float)TOP;FPOP();g_xmm0.f[0]+=g_xmm1.f[0];FPUSH(MEMF(g_esi+0xe4));g_ecx=g_esp;TOP-=MEMF(g_esi+0xe0);MEMF(g_esp+24)=g_xmm3.f[0];MEMF(g_esp+28)=g_xmm0.f[0];MEMF(g_esp+32)=g_xmm2.f[0];g_edi=MEM32(g_edi);TOP*=MEMF(g_esp+12);PUSH32(g_esp,g_edx);TOP+=MEMF(g_esi+0xe0);MEMF(g_esp+20)=(float)TOP;FPOP();CALL(sub_00087270,0x1762d6);
   g_ecx=MEM32(g_esi+0x110);{uint32_t saved=g_esp;VCALL(MEM32(g_edi+28),0x1762df,saved);}
   g_ecx=MEM32(g_esi+0x110);g_edx=MEM32(g_esp+12);g_eax=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,g_edx);VCALL(MEM32(g_eax+36),0x1762ef,saved);}
   g_ecx=MEM32(g_esi+0x110);g_edx=MEM32(g_esp+16);g_eax=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,g_edx);VCALL(MEM32(g_eax+44),0x1762ff,saved);}
  }
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_hybrid_light_flash(uint32_t entry){
 if(!enabled())return 0;
 switch(entry){case 0x175d60:case 0x1760f0:case 0x1760c0:case 0x176150:case 0x176170:case 0x1761c0:case 0x176310:case 0x120c60:break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx,frame=g_seh_ebp;
 if((s&3)||s<0x11000u||!span(s-4096,4112)||!span(0x51c4dc,4)||!span(0x51c178,1)||MEM8(0x51c178)||!apart(s-4096,4112,0x51c178,1))return 0;
 if(entry!=0x1760f0&&(!span(p,0x120)||!apart(p,0x120,s-4096,4112)))return 0;
 if(entry==0x120c60&&MEM32(p)!=0x4700c8)return 0;
 if((entry==0x1761c0||entry==0x176310)&&((unsigned)g_fp_top>7||!span(MEM32(0x51c4dc),24)))return 0;
 /* All services retain their original guest objects and valid completing
  * lifetime contract. There is no rejection or replay after these writes. */
 if(entry==0x175d60){construct();return 1;}
 if(entry==0x1761c0){update();return 1;}
 if(entry==0x1760f0){
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x394f0b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x120);CALL(sub_002D57FB,0x176110);g_esp+=4;MEM32(g_esp)=g_eax;MEM32(g_esp+12)=0;
  if(g_eax){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;CALL(sub_00175D60,0x17612e);}else g_eax=0;
  g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(entry==0x1760c0){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+0x110);
  if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);VCALL(MEM32(g_eax),0x1760d3,saved);MEM32(g_esi+0x110)=0;}
  g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_00218B30();return 1;
 }
 if(entry==0x176150){PUSH32(g_esp,g_esi);g_esi=g_ecx;CALL(sub_00176170,0x176158);if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);CALL(sub_002D5CCD,0x176165);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 if(entry==0x176170){
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x394f28);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;g_ecx=g_esi+0xd0;MEM32(g_esp+16)=0;CALL(sub_001FBA40,0x1761a0);
  g_ecx=g_esi;MEM32(g_esp+16)=0xffffffffu;CALL(sub_00218CE0,0x1761af);g_ecx=MEM32(g_esp+8);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(entry==0x120c60){PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=g_esi+0xd0;CALL(sub_001FB910,0x120c6e);g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_00218C40();return 1;}
 PUSH32(g_esp,g_ecx);g_xmm0=XMM_ZERO();PUSH32(g_esp,g_esi);g_esi=g_ecx;g_xmm1=XMM_SCALAR(MEMF(g_esi+0xd4));
 double a=g_xmm1.f[0],b=g_xmm0.f[0];unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;MEMF(g_esp+4)=g_xmm0.f[0];
 if(!(flags&0x41u)){
  PUSH32(g_esp,g_edi);g_edi=g_esi+0xd0;
  do{g_ecx=g_esi;CALL(sub_001761C0,0x176338);g_ecx=g_edi;CALL(sub_001FB930,0x17633f);CALL(sub_001F9010,0x176344);TOP+=MEMF(g_esp+8);MEMF(g_esp+8)=(float)TOP;FPUSH(MEMF(g_esi+0xd4));g_fp_cmp=RECOMP_FCMP(TOP,ST(1));FPOP();FPOP();}while(g_fp_cmp>0);
  POP32(g_esp,g_edi);
 }
 g_eax=MEM32(g_esi);g_ecx=g_esi;{uint32_t saved=g_esp;VCALL(MEM32(g_eax+36),0x176360,saved);}
 g_ecx=g_esi+0xd0;POP32(g_esp,g_esi);g_esp+=4;g_seh_ebp=frame;sub_001FB930();return 1;
}
#undef FPOP
#undef FPUSH
#undef ST
#undef TOP
#undef VCALL
#undef CALL
