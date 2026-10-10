#include "hybrid_particle_prepare_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_000643E0(void),sub_00239BA0(void),sub_00239B60(void),sub_0023AE00(void),sub_0023BEC0(void);
static int particle_prepare_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_PREPARE");enabled=v&&!strcmp(v,"1");}return enabled;}
static int particle_prepare_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_particle_prepare(unsigned kind){
 if(!particle_prepare_enabled())return 0;
 static const unsigned sizes[]={0x114,0x114,0x124,0x124,0x148};uint32_t start=g_esp;
 if(kind>4||(start&3)||start<0x10200u||!particle_prepare_span(start-512,528)||!particle_prepare_span(g_ecx,sizes[kind])||!particle_prepare_span(0x51c178,1)||MEM8(0x51c178))return 0;
 static const uint32_t profile_returns[]={0x239ad0,0x23a350,0x23ad1f,0x23b590,0x23bd72};
 static const uint32_t color_returns[]={0x239b44,0x23a3c4,0x23ada1,0x23b604,0x23be29};
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;
 unsigned saved_ebx=kind==0||kind==1||kind==3;g_esp-=kind==4?0x88:kind==2?0x68:0x64;
 if(saved_ebx)PUSH32(g_esp,g_ebx);
 PUSH32(g_esp,g_esi);
 if(kind==4){PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+0x20);SET_LO8(g_ecx,MEM8(g_eax+0xc8));}
 else{g_esi=g_ecx;g_eax=MEM32(g_esi+0x20);SET_LO8(g_ecx,MEM8(g_eax+0xc8));PUSH32(g_esp,g_edi);}
 int zero=LO8(g_ecx)==0;
 if(!zero){
  g_ebp=frame;PUSH32(g_esp,profile_returns[kind]);sub_000643E0();SET_LO8(g_ecx,MEM8(g_eax+0x32));zero=LO8(g_ecx)==0;
  if(zero){g_ecx=MEM32(frame+8);MEM32(g_ecx)=0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);if(saved_ebx)POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=zero;g_esp+=16;return 1;}
 }
 if(kind==4){
  g_edx=MEM32(g_edi+0x9c);g_eax=MEM32(frame+8);MEM32(g_eax)=g_edx;g_esi=MEM32(g_edi+0x20);g_ecx=MEM32(g_esi+0x98);g_edx=MEM32(frame+12);MEM32(g_edx)=g_ecx;
  g_eax=MEM32(g_esi+0x94);g_ecx=MEM32(frame+16);MEM32(g_ecx)=g_eax;g_eax=MEM32(g_edi+0x20);g_edx=MEM32(g_eax+0xd0);g_esi+=0x30;g_eax+=0xd0;MEM32(g_edi+0xb4)=g_edx;
  g_xmm0=XMM_SCALAR(MEMF(g_eax+4));g_xmm0.f[0]-=MEMF(g_eax);MEMF(g_edi+0xb8)=g_xmm0.f[0];g_ecx=MEM32(g_eax+8);MEM32(g_edi+0xbc)=g_ecx;
  g_xmm0=XMM_SCALAR(MEMF(g_eax+12));g_xmm0.f[0]-=MEMF(g_eax+8);MEMF(g_edi+0xc0)=g_xmm0.f[0];g_edx=MEM32(g_edi+0x20);g_xmm0=XMM_SCALAR(MEMF(g_edx+0x90));g_xmm0.f[0]*=MEMF(0x44d95c);MEMF(g_edi+0xc4)=g_xmm0.f[0];
  g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(g_esi+0x50);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_edx=g_esi+0x40;PUSH32(g_esp,g_edx);g_eax=g_esi+0x30;PUSH32(g_esp,g_eax);g_ecx=g_esi+0x20;PUSH32(g_esp,g_ecx);g_ecx=g_esp+0x44;
 }else if(kind==2){
  g_edx=MEM32(g_esi+0x9c);g_eax=MEM32(frame+8);MEM32(g_eax)=g_edx;g_edi=MEM32(g_esi+0x20);g_ecx=MEM32(g_edi+0x98);g_edx=MEM32(frame+12);g_edi+=0x30;MEM32(g_edx)=g_ecx;
  g_ecx=MEM32(g_edi+0x64);g_edx=MEM32(frame+16);MEM32(g_edx)=g_ecx;SET_LO8(g_ecx,MEM8(g_edi+0x75));
  if(!LO8(g_ecx)){g_ecx=MEM32(g_esi+0xac);SET_LO8(g_edx,MEM8(g_ecx+12));if(LO8(g_edx))MEM32(g_eax)=0;}
  g_edx=g_esi+12;PUSH32(g_esp,g_edx);g_ecx=g_esi+0xb4;g_ebp=frame;PUSH32(g_esp,0x23ad84);sub_0023AE00();
  g_eax=MEM32(g_edi+0x54);g_ecx=MEM32(g_edi+0x50);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_edx=g_edi+0x40;PUSH32(g_esp,g_edx);g_eax=g_edi+0x30;PUSH32(g_esp,g_eax);g_edi+=0x20;PUSH32(g_esp,g_edi);g_ecx=g_esp+0x24;
 }else{
  g_edx=MEM32(g_esi+0x9c);g_ecx=MEM32(frame+8);g_edi=MEM32(frame+12);MEM32(g_ecx)=g_edx;g_eax=MEM32(g_esi+0x20);g_edx=MEM32(g_eax+0x98);g_eax+=0x30;MEM32(g_edi)=g_edx;
  g_edx=MEM32(g_eax+0x64);g_edi=MEM32(frame+16);MEM32(g_edi)=g_edx;SET_LO8(g_edx,MEM8(g_eax+0x75));
  if(!LO8(g_edx)){g_edx=MEM32(g_esi+0xac);SET_LO8(g_ebx,MEM8(g_edx+12));if(LO8(g_ebx))MEM32(g_ecx)=0;}
  g_ecx=MEM32(g_eax+0x54);g_edx=MEM32(g_eax+0x50);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax+0x40;PUSH32(g_esp,g_ecx);g_edx=g_eax+0x30;PUSH32(g_esp,g_edx);g_eax+=0x20;PUSH32(g_esp,g_eax);g_ecx=g_esp+0x24;
 }
 g_ebp=frame;PUSH32(g_esp,color_returns[kind]);sub_00239BA0();
 if(kind==0){PUSH32(g_esp,g_eax);g_ecx=g_esi+0xbc;g_ebp=frame;PUSH32(g_esp,0x239b50);sub_00239B60();POP32(g_esp,g_edi);}
 else if(kind==4){
  for(unsigned i=0;i<0x50;i+=16){g_xmm0=XMM_MEM(g_eax+i);XMM_STORE(g_edi+0xcc+i,g_xmm0);}
  g_edx=MEM32(g_eax+0x50);MEM32(g_edi+0x11c)=g_edx;g_eax=MEM32(g_eax+0x54);MEM32(g_edi+0x120)=g_eax;
  g_ecx=MEM32(g_esi+0xc8);g_edx=MEM32(g_esi+0xc4);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_eax=g_esi+0xc0;PUSH32(g_esp,g_eax);g_ecx=g_esi+0xbc;PUSH32(g_esp,g_ecx);g_esi+=0xb8;PUSH32(g_esp,g_esi);g_ecx=g_esp+0x28;
  g_ebp=frame;PUSH32(g_esp,0x23be9d);sub_0023BEC0();g_edi+=0x12c;g_ecx=7;g_esi=g_eax;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;POP32(g_esp,g_edi);
 }else{
  unsigned offset=kind==1?0xbc:0xcc;
  for(unsigned i=0;i<0x50;i+=16){g_xmm0=XMM_MEM(g_eax+i);XMM_STORE(g_esi+offset+i,g_xmm0);}
  g_ecx=MEM32(g_eax+0x50);MEM32(g_esi+offset+0x50)=g_ecx;g_edx=MEM32(g_eax+0x54);
  if(kind!=3)POP32(g_esp,g_edi);
  MEM32(g_esi+offset+0x54)=g_edx;
  if(kind==3){
   g_eax=MEM32(g_esi+0x20);g_ecx=MEM32(g_eax+0xd0);g_eax+=0xd0;MEM32(g_esi+0xb4)=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_eax+4));g_xmm0.f[0]-=MEMF(g_eax);MEMF(g_esi+0xb8)=g_xmm0.f[0];
   g_edx=MEM32(g_eax+8);MEM32(g_esi+0xbc)=g_edx;g_xmm0=XMM_SCALAR(MEMF(g_eax+12));g_xmm0.f[0]-=MEMF(g_eax+8);MEMF(g_esi+0xc0)=g_xmm0.f[0];g_eax=MEM32(g_esi+0x20);g_xmm0=XMM_SCALAR(MEMF(g_eax+0x90));g_xmm0.f[0]*=MEMF(0x44d95c);POP32(g_esp,g_edi);MEMF(g_esi+0xc4)=g_xmm0.f[0];
  }
 }
 POP32(g_esp,g_esi);if(saved_ebx)POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=kind==2||kind==4?g_edi==0:g_eax==0;g_esp+=16;return 1;
}
