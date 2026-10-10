#include "hybrid_haze_room_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_00218A60(void),sub_00218B30(void),sub_00218C40(void),sub_0013BE30(void);
extern void sub_000EE000(void),sub_000EE2E0(void),sub_002D6380(void),sub_001F8DC0(void);
extern void sub_0020A140(void),sub_00207F70(void),sub_0021FC20(void),sub_00208C50(void);
extern void sub_00219530(void),sub_0023EA70(void),sub_00185DD0(void),sub_00186020(void);
extern void sub_00186110(void),sub_002D57FB(void),sub_002D5CCD(void);
static int haze_enabled(void){static int flag=-1;if(flag<0){const char*p=getenv("UCRR_NATIVE_HAZE_ROOM");flag=p&&!strcmp(p,"1");}return flag;}
static int haze_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int haze_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define HAZE_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define HAZE_FRAME_CALL(fn,pc) do{g_ebp=frame;HAZE_CALL(fn,pc);}while(0)
#define HAZE_VCALL(target,pc,saved) do{uint32_t haze_target=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(haze_target,saved);}while(0)

/* Source-valid service lifetimes remain borrowed. All owner and saved-stack
 * reads stay live after callbacks; no rejection or replay follows admission. */
static void haze_construct(void){
 uint32_t frame=g_ebp;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x395de8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x58;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+0x1c)=g_esi;HAZE_FRAME_CALL(sub_00218A60,0x18613c);
 g_edi=g_esi+0xd0;g_ebx=0;g_ecx=g_edi;MEM32(g_esp+0x6c)=g_ebx;MEM32(g_esi)=0x472b30;
 static const uint32_t descriptor_pc[3][3]={{0x186155,0x18615d,0x186176},{0x186186,0x18618e,0x1861a7},{0x1861b7,0x1861bf,0x1861d8}};
 for(unsigned i=0;i<3;++i){
  HAZE_FRAME_CALL(sub_000EE000,descriptor_pc[i][0]);g_ecx=g_edi+0x30;HAZE_FRAME_CALL(sub_000EE2E0,descriptor_pc[i][1]);
  PUSH32(g_esp,0x3f);MEM8(g_edi+0x110)=LO8(g_ebx);PUSH32(g_esp,0x4600dc);g_edi+=0x111;PUSH32(g_esp,g_edi);HAZE_FRAME_CALL(sub_002D6380,descriptor_pc[i][2]);
  if(i!=2){g_edi=g_esi+0x230+i*0x160;g_esp+=12;g_ecx=g_edi;}
 }
 g_edi=MEM32(frame+8);g_esp+=12;g_eax=g_esp+0x30;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x462428);PUSH32(g_esp,4);g_ecx=g_edi;HAZE_FRAME_CALL(sub_001F8DC0,0x1861f1);
 if(!LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(0x44c178));g_xmm1=XMM_SCALAR(MEMF(0x44c798));MEMF(g_esp+0x20)=g_xmm0.f[0];MEMF(g_esp+0x24)=g_xmm1.f[0];}
 else{g_xmm0=XMM_SCALAR(MEMF(g_esp+0x30));MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x34));MEMF(g_esp+0x24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x38));}
 g_edx=MEM32(g_esp+0x20);g_eax=MEM32(g_esp+0x24);g_ecx=g_esi+0x4f0;MEM32(g_ecx)=g_edx;MEM32(g_ecx+4)=g_eax;g_eax=g_esp+0x18;PUSH32(g_esp,g_eax);MEMF(g_esp+0x2c)=g_xmm0.f[0];g_edx=MEM32(g_esp+0x2c);PUSH32(g_esp,0x464fcc);MEM32(g_ecx+8)=g_edx;PUSH32(g_esp,2);g_ecx=g_edi;HAZE_FRAME_CALL(sub_001F8DC0,0x186264);
 int missing=LO8(g_eax)==0;g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm0=g_xmm1;if(!missing)g_xmm0=XMM_SCALAR(MEMF(g_esp+0x18));
 MEMF(g_esi+0x500)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0x40));g_xmm2=XMM_SCALAR(MEMF(g_esi+0x44));MEMF(g_esp+0x50)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0x4f4));g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c1f8);g_xmm2.f[0]=g_xmm2.f[0]-g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0x48));MEMF(g_esp+0x58)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+0x4c));MEMF(g_esp+0x5c)=g_xmm0.f[0];g_xmm0=XMM_ZERO();
 MEMF(g_esp+0x40)=g_xmm0.f[0];MEMF(g_esp+0x44)=g_xmm0.f[0];MEMF(g_esp+0x4c)=g_xmm0.f[0];MEMF(g_esp+0x24)=g_xmm0.f[0];MEMF(g_esp+0x28)=g_xmm0.f[0];MEMF(g_esp+0x2c)=g_xmm0.f[0];MEMF(g_esp+0x30)=g_xmm0.f[0];MEMF(g_esp+0x38)=g_xmm0.f[0];MEMF(g_esp+0x3c)=g_xmm0.f[0];MEMF(g_esp+0x34)=g_xmm1.f[0];
 g_xmm0=XMM_MEM(g_esp+0x30);XMM_STORE(g_esi+0x10,g_xmm0);MEMF(g_esp+0x20)=g_xmm1.f[0];g_xmm0=XMM_MEM(g_esp+0x20);XMM_STORE(g_esi+0x20,g_xmm0);MEMF(g_esp+0x48)=g_xmm1.f[0];g_xmm0=XMM_MEM(g_esp+0x40);MEMF(g_esp+0x54)=g_xmm2.f[0];XMM_STORE(g_esi+0x30,g_xmm0);g_xmm0=XMM_MEM(g_esp+0x50);PUSH32(g_esp,0x2000);g_ecx=g_esi;XMM_STORE(g_esi+0x40,g_xmm0);HAZE_FRAME_CALL(sub_0020A140,0x18633c);
 g_ecx=g_esi;HAZE_FRAME_CALL(sub_00186020,0x186343);g_ecx=MEM32(g_esi+0x54);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x51c4dc);HAZE_FRAME_CALL(sub_00207F70,0x186352);
 g_ecx=MEM32(g_esp+0x64);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=missing;g_esp+=8;
}

