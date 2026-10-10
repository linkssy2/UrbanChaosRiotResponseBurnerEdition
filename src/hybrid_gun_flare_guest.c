#include "hybrid_gun_flare_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0020BDC0(void),sub_00186980(void),sub_001F8DC0(void),sub_001F9120(void),sub_00187600(void);
extern void sub_00186BC0(void),sub_0021FC20(void),sub_00208110(void),sub_001F8FC0(void),sub_002D3780(void);
extern void sub_002D3DC0(void),sub_00011000(void),sub_00187B30(void),sub_002D5CCD(void),sub_0020A620(void);
extern void sub_00187BA0(void),sub_001867D0(void),sub_001F9140(void),sub_0020A480(void),sub_002D57FB(void),sub_00187680(void);
static int flare_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_GUN_FLARE");f=p&&!strcmp(p,"1");}return f;}
static int flare_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int flare_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define FLARE_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define FLARE_FRAME_CALL(fn,pc) do{g_ebp=frame;FLARE_CALL(fn,pc);}while(0)
#define FLARE_VCALL(target,pc,saved) do{uint32_t t_=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(t_,saved);}while(0)
static void flare_construct(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x395f3c);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x14;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+0x10)=g_esi;FLARE_CALL(sub_0020BDC0,0x1876a6);
 g_ebx=0;MEM32(g_esp+0x28)=g_ebx;MEM32(g_esi)=0x472fc8;g_edi=g_esi+0x80;MEM32(g_edi)=0x472fbc;
 MEM32(g_edi+8)=g_ebx;MEM32(g_edi+12)=g_ebx;MEM32(g_edi+16)=g_ebx;MEM32(g_edi+20)=g_ebx;g_ecx=g_esi+0x98;MEM8(g_esp+0x28)=2;FLARE_CALL(sub_00186980,0x1876da);
 g_ecx=MEM32(g_esp+0x30);g_eax=g_esp+0x30;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x458984);PUSH32(g_esp,3);MEM8(g_esp+0x34)=3;FLARE_CALL(sub_001F8DC0,0x1876f4);
 if(LO8(g_eax)){g_eax=MEM32(g_esp+0x30)&255u;SET_LO8(g_eax,g_eax!=0);}
 MEM8(g_esi+0xbc)=LO8(g_eax);MEM8(g_esi+0xc8)=LO8(g_ebx);MEM32(g_esi+0xcc)=g_ebx;MEM32(g_edi+20)=g_esi;
 g_ecx=MEM32(g_esi+0x54);PUSH32(g_esp,g_ecx);g_ecx=g_edi;FLARE_CALL(sub_001F9120,0x187724);
 g_edx=MEM32(0x4f9e14);g_eax=g_esp+0x14;g_edi=g_esi+0x84;PUSH32(g_esp,g_eax);g_ecx=g_edi;MEM32(g_esp+0x18)=g_edx;MEM32(g_esp+0x1c)=0x186c40;MEM32(g_esp+0x20)=g_ebx;FLARE_CALL(sub_00187600,0x18774c);
 g_ecx=MEM32(0x4f9e10);g_edx=g_esp+0x14;MEM32(g_esp+0x14)=g_ecx;PUSH32(g_esp,g_edx);g_ecx=g_edi;MEM32(g_esp+0x1c)=0x187150;MEM32(g_esp+0x20)=g_ebx;FLARE_CALL(sub_00187600,0x18776e);
 unsigned first=MEM8(g_esi+0xbc)!=LO8(g_ebx);MEM32(g_esi+0xd0)=g_ebx;MEM32(g_esi+0xd4)=g_ebx;MEM32(g_esi+0xc4)=g_ebx;
 if(first){g_eax=g_esp+0x30;MEM32(g_esp+0x30)=0x472f68;PUSH32(g_esp,g_eax);}
 else{g_ecx=g_esp+0x30;PUSH32(g_esp,g_ecx);g_ecx=g_esi;MEM32(g_esp+0x34)=0x472e48;FLARE_CALL(sub_00186BC0,0x1877ab);g_edx=g_esp+0x30;MEM32(g_esp+0x30)=0x472ea8;PUSH32(g_esp,g_edx);}
 g_ecx=g_esi;FLARE_CALL(sub_00186BC0,0x1877bf);g_eax=g_esp+0x30;PUSH32(g_esp,g_eax);g_ecx=g_esi;MEM32(g_esp+0x34)=0x472f00;FLARE_CALL(sub_00186BC0,0x1877d3);
 g_ecx=g_esp+0x30;PUSH32(g_esp,g_ecx);MEM32(g_esp+0x34)=0x472cf4;FLARE_CALL(sub_0021FC20,0x1877e5);
 g_edx=MEM32(g_esi+0x54);g_esp+=4;PUSH32(g_esp,2);MEM32(g_esi+0xa0)=g_eax;g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_edx);FLARE_CALL(sub_00208110,0x1877ff);
 g_ecx=MEM32(g_esp+0x20);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=0x20;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void flare_update(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_eax=MEM32(g_ebx+0x58);
 if(g_eax!=0xffffffffu){PUSH32(g_esp,g_eax);FLARE_CALL(sub_001F8FC0,0x187831);g_esp+=4;if(!g_eax){g_eax=MEM32(g_ebx);g_ecx=g_ebx;POP32(g_esp,g_ebx);g_seh_ebp=frame;RECOMP_ITAIL(MEM32(g_eax+0x24));return;}}
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx+0xd0);
 if(g_esi){PUSH32(g_esp,g_edi);do{g_eax=MEM32(g_esi+8);g_edi=0;if((int32_t)g_eax>0){do{
  g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx+g_edi*4);g_ecx=MEM32(g_edx);g_eax=MEM32(g_ecx);
  {uint32_t s=g_esp;PUSH32(g_esp,0);FLARE_VCALL(MEM32(g_eax+0x10),0x18786f,s);}
  g_eax=MEM32(g_esi+8);++g_edi;
 }while((int32_t)g_edi<(int32_t)g_eax);}g_esi=MEM32(g_esi+12);}while(g_esi);POP32(g_esp,g_edi);}
 g_eax=MEM32(g_ebx+0xcc);POP32(g_esp,g_esi);
 if((int32_t)g_eax>0){g_ecx=MEM32(g_ebx+0xd4);g_edx=MEM32(g_ebx+0xc4);g_eax=MEM32(g_ecx+4);g_ecx=MEM32(g_eax+g_edx*4);g_ecx=MEM32(g_ecx);g_edx=MEM32(g_ecx);{uint32_t s=g_esp;PUSH32(g_esp,1);FLARE_VCALL(MEM32(g_edx+0x10),0x1878a5,s);}}
 SET_LO8(g_eax,MEM8(g_ebx+0xc8));MEM8(g_ebx+0xb0)=LO8(g_eax);uint32_t count=MEM32(g_ebx+0xcc)-1u;MEM32(g_ebx+0xcc)=count;MEM8(g_ebx+0xc8)=0;POP32(g_esp,g_ebx);g_recomp_zf=count==0;g_esp+=4;
}
static void flare_transform(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-0xd8;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0xd4);PUSH32(g_esp,g_edi);
 if(g_eax){
  g_xmm0=XMM_ZERO();g_eax=g_esp+0x10;PUSH32(g_esp,g_eax);MEMF(g_esp+0x14)=g_xmm0.f[0];MEMF(g_esp+0x18)=g_xmm0.f[0];MEMF(g_esp+0x1c)=g_xmm0.f[0];
  g_xmm0=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,0x40490fdb);PUSH32(g_esp,0);MEMF(g_esp+0x28)=g_xmm0.f[0];FLARE_FRAME_CALL(sub_002D3780,0x187912);
  MEMF(g_esp+4)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_esp+=4;PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_esp+0xb0;FLARE_FRAME_CALL(sub_002D3DC0,0x187929);
  g_edx=MEM32(g_esi+0xd4);g_eax=MEM32(g_edx+4);g_ecx=MEM32(g_esi+0xc4);g_ecx=MEM32(g_eax+g_ecx*4);g_edi=MEM32(g_ecx);
  PUSH32(g_esp,0x1f4b10);PUSH32(g_esp,4);PUSH32(g_esp,16);g_edx=g_esp+0x2c;PUSH32(g_esp,g_edx);FLARE_FRAME_CALL(sub_00011000,0x187950);
  /* Exact retained source SIMD order, including the pushes in the final row. */
  g_xmm0 = XMM_MEM(g_esp + 0xA0);
  g_xmm1 = XMM_MEM(g_esi + 0x40);
  g_xmm2 = XMM_MEM(g_esi + 0x30);
  g_xmm3 = XMM_MEM(g_esi + 0x20);
  g_xmm4 = XMM_MEM(g_esi + 0x10);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0xFF);
  g_xmm6 = g_xmm1;
  g_xmm6 = XMM_MUL(g_xmm6, g_xmm5);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0xAA);
  g_xmm7 = g_xmm2;
  g_xmm7 = XMM_MUL(g_xmm7, g_xmm5);
  g_xmm7 = XMM_ADD(g_xmm7, g_xmm6);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0x55);
  g_xmm6 = g_xmm3;
  g_xmm6 = XMM_MUL(g_xmm6, g_xmm5);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0);
  g_xmm0 = g_xmm4;
  g_xmm0 = XMM_MUL(g_xmm0, g_xmm5);
  g_xmm0 = XMM_ADD(g_xmm0, g_xmm6);
  g_xmm0 = XMM_ADD(g_xmm0, g_xmm7);
  XMM_STORE(g_esp + 0x20, g_xmm0);
  g_xmm0 = XMM_MEM(g_esp + 0xB0);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0xFF);
  g_xmm6 = g_xmm1;
  g_xmm6 = XMM_MUL(g_xmm6, g_xmm5);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0xAA);
  g_xmm7 = g_xmm2;
  g_xmm7 = XMM_MUL(g_xmm7, g_xmm5);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0x55);
  g_xmm7 = XMM_ADD(g_xmm7, g_xmm6);
  g_xmm6 = g_xmm3;
  g_xmm6 = XMM_MUL(g_xmm6, g_xmm5);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0);
  g_xmm0 = g_xmm4;
  g_xmm0 = XMM_MUL(g_xmm0, g_xmm5);
  g_xmm0 = XMM_ADD(g_xmm0, g_xmm6);
  g_xmm0 = XMM_ADD(g_xmm0, g_xmm7);
  XMM_STORE(g_esp + 0x30, g_xmm0);
  g_xmm0 = XMM_MEM(g_esp + 0xC0);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0xFF);
  g_xmm6 = g_xmm1;
  g_xmm6 = XMM_MUL(g_xmm6, g_xmm5);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0xAA);
  g_xmm7 = g_xmm2;
  g_xmm7 = XMM_MUL(g_xmm7, g_xmm5);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0x55);
  g_xmm7 = XMM_ADD(g_xmm7, g_xmm6);
  g_xmm6 = g_xmm3;
  g_xmm6 = XMM_MUL(g_xmm6, g_xmm5);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0);
  g_xmm0 = g_xmm4;
  g_xmm0 = XMM_MUL(g_xmm0, g_xmm5);
  g_xmm0 = XMM_ADD(g_xmm0, g_xmm6);
  g_xmm0 = XMM_ADD(g_xmm0, g_xmm7);
  XMM_STORE(g_esp + 0x40, g_xmm0);
  g_xmm0 = XMM_MEM(g_esp + 0xD0);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0xFF);
  g_xmm1 = XMM_MUL(g_xmm1, g_xmm5);
  g_xmm5 = g_xmm0;
  g_xmm5 = XMM_SHUFFLE(g_xmm5, g_xmm0, 0xAA);
  g_xmm2 = XMM_MUL(g_xmm2, g_xmm5);
  g_xmm2 = XMM_ADD(g_xmm2, g_xmm1);
  PUSH32(g_esp, 0x1F4B10);
  g_xmm1 = g_xmm0;
  g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0x55);
  g_xmm3 = XMM_MUL(g_xmm3, g_xmm1);
  PUSH32(g_esp, 4);
  g_xmm1 = g_xmm0;
  g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0);
  g_xmm4 = XMM_MUL(g_xmm4, g_xmm1);
  PUSH32(g_esp, 0x10);
  g_eax = g_esp + 0x6C;
  g_xmm4 = XMM_ADD(g_xmm4, g_xmm3);
  g_xmm4 = XMM_ADD(g_xmm4, g_xmm2);
  PUSH32(g_esp, g_eax);
  XMM_STORE(g_esp + 0x60, g_xmm4);
  g_ebp = frame;
  PUSH32(g_esp, 0x00187A8Fu); sub_00011000();
  g_xmm0=XMM_MEM(g_esp+0x20);XMM_STORE(g_esp+0x60,g_xmm0);g_xmm0=XMM_MEM(g_esp+0x30);XMM_STORE(g_esp+0x70,g_xmm0);
  g_xmm0=XMM_MEM(g_esp+0x40);XMM_STORE(g_esp+0x80,g_xmm0);g_xmm0=XMM_MEM(g_esp+0x50);g_eax=g_esp+0x60;XMM_STORE(g_esp+0x90,g_xmm0);g_edx=MEM32(g_edi);
  {uint32_t s=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;FLARE_VCALL(MEM32(g_edx+4),0x187ac9,s);}
 }
 g_ecx=g_esi+0x40;g_edx=MEM32(g_ecx);g_esi+=0xa4;MEM32(g_esi)=g_edx;g_eax=MEM32(g_ecx+4);MEM32(g_esi+4)=g_eax;g_ecx=MEM32(g_ecx+8);POP32(g_esp,g_edi);MEM32(g_esi+8)=g_ecx;POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esi==0;g_esp+=4;
}
static void flare_retire(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=g_ecx;PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx+0xd0);
 if(g_esi){PUSH32(g_esp,g_edi);do{g_edi=MEM32(g_esi+12);g_ecx=g_esi;FLARE_CALL(sub_00187B30,0x187b0a);PUSH32(g_esp,g_esi);FLARE_CALL(sub_002D5CCD,0x187b10);g_esp+=4;g_esi=g_edi;}while(g_edi);POP32(g_esp,g_edi);}
 POP32(g_esp,g_esi);g_ecx=g_ebx;POP32(g_esp,g_ebx);g_seh_ebp=frame;sub_0020A620();
}
static void flare_destroy(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x395f66);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;
 MEM32(g_esp+12)=g_edi;g_ecx=g_edi+0x98;MEM32(g_esp+24)=1;FLARE_CALL(sub_001867D0,0x187bd2);
 g_esi=g_edi+0x80;g_ebx=0;MEM32(g_esi)=0x472fbc;g_eax=MEM32(g_esi+8);MEM8(g_esp+24)=LO8(g_ebx);
 if(g_eax!=g_ebx){PUSH32(g_esp,g_eax);FLARE_CALL(sub_002D5CCD,0x187bf1);g_esp+=4;}
 g_ecx=g_esi;MEM32(g_esi+8)=g_ebx;MEM32(g_esi+12)=g_ebx;MEM32(g_esi+16)=g_ebx;FLARE_CALL(sub_001F9140,0x187c04);
 g_ecx=g_edi;MEM32(g_esp+24)=0xffffffffu;FLARE_CALL(sub_0020A480,0x187c13);
 g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void flare_delete(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;FLARE_CALL(sub_00187BA0,0x187b88);
 if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);FLARE_CALL(sub_002D5CCD,0x187b95);g_esp+=4;}
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;
}
static void flare_factory(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x395f7b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xe0);FLARE_CALL(sub_002D57FB,0x187c50);
 g_esp+=4;MEM32(g_esp)=g_eax;unsigned allocated=g_eax!=0;MEM32(g_esp+12)=0;
 if(allocated){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;FLARE_CALL(sub_00187680,0x187c6e);}else g_eax=0;
 g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_hybrid_gun_flare(unsigned entry){
 if(!flare_enabled())return 0;
 switch(entry){case 0x187680:case 0x187820:case 0x1878c0:case 0x187af0:case 0x187b80:case 0x187ba0:case 0x187c30:break;default:return 0;}
 uint32_t s=g_esp,o=g_ecx;
 if((s&3)||s<0x12000u||!flare_span(s-8192,8200)||!flare_span(0x51c4dc,4)||MEM8(0x51c178)||
    !flare_apart(s-8192,8200,0x51c178,1)||(entry==0x1878c0&&(unsigned)g_fp_top>7))return 0;
 if(entry!=0x187c30&&(!flare_span(o,0xe0)||!flare_apart(o,0xe0,s-8192,8200)||!flare_apart(o,0xe0,0x51c178,1)))return 0;
 /* Original completing services own subsequent object validity and alias feedback.
  * No host-owned group/list cache and no decline after the first guest effect. */
 switch(entry){case 0x187680:flare_construct();break;case 0x187820:flare_update();break;case 0x1878c0:flare_transform();break;case 0x187af0:flare_retire();break;case 0x187b80:flare_delete();break;case 0x187ba0:flare_destroy();break;default:flare_factory();break;}
 return 1;
}
#undef FLARE_CALL
#undef FLARE_FRAME_CALL
#undef FLARE_VCALL
