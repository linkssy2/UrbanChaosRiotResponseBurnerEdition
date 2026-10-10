#include "hybrid_particle_rotating_draw_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#if defined(UCRR_HYBRID_NATIVE_PARTICLES)
#include "hybrid_particles_guest.h"
#endif
#if defined(UCRR_FEATURE_REMOVAL) && !UCRR_PLAY_ONLY
extern void uc_particle_rotation_observe(uint32_t);
#endif
extern void sub_0008E920(void),sub_00099980(void),sub_000999C0(void),sub_00099C40(void),sub_00095070(void),sub_00086FD0(void),sub_00087030(void);
extern void sub_0009C7A0(void),sub_0009C7E0(void),sub_003E2FD0(void),sub_003E35D0(void),sub_003E35F0(void);
extern void sub_003E3840(void),sub_003E55B0(void),sub_003E69B0(void),sub_003E6A00(void),sub_003E7080(void),sub_003E7400(void);
extern void sub_003E9350(void),sub_003E9370(void),sub_003F1900(void);
static int rotating_draw_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_ROTATING_DRAW");flag=v&&!strcmp(v,"1");}return flag;}
static int rotating_draw_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int rotating_draw_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define ROT_CALL(fn,ret) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
#define ROT_VCALL(target,ret,saved) do{uint32_t t_=(target);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(t_,saved);}while(0)
#define ROT_PUSH(v) do{double v_=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=v_;}while(0)
#define ROT_TOP g_fp_stack[g_fp_top]
#define ROT_POP() (g_fp_top=(g_fp_top+1u)&7u)
int uc_hybrid_particle_rotating_draw(unsigned legacy){
 if(!rotating_draw_enabled()||legacy>1)return 0;
#if !defined(UCRR_HYBRID_NATIVE_PARTICLES)
 if(legacy)return 0;
#endif
 uint32_t start=g_esp,owner=g_ecx,frame=g_ebp;
 if((start&3)||start<0x11000u||!rotating_draw_span(start-4096,4104)||!rotating_draw_span(owner,8)||
    !rotating_draw_apart(owner,8,start-4096,4104)||!rotating_draw_span(0x51c178,1)||MEM8(0x51c178)||
    !rotating_draw_apart(start-4096,4104,0x51c178,1)||!rotating_draw_apart(owner,8,0x51c178,1)||(unsigned)g_fp_top>7)return 0;
 /* Completing callbacks own the original live object/queue contract. No
  * rejection or replay occurs after the prologue. */
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-0x724;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edi);MEM32(g_esp+0x24)=g_esi;
 ROT_VCALL(MEM32(g_eax+0x10),0x99ded,start);
 g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);g_ebx=g_eax;--g_ebx;
 {uint32_t carry=g_ebx!=0;g_ebx=0u-g_ebx;SET_LO8(g_ebx,carry?0xff:0);SET_LO8(g_ebx,(uint8_t)(LO8(g_ebx)+1));}
 MEM8(g_esp+0x17)=LO8(g_ebx);{uint32_t saved=g_esp;ROT_VCALL(MEM32(g_edx+0xa4),0x99e08,saved);}
 g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;ROT_VCALL(MEM32(g_edx+0x1c),0x99e0f,saved);}
 g_ecx=MEM32(0x519f60);g_eax=MEM32(g_eax+0x10);g_edx=MEM32(g_ecx);MEM32(g_esp+0x3c)=g_eax;
 {uint32_t saved=g_esp;ROT_VCALL(MEM32(g_edx+0x50),0x99e21,saved);}
 g_ecx=MEM32(g_esi+4);g_edx=g_esp+0x78;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=g_esp+0x68;PUSH32(g_esp,g_edx);g_edi=g_eax;g_eax=MEM32(g_ecx);g_edx=g_esp+0x24;PUSH32(g_esp,g_edx);MEM32(g_esp+0x74)=g_edi;ROT_VCALL(MEM32(g_eax+8),0x99e3e,saved);}
 g_eax=MEM32(g_esp+0x1c);if(!g_eax)goto finish;
 g_eax=MEM32(g_edi);g_ecx=g_edi;{uint32_t saved=g_esp;ROT_VCALL(MEM32(g_eax+12),0x99e51,saved);}
 g_xmm0=XMM_MEM(g_eax);g_edx=MEM32(g_edi);XMM_STORE(g_esp+0xf0,g_xmm0);g_xmm0=XMM_MEM(g_eax+16);XMM_STORE(g_esp+0x100,g_xmm0);g_xmm0=XMM_MEM(g_eax+32);XMM_STORE(g_esp+0x110,g_xmm0);g_xmm0=XMM_MEM(g_eax+48);g_ecx=g_edi;XMM_STORE(g_esp+0x120,g_xmm0);
 {uint32_t saved=g_esp;ROT_VCALL(MEM32(g_edx+4),0x99e87,saved);}
 g_xmm0=XMM_MEM(g_eax);XMM_STORE(g_esp+0x130,g_xmm0);g_xmm0=XMM_MEM(g_eax+16);XMM_STORE(g_esp+0x140,g_xmm0);g_xmm0=XMM_MEM(g_eax+32);XMM_STORE(g_esp+0x150,g_xmm0);g_xmm0=XMM_MEM(g_eax+48);g_eax=MEM32(g_edi);g_ecx=g_edi;XMM_STORE(g_esp+0x160,g_xmm0);
 {uint32_t saved=g_esp;ROT_VCALL(MEM32(g_eax+0x18),0x99ebd,saved);}
 g_xmm0=XMM_MEM(g_eax);g_xmm1=XMM_SCALAR(MEMF(g_esp+0xf4));XMM_STORE(g_esp+0xa0,g_xmm0);g_xmm0=XMM_MEM(g_eax+16);XMM_STORE(g_esp+0xb0,g_xmm0);g_xmm0=XMM_MEM(g_eax+32);XMM_STORE(g_esp+0xc0,g_xmm0);g_xmm0=XMM_MEM(g_eax+48);XMM_STORE(g_esp+0xd0,g_xmm0);
 g_xmm0=XMM_SCALAR(MEMF(g_esp+0xf0));MEMF(g_esp+0xe0)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x100));MEMF(g_esp+0xe4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x110));MEMF(g_esp+0x90)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esp+0x104));g_ecx=g_esp+0x170;
 MEMF(g_esp+0xe8)=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEMF(g_esp+0x94)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esp+0x114));PUSH32(g_esp,g_ecx);g_ecx=g_esp+0xa4;MEMF(g_esp+0xf0)=g_xmm0.f[0];MEMF(g_esp+0x9c)=g_xmm1.f[0];MEMF(g_esp+0xa0)=g_xmm0.f[0];ROT_CALL(sub_0008E920,0x99f81);
 PUSH32(g_esp,16);g_edx=g_eax;g_ecx=0x68;ROT_CALL(sub_003E6A00,0x99f8f);PUSH32(g_esp,16);g_edx=0x4f86e0;g_ecx=0x74;ROT_CALL(sub_003E6A00,0x99fa0);PUSH32(g_esp,16);g_edx=0x4f8720;g_ecx=0x70;ROT_CALL(sub_003E6A00,0x99fb1);
 g_edx=g_esp+0x90;g_ecx=0x78;ROT_CALL(sub_003E69B0,0x99fc2);g_edx=g_esp+0xe0;g_ecx=0x79;ROT_CALL(sub_003E69B0,0x99fd3);
 g_eax=MEM32(g_esp+0x64);g_ecx=MEM32(g_esp+0x3c);g_edx=MEM32(g_esp+0x78);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_esi;MEM32(g_esp+0x20)=g_edx;ROT_CALL(sub_000999C0,0x99fec);
 g_esi=1;do{PUSH32(g_esp,0);PUSH32(g_esp,g_esi);ROT_CALL(sub_003E3840,0x99ff9);++g_esi;}while((int32_t)g_esi<4);
 g_ecx=MEM32(g_esp+0x18);if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);ROT_VCALL(MEM32(g_edx+0x1c),0x9a00e,saved);}
 g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,0);ROT_VCALL(MEM32(g_eax+0xdc),0x9a01e,saved);}
 g_ecx=g_eax;ROT_CALL(sub_00095070,0x9a025);g_edx=MEM32(g_eax+8);g_esi=g_eax+8;g_eax=g_esp+0x30;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esp+0x60;PUSH32(g_esp,g_ecx);g_ecx=g_esi;ROT_VCALL(MEM32(g_edx+0x2c),0x9a03a,saved);}
 g_xmm0=XMM_SCALAR(MEMF(g_esp+0x30));g_xmm0.f[0]*=MEMF(0x4569f8);g_ecx=MEM32(0x519f60);MEMF(g_esp+0x30)=g_xmm0.f[0];g_edx=MEM32(g_ecx);
 {uint32_t saved=g_esp;ROT_VCALL(MEM32(g_edx+0x40),0x9a059,saved);}
 g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;ROT_VCALL(MEM32(g_edx+0x10),0x9a060,saved);}
 MEMF(g_esp+0x18)=(float)ROT_TOP;ROT_POP();g_edx=MEM32(g_esi);g_eax=g_esp+0x40;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=g_esp+0x5c;PUSH32(g_esp,g_eax);g_ecx=g_esi;ROT_VCALL(MEM32(g_edx+0x1c),0x9a075,saved);}
 g_ecx=g_eax;ROT_CALL(sub_00086FD0,0x9a07c);g_ecx=MEM32(g_esp+0x18);g_xmm1=XMM_MEM(g_eax);MEM32(g_esp+0x18)=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_esp+0x18));g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm1=XMM_MUL(g_xmm1,g_xmm0);XMM_STORE(g_esp+0x40,g_xmm1);
 g_edx=MEM32(g_esp+0x4c);g_eax=MEM32(g_esp+0x48);g_ecx=MEM32(g_esp+0x44);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+0x44);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_esp+0x9c;ROT_CALL(sub_00087030,0x9a0b9);
 g_eax=MEM32(g_esp+0x8c);PUSH32(g_esp,g_eax);ROT_CALL(sub_003E7400,0x9a0c6);
 g_esi=MEM32(0x3f3d98);g_eax=MEM32(0x51a204);g_ecx=MEM32(g_esp+0x5c);g_edx=MEM32(g_esp+0x30);g_esi|=0x2000;PUSH32(g_esp,g_eax);MEM32(0x3f4110)=1;MEM32(0x3f4118)=g_ecx;MEM32(0x3f3d98)=g_esi;MEM32(0x3f411c)=g_edx;ROT_CALL(sub_0009C7A0,0x9a101);g_esp+=4;
 if(LO8(g_ebx)){
  g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;ROT_VCALL(MEM32(g_edx+0x9c),0x9a11a,saved);}
  g_edx=MEM32(g_esp+0x1c);g_esi=MEM32(g_eax+0x28);g_ecx=MEM32(g_eax+0x34);PUSH32(g_esp,0);g_esi+=g_edx;PUSH32(g_esp,g_ecx);MEM32(g_eax+0x28)=g_esi;ROT_CALL(sub_003F1900,0x9a131);
  MEM32(g_esp+0x18)=g_eax;ROT_CALL(sub_003E35D0,0x9a13a);MEM32(g_esp+0x58)=g_eax;ROT_CALL(sub_003E35F0,0x9a143);g_esi=g_eax;g_eax=MEM32(g_esp+0x18);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);MEM32(g_esp+0x5c)=g_esi;ROT_CALL(sub_003E2FD0,0x9a154);
  g_ecx=MEM32(0x51a274);PUSH32(g_esp,g_ecx);ROT_CALL(sub_0009C7E0,0x9a160);g_esp+=4;g_edx=0x1000100;g_ecx=0x40358;ROT_CALL(sub_003E7080,0x9a172);g_edx=0;g_ecx=0x40300;MEM32(0x3f40ac)=0x1000100;ROT_CALL(sub_003E7080,0x9a188);MEM32(0x3f4090)=0;
 }else{g_edx=MEM32(0x51a1f4);PUSH32(g_esp,g_edx);ROT_CALL(sub_0009C7E0,0x9a1a0);g_esi=MEM32(g_esp+0x58);g_esp+=4;}
 g_eax=MEM32(g_esp+0x1c);MEM32(g_esp+0x28)=0;
 if((int32_t)g_eax>0){
  do{
   g_eax-=MEM32(g_esp+0x28);MEM32(g_esp+0x38)=39;if((int32_t)g_eax<=39)MEM32(g_esp+0x38)=g_eax;
   g_eax=MEM32(g_esp+0x24);g_esi=MEM32(g_eax+4);g_eax=MEM32(g_esi);g_ecx=g_esp+0x250;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=g_esp+0x1b4;PUSH32(g_esp,g_edx);g_edx=MEM32(g_edi);g_ecx=g_esp+0x4c8;PUSH32(g_esp,g_ecx);g_ecx=g_edi;MEM32(g_esp+0x2c)=g_eax;ROT_VCALL(MEM32(g_edx+8),0x9a201,saved);}
   g_ebx=MEM32(g_esp+0x44);{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x38);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x38);g_ecx=g_esi;ROT_VCALL(MEM32(g_eax+12),0x9a215,saved);}
   g_esi=g_ebx;g_esi*=52;g_ecx=g_esi+5;PUSH32(g_esp,g_ecx);MEM32(g_esp+0x8c)=g_esi;ROT_CALL(sub_003E9350,0x9a22a);
