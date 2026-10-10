#include "hybrid_fx_electricity_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_00218A60(void),sub_00218B30(void),sub_00218C40(void),sub_0013BE30(void);
extern void sub_001F8DC0(void),sub_001F9010(void),sub_00208C50(void),sub_00219530(void),sub_00219470(void);
extern void sub_002D3780(void),sub_002D38F0(void),sub_0020A140(void),sub_00055EE0(void),sub_000236D0(void),sub_00022A10(void);
extern void sub_00190DF0(void),sub_00191210(void),sub_00191410(void),sub_002D57FB(void),sub_002D5CCD(void);
static int electricity_enabled(void){static int flag=-1;if(flag<0){const char*p=getenv("UCRR_NATIVE_FX_ELECTRICITY");flag=p&&!strcmp(p,"1");}return flag;}
static int electricity_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int electricity_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define ELECTRIC_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define ELECTRIC_FRAME_CALL(fn,pc) do{g_ebp=frame;ELECTRIC_CALL(fn,pc);}while(0)
#define ELECTRIC_VCALL(target,pc,saved) do{uint32_t electric_target=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(electric_target,saved);}while(0)

/* Callback results, owner registers and saved words are borrowed live. No
 * rejection or replay is possible after the first original effect. */
static void electricity_construct(void){
 uint32_t frame=g_ebp;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x3966c8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x78;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+0x10)=g_ebx;ELECTRIC_FRAME_CALL(sub_00218A60,0x190e1c);
 g_esi=MEM32(frame+8);g_eax=0;MEM32(g_ebx)=0x474410;MEM32(g_ebx+0x110)=g_eax;MEM32(g_esp+0x8c)=g_eax;MEM32(g_ebx+0x124)=g_eax;
 g_eax=g_esp+0x20;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x462428);PUSH32(g_esp,4);g_ecx=g_esi;ELECTRIC_FRAME_CALL(sub_001F8DC0,0x190e4d);
 if(!LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(0x458800));g_xmm1=XMM_SCALAR(MEMF(0x44c5e8));MEMF(g_esp+0x14)=g_xmm0.f[0];MEMF(g_esp+0x18)=g_xmm1.f[0];}
 else{g_xmm0=XMM_SCALAR(MEMF(g_esp+0x20));MEMF(g_esp+0x14)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x24));MEMF(g_esp+0x18)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x28));}
 g_edx=MEM32(g_esp+0x14);g_eax=MEM32(g_esp+0x18);g_edi=g_ebx+0xd4;g_ecx=g_edi;MEM32(g_ecx)=g_edx;MEM32(g_ecx+4)=g_eax;g_eax=g_esp+12;PUSH32(g_esp,g_eax);MEMF(g_esp+0x20)=g_xmm0.f[0];g_edx=MEM32(g_esp+0x20);PUSH32(g_esp,0x4622e4);MEM32(g_ecx+8)=g_edx;PUSH32(g_esp,3);g_ecx=g_esi;ELECTRIC_FRAME_CALL(sub_001F8DC0,0x190ec2);
 if(!LO8(g_eax))SET_LO8(g_eax,1);else{g_eax=MEM32(g_esp+12);g_eax&=255;SET_LO8(g_eax,g_eax!=0);}
 g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x474404);PUSH32(g_esp,9);g_ecx=g_esi;MEM8(g_ebx+0xd0)=LO8(g_eax);MEM32(g_esp+0x18)=0;ELECTRIC_FRAME_CALL(sub_001F8DC0,0x190ef7);
 g_edx=MEM32(g_esp+12);g_eax=g_esp+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4743f0);PUSH32(g_esp,2);g_ecx=g_esi;MEM32(g_ebx+0x110)=g_edx;ELECTRIC_FRAME_CALL(sub_001F8DC0,0x190f14);
 {int missing=!LO8(g_eax);g_xmm0=XMM_SCALAR(MEMF(0x44bf94));if(!missing)g_xmm0=XMM_SCALAR(MEMF(g_esp+12));}
 g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x4743dc);PUSH32(g_esp,2);g_ecx=g_esi;MEMF(g_ebx+0x114)=g_xmm0.f[0];ELECTRIC_FRAME_CALL(sub_001F8DC0,0x190f41);
 {int missing=!LO8(g_eax);g_xmm0=XMM_SCALAR(MEMF(0x46c31c));if(!missing)g_xmm0=XMM_SCALAR(MEMF(g_esp+12));}
 g_eax=MEM32(g_ebx+0x114);MEMF(g_esp+12)=g_xmm0.f[0];g_edx=MEM32(g_esp+12);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);MEMF(g_ebx+0x118)=g_xmm0.f[0];ELECTRIC_FRAME_CALL(sub_002D3780,0x190f72);
 MEMF(g_ebx+0x11c)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;g_esp+=8;
 g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x4622d0);PUSH32(g_esp,3);g_ecx=g_esi;ELECTRIC_FRAME_CALL(sub_001F8DC0,0x190f8e);
 if(!LO8(g_eax))SET_LO8(g_eax,1);else{g_eax=MEM32(g_esp+12);g_eax&=255;SET_LO8(g_eax,g_eax!=0);}
 g_edx=g_esp+12;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x4743cc);PUSH32(g_esp,3);g_ecx=g_esi;MEM8(g_ebx+0x120)=LO8(g_eax);ELECTRIC_FRAME_CALL(sub_001F8DC0,0x190fbb);
 if(LO8(g_eax)){g_eax=MEM32(g_esp+12);g_eax&=255;SET_LO8(g_eax,g_eax!=0);}
 MEM8(g_ebx+0x121)=LO8(g_eax);g_eax=g_esp+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x44ca30);PUSH32(g_esp,6);g_ecx=g_esi;MEM32(g_esp+0x18)=0;ELECTRIC_FRAME_CALL(sub_001F8DC0,0x190fec);
 SET_LO8(g_eax,MEM8(g_ebx+0x120));int visible=LO8(g_eax)!=0;g_ecx=MEM32(g_esp+12);MEM32(g_ebx+0x124)=g_ecx;
 if(visible){
  g_ecx=MEM32(0x51c4dc);ELECTRIC_FRAME_CALL(sub_00208C50,0x19100b);g_esi=MEM32(g_eax);g_ecx=MEM32(g_ebx+0x54);MEM32(g_esp+12)=g_eax;g_eax=g_ebx+0x110;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);ELECTRIC_FRAME_CALL(sub_00219530,0x191023);
  g_ecx=MEM32(g_esp+0x18);g_esp+=8;{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);ELECTRIC_VCALL(MEM32(g_esi+0x10),0x19102e,saved);}
 }
 g_edx=MEM32(0x51c398);g_ecx=MEM32(g_edx+12);g_eax=MEM32(g_ebx+0x54);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,0x47211c);PUSH32(g_esp,g_eax);ELECTRIC_VCALL(MEM32(g_edx+4),0x191045,saved);}
 g_xmm3=XMM_SCALAR(MEMF(0x44c1f8));g_xmm0=XMM_SCALAR(MEMF(g_edi+8));g_xmm2=XMM_SCALAR(MEMF(g_edi+4));g_xmm1=XMM_SCALAR(MEMF(g_edi));g_xmm4=XMM_SCALAR(MEMF(g_ebx+0x44));g_xmm5=XMM_SCALAR(MEMF(g_ebx+0x40));
 g_xmm0.f[0]=g_xmm0.f[0]*g_xmm3.f[0];g_xmm2.f[0]=g_xmm2.f[0]*g_xmm3.f[0];g_xmm1.f[0]=g_xmm1.f[0]*g_xmm3.f[0];g_xmm3=XMM_SCALAR(MEMF(0x44fbcc));g_xmm0.f[0]=g_xmm0.f[0]+g_xmm3.f[0];g_xmm1.f[0]=g_xmm1.f[0]+g_xmm3.f[0];g_xmm3=XMM_SCALAR(MEMF(g_ebx+0x48));g_xmm3.f[0]=g_xmm3.f[0]+g_xmm0.f[0];g_xmm4.f[0]=g_xmm4.f[0]+g_xmm2.f[0];MEMF(g_esp+0x24)=g_xmm4.f[0];g_xmm4=XMM_ZERO();MEMF(g_esp+0x28)=g_xmm3.f[0];MEMF(g_esp+0x2c)=g_xmm4.f[0];g_xmm5.f[0]=g_xmm5.f[0]+g_xmm1.f[0];MEMF(g_esp+0x20)=g_xmm5.f[0];g_xmm3=XMM_MEM(g_esp+0x20);XMM_STORE(g_esp+0x40,g_xmm3);
 g_xmm3=XMM_SCALAR(MEMF(g_ebx+0x48));g_xmm3.f[0]=g_xmm3.f[0]-g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ebx+0x44));g_xmm0.f[0]=g_xmm0.f[0]-g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_ebx+0x40));g_eax=g_esp+0x30;PUSH32(g_esp,g_eax);g_ecx=g_esp+0x44;g_xmm2.f[0]=g_xmm2.f[0]-g_xmm1.f[0];MEMF(g_esp+0x24)=g_xmm2.f[0];MEMF(g_esp+0x28)=g_xmm0.f[0];MEMF(g_esp+0x2c)=g_xmm3.f[0];MEMF(g_esp+0x30)=g_xmm4.f[0];g_xmm0=XMM_MEM(g_esp+0x24);PUSH32(g_esp,g_ecx);g_ecx=g_esp+0x58;XMM_STORE(g_esp+0x38,g_xmm0);ELECTRIC_FRAME_CALL(sub_002D38F0,0x191107);
 g_esi=g_eax;g_edi=g_ebx+0xe0;g_ecx=12;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
 g_xmm0=XMM_SCALAR(MEMF(g_ebx+0x40));g_xmm1=XMM_SCALAR(MEMF(g_ebx+0x48));MEMF(g_esp+0x14)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ebx+0x44));g_xmm0.f[0]=g_xmm0.f[0]+MEMF(0x44c5e8);PUSH32(g_esp,0x2000);g_ecx=g_ebx;MEMF(g_esp+0x20)=g_xmm1.f[0];MEMF(g_esp+0x1c)=g_xmm0.f[0];ELECTRIC_FRAME_CALL(sub_0020A140,0x19114b);
 g_edx=MEM32(g_ebx+0xd4);g_xmm0=XMM_SCALAR(MEMF(g_esp+0x14));PUSH32(g_esp,0);g_esp-=12;g_eax=g_esp;MEM32(g_eax)=g_edx;g_ecx=MEM32(g_ebx+0xd8);MEM32(g_eax+4)=g_ecx;g_edx=MEM32(g_ebx+0xdc);MEM32(g_esp+0x1c)=g_esp;MEM32(g_eax+8)=g_edx;g_esp-=12;g_eax=g_esp;MEMF(g_eax)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x34));MEMF(g_eax+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x38));MEM32(g_esp+0x28)=g_esp;MEMF(g_eax+8)=g_xmm0.f[0];g_eax=MEM32(g_ebx+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);ELECTRIC_FRAME_CALL(sub_00055EE0,0x1911a8);g_ecx=g_eax;ELECTRIC_FRAME_CALL(sub_000236D0,0x1911af);
 g_ecx=MEM32(g_esp+0x84);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=g_ebx;MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=8;
}

