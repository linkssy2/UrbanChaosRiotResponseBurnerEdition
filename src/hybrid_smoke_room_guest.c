#include "hybrid_smoke_room_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001118D0(void),sub_00111B30(void),sub_00111CD0(void);
extern void sub_00218A60(void),sub_000EE000(void),sub_000EE2E0(void),sub_002D6380(void);
extern void sub_001F8DC0(void),sub_0020A140(void),sub_00207F70(void),sub_00208C50(void);
extern void sub_00219530(void),sub_0023EA70(void),sub_0021FC20(void),sub_002D57FB(void);
extern void sub_002D38F0(void),sub_00055E80(void),sub_0004FA00(void);
static int smoke_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_SMOKE_ROOM");flag=v&&!strcmp(v,"1");}return flag;}
static int smoke_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int smoke_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define SMOKE_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define SMOKE_FRAME_CALL(fn,pc) do{g_ebp=frame;SMOKE_CALL(fn,pc);}while(0)

/* These are complete owner algorithms, not a new particle simulation or a
 * performance claim. Register and retired-stack writes preserve the retained
 * service ABI. The sources remain borrowed and are re-read after callbacks. */
static void smoke_descriptor(void){
 uint32_t frame=g_ebp;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_esp-=0x18;
 g_xmm1=XMM_ZERO();PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;
 g_xmm2=XMM_SCALAR(MEMF(g_esi+0x4f0));g_xmm0=XMM_SCALAR(MEMF(g_esi+0x4f4));g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c530);g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c538);
 MEMF(g_esp+0x10)=g_xmm1.f[0];g_ecx=MEM32(g_esp+0x10);MEMF(g_esp+0x14)=g_xmm2.f[0];g_edx=MEM32(g_esp+0x14);g_xmm2=XMM_SCALAR(MEMF(g_esi+0x4f8));g_eax=g_esi+0xd4;
 MEM32(g_eax)=g_ecx;MEM32(g_eax+4)=g_edx;MEMF(g_esi+0xec)=g_xmm1.f[0];MEMF(g_esi+0xf0)=g_xmm1.f[0];MEMF(g_esp+0x18)=g_xmm2.f[0];g_ecx=MEM32(g_esp+0x18);
 g_xmm1=g_xmm0;g_xmm1.f[0]=g_xmm1.f[0]*MEMF(0x44e594);g_edx=g_esp+8;MEMF(g_esp+0xc)=g_xmm0.f[0];g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c774);g_ebx=0;PUSH32(g_esp,g_edx);
 MEM32(g_esi+0xd0)=g_ebx;MEM32(g_eax+8)=g_ecx;MEMF(g_esi+0xf4)=g_xmm1.f[0];MEMF(g_esi+0xf8)=g_xmm0.f[0];MEM32(g_esi+0xfc)=g_ebx;MEM32(g_esp+0xc)=0x464f8c;SMOKE_FRAME_CALL(sub_0021FC20,0x11198d);
 g_xmm0=XMM_SCALAR(MEMF(0x464f88));g_xmm1=XMM_SCALAR(MEMF(g_esp+0x10));g_xmm1.f[0]=g_xmm1.f[0]*MEMF(0x457c24);MEMF(g_esi+0x1a0)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44e1d8));MEM32(g_esi+0x164)=g_eax;MEMF(g_esi+0x1a4)=g_xmm0.f[0];g_xmm0=XMM_ZERO();
 MEMF(g_esp+0x14)=g_xmm0.f[0];g_ecx=MEM32(g_esp+0x14);MEMF(g_esp+0x18)=g_xmm1.f[0];g_edx=MEM32(g_esp+0x18);g_eax=g_esi+0x100;MEM32(g_eax)=g_ecx;MEM32(g_eax+4)=g_edx;MEMF(g_esp+0x1c)=g_xmm0.f[0];g_ecx=MEM32(g_esp+0x1c);MEM32(g_eax+8)=g_ecx;
 MEMF(g_esp+0x14)=g_xmm0.f[0];g_eax=MEM32(g_esp+0x14);MEMF(g_esp+0x18)=g_xmm0.f[0];g_ecx=MEM32(g_esp+0x18);g_edx=g_esi+0x10c;MEM32(g_edx)=g_eax;MEMF(g_esp+0x1c)=g_xmm0.f[0];g_eax=MEM32(g_esp+0x1c);MEM32(g_edx+4)=g_ecx;
 MEMF(g_esp+0x14)=g_xmm0.f[0];MEMF(g_esp+0x18)=g_xmm0.f[0];MEMF(g_esp+0x1c)=g_xmm0.f[0];MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm1=XMM_MEM(g_esp+0x14);MEM32(g_esi+0x190)=3;MEM32(g_edx+8)=g_eax;MEMF(g_esi+0x118)=g_xmm0.f[0];XMM_STORE(g_esi+0x120,g_xmm1);
 g_xmm1=XMM_SCALAR(MEMF(g_esi+0x4fc));g_xmm1.f[0]=g_xmm1.f[0]*MEMF(0x44c50c);MEMF(g_esp+0x14)=g_xmm1.f[0];MEMF(g_esp+0x18)=g_xmm1.f[0];MEMF(g_esp+0x1c)=g_xmm1.f[0];MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm1=XMM_MEM(g_esp+0x14);g_esp+=4;XMM_STORE(g_esi+0x130,g_xmm1);
 MEMF(g_esp+0x10)=g_xmm0.f[0];MEMF(g_esp+0x14)=g_xmm0.f[0];MEMF(g_esp+0x18)=g_xmm0.f[0];MEMF(g_esp+0x1c)=g_xmm0.f[0];g_xmm1=XMM_MEM(g_esp+0x10);XMM_STORE(g_esi+0x140,g_xmm1);
 g_xmm1=XMM_SCALAR(MEMF(0x47df9c));MEMF(g_esi+0x150)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44e548));MEMF(g_esi+0x154)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44c140));MEMF(g_esi+0x158)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44e088));MEMF(g_esi+0x15c)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44bf18));
 MEM32(g_esi+0x194)=g_ebx;MEM8(g_esi+0x16c)=LO8(g_ebx);MEM8(g_esi+0x175)=LO8(g_ebx);MEMF(g_esi+0x160)=g_xmm1.f[0];MEM32(g_esi+0x168)=5;MEMF(g_esi+0x170)=g_xmm0.f[0];MEM8(g_esi+0x174)=1;
 POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;
}

