#include "hybrid_particle_triangle_draw_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D7290(void),sub_00099980(void),sub_000999C0(void),sub_00099C40(void),sub_0009C7A0(void),sub_0009C7E0(void);
extern void sub_003E2FD0(void),sub_003E35D0(void),sub_003E35F0(void),sub_003E3840(void),sub_003E55B0(void),sub_003E7080(void);
extern void sub_003EF600(void),sub_003EF6B0(void),sub_003EF700(void),sub_003EF740(void),sub_003F1900(void);
static int triangle_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_TRIANGLE_DRAW");flag=v&&!strcmp(v,"1");}return flag;}
static int triangle_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int triangle_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define TRI_CALL(fn,ret) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
#define TRI_VCALL(target,ret,saved) do{uint32_t t_=(target);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(t_,saved);}while(0)
int uc_hybrid_particle_triangle_draw(void){
 if(!triangle_enabled())return 0;
 uint32_t start=g_esp,owner=g_ecx,frame=g_ebp;
 if((start&3)||start<0x18000u||!triangle_span(start-32768,32776)||!triangle_span(owner,16)||
    !triangle_apart(owner,16,start-32768,32776)||!triangle_span(0x51c178,1)||MEM8(0x51c178)||
    !triangle_apart(start-32768,32776,0x51c178,1)||!triangle_apart(owner,16,0x51c178,1))return 0;
 /* Callbacks keep the source object/buffer lifetime contract. There is no
  * rejection after publication and no cached source, vertex or UV snapshot. */
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_eax=0x36b4;TRI_CALL(sub_002D7290,0x9a8b0);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);TRI_VCALL(MEM32(g_eax+0xa4),0x9a8c3,saved);}
 g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;TRI_VCALL(MEM32(g_edx+0x1c),0x9a8ca,saved);}
 g_ecx=MEM32(0x519f60);g_eax=MEM32(g_eax+0x10);g_edx=MEM32(g_ecx);MEM32(g_esp+0x28)=g_eax;
 {uint32_t saved=g_esp;TRI_VCALL(MEM32(g_edx+0x50),0x9a8dc,saved);}
 g_xmm0=XMM_SCALAR(MEMF(g_edi+8));unsigned flags;
 {double a=g_xmm0.f[0],b=MEMF(0x44c174);flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
 g_ebx=g_eax;SET_HI8(g_eax,flags);MEM32(g_esp+0x24)=g_ebx;
 if(!RECOMP_PARITY8(flags&0x44u)){
  g_xmm0=XMM_SCALAR(MEMF(g_edi+12));double a=g_xmm0.f[0],b=MEMF(0x44bf94);flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;SET_HI8(g_eax,flags);
  if(!RECOMP_PARITY8(flags&0x44u)){MEM8(g_esp+0x17)=0;goto source;}
 }
 g_eax=MEM32(g_ebx);g_ecx=g_ebx;MEM8(g_esp+0x17)=1;{uint32_t saved=g_esp;TRI_VCALL(MEM32(g_eax+0x10),0x9a91c,saved);}
 /* Preserve the exact source SSE and scratch-write order, including aliases
  * with the projection getter's returned matrix and later camera callback. */
 g_xmm2=XMM_MEM(g_eax);g_xmm1=XMM_SCALAR(MEMF(g_edi+8));g_xmm0=XMM_SCALAR(MEMF(g_edi+12));g_xmm0.f[0]-=MEMF(g_edi+8);
 XMM_STORE(g_esp+0x80,g_xmm2);g_xmm3=XMM_MEM(g_eax+16);XMM_STORE(g_esp+0x90,g_xmm3);g_xmm4=XMM_MEM(g_eax+32);XMM_STORE(g_esp+0xa0,g_xmm4);g_xmm5=XMM_MEM(g_eax+48);
 XMM_STORE(g_esp+0x40,g_xmm2);g_xmm2=XMM_SCALAR(MEMF(g_esp+0x4c));g_xmm2.f[0]*=g_xmm1.f[0];XMM_STORE(g_esp+0x50,g_xmm3);g_xmm3=XMM_SCALAR(MEMF(g_esp+0x48));g_xmm3.f[0]*=g_xmm0.f[0];g_xmm2.f[0]+=g_xmm3.f[0];
 g_xmm3=XMM_SCALAR(MEMF(g_esp+0x58));MEMF(g_esp+0x48)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_esp+0x5c));g_xmm2.f[0]*=g_xmm1.f[0];g_xmm3.f[0]*=g_xmm0.f[0];g_xmm2.f[0]+=g_xmm3.f[0];MEMF(g_esp+0x58)=g_xmm2.f[0];
 XMM_STORE(g_esp+0x60,g_xmm4);g_xmm3=XMM_SCALAR(MEMF(g_esp+0x68));g_xmm2=XMM_SCALAR(MEMF(g_esp+0x6c));g_xmm2.f[0]*=g_xmm1.f[0];XMM_STORE(g_esp+0x70,g_xmm5);g_xmm3.f[0]*=g_xmm0.f[0];g_xmm2.f[0]+=g_xmm3.f[0];MEMF(g_esp+0x68)=g_xmm2.f[0];
 g_xmm2=XMM_SCALAR(MEMF(g_esp+0x7c));g_xmm2.f[0]*=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esp+0x78));g_eax=g_esp+0x40;XMM_STORE(g_esp+0xb0,g_xmm5);g_edx=MEM32(g_ebx);g_xmm1.f[0]*=g_xmm0.f[0];g_xmm2.f[0]+=g_xmm1.f[0];
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_ebx;MEMF(g_esp+0x7c)=g_xmm2.f[0];TRI_VCALL(MEM32(g_edx+0x28),0x9a9f2,saved);}
source:
 g_ecx=MEM32(g_edi+4);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;TRI_VCALL(MEM32(g_edx+0x10),0x9a9fa,saved);}
 g_ecx=MEM32(g_edi+4);--g_eax;unsigned carry=g_eax!=0;g_eax=0u-g_eax;g_edx=g_esp+0x2c;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);SET_LO8(g_eax,carry?0xff:0);g_edx=g_esp+0x3c;SET_LO8(g_eax,(uint8_t)(LO8(g_eax)+1));PUSH32(g_esp,g_edx);g_edx=g_esp+0x20;MEM8(g_esp+0x1d)=LO8(g_eax);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edx);TRI_VCALL(MEM32(g_eax+8),0x9aa1c,saved);}
 g_eax=MEM32(g_esp+0x18);if(!g_eax)goto finish;
 g_eax=MEM32(g_esp+0x38);g_ecx=MEM32(g_esp+0x28);g_esi=MEM32(g_esp+0x2c);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_edi;MEM32(g_esp+0x24)=g_esi;MEM8(g_esp+0x1e)=1;TRI_CALL(sub_000999C0,0x9aa46);
 if(g_esi){
  g_esi=1;do{PUSH32(g_esp,0);PUSH32(g_esp,g_esi);TRI_CALL(sub_003E3840,0x9aa5b);++g_esi;}while((int32_t)g_esi<4);
  g_ecx=MEM32(g_esp+0x1c);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,0);TRI_VCALL(MEM32(g_edx+0x1c),0x9aa6c,saved);}
  g_eax=MEM32(0x51a150);PUSH32(g_esp,g_eax);TRI_CALL(sub_0009C7A0,0x9aa77);SET_LO8(g_eax,MEM8(g_esp+0x19));g_esp+=4;
  if(LO8(g_eax)){
   g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;TRI_VCALL(MEM32(g_edx+0x9c),0x9aa94,saved);}
   g_edx=MEM32(g_esp+0x18);g_esi=MEM32(g_eax+0x28);g_ecx=MEM32(g_eax+0x34);PUSH32(g_esp,0);g_esi+=g_edx;PUSH32(g_esp,g_ecx);MEM32(g_eax+0x28)=g_esi;TRI_CALL(sub_003F1900,0x9aaab);
   g_esi=g_eax;MEM32(g_esp+0x3c)=g_esi;TRI_CALL(sub_003E35D0,0x9aab6);MEM32(g_esp+0x34)=g_eax;TRI_CALL(sub_003E35F0,0x9aabf);
   PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);MEM32(g_esp+0x24)=g_eax;TRI_CALL(sub_003E2FD0,0x9aaca);
   g_eax=MEM32(0x51a274);PUSH32(g_esp,g_eax);TRI_CALL(sub_0009C7E0,0x9aad5);g_esp+=4;g_edx=0x1000100;g_ecx=0x40358;TRI_CALL(sub_003E7080,0x9aae7);
   g_edx=0;g_ecx=0x40300;MEM32(0x3f40ac)=0x1000100;TRI_CALL(sub_003E7080,0x9aafd);MEM32(0x3f4090)=0;
  }else{g_ecx=MEM32(0x51a1f4);PUSH32(g_esp,g_ecx);TRI_CALL(sub_0009C7E0,0x9ab15);g_esp+=4;}
 }else{
  MEM8(g_esp+0x16)=0;g_esi=0;do{PUSH32(g_esp,0);PUSH32(g_esp,g_esi);TRI_CALL(sub_003E3840,0x9ab29);++g_esi;}while((int32_t)g_esi<4);
  g_edx=MEM32(0x51a18c);PUSH32(g_esp,g_edx);TRI_CALL(sub_0009C7A0,0x9ab3b);g_eax=MEM32(0x51a1c0);PUSH32(g_esp,g_eax);TRI_CALL(sub_0009C7E0,0x9ab46);g_esp+=8;
 }
 g_edx=MEM32(g_ebx);g_ecx=g_ebx;{uint32_t saved=g_esp;TRI_VCALL(MEM32(g_edx+0x30),0x9ab50,saved);}
 g_eax=MEM32(g_esp+0x18);MEM32(g_esp+0x20)=0;
 if((int32_t)g_eax>0){
  do{
   g_esi=MEM32(g_edi+4);g_eax=MEM32(g_esi);g_ecx=g_esp+0x1ec0;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=g_esp+0xc4;PUSH32(g_esp,g_edx);g_edx=MEM32(g_ebx);g_ecx=g_esp+0xcc8;PUSH32(g_esp,g_ecx);g_ecx=g_ebx;MEM32(g_esp+0x3c)=g_eax;TRI_VCALL(MEM32(g_edx+8),0x9ab8c,saved);}
   {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x30);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x48);g_ecx=g_esi;TRI_VCALL(MEM32(g_eax+12),0x9ab9d,saved);}
   PUSH32(g_esp,5);TRI_CALL(sub_003EF700,0x9aba4);g_ecx=g_esp+0x1ec0;PUSH32(g_esp,g_ecx);TRI_CALL(sub_00099980,0x9abb1);g_esp+=4;PUSH32(g_esp,g_eax);PUSH32(g_esp,2);TRI_CALL(sub_003EF6B0,0x9abbc);
   SET_LO8(g_ebx,MEM8(g_esp+0x16));
   if(LO8(g_ebx)){g_edx=MEM32(g_esp+0xc4);g_eax=MEM32(g_esp+0xc0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,3);TRI_CALL(sub_003EF600,0x9abe2);}
   g_ecx=MEM32(g_esp+0xcc8);g_edx=MEM32(g_esp+0xcc4);g_eax=MEM32(g_esp+0xcc0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0xffffffffu);TRI_CALL(sub_003EF600,0x9ac06);
   if(LO8(g_ebx)){g_ecx=MEM32(g_esp+0xd4);g_edx=MEM32(g_esp+0xd0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,3);TRI_CALL(sub_003EF600,0x9ac28);}
   g_eax=MEM32(g_esp+0xce0);g_ecx=MEM32(g_esp+0xcdc);g_edx=MEM32(g_esp+0xcd8);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0xffffffffu);TRI_CALL(sub_003EF600,0x9ac4c);
   if(LO8(g_ebx)){g_eax=MEM32(g_esp+0xcc);g_ecx=MEM32(g_esp+0xc8);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,3);TRI_CALL(sub_003EF600,0x9ac6e);}
   g_edx=MEM32(g_esp+0xcd4);g_eax=MEM32(g_esp+0xcd0);g_ecx=MEM32(g_esp+0xccc);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xffffffffu);TRI_CALL(sub_003EF600,0x9ac92);
   TRI_CALL(sub_003EF740,0x9ac97);g_eax=MEM32(g_esp+0x20);g_ecx=MEM32(g_esp+0x18);g_ebx=MEM32(g_esp+0x24);++g_eax;MEM32(g_esp+0x20)=g_eax;
  }while((int32_t)g_eax<(int32_t)g_ecx);
 }
 g_edx=MEM32(g_esp+0x28);PUSH32(g_esp,g_edx);g_ecx=g_edi;TRI_CALL(sub_00099C40,0x9acbc);SET_LO8(g_eax,MEM8(g_esp+0x15));
 if(LO8(g_eax)){
  g_eax=MEM32(g_esp+0x1c);g_esi=MEM32(g_esp+0x34);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);TRI_CALL(sub_003E2FD0,0x9acd3);
  g_edx=0x10101;g_ecx=0x40358;TRI_CALL(sub_003E7080,0x9ace2);g_ecx=MEM32(g_esp+0x3c);PUSH32(g_esp,g_ecx);MEM32(0x3f40ac)=0x10101;TRI_CALL(sub_003E55B0,0x9acf6);
  PUSH32(g_esp,g_esi);TRI_CALL(sub_003E55B0,0x9acfc);g_edx=MEM32(g_esp+0x1c);PUSH32(g_esp,g_edx);TRI_CALL(sub_003E55B0,0x9ad06);
 }
finish:
 g_edi=MEM32(g_edi+4);g_eax=MEM32(g_edi);g_ecx=g_edi;{uint32_t saved=g_esp;TRI_VCALL(MEM32(g_eax+0x18),0x9ad10,saved);}
 SET_LO8(g_eax,MEM8(g_esp+0x17));int final_zf=LO8(g_eax)==0;
 if(!final_zf){g_edx=MEM32(g_ebx);g_eax=g_esp+0x80;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_ebx;TRI_VCALL(MEM32(g_edx+0x28),0x9ad27,saved);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=final_zf;g_esp+=8;return 1;
}
#undef TRI_CALL
#undef TRI_VCALL
