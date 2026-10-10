#include "hybrid_particle_renderer_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0008E920(void),sub_00099980(void),sub_000999C0(void),sub_00099C40(void);
extern void sub_0009C7A0(void),sub_0009C7E0(void),sub_003E2FD0(void),sub_003E35D0(void),sub_003E35F0(void);
extern void sub_003E3840(void),sub_003E55B0(void),sub_003E69B0(void),sub_003E6A00(void),sub_003E7080(void);
extern void sub_003E9350(void),sub_003E9370(void),sub_003F1900(void);
static int renderer_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_RENDERER");flag=v&&!strcmp(v,"1");}return flag;}
static int renderer_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int renderer_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define DRAW_CALL(fn,ret) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
#define DRAW_VCALL(target,ret,saved) do{uint32_t t_=(target);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(t_,saved);}while(0)
int uc_hybrid_particle_renderer_plain(void){
 if(!renderer_enabled())return 0;
 uint32_t start=g_esp,owner=g_ecx,frame=g_ebp;
 if((start&3)||start<0x11000u||!renderer_span(start-4096,4104)||!renderer_span(owner,8)||
    !renderer_apart(owner,8,start-4096,4104)||!renderer_span(0x51c178,1)||MEM8(0x51c178)||
    !renderer_apart(start-4096,4104,0x51c178,1)||!renderer_apart(owner,8,0x51c178,1))return 0;
 /* Completing callbacks own the same live-object/queue validity as the source.
  * There is no snapshot, reordered draw, or rejection after the prologue. */
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-0x664;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);
 PUSH32(g_esp,g_edi);MEM32(g_esp+0x28)=g_esi;DRAW_VCALL(MEM32(g_eax+0x10),0x9b16d,start);
 g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);--g_eax;
 {uint32_t carry=g_eax!=0;g_eax=0u-g_eax;SET_LO8(g_eax,carry?0xff:0);SET_LO8(g_eax,(uint8_t)(LO8(g_eax)+1));}
 MEM8(g_esp+0x13)=LO8(g_eax);
 {uint32_t saved=g_esp;DRAW_VCALL(MEM32(g_edx+0xa4),0x9b186,saved);}
 g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;DRAW_VCALL(MEM32(g_edx+0x1c),0x9b18d,saved);}
 g_edi=MEM32(g_eax+0x10);g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);MEM32(g_esp+0x50)=g_edi;
 {uint32_t saved=g_esp;DRAW_VCALL(MEM32(g_eax+0x50),0x9b19f,saved);}
 g_xmm0=XMM_SCALAR(MEMF(0x51a344));g_ecx=MEM32(g_esi+4);g_xmm0.f[0]-=MEMF(0x456a1c);
 g_edx=MEM32(g_ecx);g_ebx=g_eax;g_eax=g_esp+0x68;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=g_esp+0x68;PUSH32(g_esp,g_eax);g_eax=g_esp+0x24;PUSH32(g_esp,g_eax);
  MEM32(g_esp+0x30)=g_ebx;MEMF(0x51a344)=g_xmm0.f[0];DRAW_VCALL(MEM32(g_edx+8),0x9b1d4,saved);}
 g_eax=MEM32(g_esp+0x1c);if(!g_eax)goto finish;
 g_edx=MEM32(g_ebx);g_ecx=g_ebx;{uint32_t saved=g_esp;DRAW_VCALL(MEM32(g_edx+12),0x9b1e7,saved);}
 g_xmm0=XMM_MEM(g_eax);XMM_STORE(g_esp+0xd0,g_xmm0);g_xmm0=XMM_MEM(g_eax+16);XMM_STORE(g_esp+0xe0,g_xmm0);
 g_xmm0=XMM_MEM(g_eax+32);XMM_STORE(g_esp+0xf0,g_xmm0);g_xmm0=XMM_MEM(g_eax+48);g_eax=MEM32(g_ebx);g_ecx=g_ebx;XMM_STORE(g_esp+0x100,g_xmm0);
 {uint32_t saved=g_esp;DRAW_VCALL(MEM32(g_eax+4),0x9b21d,saved);}
 g_xmm0=XMM_MEM(g_eax);g_edx=MEM32(g_ebx);XMM_STORE(g_esp+0x110,g_xmm0);g_xmm0=XMM_MEM(g_eax+16);XMM_STORE(g_esp+0x120,g_xmm0);
 g_xmm0=XMM_MEM(g_eax+32);XMM_STORE(g_esp+0x130,g_xmm0);g_xmm0=XMM_MEM(g_eax+48);g_ecx=g_ebx;XMM_STORE(g_esp+0x140,g_xmm0);
 {uint32_t saved=g_esp;DRAW_VCALL(MEM32(g_edx+0x18),0x9b253,saved);}
 g_xmm0=XMM_MEM(g_eax);g_xmm1=XMM_SCALAR(MEMF(g_esp+0xd4));XMM_STORE(g_esp+0x80,g_xmm0);
 g_xmm0=XMM_MEM(g_eax+16);XMM_STORE(g_esp+0x90,g_xmm0);g_xmm0=XMM_MEM(g_eax+32);XMM_STORE(g_esp+0xa0,g_xmm0);
 g_xmm0=XMM_MEM(g_eax+48);XMM_STORE(g_esp+0xb0,g_xmm0);g_xmm0=XMM_SCALAR(MEMF(g_esp+0xd0));MEMF(g_esp+0xc0)=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(g_esp+0xe0));MEMF(g_esp+0xc4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0xf0));MEMF(g_esp+0x70)=g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(MEMF(g_esp+0xe4));g_eax=g_esp+0x150;MEMF(g_esp+0xc8)=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEMF(g_esp+0x74)=g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(MEMF(g_esp+0xf4));PUSH32(g_esp,g_eax);g_ecx=g_esp+0x84;MEMF(g_esp+0xd0)=g_xmm0.f[0];MEMF(g_esp+0x7c)=g_xmm1.f[0];MEMF(g_esp+0x80)=g_xmm0.f[0];
 DRAW_CALL(sub_0008E920,0x9b30e);
 PUSH32(g_esp,16);g_edx=g_eax;g_ecx=0x68;DRAW_CALL(sub_003E6A00,0x9b31c);
 PUSH32(g_esp,16);g_edx=0x4f87d0;g_ecx=0x74;DRAW_CALL(sub_003E6A00,0x9b32d);
 PUSH32(g_esp,16);g_edx=0x4f8810;g_ecx=0x70;DRAW_CALL(sub_003E6A00,0x9b33e);
 g_edx=g_esp+0x70;g_ecx=0x78;DRAW_CALL(sub_003E69B0,0x9b34c);g_edx=g_esp+0xc0;g_ecx=0x79;DRAW_CALL(sub_003E69B0,0x9b35d);
 g_ecx=MEM32(g_esp+0x68);g_edx=MEM32(g_esp+0x64);PUSH32(g_esp,g_edx);MEM32(g_esp+0x18)=g_ecx;PUSH32(g_esp,g_edi);g_ecx=g_esi;DRAW_CALL(sub_000999C0,0x9b372);
 g_edi=1;do{PUSH32(g_esp,0);PUSH32(g_esp,g_edi);DRAW_CALL(sub_003E3840,0x9b37f);++g_edi;}while((int32_t)g_edi<4);
 g_ecx=MEM32(g_esp+0x14);if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);DRAW_VCALL(MEM32(g_eax+0x1c),0x9b394,saved);}
 g_ecx=MEM32(0x51a204);PUSH32(g_esp,g_ecx);DRAW_CALL(sub_0009C7A0,0x9b3a0);
 SET_LO8(g_eax,MEM8(g_esp+0x17));g_esp+=4;
 if(LO8(g_eax)){
  g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;DRAW_VCALL(MEM32(g_edx+0x9c),0x9b3bd,saved);}
  g_edx=MEM32(g_esp+0x1c);g_edi=MEM32(g_eax+0x28);g_ecx=MEM32(g_eax+0x34);PUSH32(g_esp,0);g_edi+=g_edx;PUSH32(g_esp,g_ecx);MEM32(g_eax+0x28)=g_edi;DRAW_CALL(sub_003F1900,0x9b3d4);
  g_edi=g_eax;MEM32(g_esp+0x4c)=g_edi;DRAW_CALL(sub_003E35D0,0x9b3df);MEM32(g_esp+0x60)=g_eax;DRAW_CALL(sub_003E35F0,0x9b3e8);
  PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);MEM32(g_esp+0x34)=g_eax;DRAW_CALL(sub_003E2FD0,0x9b3f3);
  g_eax=MEM32(0x51a274);PUSH32(g_esp,g_eax);DRAW_CALL(sub_0009C7E0,0x9b3fe);g_esp+=4;g_edx=0x1000100;g_ecx=0x40358;DRAW_CALL(sub_003E7080,0x9b410);
  g_edx=0;g_ecx=0x40300;MEM32(0x3f40ac)=0x1000100;DRAW_CALL(sub_003E7080,0x9b426);MEM32(0x3f4090)=0;
 }else{g_ecx=MEM32(0x51a1f4);PUSH32(g_esp,g_ecx);DRAW_CALL(sub_0009C7E0,0x9b43e);g_esp+=4;}
 g_eax=MEM32(g_esp+0x1c);MEM32(g_esp+0x18)=0;
 if((int32_t)g_eax>0){
  g_xmm0=XMM_ZERO();g_xmm1=XMM_SCALAR(MEMF(0x44bf94));MEMF(g_esp+0x54)=g_xmm1.f[0];MEMF(g_esp+0x58)=g_xmm0.f[0];MEMF(g_esp+0x5c)=g_xmm0.f[0];MEMF(g_esp+0x30)=g_xmm0.f[0];MEMF(g_esp+0x34)=g_xmm1.f[0];MEMF(g_esp+0x38)=g_xmm0.f[0];
  do{
   g_eax-=MEM32(g_esp+0x18);g_edi=39;if((int32_t)g_eax<=39)g_edi=g_eax;
   g_esi=MEM32(g_esi+4);g_eax=MEM32(g_esi);g_edx=g_esp+0x400;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_ebx);g_ecx=g_esp+0x194;PUSH32(g_esp,g_ecx);g_ecx=g_ebx;MEM32(g_esp+0x1c)=g_eax;DRAW_VCALL(MEM32(g_edx+8),0x9b4b4,saved);}
   {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x24);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x28);g_ecx=g_esi;DRAW_VCALL(MEM32(g_eax+12),0x9b4c4,saved);}
   g_esi=g_edi;g_esi*=0x34;g_ecx=g_esi+5;PUSH32(g_esp,g_ecx);MEM32(g_esp+0x4c)=g_esi;DRAW_CALL(sub_003E9350,0x9b4d6);
   g_ecx=MEM32(g_esp+0x54);g_edx=(g_esi<<18)+0x40001818;MEM32(g_eax+8)=g_edx;g_edx=MEM32(g_esp+0x58);MEM32(g_esp+0x9c)=g_ecx;
   g_ecx=MEM32(g_esp+0x5c);MEM32(g_esp+0xa0)=g_edx;g_edx=MEM32(g_esp+0x30);MEM32(g_eax)=0x417fc;MEM32(g_eax+4)=8;g_eax+=12;
   MEM32(g_esp+0xa4)=g_ecx;g_ecx=MEM32(g_esp+0x34);MEM32(g_esp+0xa8)=g_edx;g_edx=MEM32(g_esp+0x38);MEM32(g_esp+0x6c)=g_eax;MEM32(g_esp+0xac)=g_ecx;MEM32(g_esp+0xb0)=g_edx;
   if((int32_t)g_edi>0){
    g_esi=0;g_ebx=g_eax+0x34;MEM32(g_esp+0x20)=g_esi;MEM32(g_esp+0x14)=g_edi;
    do{
     g_eax=g_esp+g_esi+0x400;PUSH32(g_esp,g_eax);DRAW_CALL(sub_00099980,0x9b56d);
     g_xmm0=XMM_SCALAR(MEMF(g_esp+g_esi+0x194));g_ecx=g_esp+g_esi+0x19c;MEMF(g_esp+0x40)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ecx-4));g_edx=MEM32(g_esp+0x40);MEMF(g_esp+0x44)=g_xmm0.f[0];
     g_xmm0=XMM_SCALAR(MEMF(g_ecx));MEM32(g_esp+0x84)=g_edx;g_edx=MEM32(g_esp+0x44);MEMF(g_esp+0x48)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ecx+4));MEM32(g_esp+0x88)=g_edx;
     g_edx=MEM32(g_esp+0x48);MEMF(g_esp+0x94)=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEM32(g_esp+0x90)=g_eax;g_eax=MEM32(g_esp+0x18);MEM32(g_esp+0x8c)=g_edx;
     g_edi=g_ebx-52;g_ecx=13;g_esi=g_esp+0x84;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
     g_edi=g_ebx;g_ecx=13;g_esi=g_esp+0x84;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
     MEMF(g_ebx-32)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_edi=g_ebx+52;g_ecx=13;g_esi=g_esp+0x84;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
     MEMF(g_ebx+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44c530));g_edi=g_ebx+104;g_ecx=13;g_esi=g_esp+0x84;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
     g_ecx=MEM32(g_esp+0x24);MEMF(g_ebx+0x48)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x46c31c));MEMF(g_ebx+0x7c)=g_xmm0.f[0];g_ecx+=16;g_esp+=4;g_ebx+=208;--g_eax;MEM32(g_esp+0x20)=g_ecx;MEM32(g_esp+0x14)=g_eax;
     if(g_eax)g_esi=MEM32(g_esp+0x20);
    }while(g_eax);
    g_esi=MEM32(g_esp+0x48);g_eax=MEM32(g_esp+0x6c);
   }
   g_ebx=MEM32(g_esp+0x24);MEM32(g_eax+g_esi*4)=0x417fc;MEM32(g_eax+g_esi*4+4)=0;g_eax+=g_esi*4+8;PUSH32(g_esp,g_eax);DRAW_CALL(sub_003E9370,0x9b688);
   g_ecx=MEM32(g_esp+0x18);g_eax=MEM32(g_esp+0x1c);g_esi=MEM32(g_esp+0x28);g_ecx+=39;MEM32(g_esp+0x18)=g_ecx;
  }while((int32_t)g_ecx<(int32_t)g_eax);
 }
 g_edx=MEM32(g_ebx);g_eax=g_esp+0x110;{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_ebx;DRAW_VCALL(MEM32(g_edx+0x1c),0x9b6b2,saved);}
 g_edx=MEM32(g_ebx);g_eax=g_esp+0xd0;{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_ebx;DRAW_VCALL(MEM32(g_edx+0x24),0x9b6c1,saved);}
 g_edx=MEM32(g_ebx);g_ecx=g_ebx;{uint32_t saved=g_esp;DRAW_VCALL(MEM32(g_edx+0x30),0x9b6c8,saved);}
 g_eax=MEM32(g_esp+0x50);PUSH32(g_esp,g_eax);g_ecx=g_esi;DRAW_CALL(sub_00099C40,0x9b6d4);SET_LO8(g_eax,MEM8(g_esp+0x13));
 if(LO8(g_eax)){
  g_ebx=MEM32(g_esp+0x2c);g_edi=MEM32(g_esp+0x60);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);DRAW_CALL(sub_003E2FD0,0x9b6eb);
  g_edx=0x10101;g_ecx=0x40358;DRAW_CALL(sub_003E7080,0x9b6fa);g_ecx=MEM32(g_esp+0x4c);PUSH32(g_esp,g_ecx);MEM32(0x3f40ac)=0x10101;DRAW_CALL(sub_003E55B0,0x9b70e);
  PUSH32(g_esp,g_edi);DRAW_CALL(sub_003E55B0,0x9b714);PUSH32(g_esp,g_ebx);DRAW_CALL(sub_003E55B0,0x9b71a);
 }
 finish:
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;DRAW_VCALL(MEM32(g_edx+0x18),0x9b722,saved);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=8;return 1;
}
#undef DRAW_CALL
#undef DRAW_VCALL