static void smoke_setup(void){
 uint32_t frame=g_seh_ebp;
 g_esp-=0x18;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;SMOKE_CALL(sub_001118D0,0x111b3e);
 g_xmm0=XMM_SCALAR(MEMF(g_esi+0x4f0));g_xmm0.f[0]=g_xmm0.f[0]*MEMF(g_esi+0x4f8);g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c728);MEMF(g_esi+0x504)=g_xmm0.f[0];g_ecx=MEM32(0x51c4dc);SMOKE_CALL(sub_00208C50,0x111b69);g_ebx=g_esi+0xd0;
 static const uint32_t source_pc[]={0x111b7f,0x111bd1},emitter_pc[]={0x111ba6,0x111bf8},publish_pc[]={0x111baf,0x111c01};
 for(unsigned i=0;i<2;i++){
  g_edi=g_eax;g_eax=MEM32(g_esi+0x54);frame=MEM32(g_edi);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0);PUSH32(g_esp,g_eax);SMOKE_CALL(sub_00219530,source_pc[i]);
  g_xmm0=XMM_SCALAR(MEMF(g_esi+0x508));g_xmm0.f[0]=g_xmm0.f[0]*MEMF(g_esi+0x504);g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c50c);g_esp+=8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];SMOKE_CALL(sub_0023EA70,emitter_pc[i]);g_esp+=4;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;uint32_t target=MEM32(frame+20);PUSH32(g_esp,publish_pc[i]);RECOMP_ICALL_SAFE(target,saved);}
  if(!i){MEM8(g_esi+0x175)=1;g_ecx=MEM32(0x51c4dc);SMOKE_CALL(sub_00208C50,0x111bc1);}
 }
 SET_LO8(g_eax,MEM8(g_esi+0x500));
 if(LO8(g_eax)){
  g_xmm2=XMM_SCALAR(MEMF(0x44c1f8));g_xmm0=XMM_SCALAR(MEMF(g_esi+0x4f0));g_xmm1=XMM_SCALAR(MEMF(g_esi+0x4f8));g_xmm3=XMM_SCALAR(MEMF(g_esi+0x44));g_xmm1.f[0]=g_xmm1.f[0]*g_xmm2.f[0];g_xmm0.f[0]=g_xmm0.f[0]*g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_esi+0x40));g_xmm2.f[0]=g_xmm2.f[0]-g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0x48));g_xmm0.f[0]=g_xmm0.f[0]-g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+0x4f0));g_xmm1.f[0]=g_xmm1.f[0]+g_xmm2.f[0];
  PUSH32(g_esp,1);MEMF(g_esp+0x14)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+0x4f4));g_esp-=0x30;g_xmm1.f[0]=g_xmm1.f[0]+g_xmm3.f[0];g_ecx=g_esp;g_eax=g_esp+0x44;MEMF(g_esp+0x48)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+0x4f8));g_esi=MEM32(g_esi+0x54);PUSH32(g_esp,g_eax);g_edx=g_esp+0x54;g_xmm1.f[0]=g_xmm1.f[0]+g_xmm0.f[0];PUSH32(g_esp,g_edx);
  MEMF(g_esp+0x58)=g_xmm2.f[0];MEMF(g_esp+0x5c)=g_xmm3.f[0];MEMF(g_esp+0x60)=g_xmm0.f[0];MEMF(g_esp+0x54)=g_xmm1.f[0];SMOKE_CALL(sub_002D38F0,0x111ca7);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_esi);SMOKE_CALL(sub_00055E80,0x111cb3);g_ecx=g_eax;SMOKE_CALL(sub_0004FA00,0x111cba);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=0x18;g_recomp_zf=g_esp==0;g_esp+=4;
}