static void haze_setup(uint32_t frame){
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;HAZE_CALL(sub_00185DD0,0x18602b);
 g_xmm0=XMM_SCALAR(MEMF(g_esi+0x4f8));g_xmm0.f[0]=g_xmm0.f[0]*MEMF(g_esi+0x4f0);g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c728);MEMF(g_esi+0x4fc)=g_xmm0.f[0];g_ecx=MEM32(0x51c4dc);HAZE_CALL(sub_00208C50,0x186056);
 g_ebx=g_esi+0xd0;
 static const uint32_t source_pc[2]={0x18606c,0x1860d7},emitter_pc[2]={0x186093,0x1860fe},publish_pc[2]={0x18609c,0x186107};
 for(unsigned i=0;i<2;++i){
  g_edi=g_eax;g_eax=MEM32(g_esi+0x54);frame=MEM32(g_edi);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0);PUSH32(g_esp,g_eax);HAZE_CALL(sub_00219530,source_pc[i]);
  g_xmm0=XMM_SCALAR(MEMF(g_esi+0x500));g_xmm0.f[0]=g_xmm0.f[0]*MEMF(g_esi+0x4fc);g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44fe58);g_esp+=8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];HAZE_CALL(sub_0023EA70,emitter_pc[i]);g_esp+=4;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;HAZE_VCALL(MEM32(frame+20),publish_pc[i],saved);}
  if(!i){g_xmm0=XMM_SCALAR(MEMF(0x464f88));MEMF(g_esi+0x1a0)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44c5ec));MEMF(g_esi+0x1a4)=g_xmm0.f[0];g_ecx=MEM32(0x51c4dc);HAZE_CALL(sub_00208C50,0x1860c7);}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=g_esp==0;g_esp+=4;
}

/* The descriptor's straight-line stores retain their original order. */
static void haze_descriptor(void);