#if defined(UCRR_HYBRID_NATIVE_PARTICLES)
   /* Preserve the existing optional hook's truth-test, including negative
    * completion, and its exact continuation. This does not requalify it. */
   if(legacy&&uc_hybrid_particles_try_draw(g_esp,g_ebx)){SET_LO8(g_ebx,MEM8(g_esp+0x17));goto next_batch;}
#endif
   g_edx=g_esi;g_edx=(g_edx<<18)+0x40001818;MEM32(g_eax)=0x417fc;MEM32(g_eax+4)=8;MEM32(g_eax+8)=g_edx;g_eax+=12;MEM32(g_esp+0x60)=g_eax;MEM32(g_esp+0x34)=0;
   if((int32_t)g_ebx>0){
    g_xmm0=XMM_ZERO();g_esi=0;MEMF(g_esp+0x84)=g_xmm0.f[0];MEMF(g_esp+0x48)=g_xmm0.f[0];g_ebx=g_eax+52;MEM32(g_esp+0x2c)=g_esi;
    do{
     SET_LO8(g_eax,MEM8(g_esp+0x17));if(LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(g_esp+g_esi+0x25c));g_xmm0.f[0]*=MEMF(g_esp+g_esi+0x258);MEMF(g_esp+g_esi+0x258)=g_xmm0.f[0];}
     g_eax=g_esp+g_esi+0x250;PUSH32(g_esp,g_eax);ROT_CALL(sub_00099980,0x9a2b0);g_ecx=MEM32(g_esp+0x38);
#if defined(UCRR_FEATURE_REMOVAL) && !UCRR_PLAY_ONLY
     uc_particle_rotation_observe(MEM32(g_esp+g_ecx*4+0x1b4));
#endif
     ROT_PUSH(MEMF(g_esp+g_ecx*4+0x1b4));g_xmm0=XMM_SCALAR(MEMF(g_esp+g_esi+0x4c4));ROT_PUSH(ROT_TOP);g_ecx=g_esp+g_esi+0x4cc;ROT_TOP=sin(ROT_TOP);MEMF(g_esp+0x70)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ecx-4));g_edx=MEM32(g_esp+0x70);MEM32(g_esp+0xa4)=g_edx;MEMF(g_esp+0x74)=g_xmm0.f[0];
     g_xmm0=XMM_SCALAR(MEMF(g_ecx));g_edx=MEM32(g_esp+0x74);MEM32(g_esp+0xa8)=g_edx;MEM32(g_esp+0xb0)=g_eax;MEMF(g_esp+0x78)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_ecx+4));g_edx=MEM32(g_esp+0x78);MEM32(g_esp+0xac)=g_edx;MEMF(g_esp+0xb4)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_edi=g_ebx-52;g_esi=g_esp+0xa4;g_esp+=4;
     MEMF(g_esp+0x20)=(float)ROT_TOP;ROT_POP();g_eax=MEM32(g_esp+0x20);ROT_TOP=cos(ROT_TOP);g_edx=g_eax;MEM32(g_esp+0x80)=g_eax;g_eax=MEM32(g_esp+0x84);MEM32(g_esp+0xc0)=g_edx;MEM32(g_esp+0xc4)=g_eax;g_eax=MEM32(g_esp+0x48);MEM32(g_esp+0xd0)=g_eax;g_eax=MEM32(g_esp+0x34);++g_eax;MEM32(g_esp+0x34)=g_eax;
     MEMF(g_esp+0x7c)=(float)ROT_TOP;ROT_PUSH(MEMF(g_esp+0x20));g_ecx=MEM32(g_esp+0x7c);MEM32(g_esp+0xbc)=g_ecx;ROT_TOP=-ROT_TOP;MEMF(g_esp+0x40)=(float)ROT_TOP;ROT_POP();g_ecx=MEM32(g_esp+0x40);MEM32(g_esp+0xc8)=g_ecx;MEMF(g_esp+0x44)=(float)ROT_TOP;ROT_POP();g_edx=MEM32(g_esp+0x44);MEM32(g_esp+0xcc)=g_edx;g_edx=MEM32(g_esp+0x2c);g_ecx=13;
     memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;g_edi=g_ebx;MEMF(g_ebx-32)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_ecx=13;g_esi=g_esp+0xa0;
     memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;g_edi=g_ebx+52;MEMF(g_ebx+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44c530));g_ecx=13;g_esi=g_esp+0xa0;
     memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;g_edi=g_ebx+104;MEMF(g_ebx+0x48)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x46c31c));g_ecx=13;g_esi=g_esp+0xa0;
     memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;g_ecx=MEM32(g_esp+0x38);MEMF(g_ebx+0x7c)=g_xmm0.f[0];g_ebx+=208;g_edx+=16;MEM32(g_esp+0x2c)=g_edx;
     if((int32_t)g_eax<(int32_t)g_ecx)g_esi=MEM32(g_esp+0x2c);
    }while((int32_t)g_eax<(int32_t)g_ecx);
    g_edi=MEM32(g_esp+0x68);g_esi=MEM32(g_esp+0x88);g_eax=MEM32(g_esp+0x60);
   }
   SET_LO8(g_ebx,MEM8(g_esp+0x17));g_ecx=g_eax+g_esi*4+8;PUSH32(g_esp,g_ecx);MEM32(g_eax+g_esi*4)=0x417fc;MEM32(g_eax+g_esi*4+4)=0;ROT_CALL(sub_003E9370,0x9a44d);
