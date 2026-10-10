#include "hybrid_lightning_beam_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_0020BDC0(void),sub_001F8DC0(void),sub_0021FC20(void),sub_001F9120(void),sub_001752D0(void);
extern void sub_001749E0(void),sub_001F92B0(void),sub_00174AE0(void),sub_00208110(void),sub_002D37D0(void);
extern void sub_00175740(void),sub_001F8FC0(void),sub_001F9140(void),sub_0020A480(void),sub_002D5CCD(void);
extern void sub_002D57FB(void),sub_00175360(void),sub_00174620(void),sub_00175C40(void),sub_000F9130(void);
extern void sub_000159C0(void),sub_00174550(void),sub_001F8DE0(void),sub_00207CD0(void),sub_001F8BA0(void),sub_001F90A0(void);
static int beam_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_LIGHTNING_BEAM");f=p&&!strcmp(p,"1");}return f;}
static int beam_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int beam_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define BEAM_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define BEAM_FRAME_CALL(fn,pc) do{g_ebp=frame;BEAM_CALL(fn,pc);}while(0)
#define BEAM_VCALL(target,pc,saved) do{uint32_t t_=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(t_,saved);}while(0)
static void beam_construct(void){
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x394e5c);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x34;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+0x1c)=g_esi;BEAM_CALL(sub_0020BDC0,0x175387);
 g_eax=0;frame=g_esi+0x80;MEM32(g_esp+0x4c)=g_eax;MEM32(frame)=0x4527c8;MEM32(g_esi)=0x470058;MEM32(frame)=0x47003c;
 g_edi=g_esi+0x2d8;MEM32(g_edi)=0x470004;MEM32(g_edi+8)=g_eax;MEM32(g_edi+12)=g_eax;MEM32(g_edi+16)=g_eax;MEM32(g_edi+20)=g_eax;
 g_ebx=MEM32(g_esp+0x54);g_eax=g_esp+0x2c;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x458968);PUSH32(g_esp,4);g_ecx=g_ebx;MEM8(g_esp+0x58)=3;BEAM_CALL(sub_001F8DC0,0x1753db);
 if(!LO8(g_eax)){g_xmm0=XMM_ZERO();MEMF(g_esp+0x20)=g_xmm0.f[0];MEMF(g_esp+0x24)=g_xmm0.f[0];}
 else{g_xmm0=XMM_SCALAR(MEMF(g_esp+0x2c));MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x30));MEMF(g_esp+0x24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x34));}
 g_edx=MEM32(g_esp+0x20);g_eax=MEM32(g_esp+0x24);g_ecx=g_esi+0x2b4;MEM32(g_ecx)=g_edx;MEM32(g_ecx+4)=g_eax;
 g_eax=g_esp+0x2c;PUSH32(g_esp,g_eax);MEMF(g_esp+0x2c)=g_xmm0.f[0];g_edx=MEM32(g_esp+0x2c);PUSH32(g_esp,0x458958);MEM32(g_ecx+8)=g_edx;PUSH32(g_esp,4);g_ecx=g_ebx;BEAM_CALL(sub_001F8DC0,0x175441);
 if(!LO8(g_eax)){g_xmm0=XMM_ZERO();MEMF(g_esp+0x20)=g_xmm0.f[0];MEMF(g_esp+0x24)=g_xmm0.f[0];}
 else{g_xmm0=XMM_SCALAR(MEMF(g_esp+0x2c));MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x30));MEMF(g_esp+0x24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x34));}
 g_edx=MEM32(g_esp+0x20);g_eax=MEM32(g_esp+0x24);g_ecx=g_esi+0x2c0;MEM32(g_ecx)=g_edx;MEM32(g_ecx+4)=g_eax;
 g_eax=g_esp+0x54;PUSH32(g_esp,g_eax);MEMF(g_esp+0x2c)=g_xmm0.f[0];g_edx=MEM32(g_esp+0x2c);PUSH32(g_esp,0x450ef0);MEM32(g_ecx+8)=g_edx;PUSH32(g_esp,2);g_ecx=g_ebx;BEAM_CALL(sub_001F8DC0,0x1754a7);
 g_xmm0=XMM_SCALAR(MEMF(0x44d964));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+0x54));
 g_ecx=g_esp+0x54;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x450e84);PUSH32(g_esp,2);g_ecx=g_ebx;MEMF(g_esi+0x2d4)=g_xmm0.f[0];BEAM_CALL(sub_001F8DC0,0x1754d4);
 g_xmm0=XMM_ZERO();if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+0x54));
 g_edx=g_esp+0x54;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x45893c);PUSH32(g_esp,2);g_ecx=g_ebx;MEMF(g_esi+0x2cc)=g_xmm0.f[0];BEAM_CALL(sub_001F8DC0,0x1754fc);
 g_xmm0=XMM_ZERO();if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+0x54));
 g_eax=g_esp+0x54;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x458930);PUSH32(g_esp,2);g_ecx=g_ebx;MEMF(g_esp+0x20)=g_xmm0.f[0];BEAM_CALL(sub_001F8DC0,0x175522);
 g_xmm0=XMM_SCALAR(MEMF(0x44bf94));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+0x54));
 g_ecx=g_esp+0x18;MEMF(g_esp+0x54)=g_xmm0.f[0];g_xmm0=XMM_ZERO();PUSH32(g_esp,g_ecx);MEMF(g_esi+0x2d0)=g_xmm0.f[0];MEM32(g_esp+0x1c)=0x470010;BEAM_CALL(sub_0021FC20,0x175557);
 MEM32(g_esi+0x2b0)=g_eax;g_eax=MEM32(g_esp+0x58);g_ecx=MEM32(0x5150b0);g_edx=MEM32(g_ecx);g_esp+=4;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x18);PUSH32(g_esp,g_eax);PUSH32(g_esp,frame);BEAM_VCALL(MEM32(g_edx+0x90),0x175579,saved);}
 MEM32(g_esi+0x84)=g_eax;MEM32(g_edi+20)=g_esi;g_ecx=MEM32(g_esi+0x54);PUSH32(g_esp,g_ecx);g_ecx=g_edi;BEAM_CALL(sub_001F9120,0x17558d);
 g_ebx=MEM32(0x4f9e8c);g_edx=0;g_edi=0;g_ecx=0;MEM32(g_esp+0x3c)=g_edx;MEM32(g_esp+0x40)=g_edi;g_edx=g_esp+0x2c;g_eax=0x174aa0;MEM32(g_esp+0x38)=g_ecx;g_edi=g_esi+0x2dc;
 PUSH32(g_esp,g_edx);g_ecx=g_edi;MEM32(g_esp+0x30)=g_ebx;MEM32(g_esp+0x38)=g_eax;BEAM_CALL(sub_001752D0,0x1755c4);
 frame=MEM32(0x4f9e88);g_eax=0x174ac0;g_ecx=0;MEM32(g_esp+0x34)=g_eax;g_eax=g_esp+0x2c;g_edx=0;MEM32(g_esp+0x38)=g_ecx;g_ebx=0;
 PUSH32(g_esp,g_eax);g_ecx=g_edi;MEM32(g_esp+0x30)=frame;MEM32(g_esp+0x40)=g_edx;MEM32(g_esp+0x44)=g_ebx;BEAM_CALL(sub_001752D0,0x1755f5);
 g_ecx=g_esi+0x88;BEAM_CALL(sub_001749E0,0x175600);g_ecx=g_esi+0xa0;BEAM_CALL(sub_001749E0,0x17560b);
 g_ecx=g_esp+0x54;PUSH32(g_esp,0x46ff54);PUSH32(g_esp,g_ecx);BEAM_CALL(sub_001F92B0,0x17561a);
 g_xmm0=XMM_ZERO();g_esp+=8;g_ecx=g_esi;MEMF(g_esi+0x2f0)=g_xmm0.f[0];BEAM_CALL(sub_00174AE0,0x17562f);
 g_edx=MEM32(g_esi+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,3);PUSH32(g_esp,g_edx);BEAM_CALL(sub_00208110,0x175640);
 g_ecx=MEM32(g_esp+0x44);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp+=0x40;g_recomp_zf=g_esp==0;g_esp+=8;
}
static unsigned beam_compare(double a,double b){return a!=a||b!=b?0x47u:a<b?3u:a>b?2u:0x42u;}
static void beam_update(void){
 uint32_t frame=g_seh_ebp;g_eax=MEM32(0x51c4dc);g_xmm0=XMM_SCALAR(MEMF(g_eax+20));PUSH32(g_esp,g_esi);g_esi=g_ecx;g_xmm1=XMM_SCALAR(MEMF(g_esi+0x2cc));
 unsigned flags=beam_compare(g_xmm1.f[0],MEMF(0x44c174));g_xmm0.f[0]+=MEMF(g_esi+0x2d0);SET_HI8(g_eax,flags);MEMF(g_esi+0x2d0)=g_xmm0.f[0];
 if(RECOMP_PARITY8(HI8(g_eax)&0x44u)){flags=beam_compare(g_xmm0.f[0],g_xmm1.f[0]);if(!(flags&0x41u))MEM32(g_esi+0x5c)=1;}
 g_ecx=MEM32(0x51c4dc);g_xmm0=XMM_SCALAR(MEMF(g_ecx+20));g_xmm0.f[0]*=MEMF(0x44ec38);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];BEAM_CALL(sub_002D37D0,0x1756bc);g_esp+=4;
 if(LO8(g_eax)){g_ecx=g_esi+0x88;BEAM_CALL(sub_001749E0,0x1756ce);}
 g_edx=MEM32(0x51c4dc);g_xmm0=XMM_SCALAR(MEMF(g_edx+20));g_xmm0.f[0]*=MEMF(0x44ec38);PUSH32(g_esp,g_ecx);MEMF(g_esp)=g_xmm0.f[0];BEAM_CALL(sub_002D37D0,0x1756ec);g_esp+=4;
 if(LO8(g_eax)){g_ecx=g_esi+0xa0;BEAM_CALL(sub_001749E0,0x1756fe);}
 g_ecx=g_esi+0x88;BEAM_CALL(sub_00175740,0x175709);g_ecx=g_esi+0xa0;BEAM_CALL(sub_00175740,0x175714);
 g_eax=MEM32(g_esi+0x58);if(g_eax!=0xffffffffu){PUSH32(g_esp,g_eax);BEAM_CALL(sub_001F8FC0,0x175722);g_esp+=4;if(!g_eax){g_eax=MEM32(g_esi);g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;RECOMP_ITAIL(MEM32(g_eax+0x24));return;}}
 POP32(g_esp,g_esi);g_esp+=4;
}
static void beam_destroy(void){
 uint32_t saved=g_esp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x394eaf);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);
 MEM32(g_esp+12)=g_esi;MEM32(g_esi)=0x470058;MEM32(g_esi+0x80)=0x47003c;g_ecx=MEM32(0x5150b0);g_edx=MEM32(g_esi+0x84);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edx);MEM32(g_esp+0x1c)=2;BEAM_VCALL(MEM32(g_eax+0x94),0x175c8d,saved);
 g_edi=g_esi+0x2d8;MEM32(g_edi)=0x470004;g_eax=MEM32(g_edi+8);unsigned present=g_eax!=0;MEM8(g_esp+0x18)=1;if(present){PUSH32(g_esp,g_eax);BEAM_CALL(sub_002D5CCD,0x175cab);g_esp+=4;}
 g_ecx=g_edi;MEM32(g_edi+8)=0;MEM32(g_edi+12)=0;MEM32(g_edi+16)=0;BEAM_CALL(sub_001F9140,0x175cca);
 g_ecx=g_esi;MEM32(g_esi+0x80)=0x4527c8;MEM32(g_esp+0x18)=0xffffffffu;BEAM_CALL(sub_0020A480,0x175ce3);
 g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void beam_delete(int attached){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;if(attached){BEAM_CALL(sub_000F9130,0x1b2708);}else{BEAM_CALL(sub_00175C40,0x175c28);}
 if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);if(attached){BEAM_CALL(sub_002D5CCD,0x1b2715);}else{BEAM_CALL(sub_002D5CCD,0x175c35);}g_esp+=4;}
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;
}
static void beam_factory(int attached){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,attached?0x394deb:0x394ecb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,attached?0x90:0x300);
 if(attached){BEAM_CALL(sub_002D57FB,0x174950);}else{BEAM_CALL(sub_002D57FB,0x175d20);}
 g_esp+=4;MEM32(g_esp)=g_eax;unsigned allocated=g_eax!=0;MEM32(g_esp+12)=0;
 if(allocated){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;if(attached){BEAM_CALL(sub_00174620,0x17496e);}else{BEAM_CALL(sub_00175360,0x175d3e);}}else g_eax=0;
 g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void attached_construct(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x394dd6);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x2dc;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+0x14)=g_esi;BEAM_FRAME_CALL(sub_0020BDC0,0x17464e);
 g_edi=MEM32(frame+8);g_eax=g_esp+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x450ec8);PUSH32(g_esp,1);g_ecx=g_edi;MEM32(g_esp+0x2f8)=0;MEM32(g_esi)=0x46ff98;BEAM_FRAME_CALL(sub_001F8DC0,0x174675);
 if(LO8(g_eax))g_eax=MEM32(g_esp+12);else g_eax=0xffffffffu;
 g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x450ee4);PUSH32(g_esp,11);g_ecx=g_edi;MEM32(g_esi+0x80)=g_eax;BEAM_FRAME_CALL(sub_001F8DC0,0x17469b);
 unsigned present=LO8(g_eax)!=0;g_eax=0x44d9b8;if(present)g_eax=MEM32(g_esp+12);
 g_edx=g_esp+12;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x450ed8);PUSH32(g_esp,11);g_ecx=g_edi;MEM32(g_esi+0x84)=g_eax;BEAM_FRAME_CALL(sub_001F8DC0,0x1746c1);
 present=LO8(g_eax)!=0;g_eax=0x44d9b8;if(present)g_eax=MEM32(g_esp+12);
 MEM32(g_esi+0x88)=g_eax;g_eax=g_esp+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x450ef0);PUSH32(g_esp,2);g_ecx=g_edi;BEAM_FRAME_CALL(sub_001F8DC0,0x1746e7);
 g_xmm0=XMM_SCALAR(MEMF(0x44d964));if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+12));
 g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x450e84);PUSH32(g_esp,2);g_ecx=g_edi;MEMF(g_esp+0x1c)=g_xmm0.f[0];BEAM_FRAME_CALL(sub_001F8DC0,0x174712);
 g_xmm0=XMM_ZERO();if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(g_esp+12));
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0);g_ecx=g_esp+0x38;MEMF(g_esp+0x14)=g_xmm0.f[0];BEAM_FRAME_CALL(sub_000159C0,0x174732);
 g_edx=MEM32(g_esi+0x84);PUSH32(g_esp,g_edx);g_eax=g_esp+0x28;PUSH32(g_esp,g_eax);g_ecx=g_esi;MEM8(g_esp+0x2f4)=1;BEAM_FRAME_CALL(sub_00174550,0x17474d);
 PUSH32(g_esp,g_eax);PUSH32(g_esp,0x458968);PUSH32(g_esp,4);g_ecx=g_esp+0x3c;BEAM_FRAME_CALL(sub_001F8DE0,0x17475e);
 g_ecx=MEM32(g_esi+0x88);PUSH32(g_esp,g_ecx);g_edx=g_esp+0x1c;PUSH32(g_esp,g_edx);g_ecx=g_esi;BEAM_FRAME_CALL(sub_00174550,0x174771);
 PUSH32(g_esp,g_eax);PUSH32(g_esp,0x458958);PUSH32(g_esp,4);g_ecx=g_esp+0x3c;BEAM_FRAME_CALL(sub_001F8DE0,0x174782);
 g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_eax=g_esp+16;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x450ef0);PUSH32(g_esp,2);g_ecx=g_esp+0x3c;MEMF(g_esp+0x1c)=g_xmm0.f[0];BEAM_FRAME_CALL(sub_001F8DE0,0x1747a3);
 g_xmm0=XMM_SCALAR(MEMF(g_esp+12));g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x450e84);PUSH32(g_esp,2);g_ecx=g_esp+0x3c;MEMF(g_esp+0x18)=g_xmm0.f[0];BEAM_FRAME_CALL(sub_001F8DE0,0x1747c4);
 g_edx=g_esp+0x30;PUSH32(g_esp,g_edx);PUSH32(g_esp,0xffffffffu);g_eax=g_esp+0x14;PUSH32(g_esp,0x458948);PUSH32(g_esp,g_eax);BEAM_FRAME_CALL(sub_001F92B0,0x1747da);
 g_ecx=MEM32(0x51c4dc);g_esp+=8;PUSH32(g_esp,g_eax);BEAM_FRAME_CALL(sub_00207CD0,0x1747e9);
 MEM32(g_esi+0x8c)=g_eax;g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,2);PUSH32(g_esp,g_eax);BEAM_FRAME_CALL(sub_00208110,0x174800);
 g_ecx=g_esp+0x30;MEM8(g_esp+0x2ec)=0;BEAM_FRAME_CALL(sub_001F8BA0,0x174811);
 g_ecx=MEM32(g_esp+0x2e4);POP32(g_esp,g_edi);g_eax=g_esi;MEM32(0)=g_ecx;POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=8;
}
static void attached_update(void){
 g_esp-=0x34;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x80);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);BEAM_CALL(sub_001F8FC0,0x174843);
 g_ecx=MEM32(g_esi+0x8c);PUSH32(g_esp,g_ecx);g_edi=g_eax;BEAM_CALL(sub_001F8FC0,0x174851);g_esp+=8;
 if(g_edi&&g_eax){
  g_edx=MEM32(g_esi+0x84);PUSH32(g_esp,g_edx);g_eax=g_esp+0x28;PUSH32(g_esp,g_eax);g_ecx=g_esi;BEAM_CALL(sub_00174550,0x174877);
  g_ecx=MEM32(g_eax);MEM32(g_esp+12)=g_ecx;g_edx=MEM32(g_eax+4);g_ecx=MEM32(g_esi+0x54);MEM32(g_esp+16)=g_edx;g_eax=MEM32(g_eax+8);g_edx=g_esp+8;
  PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+0x8c);MEM32(g_esp+0x18)=g_eax;PUSH32(g_esp,12);MEM32(g_esp+16)=g_ecx;g_ecx=MEM32(0x4f9e8c);g_eax=g_esp+0x14;
  PUSH32(g_esp,g_eax);g_eax=MEM32(0x51c398);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_eax+8);PUSH32(g_esp,g_edx);BEAM_CALL(sub_001F90A0,0x1748b9);
  g_ecx=MEM32(g_esi+0x88);PUSH32(g_esp,g_ecx);g_edx=g_esp+0x34;PUSH32(g_esp,g_edx);g_ecx=g_esi;BEAM_CALL(sub_00174550,0x1748cc);
  g_ecx=MEM32(g_eax);MEM32(g_esp+0x18)=g_ecx;g_edx=MEM32(g_eax+4);g_ecx=MEM32(g_esi+0x54);MEM32(g_esp+0x1c)=g_edx;g_eax=MEM32(g_eax+8);g_edx=g_esp+8;
  PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+0x8c);MEM32(g_esp+0x24)=g_eax;PUSH32(g_esp,12);MEM32(g_esp+16)=g_ecx;g_ecx=MEM32(0x4f9e88);g_eax=g_esp+0x20;
  PUSH32(g_esp,g_eax);g_eax=MEM32(0x51c398);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_eax+8);PUSH32(g_esp,g_edx);BEAM_CALL(sub_001F90A0,0x17490e);
 }else{g_edx=MEM32(g_esi);g_ecx=g_esi;uint32_t saved=g_esp;BEAM_VCALL(MEM32(g_edx+0x24),0x17491b,saved);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=0x34;g_recomp_zf=g_esp==0;g_esp+=4;
}

