#include "hybrid_particle_sprite_fill_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_00239F70(void),sub_0023A010(void);
static int sprite_fill_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_SPRITE_FILL");enabled=v&&!strcmp(v,"1");}return enabled;}
static int sprite_fill_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}

int uc_hybrid_particle_sprite_fill(unsigned operation){
 if(!sprite_fill_enabled())return 0;
 uint32_t start=g_esp;
 if(operation>1||(start&3)||start<0x10100u||!sprite_fill_span(start-256,284)||
    (!operation&&!sprite_fill_span(g_ecx,0x114))||!sprite_fill_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;
 if(operation==1){
  g_esp-=16;g_eax=MEM32(frame+12);g_xmm0=XMM_SCALAR(MEMF(g_eax));MEMF(g_esp)=g_xmm0.f[0];
  g_xmm0=XMM_SCALAR(MEMF(g_eax+4));MEMF(g_esp+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+8));g_eax=MEM32(frame+16);
  g_xmm2=XMM_MEM(g_eax+32);g_xmm3=XMM_MEM(g_eax+16);MEMF(g_esp+8)=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEMF(g_esp+12)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp);
  g_xmm1=g_xmm0;g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm0,0xaa);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);
  g_xmm1=g_xmm0;g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm0,0x55);g_xmm3=XMM_MUL(g_xmm3,g_xmm1);g_xmm1=XMM_MEM(g_eax+48);g_xmm3=XMM_ADD(g_xmm3,g_xmm2);
  g_xmm2=g_xmm0;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0);g_xmm0=XMM_MEM(g_eax);g_eax=MEM32(frame+8);
  g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm0=XMM_ADD(g_xmm0,g_xmm1);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);XMM_STORE(g_esp,g_xmm0);
  g_xmm0=XMM_SCALAR(MEMF(g_esp));MEMF(g_eax)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+4));MEMF(g_eax+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+8));MEMF(g_eax+8)=g_xmm0.f[0];
  g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 g_esp-=0x64;PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_edx=MEM32(g_ebx+0x20);SET_LO8(g_eax,MEM8(g_edx+0x9c));
 int camera=LO8(g_eax)!=0;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(g_esp+0x24)=g_edx;
 if(camera){
  g_ecx=MEM32(frame+12);
  if((int32_t)g_ecx>0){
   g_esi=MEM32(frame+8);g_edi=MEM32(frame+0x18);g_eax=MEM32(frame+0x14);g_esi<<=5;g_eax-=g_edi;MEM32(g_esp+0x20)=g_eax;MEM32(g_esp+0x1c)=g_ecx;
   do{
    g_eax=MEM32(g_ebx+16);g_ecx=MEM32(frame+16);g_xmm0=XMM_SCALAR(MEMF(g_eax+g_esi+12));g_eax+=g_esi;
    PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_edx=g_esp+0x38;PUSH32(g_esp,g_edx);MEMF(g_esp+0x34)=g_xmm0.f[0];g_ebp=frame;PUSH32(g_esp,0x239ce4);sub_00239F70();
    g_xmm0=XMM_SCALAR(MEMF(g_eax));MEMF(g_esp+0x4c)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+4));MEMF(g_esp+0x50)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+8));g_eax=MEM32(g_esp+0x2c);MEMF(g_esp+0x54)=g_xmm0.f[0];
    g_xmm0=XMM_SCALAR(MEMF(g_esp+0x34));MEMF(g_esp+0x58)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+0x4c);XMM_STORE(g_eax+g_edi,g_xmm0);
    g_eax=MEM32(g_esp+0x30);SET_LO8(g_ecx,MEM8(g_eax+0xa6));g_esp+=12;
    int thermal=LO8(g_ecx)!=0;if(thermal){g_ecx=MEM32(g_ebx+0xac);SET_LO8(g_edx,MEM8(g_ecx+12));thermal=LO8(g_edx)!=0;}
    if(thermal)g_xmm0=XMM_MEM(g_eax+0xb0);
    else{g_edx=MEM32(g_ebx+16);g_eax=MEM16(g_edx+g_esi+0x1c);PUSH32(g_esp,g_ecx);g_xmm0.f[0]=(float)(int32_t)g_eax;g_xmm0.f[0]*=MEMF(0x482fbc);g_ecx=g_esp+0x64;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_ecx);g_ecx=g_ebx+0xbc;g_ebp=frame;PUSH32(g_esp,0x239d6e);sub_0023A010();g_xmm0=XMM_MEM(g_eax);}
    g_eax=MEM32(g_esp+0x1c);XMM_STORE(g_edi,g_xmm0);g_esi+=32;g_edi+=16;g_eax--;MEM32(g_esp+0x1c)=g_eax;
   }while(g_eax);
   POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=24;return 1;
  }
 }else{
  g_eax=MEM32(frame+16);g_xmm1=XMM_MEM(g_eax+32);g_xmm0=XMM_MEM(g_eax+48);g_xmm2=XMM_MEM(0x51e4a0);SET_LO8(g_eax,MEM8(0x4fe7c4));
  g_xmm2=XMM_XOR(g_xmm2,g_xmm0);g_xmm0=g_xmm1;g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm2=g_xmm0;g_xmm2=XMM_MOVE_HIGH_TO_LOW(g_xmm2,g_xmm0);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);g_xmm2=g_xmm0;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,1);g_xmm0.f[0]+=g_xmm2.f[0];
  g_xmm2=g_xmm1;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm1,0x27);g_xmm1=g_xmm2;g_xmm1.f[0]=g_xmm0.f[0];g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm1,0x27);XMM_STORE(g_esp+0x60,g_xmm1);
  if(LO8(g_eax)){SET_LO8(g_eax,MEM8(g_edx+0xc8));MEM8(g_esp+0x1b)=1;if(LO8(g_eax))MEM8(g_esp+0x1b)=0;}
  else MEM8(g_esp+0x1b)=0;
  g_eax=MEM32(frame+12);
  if((int32_t)g_eax>0){
   g_esi=MEM32(frame+8);g_edi=MEM32(frame+0x18);g_ecx=MEM32(frame+0x14);g_esi<<=5;g_ecx-=g_edi;MEM32(g_esp+0x20)=g_ecx;MEM32(g_esp+0x28)=g_eax;
   do{
    g_eax=MEM32(g_ebx+16);g_xmm0=XMM_SCALAR(MEMF(g_eax+g_esi));g_xmm3=XMM_SCALAR(MEMF(g_eax+g_esi+12));g_eax+=g_esi;MEMF(g_esp+0x40)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+4));MEMF(g_esp+0x44)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+8));SET_LO8(g_eax,MEM8(g_esp+0x1b));
    MEMF(g_esp+0x48)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf94));MEMF(g_esp+0x4c)=g_xmm0.f[0];g_xmm2=XMM_MEM(g_esp+0x40);XMM_STORE(g_esp+0x50,g_xmm2);MEMF(g_esp+0x1c)=g_xmm0.f[0];
    if(LO8(g_eax)){
     g_xmm0=g_xmm1;g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm4=g_xmm0;g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm0,2);g_xmm2=g_xmm1;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm1,3);g_xmm4.f[0]+=g_xmm2.f[0];g_xmm2=g_xmm0;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,1);g_xmm0.f[0]+=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(0x482f40));g_xmm0.f[0]+=g_xmm4.f[0];
     g_eax=g_esp+0x2c;MEMF(g_eax)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x2c));double a=g_xmm2.f[0];double b=g_xmm0.f[0];unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;
     if(!(flags&0x41u)){g_xmm0.f[0]*=MEMF(0x4fe7d0);g_xmm0.f[0]*=g_xmm0.f[0];MEMF(g_esp+0x1c)=g_xmm0.f[0];}
     g_xmm0=XMM_SCALAR(MEMF(g_esp+0x1c));g_xmm0.f[0]*=g_xmm3.f[0];g_xmm3=g_xmm0;
    }
    MEMF(g_esp+0x5c)=g_xmm3.f[0];g_xmm0=XMM_MEM(g_esp+0x50);XMM_STORE(g_edi+g_ecx,g_xmm0);SET_LO8(g_eax,MEM8(g_edx+0xa6));
    int thermal=LO8(g_eax)!=0;if(thermal){g_eax=MEM32(g_ebx+0xac);thermal=MEM8(g_eax+12)!=0;}
    if(thermal)g_xmm0=XMM_MEM(g_edx+0xb0);
    else{g_ecx=MEM32(g_ebx+16);g_edx=MEM16(g_esi+g_ecx+0x1c);PUSH32(g_esp,g_ecx);g_xmm0.f[0]=(float)(int32_t)g_edx;g_xmm0.f[0]*=MEMF(0x482fbc);g_eax=g_esp+0x34;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_eax);g_ecx=g_ebx+0xbc;g_ebp=frame;PUSH32(g_esp,0x239f2b);sub_0023A010();
     g_xmm0=XMM_SCALAR(MEMF(g_esp+0x3c));g_xmm0.f[0]*=MEMF(g_esp+0x1c);g_xmm1=XMM_MEM(g_esp+0x60);g_edx=MEM32(g_esp+0x24);g_ecx=MEM32(g_esp+0x20);MEMF(g_esp+0x3c)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+0x30);}
    g_eax=MEM32(g_esp+0x28);XMM_STORE(g_edi,g_xmm0);g_esi+=32;g_edi+=16;g_eax--;MEM32(g_esp+0x28)=g_eax;
   }while(g_eax);
  }
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=24;return 1;
}