#if defined(UCRR_HYBRID_NATIVE_PARTICLES)
next_batch:
#endif
   g_ecx=MEM32(g_esp+0x28);g_eax=MEM32(g_esp+0x1c);g_ecx+=39;MEM32(g_esp+0x28)=g_ecx;
  }while((int32_t)g_ecx<(int32_t)g_eax);
  g_esi=MEM32(g_esp+0x54);
 }
 g_edx=MEM32(g_edi);g_eax=g_esp+0x130;{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;ROT_VCALL(MEM32(g_edx+0x1c),0x9a477,saved);}
 g_edx=MEM32(g_edi);g_eax=g_esp+0xf0;{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;ROT_VCALL(MEM32(g_edx+0x24),0x9a486,saved);}
 g_edx=MEM32(g_edi);g_ecx=g_edi;{uint32_t saved=g_esp;ROT_VCALL(MEM32(g_edx+0x30),0x9a48d,saved);}
 g_eax=MEM32(g_esp+0x3c);g_ecx=MEM32(g_esp+0x24);PUSH32(g_esp,g_eax);ROT_CALL(sub_00099C40,0x9a49b);
 if(LO8(g_ebx)){
  g_edi=MEM32(g_esp+0x58);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);ROT_CALL(sub_003E2FD0,0x9a4aa);g_edx=0x10101;g_ecx=0x40358;ROT_CALL(sub_003E7080,0x9a4b9);g_ecx=MEM32(g_esp+0x18);PUSH32(g_esp,g_ecx);MEM32(0x3f40ac)=0x10101;ROT_CALL(sub_003E55B0,0x9a4cd);PUSH32(g_esp,g_edi);ROT_CALL(sub_003E55B0,0x9a4d3);PUSH32(g_esp,g_esi);ROT_CALL(sub_003E55B0,0x9a4d9);
 }
 g_esi=MEM32(g_esp+0x24);
finish:
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;ROT_VCALL(MEM32(g_edx+0x18),0x9a4e5,saved);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=8;return 1;
}
#undef ROT_CALL
#undef ROT_VCALL
#undef ROT_PUSH
#undef ROT_TOP
#undef ROT_POP