int uc_hybrid_lightning_beam(unsigned entry){
 if(!beam_enabled())return 0;
 switch(entry){case 0x175360:case 0x175660:case 0x175c10:case 0x175c20:case 0x175c40:case 0x175d00:case 0x174620:case 0x174830:case 0x174930:case 0x1b2700:case 0x174aa0:case 0x174ac0:break;default:return 0;}
 uint32_t s=g_esp,o=g_ecx;unsigned factory=entry==0x175d00||entry==0x174930,attached=entry==0x1b2700||entry==0x174620||entry==0x174830,bytes=attached?0x90:0x300;
 if((s&3)||s<0x12000u||!beam_span(s-8192,8224)||!beam_span(0x51c4dc,4)||MEM8(0x51c178)||!beam_apart(s-8192,8224,0x51c178,1))return 0;
 if(!factory&&(!beam_span(o,bytes)||!beam_apart(o,bytes,s-8192,8224)||!beam_apart(o,bytes,0x51c178,1)))return 0;
 if(entry==0x1b2700&&MEM32(o)!=0x46ff98)return 0;
 /* No fallback after this point. Original completing services retain all
  * subsequent pointer validity, callback timing and guest ownership. */
 switch(entry){
 case 0x175360:beam_construct();break;
 case 0x175660:beam_update();break;
 case 0x175c10:{uint32_t frame=g_seh_ebp;g_seh_ebp=frame;sub_00174AE0();break;}
 case 0x175c40:beam_destroy();break;
 case 0x175c20:beam_delete(0);break;
 case 0x1b2700:beam_delete(1);break;
 case 0x175d00:beam_factory(0);break;
 case 0x174930:beam_factory(1);break;
 case 0x174620:attached_construct();break;
 case 0x174830:attached_update();break;
 default:g_eax=MEM32(g_esp+4);g_edx=MEM32(g_eax);g_ecx+=entry==0x174aa0?0x2b4:0x2c0;MEM32(g_ecx)=g_edx;g_edx=MEM32(g_eax+4);MEM32(g_ecx+4)=g_edx;g_eax=MEM32(g_eax+8);MEM32(g_ecx+8)=g_eax;g_recomp_zf=g_ecx==0;g_esp+=12;break;
 }
 return 1;
}
#undef BEAM_CALL
#undef BEAM_FRAME_CALL
#undef BEAM_VCALL
