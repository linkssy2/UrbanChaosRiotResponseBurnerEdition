#include "migration_interface_movieupdate.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_000643E0(void),sub_00087960(void),sub_00274740(void),sub_000656D0(void),sub_000877A0(void),sub_000877B0(void),sub_00017060(void),sub_00017160(void),sub_002D6322(void),sub_00017E40(void),sub_00018CE0(void);
static int mu_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned mu_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
#define MU_TOP g_fp_stack[g_fp_top]
#define MU_POP() (g_fp_top=(g_fp_top+1)&7)
static void movie_fade(void){
 g_xmm0=XMM_SCALAR(MEMF(0x4523d4));
 if(MEM8(g_esi+12)&6){g_ecx=MEM32(g_esi+32)<<24;g_edx=g_esp+12;MEMF(g_esp+16)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x4523d0));PUSH32(g_esp,g_edx);g_eax=g_esp+20;MEM32(g_esp+16)=g_ecx;MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_ZERO();PUSH32(g_esp,g_eax);g_ecx=g_esp+32;MEMF(g_esp+32)=g_xmm0.f[0];MEMF(g_esp+36)=g_xmm0.f[0];PUSH32(g_esp,g_ecx);}
 else{g_edx=MEM32(g_esi+32);MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x51cf64));g_edx<<=24;g_eax=g_esp+12;MEMF(g_esp+28)=g_xmm0.f[0];g_xmm0=XMM_ZERO();PUSH32(g_esp,g_eax);g_ecx=g_esp+28;MEM32(g_esp+16)=g_edx;MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x51cf4c));PUSH32(g_esp,g_ecx);g_edx=g_esp+24;MEMF(g_esp+28)=g_xmm0.f[0];PUSH32(g_esp,g_edx);}
 PUSH32(g_esp,0x272d28);sub_000656D0();g_eax=MEM32(g_esi+32);g_edi=255;g_esp+=12;if(g_eax>=g_edi)MEM8(g_esi+23)=1;g_eax+=(MEM8(g_esi+12)&4)?48:16;MEM32(g_esi+32)=g_eax<g_edi?g_eax:g_edi;
}
static void movie_clock_draw(void){
 g_eax=MEM32(g_esi+60);g_ecx=(g_eax>>16)&255;PUSH32(g_esp,g_ecx);g_edx=g_esp+12;PUSH32(g_esp,0x452d5c);PUSH32(g_esp,g_edx);MEM32(g_esp+48)=g_eax;g_edi=ZX8(HI8(g_eax));PUSH32(g_esp,0x272e91);sub_002D6322();g_ecx=MEM32(0x509704);g_esp+=12;PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);g_eax=g_esp+20;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x272ead);sub_00017E40();
 MU_TOP-=MEMF(0x44ec38);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);MU_TOP=MEMF(0x48902c)-MU_TOP;PUSH32(g_esp,128);PUSH32(g_esp,0x42080000);g_edx=g_esp+28;MEMF(g_esp+56)=(float)MU_TOP;MU_POP();g_ecx=MEM32(g_esp+56);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x272ee2);sub_00018CE0();g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);g_eax=g_esp+20;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x272efb);sub_00017E40();MU_TOP+=MEMF(g_esp+36);g_xmm0=XMM_SCALAR(MEMF(g_esi+64));unsigned cmp=mu_cmp(g_xmm0.f[0],MEMF(0x44c1f8));MEMF(g_esp+36)=(float)MU_TOP;MU_POP();
 if(!(cmp&0x41)){g_ecx=MEM32(g_esp+36);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,128);PUSH32(g_esp,0x41f80000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x44c3f4);PUSH32(g_esp,0x272f36);sub_00018CE0();}
 PUSH32(g_esp,g_edi);g_edx=g_esp+12;PUSH32(g_esp,0x452d5c);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x272f46);sub_002D6322();g_xmm0=XMM_SCALAR(MEMF(g_esp+48));g_xmm0.f[0]+=MEMF(0x44eef8);g_esp+=12;PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,128);PUSH32(g_esp,0x42080000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);g_eax=g_esp+32;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_eax);PUSH32(g_esp,0x272f7d);sub_00018CE0();
}
static void movie_clock(void){
 g_xmm0=XMM_SCALAR(MEMF(g_esi+28));
 if(!(mu_cmp(g_xmm0.f[0],MEMF(g_eax+4))&1)){g_eax=MEM32(g_esi+56);if((int32_t)g_eax>8){g_eax-=8;MEM32(g_esi+56)=g_eax;}else MEM32(g_esi+56)=0;}
 else{g_xmm0=XMM_SCALAR(MEMF(g_esi+28));if(!(mu_cmp(g_xmm0.f[0],MEMF(g_eax))&1)){g_ecx=MEM32(g_esi+56)+8;g_eax=g_ecx;MEM32(g_esi+56)=g_ecx;if((int32_t)g_eax>(int32_t)g_edi)MEM32(g_esi+56)=g_edi;}}
 g_ecx=MEM32(0x509704);PUSH32(g_esp,4);PUSH32(g_esp,0x272dd2);sub_00017060();g_eax=MEM32(g_esi+56);g_ecx=MEM32(0x509704);g_eax<<=24;PUSH32(g_esp,0xff000000);g_eax+=0xffffff;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x272dee);sub_00017160();g_xmm0=XMM_SCALAR(MEMF(g_esi+64));g_xmm0.f[0]+=MEMF(g_esp+36);g_xmm1=XMM_SCALAR(MEMF(0x44bf94));unsigned cmp=mu_cmp(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_esi+64)=g_xmm0.f[0];
 while(!(cmp&1)){
  g_eax=MEM32(g_esi+60)+1;g_ecx=g_eax&g_edi;MEM32(g_esi+60)=g_eax;
  if((int32_t)g_ecx>=60){g_eax+=256;MEM32(g_esi+60)=g_eax;MEM8(g_esi+60)=0;g_eax=MEM32(g_esi+60);g_edx=g_eax&0xff00;if((int32_t)g_edx>=0x3c00){g_eax+=65536;MEM32(g_esi+60)=g_eax;g_eax=0;MEM8(g_esi+61)=0;SET_LO8(g_eax,MEM8(g_esi+62));g_eax&=g_edi;if((int16_t)LO16(g_eax)>23)MEM8(g_esi+62)=0;}}
  g_xmm0=XMM_SCALAR(MEMF(g_esi+64));g_xmm0.f[0]-=g_xmm1.f[0];cmp=mu_cmp(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_esi+64)=g_xmm0.f[0];
 }
 movie_clock_draw();
}
int migration_interface_movieupdate(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_MOVIEUPDATE");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!mu_span(s-1024,1040)||!mu_span(g_ecx,72)||!mu_span(MEM32(0x5096e0),44))return 0;
 unsigned clock=MEM32(g_ecx+16);if(clock&&!mu_span(clock,8))return 0;
 unsigned frame=g_seh_ebp;g_esp-=24;g_xmm0=XMM_SCALAR(MEMF(g_esp+28));PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+4);SET_LO8(g_eax,MEM8(g_esi+22));g_xmm0.f[0]+=MEMF(g_esi+28);MEM8(g_esp+4)=!g_ecx;PUSH32(g_esp,g_edi);MEMF(g_esi+28)=g_xmm0.f[0];int done=0;
 if(!LO8(g_eax)){g_eax=MEM32(0x5096e0);g_ecx=MEM32(g_eax+40);g_eax=MEM32(g_esp+8);g_edx=0;SET_LO8(g_edx,MEM8(g_esi+21));PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x272bf5);sub_000643E0();g_ecx=(uint32_t)(int32_t)SMEM8(g_eax+59);PUSH32(g_esp,g_ecx);g_xmm0.f[0]=(float)(int32_t)g_ecx;g_xmm0.f[0]*=MEMF(0x44ff80);g_edx=g_esi+36;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_edx);PUSH32(g_esp,0x272c14);sub_00087960();MEM8(0x5154bc)=1;g_esp+=28;MEM8(g_esi+22)=1;g_edi=255;}
 else{
  int direct_fade=0;
  if(!(mu_cmp(g_xmm0.f[0],MEMF(g_esi+8))&0x41)){SET_LO8(g_eax,MEM8(g_esi+23));if(!LO8(g_eax)){SET_LO8(g_eax,MEM8(g_esi+24));if(LO8(g_eax))direct_fade=1;else{SET_LO8(g_eax,MEM8(g_esi+20));int all=LO8(g_eax)!=0;SET_LO8(g_eax,(uint32_t)(-(int32_t)LO8(g_eax)));PUSH32(g_esp,frame);g_eax=all?4:1;frame=g_eax;g_edi=0;do{PUSH32(g_esp,22);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x272c5c);sub_00274740();g_esp+=8;if(!LO8(g_eax)){PUSH32(g_esp,29);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x272c6b);sub_00274740();g_esp+=8;}if(LO8(g_eax)){if(MEM8(g_esi+12)&7)MEM8(g_esi+24)=1;else MEM8(g_esi+23)=1;}g_edi++;}while((int32_t)g_edi<(int32_t)frame);POP32(g_esp,frame);}}}
  if(!direct_fade)SET_LO8(g_eax,MEM8(g_esi+24));if(LO8(g_eax))movie_fade();else g_edi=255;
  SET_LO8(g_eax,MEM8(g_esi+23));done=LO8(g_eax)!=0;if(!done){SET_LO8(g_eax,MEM8(g_esp+8));if(!LO8(g_eax)){PUSH32(g_esp,0x272d72);sub_000877A0();done=LO8(g_eax)!=0;}}
 }
 if(done){PUSH32(g_esp,0x272f8c);sub_000877B0();g_eax=MEM32(g_esi+4);if(!g_eax)MEM8(0x5154bc)=0;g_eax=MEM32(g_esi+68);if(g_eax){SET_LO8(g_ecx,MEM8(g_esi+23));if(LO8(g_ecx))g_eax=MEM32(g_esi+4);}else g_eax=MEM32(g_esi+4);POP32(g_esp,g_edi);POP32(g_esp,g_esi);}
 else{g_eax=MEM32(g_esi+16);if(g_eax)movie_clock();POP32(g_esp,g_edi);g_eax=0;POP32(g_esp,g_esi);}
 g_esp+=24;g_recomp_zf=!g_esp;g_esp+=12;return 1;
}
#undef MU_TOP
#undef MU_POP