int uc_hybrid_haze_room(uint32_t entry){
 if(!haze_enabled())return 0;
 switch(entry){case 0x185dc0:case 0x185dd0:case 0x186010:case 0x186020:case 0x186110:case 0x186370:case 0xeb4d0:break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx,frame=g_seh_ebp;
 if((s&3)||s<0x12000u||!haze_span(s-8192,8224)||!haze_span(0x51c178,1)||MEM8(0x51c178)||!haze_apart(s-8192,8224,0x51c178,1))return 0;
 if(entry!=0x186370&&(!haze_span(p,0x510)||!haze_apart(p,0x510,s-8192,8224)||!haze_apart(p,0x510,0x51c178,1)))return 0;
 if((entry==0x185dc0||entry==0xeb4d0)&&MEM32(p)!=0x472b30)return 0;
 if(entry==0x185dd0){haze_descriptor();return 1;}
 if(entry==0x186110){haze_construct();return 1;}
 if(entry==0x186020){haze_setup(frame);return 1;}
 if(entry==0x185dc0){g_seh_ebp=frame;sub_00218C40();return 1;}
 if(entry==0x186010){g_seh_ebp=frame;sub_00218B30();return 1;}
 if(entry==0xeb4d0){PUSH32(g_esp,g_esi);g_esi=g_ecx;HAZE_CALL(sub_0013BE30,0xeb4d8);if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);HAZE_CALL(sub_002D5CCD,0xeb4e5);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x395e0b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x510);HAZE_CALL(sub_002D57FB,0x186390);g_esp+=4;MEM32(g_esp)=g_eax;MEM32(g_esp+12)=0;
 if(g_eax){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;HAZE_CALL(sub_00186110,0x1863ae);}else g_eax=0;
 g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}

