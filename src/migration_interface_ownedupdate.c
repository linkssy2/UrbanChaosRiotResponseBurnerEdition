#include "migration_interface_ownedupdate.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002584C0(void),sub_00017180(void),sub_00017060(void),sub_00017730(void),sub_00017160(void),sub_00018CE0(void),sub_00017E40(void),sub_00254290(void);
static int ou_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
#define OU_TOP g_fp_stack[g_fp_top]
#define OU_POP() (g_fp_top=(g_fp_top+1)&7)
static void owned_hint(void){
 g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x2548e9);sub_00017160();g_edx=MEM32(g_esp+128);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,0x43d48000);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x25490a);sub_00018CE0();
 g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x25491f);sub_00017E40();OU_TOP=MEMF(g_esp+128)-OU_TOP;g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);OU_TOP-=MEMF(0x44c530);PUSH32(g_esp,0xffffffff);MEMF(g_esp+136)=(float)OU_TOP;OU_POP();PUSH32(g_esp,0x254945);sub_00017160();
 g_eax=MEM32(g_esp+128);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,129);PUSH32(g_esp,0x43d48000);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x254969);sub_00018CE0();g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x25497e);sub_00017E40();OU_TOP=MEMF(g_esp+128)-OU_TOP;g_edx=MEM32(g_esp+16);OU_TOP-=MEMF(0x44c240);MEMF(g_esp+128)=(float)OU_TOP;OU_POP();
}
int migration_interface_ownedupdate(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_OWNEDUPDATE");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!ou_span(s-1024,1040)||!ou_span(g_ecx,16)||!ou_span(MEM32(s+8),72)||!ou_span(MEM32(0x5096e0),44))return 0;
 unsigned child=MEM32(g_ecx+4);if(!ou_span(child,4)||!ou_span(MEM32(child),8))return 0;
 unsigned frame=g_seh_ebp;g_esp-=104;g_edx=MEM32(g_esp+108);unsigned saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+120);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);PUSH32(g_esp,frame);PUSH32(g_esp,g_edx);MEM32(g_esp+36)=g_esi;unsigned target=MEM32(g_eax+4);PUSH32(g_esp,0x2546cf);RECOMP_ICALL_SAFE(target,saved);
 g_edi=g_eax;g_eax=g_esp+124;PUSH32(g_esp,g_eax);PUSH32(g_esp,frame);MEM32(g_esp+32)=g_edi;PUSH32(g_esp,0x2546e0);sub_002584C0();OU_TOP*=MEMF(0x451290);g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_xmm0.f[0]-=MEMF(0x4859f0);g_ecx=MEM32(0x509704);MEMF(g_esp+28)=(float)OU_TOP;OU_POP();g_fp_top=(g_fp_top+7)&7;OU_TOP=MEMF(g_esp+132);g_ebx=MEM32(0x51cf5c);OU_TOP*=MEMF(0x451290);g_esp+=8;PUSH32(g_esp,0x44138000);PUSH32(g_esp,0x42480000);MEMF(g_esp+132)=(float)OU_TOP;OU_POP();MEMF(g_esp+136)=g_xmm0.f[0];PUSH32(g_esp,0x254735);sub_00017180();g_ecx=MEM32(0x509704);PUSH32(g_esp,1);PUSH32(g_esp,0x254742);sub_00017060();g_ecx=MEM32(g_esp+124);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x254752);sub_00017730();
 g_xmm0=XMM_SCALAR(MEMF(g_esp+128));g_eax=MEM32(frame+68);g_xmm0.f[0]+=MEMF(g_esp+20);g_edx=0;
 static const unsigned hints[]={1,0x4847e8,2,0x453414,3,0x453418,0,0x453410,8,0x4847e4,10,0x4847e0,9,0x4847dc,7,0x4847d8,4,0x3f2f5c,5,0x4847d4,6,0x4847d0};for(unsigned i=0;i<22;i++)MEM32(g_esp+32+i*4)=hints[i];MEMF(g_esp+128)=g_xmm0.f[0];
 if(g_eax){g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x254830);sub_00017160();g_edx=MEM32(g_esp+128);g_eax=MEM32(frame+68);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,0x43d48000);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x254854);sub_00018CE0();}
 else{
  MEM32(g_esp+16)=g_edx;
  do{g_edi=MEM32(frame+g_edx*4+24);if(g_edi){g_eax=0;while(g_eax<11&&MEM32(g_esp+g_eax*8+32)!=g_edx)g_eax++;
   if(g_eax<11){g_ecx=MEM32(0x5096e0);g_ecx=MEM32(g_ecx+40);unsigned mode=g_ecx;if(mode==1||mode==2){g_ecx=MEM32(g_esp+g_eax*8+32);if(g_ecx==5)g_esi=mode==1?0x4847cc:0x4847c4;else if(g_ecx==6)g_esi=mode==1?0x4847c8:0x4847c0;else g_esi=MEM32(g_esp+g_eax*8+36);}else g_esi=MEM32(g_esp+g_eax*8+36);if(g_esi)owned_hint();}
  }g_edx++;MEM32(g_esp+16)=g_edx;}while((int32_t)g_edx<11);
  g_edi=MEM32(g_esp+24);g_esi=MEM32(g_esp+28);
 }
 g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0x2549b9);sub_00017730();
 if(!(MEM32(g_esi+12)&0x20000)){g_edx=MEM32(g_esp+20);g_ecx=0;SET_LO8(g_ecx,MEM8(0x51cecc));PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,1);PUSH32(g_esp,0x43dd0000);PUSH32(g_esp,0x2549dc);sub_00254290();g_esp+=16;}
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=104;g_recomp_zf=!g_esp;g_esp+=12;return 1;
}
#undef OU_TOP
#undef OU_POP
