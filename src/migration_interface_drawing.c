#include "migration_interface_drawing.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_0005AAB0(void),sub_00017E40(void),sub_00018130(void),sub_00017160(void),sub_00018CE0(void),ucrr_menu_row_bounds(void);
#define DCALL(a,r) do{PUSH32(g_esp,r);sub_##a();RECOMP_TRACE_ESP("sub_002641C0","after call 0x" #a);}while(0)
#define DTOP g_fp_stack[g_fp_top]
#define DNEXT g_fp_stack[(g_fp_top+1)&7]
#define DPOP() (g_fp_top=(g_fp_top+1)&7)
#define DPUSH(v) do{double value=(v);g_fp_top=(g_fp_top+7)&7;DTOP=value;}while(0)
static unsigned drawing_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static int drawing_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void drawing_actor(unsigned offset,unsigned ret){g_eax=MEM32(g_eax+324);g_edx=MEM32(g_eax);g_ecx=g_eax;unsigned saved=g_esp;PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(MEM32(g_edx+offset),saved);}
int migration_interface_drawing(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_DRAWING");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||(unsigned)g_fp_top>7||!drawing_span(s-1024,1048)||!drawing_span(g_ecx,32)||!drawing_span(0x51cf80,4))return 0;
 unsigned frame=g_seh_ebp;g_esp-=12;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;SET_LO8(g_eax,MEM8(g_edi+17));MEM8(g_esp+28)=0;
 if(LO8(g_eax)){DCALL(0005AAB0,0x2641d9);g_ecx=MEM32(g_eax+324);if(g_ecx){DCALL(0005AAB0,0x2641e8);drawing_actor(236,0x2641f8);if(LO8(g_eax)){DCALL(0005AAB0,0x264201);drawing_actor(384,0x264211);int on=!!LO8(g_eax);if(!on){DCALL(0005AAB0,0x26421a);drawing_actor(388,0x26422a);on=!!LO8(g_eax);}if(on)MEM8(g_esp+28)=1;}}}
 SET_LO8(g_ecx,MEM8(g_esp+36));g_eax=LO8(g_ecx)?0x51cf5c:0x51cf38;g_ebx=MEM32(g_eax);MEM32(g_esp+12)=g_ebx;g_eax=LO8(g_ecx)?0x51cf38:0x51cf80;g_eax=MEM32(g_eax);MEM32(g_esp+16)=g_eax;SET_LO8(g_eax,MEM8(g_esp+28));
 if(LO8(g_eax)){DCALL(0005AAB0,0x26426c);g_xmm0=XMM_SCALAR(MEMF(g_eax+336));g_ecx=(int32_t)g_xmm0.f[0];g_xmm1.f[0]=(float)(int32_t)g_ecx;g_xmm0.f[0]-=g_xmm1.f[0];int flash=0;
  if(!(drawing_cmp(g_xmm0.f[0],MEMF(0x44c174))&1)){g_xmm1=XMM_SCALAR(MEMF(0x44d660));if(!(drawing_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41))flash=1;}
  if(!flash&&!(drawing_cmp(g_xmm0.f[0],MEMF(0x44c1f8))&1)){g_xmm1=XMM_SCALAR(MEMF(0x44c47c));if(!(drawing_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41))flash=1;}
  if(flash){g_ebx=MEM32(0x51cf60);MEM32(g_esp+12)=g_ebx;MEM32(g_esp+16)=g_ebx;}
 }
 g_edx=MEM32(g_edi+8);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);DCALL(00017E40,0x2642d2);MEMF(g_esp+20)=(float)DTOP;DPOP();g_xmm0=XMM_SCALAR(MEMF(0x4875ec));g_esi=g_ebx;MEMF(g_esp+28)=g_xmm0.f[0];
 int animate=0;if(MEM8(g_esp+40)&2){SET_LO8(g_eax,MEM8(g_esp+36));animate=!!LO8(g_eax);}
 if(animate){unsigned cmp=drawing_cmp(g_xmm0.f[0],MEMF(g_edi+24));g_xmm3=XMM_SCALAR(MEMF(0x44c530));
  if(!(cmp&0x41)){g_xmm1=XMM_SCALAR(MEMF(g_edi+24));g_xmm1.f[0]*=MEMF(0x4876dc);g_xmm2=XMM_SCALAR(MEMF(0x4876d8));g_xmm1.f[0]+=g_xmm0.f[0];cmp=drawing_cmp(g_xmm1.f[0],g_xmm2.f[0]);MEMF(g_esp+28)=g_xmm1.f[0];if(!(cmp&1))MEMF(g_esp+28)=g_xmm2.f[0];}
  else if(!(drawing_cmp(g_xmm3.f[0],MEMF(g_edi+24))&0x41)){g_xmm1=XMM_SCALAR(MEMF(g_edi+24));g_xmm1.f[0]-=g_xmm0.f[0];g_xmm1.f[0]*=MEMF(0x4876d4);g_xmm1.f[0]+=MEMF(0x4876d8);cmp=drawing_cmp(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_esp+28)=g_xmm1.f[0];if(!(cmp&0x41))MEMF(g_esp+28)=g_xmm0.f[0];}
  g_xmm1=XMM_SCALAR(MEMF(g_edi+24));if(!(drawing_cmp(g_xmm1.f[0],MEMF(0x4875e8))&1)){
   g_xmm1=XMM_SCALAR(MEMF(g_edi+28));g_xmm2=g_xmm1;if(!(drawing_cmp(g_xmm2.f[0],MEMF(0x44c1f8))&0x41)){g_xmm1=g_xmm0;g_xmm1.f[0]-=g_xmm2.f[0];}
   g_eax=MEM8(g_esp+14);g_ecx=HI8(g_ebx);g_esi=g_ecx-80;g_ecx-=g_esi;g_xmm1.f[0]*=g_xmm3.f[0];g_xmm0.f[0]-=g_xmm1.f[0];g_ebx&=255;g_xmm1.f[0]=(float)(int32_t)g_ecx;g_edx=g_ebx;g_ebx=g_eax-80;g_xmm1.f[0]*=g_xmm0.f[0];g_eax-=g_ebx;g_ecx=(int32_t)g_xmm1.f[0];g_ecx+=g_esi;g_xmm1.f[0]=(float)(int32_t)g_eax;PUSH32(g_esp,frame);frame=g_edx-80;g_xmm1.f[0]*=g_xmm0.f[0];g_ecx<<=8;g_eax=(int32_t)g_xmm1.f[0];g_edx-=frame;g_eax+=g_ebx;g_esi=g_ecx;g_eax<<=16;g_xmm1.f[0]=(float)(int32_t)g_edx;g_xmm1.f[0]*=g_xmm0.f[0];g_esi|=g_eax;g_ecx=(int32_t)g_xmm1.f[0];g_ecx+=frame;g_esi|=g_ecx;g_esi|=0xff000000;RECOMP_TRACE_ESP("sub_002641C0","pop ebp");POP32(g_esp,frame);
  }else{g_xmm0=XMM_ZERO();MEMF(g_edi+28)=g_xmm0.f[0];}
 }else{g_xmm0=XMM_ZERO();MEMF(g_edi+24)=g_xmm0.f[0];MEMF(g_edi+28)=g_xmm0.f[0];}
 g_edx=MEM32(g_edi+8);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);DCALL(00017E40,0x26441e);MEMF(g_esp+36)=(float)DTOP;DPOP();ucrr_menu_row_bounds();g_ebx=MEM32(g_esp+28);g_eax=MEM32(g_edi+8);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);DCALL(00017E40,0x26443a);
 DTOP=MEMF(g_esp+36)-DTOP;g_ecx=MEM32(0x509704);DTOP*=MEMF(0x44c1f8);MEMF(g_esp+12)=(float)DTOP;DPOP();DCALL(00018130,0x264453);DTOP*=MEMF(g_esp+28);g_ecx=MEM32(0x509704);MEMF(g_esp+36)=(float)DTOP;DPOP();DCALL(00018130,0x264466);DTOP=MEMF(g_esp+36)-DTOP;g_eax=MEM32(g_edi+12);g_ecx=MEM32(0x509704);DTOP*=MEMF(0x44c1f8);if(!g_eax)g_esi=MEM32(g_esp+16);PUSH32(g_esp,0xff000000);MEMF(g_esp+40)=(float)DTOP;DPOP();PUSH32(g_esp,g_esi);DCALL(00017160,0x26448f);
 g_xmm0=XMM_SCALAR(MEMF(g_esp+32));g_xmm0.f[0]-=MEMF(g_esp+36);g_ecx=MEM32(g_edi+8);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);PUSH32(g_esp,4);g_esp-=8;MEMF(g_esp+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+36));g_xmm0.f[0]+=MEMF(g_esp+68);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);DCALL(00018CE0,0x2644cb);
 g_edx=MEM32(g_edi+8);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);DCALL(00017E40,0x2644e3);DTOP+=MEMF(g_esp+44);int marker=MEM8(g_esp+40)&1;RECOMP_TRACE_ESP("sub_002641C0","pop edi");POP32(g_esp,g_edi);RECOMP_TRACE_ESP("sub_002641C0","pop esi");POP32(g_esp,g_esi);RECOMP_TRACE_ESP("sub_002641C0","pop ebx");POP32(g_esp,g_ebx);
 if(marker){DPUSH(MEMF(0x4859f4));g_eax=MEM32(0x51cf2c);g_ecx=MEM32(0x509704);DTOP+=DNEXT;PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_eax);MEMF(g_esp+24)=(float)DTOP;DPOP();DPOP();DCALL(00017160,0x264515);g_ecx=MEM32(g_esp+20);g_edx=MEM32(g_esp+16);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x48762c);DCALL(00018CE0,0x264537);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x48762c);DCALL(00017E40,0x264550);DPUSH(MEMF(0x4859f4));DTOP+=DTOP;DNEXT+=DTOP;DPOP();DTOP+=MEMF(g_esp+8);
 }else{DPOP();DPUSH(MEMF(g_esp+8));}
 g_esp+=12;g_recomp_zf=!g_esp;RECOMP_TRACE_EXIT("sub_002641C0",0x2641c0);g_esp+=24;return 1;
}
#undef DCALL
#undef DTOP
#undef DNEXT
#undef DPOP
#undef DPUSH