static void electricity_update(void){
 uint32_t frame=g_ebp;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_esp-=0x24;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;ELECTRIC_FRAME_CALL(sub_00218C40,0x191503);
 SET_LO8(g_eax,MEM8(g_esi+0x120));
 if(LO8(g_eax)){
  g_xmm0=XMM_ZERO();double a=g_xmm0.f[0],b=MEMF(g_esi+0x11c);unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;
  if(!(flags&0x41u)){
   g_eax=MEM32(0x51e390);g_xmm1=XMM_SCALAR(MEMF(g_esi+0xd4));g_eax-=16;g_ecx=g_eax;g_ecx-=0x51e1d0;g_ecx=(uint32_t)((int32_t)g_ecx>>31);g_ecx&=0x1c0;g_eax+=g_ecx;g_ecx=MEM32(0x51e394);g_ecx-=16;MEMF(g_esp+0x1c)=g_xmm0.f[0];g_edx=g_ecx;MEMF(g_esp+0x10)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+0xd8));g_edx-=0x51e1d0;MEMF(g_esp+0x14)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+0xdc));g_edx=(uint32_t)((int32_t)g_edx>>31);MEMF(g_esp+0x18)=g_xmm1.f[0];g_xmm1=XMM_MEM(g_esp+0x10);MEM32(0x51e390)=g_eax;g_edx&=0x1c0;g_ecx+=g_edx;MEM32(0x51e394)=g_ecx;
   g_xmm2=XMM_MEM(g_ecx);g_xmm0=XMM_MEM(g_eax);g_xmm0=XMM_SUB(g_xmm0,g_xmm2);g_xmm2=g_xmm0;g_xmm2=XMM_XOR(g_xmm2,g_xmm0);g_xmm3=g_xmm0;g_xmm3=XMM_CMP_LT(g_xmm3,g_xmm2);g_xmm2=XMM_MEM(0x51e1b0);g_xmm2=XMM_AND(g_xmm2,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);XMM_STORE(g_eax,g_xmm0);g_xmm2=XMM_MEM(0x51e1a0);g_ecx=MEM32(0x51c4dc);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);g_xmm1=XMM_MEM(0x51e1c0);g_xmm0=XMM_SUB(g_xmm0,g_xmm1);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);XMM_STORE(g_esp+0x20,g_xmm0);
   g_xmm0=XMM_SCALAR(MEMF(g_esi+0x40));g_xmm0.f[0]=g_xmm0.f[0]+MEMF(g_esp+0x20);MEMF(g_esp+0x10)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0x44));g_xmm0.f[0]=g_xmm0.f[0]+MEMF(g_esp+0x24);MEMF(g_esp+0x14)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0x48));g_xmm0.f[0]=g_xmm0.f[0]+MEMF(g_esp+0x28);MEMF(g_esp+0x18)=g_xmm0.f[0];ELECTRIC_FRAME_CALL(sub_00208C50,0x191618);
   g_edi=g_eax;g_ebx=MEM32(g_edi);g_eax=g_esi+0x110;PUSH32(g_esp,g_eax);g_ecx=g_esp+0x14;PUSH32(g_esp,g_ecx);ELECTRIC_FRAME_CALL(sub_00219470,0x19162d);g_esp+=4;{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;ELECTRIC_VCALL(MEM32(g_ebx+0x10),0x191636,saved);}
   g_edx=MEM32(g_esi+0x118);g_eax=MEM32(g_esi+0x114);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);ELECTRIC_FRAME_CALL(sub_002D3780,0x191649);g_esp+=8;
  }else{ELECTRIC_FRAME_CALL(sub_001F9010,0x191653);g_fp_stack[g_fp_top]=MEMF(g_esi+0x11c)-g_fp_stack[g_fp_top];}
  MEMF(g_esi+0x11c)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;
 }
 SET_LO8(g_eax,MEM8(g_esi+0xd0));int damage_disabled=LO8(g_eax)==0;
 if(!damage_disabled){g_ecx=g_esi;ELECTRIC_FRAME_CALL(sub_00191210,0x191670);g_ecx=g_esi;ELECTRIC_FRAME_CALL(sub_00191410,0x191677);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=damage_disabled;g_esp+=4;
}

