#include "migration_interface_waitscreen.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00087030(void),sub_000658C0(void),sub_000656D0(void),sub_00065760(void),sub_00017060(void),sub_00017160(void),sub_00017180(void),sub_00018CE0(void);
#define WAIT_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
static unsigned wait_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static void wait_background(void){
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));MEMF(g_esp+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x485cd4));g_eax=g_esp+24;PUSH32(g_esp,g_eax);MEMF(g_esp+12)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_ecx=g_esp+8;PUSH32(g_esp,g_ecx);g_edx=g_esp+20;MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x485cd0));PUSH32(g_esp,g_edx);MEM32(g_esp+36)=0xff000000;MEMF(g_esp+28)=g_xmm0.f[0];WAIT_CALL(000656D0,0x25ac32);
 g_xmm0=XMM_SCALAR(MEMF(0x4e8394));MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x485ccc));g_eax=g_esp+36;PUSH32(g_esp,g_eax);MEMF(g_esp+32)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);g_edx=g_esp+24;MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x485cc8));PUSH32(g_esp,g_edx);MEM32(g_esp+48)=0xff000000;MEMF(g_esp+32)=g_xmm0.f[0];WAIT_CALL(000656D0,0x25ac81);
 for(unsigned i=0;i<2;i++){unsigned base=i?20:0;g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_edx=MEM32(0x51cf20);PUSH32(g_esp,0);MEMF(g_esp+40+base)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(i?0x485cc4:0x485cd4));PUSH32(g_esp,0x51cf30);g_eax=g_esp+44+base;PUSH32(g_esp,g_eax);MEMF(g_esp+52+base)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_ecx=g_esp+40+base;PUSH32(g_esp,g_ecx);MEMF(g_esp+44+base)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(i?0x485cc8:0x44c260));PUSH32(g_esp,g_edx);MEMF(g_esp+52+base)=g_xmm0.f[0];PUSH32(g_esp,i?0x25ad21:0x25acd1);sub_00065760();}g_esp+=64;
}
int migration_interface_waitscreen(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_WAITSCREEN");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation||g_esp<0x10400||(g_esp&3)||(uint64_t)g_esp+16>g_xbox_total_ram||g_ecx<0x10000||(uint64_t)g_ecx+36>g_xbox_total_ram)return 0;
 g_esp-=16;PUSH32(g_esp,g_esi);g_esi=g_ecx;SET_LO8(g_eax,MEM8(g_esi+32));if(!LO8(g_eax)){unsigned saved=g_esp,target=MEM32(g_esi+8);PUSH32(g_esp,0x25addf);RECOMP_ICALL_SAFE(target,saved);goto finish;}
 SET_LO8(g_eax,MEM8(g_esi+24));g_xmm2=XMM_SCALAR(MEMF(g_esp+24));g_xmm0=g_xmm2;g_xmm0.f[0]*=MEMF(0x46c320);if(LO8(g_eax)){g_xmm0.f[0]+=MEMF(g_esi+20);MEMF(g_esi+20)=g_xmm0.f[0];}else{g_xmm1=XMM_SCALAR(MEMF(g_esi+20));g_xmm1.f[0]-=g_xmm0.f[0];MEMF(g_esi+20)=g_xmm1.f[0];}
 g_xmm0=XMM_SCALAR(MEMF(g_esi+20));g_xmm1=XMM_SCALAR(MEMF(0x44bf94));if(!(wait_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41)){g_xmm1=XMM_ZERO();if(wait_cmp(g_xmm0.f[0],g_xmm1.f[0])&0x41)g_xmm0=g_xmm1;}else g_xmm0=g_xmm1;
 MEMF(g_esi+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+16));g_xmm0.f[0]+=g_xmm2.f[0];MEMF(g_esi+16)=g_xmm0.f[0];if(!LO8(g_eax)&&!(wait_cmp(g_xmm0.f[0],MEMF(0x46c31c))&0x41)){unsigned saved=g_esp,target=MEM32(g_esi+8);PUSH32(g_esp,0x25abb3);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+28)=g_eax;MEM8(g_esi+24)=1;}
 SET_LO8(g_eax,MEM8(g_esi+14));if(LO8(g_eax)){PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_esp+40;WAIT_CALL(00087030,0x25abd5);PUSH32(g_esp,g_eax);WAIT_CALL(000658C0,0x25abdb);g_esp+=4;}else wait_background();
 g_ecx=MEM32(0x509704);PUSH32(g_esp,2);WAIT_CALL(00017060,0x25ad31);g_ecx=MEM32(0x509704);PUSH32(g_esp,0xff000000);PUSH32(g_esp,0xffffffff);WAIT_CALL(00017160,0x25ad43);g_xmm0=XMM_SCALAR(MEMF(0x4e8394));g_xmm0.f[0]-=MEMF(0x4859f0);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,0x42480000);WAIT_CALL(00017180,0x25ad69);
 g_eax=MEM32(0x4e83a0);g_ecx=MEM32(0x4e839c);g_edx=MEM32(g_esi+4);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,18);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,g_edx);WAIT_CALL(00018CE0,0x25ad8d);g_eax=MEM32(g_esi+20);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_esp+40;WAIT_CALL(00087030,0x25ada0);PUSH32(g_esp,g_eax);WAIT_CALL(000658C0,0x25ada6);
 SET_LO8(g_eax,MEM8(g_esi+24));g_esp+=4;if(LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(g_esi+20));SET_HI8(g_eax,wait_cmp(g_xmm0.f[0],MEMF(0x44bf94)));if(!RECOMP_PARITY8(HI8(g_eax)&0x44)){MEM8(0x5154bc)=0;g_eax=MEM32(g_esi+28);goto finish;}}g_eax=0;
finish: POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=!g_esp;g_esp+=12;return 1;
}
