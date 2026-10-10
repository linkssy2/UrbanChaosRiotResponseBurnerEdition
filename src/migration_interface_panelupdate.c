#include "migration_interface_panelupdate.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273730(void),sub_00273660(void),sub_00087030(void),sub_00065760(void),sub_00065550(void);
#define PU_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
#define PU_FRAME(n,r) do{g_ebp=frame;PU_CALL(n,r);}while(0)
static unsigned pu_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static int pu_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void panelupdate_base(void){
 g_esp-=16;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+28);PUSH32(g_esp,g_edi);g_esi=g_ecx;PU_CALL(00273730,0x25d2d1);g_xmm1=XMM_ZERO();
 if(pu_cmp(g_xmm1.f[0],MEMF(g_esi+40))&1){
  g_xmm3=XMM_SCALAR(MEMF(g_esi+64));SET_HI8(g_eax,pu_cmp(g_xmm3.f[0],g_xmm1.f[0]));int first=!RECOMP_PARITY8(HI8(g_eax)&0x44);g_xmm0=g_xmm3;g_xmm0.f[0]+=MEMF(g_esp+28);MEMF(g_esi+64)=g_xmm0.f[0];g_eax=first?1:0;MEM8(g_esi+80)=LO8(g_eax);MEM8(g_esi+81)=0;
  g_xmm1=XMM_SCALAR(MEMF(g_esi+32));MEMF(g_esp+16)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+36));MEMF(g_esp+20)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+68));g_xmm2=g_xmm1;g_xmm1.f[0]+=MEMF(g_esi+32);MEMF(g_esp+8)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+36));g_xmm1.f[0]+=g_xmm2.f[0];MEMF(g_esp+12)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x48730c));int drawn=0;
  if(!(pu_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41)){
   g_eax=MEM32(g_esi+72);g_xmm0.f[0]*=MEMF(0x453014);PUSH32(g_esp,g_eax);g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);g_edx=g_esp+24;PUSH32(g_esp,g_edx);MEMF(g_esp+40)=g_xmm0.f[0];PU_CALL(00273660,0x25d370);g_xmm0=XMM_SCALAR(MEMF(g_esp+40));g_xmm0.f[0]*=MEMF(0x46c31c);g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_esp+=12;if(!(pu_cmp(g_xmm0.f[0],g_xmm1.f[0])&0x41))g_xmm0=g_xmm1;PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3f800000);g_ecx=g_esp+48;PU_CALL(00087030,0x25d3b1);g_edx=MEM32(g_esi+60);PUSH32(g_esp,g_eax);g_eax=g_esp+16;PUSH32(g_esp,g_eax);g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);
  }else{
   g_xmm2=XMM_SCALAR(MEMF(0x487310));
   if(!(pu_cmp(g_xmm2.f[0],g_xmm0.f[0])&0x41)){
    g_xmm0.f[0]-=g_xmm1.f[0];g_xmm0.f[0]*=MEMF(0x487374);g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm1.f[0]-=MEMF(g_esi+72);PUSH32(g_esp,g_ecx);g_xmm0.f[0]*=g_xmm1.f[0];g_xmm0.f[0]+=MEMF(g_esi+72);MEMF(g_esp)=g_xmm0.f[0];g_eax=g_esp+12;PUSH32(g_esp,g_eax);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);PU_CALL(00273660,0x25d409);PUSH32(g_esp,0);g_edx=g_esp+44;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+60);g_eax=g_esp+28;PUSH32(g_esp,g_eax);g_ecx=g_esp+40;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);MEM32(g_esp+60)=0xffffffff;PU_CALL(00065760,0x25d42b);g_esp+=32;drawn=1;
   }else{
    g_xmm1=XMM_SCALAR(MEMF(0x487314));if(!(pu_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41)&&!(pu_cmp(g_xmm2.f[0],g_xmm3.f[0])&0x41))MEM8(g_esi+81)=1;
    g_eax=MEM32(g_esi);g_ecx=g_esp+8;unsigned saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=g_esp+20;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_edi);g_ecx=g_esi;unsigned target=MEM32(g_eax+16);PUSH32(g_esp,0x25d458);RECOMP_ICALL_SAFE(target,saved);
    PUSH32(g_esp,0);g_eax=g_esp+32;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+60);g_ecx=g_esp+16;PUSH32(g_esp,g_ecx);g_edx=g_esp+28;PUSH32(g_esp,g_edx);MEM32(g_esp+44)=0xffffffff;PUSH32(g_esp,g_eax);
   }
  }
  if(!drawn){PU_CALL(00065760,0x25d47a);g_esp+=20;}
  g_xmm0=XMM_SCALAR(MEMF(g_esi+64));if(!(pu_cmp(g_xmm0.f[0],MEMF(0x487318))&0x41))MEM8(g_esi+82)=1;SET_LO8(g_eax,MEM8(g_esi+80));
  if(LO8(g_eax)){g_edx=MEM32(g_esi);g_ecx=g_esi;unsigned saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x25d49d);RECOMP_ICALL_SAFE(target,saved);}SET_LO8(g_eax,MEM8(g_esi+81));
  if(LO8(g_eax)){g_eax=MEM32(g_esi);g_ecx=g_esi;unsigned saved=g_esp,target=MEM32(g_eax+12);PUSH32(g_esp,0x25d4ab);RECOMP_ICALL_SAFE(target,saved);}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=!g_esp;g_esp+=8;
}
static void panelupdate_overlay(unsigned frame){
 g_esi=g_edi+116;MEM32(g_esp+16)=4;
 do{
  g_eax=MEM32(frame+12);g_xmm0=XMM_SCALAR(MEMF(g_eax));g_edx=MEM32(g_esi-4);MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+4));PUSH32(g_esp,g_edx);MEMF(g_esp+32)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ebx));g_eax=g_esp+36;PUSH32(g_esp,g_eax);g_ecx=g_esp+32;MEMF(g_esp+40)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ebx+4));PUSH32(g_esp,g_ecx);MEMF(g_esp+48)=g_xmm0.f[0];PU_FRAME(00273660,0x25da80);
  g_edx=MEM32(g_esi-8);g_xmm0=XMM_SCALAR(MEMF(g_esi+4));g_xmm0.f[0]*=MEMF(frame+8);g_xmm0.f[0]+=MEMF(g_esi);g_esp+=12;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3f800000);g_ecx=g_esp+36;MEMF(g_esi)=g_xmm0.f[0];PU_FRAME(00087030,0x25dab1);
  g_eax=MEM32(g_esi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);g_edx=g_esi-16;PUSH32(g_esp,g_edx);g_eax=g_esi-24;PUSH32(g_esp,g_eax);g_eax=MEM32(g_edi+220);g_ecx=g_esp+52;PUSH32(g_esp,g_ecx);g_edx=g_esp+48;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PU_FRAME(00065550,0x25dad9);g_eax=MEM32(g_esp+48);g_esp+=32;g_esi+=32;g_eax--;MEM32(g_esp+16)=g_eax;
 }while(g_eax);
}
int migration_interface_panelupdate(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_PANELUPDATE");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;
 unsigned s=g_esp;if(s<0x10800||(s&3)||!pu_span(s-2048,2080)||!pu_span(g_ecx,op?232:84))return 0;if(!op){if(!pu_span(MEM32(g_ecx),20))return 0;panelupdate_base();return 1;}if(!pu_span(MEM32(s+8),8)||!pu_span(MEM32(s+12),8))return 0;
 PUSH32(g_esp,g_ebp);unsigned frame=g_esp;g_ebp=frame;g_esp&=~7u;g_esp-=28;g_xmm0=XMM_SCALAR(MEMF(0x487314));PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+16);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;
 if(!(pu_cmp(g_xmm0.f[0],MEMF(g_edi+64))&0x41)){
  g_eax=MEM32(frame+12);g_xmm1=XMM_SCALAR(MEMF(g_eax));g_xmm0=XMM_SCALAR(MEMF(g_edi+64));g_xmm0.f[0]-=MEMF(0x487310);g_xmm0.f[0]*=MEMF(0x44c72c);MEMF(g_esp+32)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_eax+4));PUSH32(g_esp,g_ecx);MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0.f[0]+=MEMF(0x44bf94);MEMF(g_esp+40)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_ebx));MEMF(g_esp)=g_xmm0.f[0];g_eax=g_esp+28;PUSH32(g_esp,g_eax);g_ecx=g_esp+40;MEMF(g_esp+32)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_ebx+4));PUSH32(g_esp,g_ecx);MEMF(g_esp+40)=g_xmm1.f[0];PU_FRAME(00273660,0x25d9d8);
  g_xmm0=XMM_SCALAR(MEMF(g_esp+28));g_xmm0.f[0]*=MEMF(0x44c5ec);g_xmm0.f[0]+=MEMF(0x48730c);g_esp+=12;PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3f800000);g_ecx=g_esp+40;PU_FRAME(00087030,0x25da11);g_ecx=MEM32(g_edi+60);PUSH32(g_esp,g_eax);g_edx=g_esp+32;PUSH32(g_esp,g_edx);g_eax=g_esp+44;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PU_FRAME(00065760,0x25da25);g_esp+=20;
 }
 SET_LO8(g_eax,MEM8(g_edi+88));if(LO8(g_eax))panelupdate_overlay(frame);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=16;return 1;
}