int uc_hybrid_fx_electricity(uint32_t entry){
 if(!electricity_enabled())return 0;
 switch(entry){case 0x190df0:case 0x1911d0:case 0x1913b0:case 0x1914f0:case 0xeb4d0:break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx,frame=g_seh_ebp;
 if((s&3)||s<0x12000u||!electricity_span(s-8192,8224)||!electricity_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7||!electricity_apart(s-8192,8224,0x51c178,1))return 0;
 if(entry!=0x1913b0&&(!electricity_span(p,0x130)||!electricity_apart(p,0x130,s-8192,8224)||!electricity_apart(p,0x130,0x51c178,1)))return 0;
 if(entry==0xeb4d0&&MEM32(p)!=0x474410)return 0;
 if(entry==0x190df0){electricity_construct();return 1;}
 if(entry==0x1914f0){electricity_update();return 1;}
 if(entry==0x1911d0){PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);ELECTRIC_CALL(sub_00055EE0,0x1911e2);g_ecx=g_eax;ELECTRIC_CALL(sub_00022A10,0x1911e9);g_eax=MEM32(0x51c398);g_ecx=MEM32(g_eax+12);g_eax=MEM32(g_esi+0x54);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,0x47211c);PUSH32(g_esp,g_eax);ELECTRIC_VCALL(MEM32(g_edx+8),0x1911ff,saved);}g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_00218B30();return 1;}
 if(entry==0xeb4d0){PUSH32(g_esp,g_esi);g_esi=g_ecx;ELECTRIC_CALL(sub_0013BE30,0xeb4d8);if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);ELECTRIC_CALL(sub_002D5CCD,0xeb4e5);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x3966eb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x130);ELECTRIC_CALL(sub_002D57FB,0x1913d0);g_esp+=4;MEM32(g_esp)=g_eax;MEM32(g_esp+12)=0;
 if(g_eax){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;ELECTRIC_CALL(sub_00190DF0,0x1913ee);}else g_eax=0;
 g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
