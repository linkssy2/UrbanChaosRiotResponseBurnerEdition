#include "migration_interface_dialogbuild.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_000176D0(void),sub_00018190(void),sub_000650D0(void),sub_00072340(void),sub_00273410(void),sub_00273590(void),sub_002D3780(void),sub_002D57FB(void);
#define DB_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
#define DB_FP g_fp_stack[g_fp_top]
static void dialogbuild_pop(unsigned out){MEMF(out)=(float)DB_FP;g_fp_top=(g_fp_top+1)&7;}
static unsigned dialogbuild_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static int dialogbuild_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void dialogbuild_caption(unsigned kind){
 /* Three original caption layouts share one constructor ABI; stack slots and return sites remain distinct. */
 unsigned slot=kind==2?36:32,xy=kind==0?56:kind==1?64:72;
 DB_CALL(002D57FB,kind==0?0x282d86:kind==1?0x282e12:0x282e8d);g_esp+=4;MEM32(g_esp+slot)=g_eax;MEM32(g_esp+88)=kind+2;
 if(g_eax!=g_ebx){
  g_ecx=kind==0?MEM32(0x5031b8):MEM32(g_esp+108);g_xmm0=XMM_SCALAR(MEMF(kind==0?0x48babc:g_esp+96));g_edx=MEM32(0x51cf5c);PUSH32(g_esp,0x44d9b8);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edi+16);PUSH32(g_esp,kind==0?28:kind==1?12:13);PUSH32(g_esp,g_edx);MEMF(g_esp+xy+16)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(kind==0?0x48bac4:g_esp+116));PUSH32(g_esp,5);MEMF(g_esp+xy+24)=g_xmm0.f[0];g_edx=MEM32(g_ecx+4);g_ecx=MEM32(g_esp+132);PUSH32(g_esp,g_edx);PUSH32(g_esp,kind==2?7:6);PUSH32(g_esp,g_ecx);g_edx=g_esp+xy+32;PUSH32(g_esp,g_edx);g_ecx=g_eax;DB_CALL(00273590,kind==0?0x282ded:kind==1?0x282e6f:0x282eea);
 }else g_eax=0;
 MEM32(g_esp+slot)=g_eax;g_edx=g_esp+slot;
}
static void dialogbuild_sprite(unsigned *frame){
 PUSH32(g_esp,80);DB_CALL(002D57FB,0x282c94);g_esi=g_eax;g_esp+=4;MEM32(g_esp+32)=g_esi;MEM32(g_esp+88)=g_ebx;
 if(g_esi!=g_ebx){
  g_xmm0=XMM_SCALAR(MEMF(0x50321c));g_eax=MEM32(g_esp+112);g_edx=MEM32(g_edi+16);*frame=MEM32(g_edx+4);g_ebx=MEM32(0x51cf94);PUSH32(g_esp,0x3f800000);MEMF(g_esp+44)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x503220));PUSH32(g_esp,6);MEMF(g_esp+52)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x503218));PUSH32(g_esp,g_eax);g_ecx=g_esp+60;MEMF(g_esp+60)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x5031ec));PUSH32(g_esp,g_ecx);g_ecx=g_esi;MEMF(g_esp+68)=g_xmm0.f[0];DB_CALL(00273410,0x282d05);
  PUSH32(g_esp,*frame);MEM8(g_esp+92)=1;MEM32(g_esi)=0x452f1c;DB_CALL(000650D0,0x282d16);g_xmm0=XMM_SCALAR(MEMF(g_esp+44));MEM32(g_esi+60)=g_eax;MEMF(g_esi+64)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+48));MEMF(g_esi+68)=g_xmm0.f[0];g_esp+=4;MEM32(g_esi+72)=g_ebx;MEM8(g_esi+76)=0;g_ebx=0;
 }else g_esi=0;
 MEM32(g_esp+32)=g_esi;g_edx=g_esp+32;
}
static void dialogbuild_rows(unsigned *frame){
 for(;;){
  g_ecx=MEM32(g_edi+16);g_eax=MEM32(g_ecx);if(g_eax==5)break;
  unsigned type=g_eax;g_xmm0=XMM_SCALAR(MEMF(type==g_ebx?0x5031bc:0x5031b8));MEMF(g_esp+108)=g_xmm0.f[0];if(type==1){g_xmm0=XMM_SCALAR(MEMF(0x5031c0));MEMF(g_esp+108)=g_xmm0.f[0];}
  if(type==1||type==3){g_ecx=MEM32(0x509704);PUSH32(g_esp,5);DB_CALL(000176D0,0x282bd6);DB_FP*=MEMF(g_esp+108);DB_FP+=MEMF(g_esp+20);dialogbuild_pop(g_esp+20);}
  g_edx=MEM32(g_edi+16);g_eax=MEM32(g_edx);type=g_eax;g_xmm0=XMM_SCALAR(MEMF(0x5031b4));MEMF(g_esp+112)=g_xmm0.f[0];
  if(type==2||type==3){
   g_eax=MEM32(0x5031d4);g_ecx=MEM32(0x5031d0);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);DB_CALL(002D3780,0x282c15);DB_FP+=MEMF(g_esp+120);g_edx=MEM32(g_edi+16);g_eax=MEM32(g_edx+4);g_ecx=MEM32(0x509704);dialogbuild_pop(g_esp+120);g_esp+=8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);DB_CALL(00018190,0x282c33);
   g_xmm0.f[0]=(float)(int32_t)g_eax;g_xmm0.f[0]*=MEMF(g_esp+108);if(!(dialogbuild_cmp(g_xmm0.f[0],MEMF(g_esp+24))&0x41))MEMF(g_esp+24)=g_xmm0.f[0];g_eax=MEM32(g_esp+16)+1;g_ecx=MEM32(g_esp+28);MEM32(g_esp+16)=g_eax;
   if((int32_t)g_eax>(int32_t)g_ecx){g_xmm0=XMM_SCALAR(MEMF(g_esp+20));MEMF(g_esp+100)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x5031cc));g_xmm0.f[0]+=MEMF(g_esp+96);MEM32(g_esp+16)=g_ebx;MEMF(g_esp+96)=g_xmm0.f[0];}
  }
  g_ecx=MEM32(g_edi+16);g_eax=MEM32(g_ecx);
  if(g_eax==11)dialogbuild_sprite(frame);
  else{type=g_eax;PUSH32(g_esp,180);if(type==4){g_xmm0=XMM_SCALAR(MEMF(0x48babc));MEMF(g_esp+100)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x48bac4));MEMF(g_esp+104)=g_xmm0.f[0];dialogbuild_caption(0);}else{g_xmm0=XMM_SCALAR(MEMF(g_esp+108));SET_HI8(g_eax,dialogbuild_cmp(g_xmm0.f[0],MEMF(0x44c174)));dialogbuild_caption(RECOMP_PARITY8(HI8(g_eax)&0x44)?1:2);}}
  g_eax=MEM32(g_edi+56);g_ecx=g_edi+52;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);MEM32(g_esp+96)=0xffffffff;DB_CALL(00072340,0x282f0b);g_ecx=MEM32(0x509704);PUSH32(g_esp,5);DB_CALL(000176D0,0x282f18);DB_FP*=MEMF(g_esp+108);g_edx=MEM32(g_edi+16)+12;DB_FP+=MEMF(g_esp+100);g_eax=g_edx;MEM32(g_edi+16)=g_edx;g_ecx=MEM32(g_eax);dialogbuild_pop(g_esp+100);if(g_ecx==6)break;
 }
}
int migration_interface_dialogbuild(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_DIALOGBUILD");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned s=g_esp;if(s<0x10800||(s&3)||!dialogbuild_span(s-2048,2080)||!dialogbuild_span(g_ecx,64))return 0;unsigned p=MEM32(g_ecx+16),i=0;for(;i<4096;i++){if(!dialogbuild_span((uint64_t)p+i*12,12))return 0;unsigned t=MEM32(p+i*12);if(t==6||t==10)break;}if(i==4096||!MEM32(0x5031c4))return 0;
 unsigned frame=g_seh_ebp;g_eax=MEM32(0);PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x3a0344);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=64;PUSH32(g_esp,g_edi);g_edi=g_ecx;
 for(;;){g_ecx=MEM32(g_edi+16);g_eax=MEM32(g_ecx);if(g_eax!=6&&g_eax!=5)break;g_ecx+=12;MEM32(g_edi+16)=g_ecx;}
 g_eax=MEM32(g_edi+16);
 if(MEM32(g_eax)!=10){
  g_xmm0=XMM_SCALAR(MEMF(g_esp+96));SET_HI8(g_eax,dialogbuild_cmp(g_xmm0.f[0],MEMF(0x44c174)));int centered=!RECOMP_PARITY8(HI8(g_eax)&0x44);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+108);
  if(centered){g_ecx=MEM32(0x509704);PUSH32(g_esp,5);DB_CALL(000176D0,0x282ac9);g_edx=MEM32(g_edi+16);g_eax=MEM32(g_edx);g_ecx=g_esi-1;MEM32(g_esp+104)=g_ecx;DB_FP*=(double)SMEM32(g_esp+104);g_ecx=MEM32(0x509704);g_ebx=0;int first=g_eax==g_ebx;PUSH32(g_esp,5);DB_FP*=MEMF(0x5031b8);DB_FP=MEMF(g_esp+100)-DB_FP;dialogbuild_pop(g_esp+100);DB_CALL(000176D0,first?0x282afa:0x282b0f);DB_FP*=MEMF(first?0x5031bc:0x5031b8);DB_FP=MEMF(g_esp+96)-DB_FP;dialogbuild_pop(g_esp+96);}else g_ebx=0;
  g_xmm0=XMM_SCALAR(MEMF(g_esp+96));g_ecx=MEM32(0x5031c4);MEMF(g_esp+16)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44c530));MEM32(g_esp+24)=g_esi;
  if((int32_t)g_esi>(int32_t)g_ecx){g_xmm2=XMM_SCALAR(MEMF(0x44bf94));g_xmm1.f[0]=(float)(int32_t)g_esi;do{g_xmm3=g_xmm1;g_xmm3.f[0]/=g_xmm0.f[0];g_eax=(int32_t)g_xmm3.f[0];MEM32(g_esp+24)=g_eax;g_xmm0.f[0]+=g_xmm2.f[0];}while((int32_t)g_eax>(int32_t)g_ecx);}
  g_eax=MEM32(g_edi+16);int rows=MEM32(g_eax)!=6;g_xmm0=XMM_ZERO();MEM32(g_esp+12)=g_ebx;MEMF(g_esp+20)=g_xmm0.f[0];if(rows){PUSH32(g_esp,frame);dialogbuild_rows(&frame);POP32(g_esp,frame);}POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
 }
 g_ecx=MEM32(g_esp+68);POP32(g_esp,g_edi);MEM32(0)=g_ecx;g_esp+=76;g_recomp_zf=!g_esp;g_esp+=24;return 1;
}