static void smoke_construct(void){
 uint32_t frame=g_ebp;unsigned flags;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x390498);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x58;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+0x1c)=g_esi;SMOKE_FRAME_CALL(sub_00218A60,0x111cfc);
 g_edi=g_esi+0xd0;g_ebx=0;g_ecx=g_edi;MEM32(g_esp+0x6c)=g_ebx;MEM32(g_esi)=0x464fd8;
 static const uint32_t pc[3][3]={{0x111d15,0x111d1d,0x111d36},{0x111d46,0x111d4e,0x111d67},{0x111d77,0x111d7f,0x111d98}};
 for(unsigned i=0;i<3;i++){
  SMOKE_FRAME_CALL(sub_000EE000,pc[i][0]);g_ecx=g_edi+0x30;SMOKE_FRAME_CALL(sub_000EE2E0,pc[i][1]);PUSH32(g_esp,0x3f);MEM8(g_edi+0x110)=LO8(g_ebx);PUSH32(g_esp,0x4600dc);g_edi+=0x111;PUSH32(g_esp,g_edi);SMOKE_FRAME_CALL(sub_002D6380,pc[i][2]);
  if(i!=2){g_edi=g_esi+0x230+i*0x160;g_esp+=12;g_ecx=g_edi;}
 }
 g_edi=MEM32(frame+8);g_esp+=12;g_eax=g_esp+0x30;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x462428);PUSH32(g_esp,4);g_ecx=g_edi;SMOKE_FRAME_CALL(sub_001F8DC0,0x111db1);
 if(!LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(0x44c1e0));g_xmm1=XMM_SCALAR(MEMF(0x44c5e4));MEMF(g_esp+0x20)=g_xmm0.f[0];MEMF(g_esp+0x24)=g_xmm1.f[0];}
 else{g_xmm0=XMM_SCALAR(MEMF(g_esp+0x30));MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x34));MEMF(g_esp+0x24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x38));}
 g_edx=MEM32(g_esp+0x20);g_eax=MEM32(g_esp+0x24);g_ecx=g_esi+0x4f0;MEM32(g_ecx)=g_edx;MEM32(g_ecx+4)=g_eax;g_eax=g_esp+0x18;PUSH32(g_esp,g_eax);MEMF(g_esp+0x2c)=g_xmm0.f[0];g_edx=MEM32(g_esp+0x2c);PUSH32(g_esp,0x464fcc);MEM32(g_ecx+8)=g_edx;PUSH32(g_esp,2);g_ecx=g_edi;SMOKE_FRAME_CALL(sub_001F8DC0,0x111e24);
 g_xmm0=XMM_SCALAR(MEMF(0x44bf94));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+0x18));g_ecx=g_esp+0x18;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x464fbc);PUSH32(g_esp,2);g_ecx=g_edi;MEMF(g_esi+0x508)=g_xmm0.f[0];SMOKE_FRAME_CALL(sub_001F8DC0,0x111e51);
 g_xmm0=XMM_SCALAR(MEMF(0x44bf94));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+0x18));g_edx=g_esp+0x18;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x464fa8);PUSH32(g_esp,3);g_ecx=g_edi;MEMF(g_esi+0x4fc)=g_xmm0.f[0];SMOKE_FRAME_CALL(sub_001F8DC0,0x111e7e);
 if(!LO8(g_eax))SET_LO8(g_eax,1);else{g_eax=MEM32(g_esp+0x18);g_eax&=0xff;SET_LO8(g_eax,g_eax!=0);}
 g_xmm1=XMM_SCALAR(MEMF(g_esi+0x4fc));g_xmm0=XMM_SCALAR(MEMF(0x44c530));
 {double a=g_xmm1.f[0],b=g_xmm0.f[0];flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
 MEM8(g_esi+0x500)=LO8(g_eax);if(!(flags&0x41))MEMF(g_esi+0x4fc)=g_xmm0.f[0];g_xmm0=XMM_ZERO();
 {double a=g_xmm0.f[0],b=MEMF(g_esi+0x4fc);flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
 if(!(flags&0x41))MEMF(g_esi+0x4fc)=g_xmm0.f[0];
 g_xmm1=XMM_SCALAR(MEMF(g_esi+0x40));g_xmm2=XMM_SCALAR(MEMF(g_esi+0x44));MEMF(g_esp+0x50)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+0x4f4));g_xmm1.f[0]=g_xmm1.f[0]*MEMF(0x44c1f8);g_xmm2.f[0]=g_xmm2.f[0]-g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+0x48));MEMF(g_esp+0x58)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esi+0x4c));MEMF(g_esp+0x5c)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44bf94));
 MEMF(g_esp+0x40)=g_xmm0.f[0];MEMF(g_esp+0x44)=g_xmm0.f[0];MEMF(g_esp+0x4c)=g_xmm0.f[0];MEMF(g_esp+0x24)=g_xmm0.f[0];MEMF(g_esp+0x28)=g_xmm0.f[0];MEMF(g_esp+0x2c)=g_xmm0.f[0];MEMF(g_esp+0x30)=g_xmm0.f[0];MEMF(g_esp+0x38)=g_xmm0.f[0];MEMF(g_esp+0x3c)=g_xmm0.f[0];MEMF(g_esp+0x34)=g_xmm1.f[0];
 g_xmm0=XMM_MEM(g_esp+0x30);XMM_STORE(g_esi+0x10,g_xmm0);MEMF(g_esp+0x20)=g_xmm1.f[0];g_xmm0=XMM_MEM(g_esp+0x20);XMM_STORE(g_esi+0x20,g_xmm0);MEMF(g_esp+0x48)=g_xmm1.f[0];g_xmm0=XMM_MEM(g_esp+0x40);MEMF(g_esp+0x54)=g_xmm2.f[0];XMM_STORE(g_esi+0x30,g_xmm0);g_xmm0=XMM_MEM(g_esp+0x50);PUSH32(g_esp,0x2000);g_ecx=g_esi;XMM_STORE(g_esi+0x40,g_xmm0);SMOKE_FRAME_CALL(sub_0020A140,0x111f89);
 g_ecx=g_esi;SMOKE_FRAME_CALL(sub_00111B30,0x111f90);g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);SMOKE_FRAME_CALL(sub_00207F70,0x111f9f);
 g_ecx=MEM32(g_esp+0x64);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=!!(flags&0x40);g_esp+=8;
}

int uc_hybrid_smoke_room(uint32_t entry){
 if(!smoke_enabled())return 0;
 if(entry!=0x1118d0&&entry!=0x111b30&&entry!=0x111cd0&&entry!=0x111fc0)return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x12000u||!smoke_span(s-8192,8224)||!smoke_span(0x51c178,1)||MEM8(0x51c178)||!smoke_apart(s-8192,8224,0x51c178,1))return 0;
 if(entry!=0x111fc0&&(!smoke_span(p,0x510)||!smoke_apart(p,0x510,s-8192,8224)||!smoke_apart(p,0x510,0x51c178,1)))return 0;
 if(entry==0x1118d0){smoke_descriptor();return 1;}if(entry==0x111b30){smoke_setup();return 1;}if(entry==0x111cd0){smoke_construct();return 1;}
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x3904bb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x510);SMOKE_CALL(sub_002D57FB,0x111fe0);g_esp+=4;MEM32(g_esp)=g_eax;MEM32(g_esp+12)=0;
 if(g_eax){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;SMOKE_CALL(sub_00111CD0,0x111ffe);}else g_eax=0;
 g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
#undef SMOKE_CALL
#undef SMOKE_FRAME_CALL