static void haze_descriptor(void)
{
    uint32_t frame = g_ebp;
    PUSH32(g_esp, frame);
    frame = g_esp;
    g_ebp = frame;
    g_esp = g_esp & 0xFFFFFFF0u;
    g_esp = g_esp - 0x18;
    PUSH32(g_esp, g_ebx);
    PUSH32(g_esp, g_esi);
    g_esi = g_ecx;
    g_xmm0 = XMM_SCALAR(MEMF(g_esi + 0x4F4));
    g_xmm0.f[0] = g_xmm0.f[0] * MEMF(0x44C604);
    g_xmm1 = XMM_SCALAR(MEMF(g_esi + 0x4F0));
    MEMF(g_esp + 0xC) = g_xmm0.f[0];
    g_xmm0 = XMM_ZERO();
    MEMF(g_esp + 0x10) = g_xmm0.f[0];
    g_ecx = MEM32(g_esp + 0x10);
    MEMF(g_esp + 0x14) = g_xmm1.f[0];
    g_edx = MEM32(g_esp + 0x14);
    g_xmm1 = XMM_SCALAR(MEMF(g_esi + 0x4F8));
    g_eax = g_esi + 0xD4;
    MEM32(g_eax) = g_ecx;
    MEM32(g_eax + 4) = g_edx;
    MEMF(g_esi + 0xEC) = g_xmm0.f[0];
    MEMF(g_esi + 0xF0) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(0x44C178));
    MEMF(g_esp + 0x18) = g_xmm1.f[0];
    g_ecx = MEM32(g_esp + 0x18);
    g_edx = g_esp + 8;
    g_ebx = 0;
    MEMF(g_esi + 0xF4) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(0x44C5E4));
    PUSH32(g_esp, g_edx);
    MEM32(g_esi + 0xD0) = g_ebx;
    MEM32(g_eax + 8) = g_ecx;
    MEMF(g_esi + 0xF8) = g_xmm0.f[0];
    MEM32(g_esi + 0xFC) = g_ebx;
    MEM32(g_esp + 0xC) = 0x472B0C;
    g_ebp = frame;
    PUSH32(g_esp, 0x00185E82u); sub_0021FC20();
    g_xmm0 = XMM_SCALAR(MEMF(0x44E1D8));
    MEMF(g_esi + 0x1A0) = g_xmm0.f[0];
    g_xmm0 = XMM_SCALAR(MEMF(0x44C5F0));
    MEM32(g_esi + 0x164) = g_eax;
    MEMF(g_esi + 0x1A4) = g_xmm0.f[0];
    g_xmm0 = XMM_ZERO();
    MEMF(g_esp + 0x14) = g_xmm0.f[0];
    g_ecx = MEM32(g_esp + 0x14);
    MEMF(g_esp + 0x18) = g_xmm0.f[0];
    g_edx = MEM32(g_esp + 0x18);
    MEMF(g_esp + 0x1C) = g_xmm0.f[0];
    g_eax = g_esi + 0x100;
    MEM32(g_eax) = g_ecx;
    g_ecx = MEM32(g_esp + 0x1C);
    MEM32(g_eax + 4) = g_edx;
    MEM32(g_eax + 8) = g_ecx;
    MEMF(g_esp + 0x14) = g_xmm0.f[0];
    g_eax = MEM32(g_esp + 0x14);
    MEMF(g_esp + 0x18) = g_xmm0.f[0];
    g_ecx = MEM32(g_esp + 0x18);
    MEMF(g_esp + 0x1C) = g_xmm0.f[0];
    g_edx = g_esi + 0x10C;
    MEM32(g_edx) = g_eax;
    g_eax = MEM32(g_esp + 0x1C);
    MEM32(g_edx + 4) = g_ecx;
    MEMF(g_esp + 0x14) = g_xmm0.f[0];
    MEMF(g_esp + 0x18) = g_xmm0.f[0];
    MEMF(g_esp + 0x1C) = g_xmm0.f[0];
    MEMF(g_esp + 0x20) = g_xmm0.f[0];
    g_xmm1 = XMM_MEM(g_esp + 0x14);
    MEM32(g_esi + 0x190) = 3;
    MEM32(g_edx + 8) = g_eax;
    MEMF(g_esi + 0x118) = g_xmm0.f[0];
    XMM_STORE(g_esi + 0x120, g_xmm1);
    g_xmm1 = XMM_SCALAR(MEMF(0x44C1F8));
    MEMF(g_esp + 0x14) = g_xmm1.f[0];
    MEMF(g_esp + 0x18) = g_xmm1.f[0];
    MEMF(g_esp + 0x1C) = g_xmm1.f[0];
    MEMF(g_esp + 0x20) = g_xmm0.f[0];
    g_xmm1 = XMM_MEM(g_esp + 0x14);
    XMM_STORE(g_esi + 0x130, g_xmm1);
    MEMF(g_esp + 0x14) = g_xmm0.f[0];
    MEMF(g_esp + 0x18) = g_xmm0.f[0];
    MEMF(g_esp + 0x1C) = g_xmm0.f[0];
    MEMF(g_esp + 0x20) = g_xmm0.f[0];
    g_xmm1 = XMM_MEM(g_esp + 0x14);
    XMM_STORE(g_esi + 0x140, g_xmm1);
    g_xmm1 = XMM_SCALAR(MEMF(0x44C538));
    g_esp = g_esp + 4;
    MEMF(g_esi + 0x150) = g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(0x44E548));
    MEMF(g_esi + 0x154) = g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(0x44C178));
    g_eax = 1;
    MEMF(g_esi + 0x158) = g_xmm1.f[0];
    MEMF(g_esi + 0x15C) = g_xmm1.f[0];
    g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0xC));
    MEM8(g_esi + 0x16C) = LO8(g_ebx);
    MEM8(g_esi + 0x175) = LO8(g_ebx);
    MEMF(g_esi + 0x160) = g_xmm1.f[0];
    MEM32(g_esi + 0x168) = g_eax;
    MEM32(g_esi + 0x194) = g_eax;
    MEMF(g_esi + 0x170) = g_xmm0.f[0];
    MEM8(g_esi + 0x174) = LO8(g_eax);
    POP32(g_esp, g_esi);
    POP32(g_esp, g_ebx);
    g_esp = frame;
    POP32(g_esp, frame);
    g_recomp_zf = !!((g_esp == 0));
    g_esp += 4; return;
}
#undef HAZE_CALL
#undef HAZE_FRAME_CALL
#undef HAZE_VCALL
