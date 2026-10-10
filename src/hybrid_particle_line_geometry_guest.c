#include "hybrid_particle_line_geometry_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_000643E0(void);
static int line_geometry_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_LINE_GEOMETRY");enabled=v&&!strcmp(v,"1");}return enabled;}
static int line_geometry_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}

/* This is the complete source-owned calculation, not private speculative
 * output. Reads and writes deliberately retain their alias-sensitive order.
 * Valid live objects/buffers remain the same precondition as the source. */
int uc_hybrid_particle_line_geometry(unsigned operation){
 if(!line_geometry_enabled())return 0;
 uint32_t start=g_esp;
 if(operation>3||(start&3)||start<0x10100u||!line_geometry_span(start-256,280)||
    !line_geometry_span(g_ecx,0xb4)||!line_geometry_span(0x51c178,1)||MEM8(0x51c178)||
    (operation==1&&(g_fp_top<0||g_fp_top>7)))return 0;
 if(operation==0){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x20);SET_LO8(g_ecx,MEM8(g_eax+0xc8));
  int zero=LO8(g_ecx)==0;
  if(!zero){PUSH32(g_esp,0x23d1c5);sub_000643E0();SET_LO8(g_ecx,MEM8(g_eax+0x32));zero=LO8(g_ecx)==0;
   if(zero){g_ecx=MEM32(g_esp+8);MEM32(g_ecx)=0;POP32(g_esp,g_esi);g_recomp_zf=zero;g_esp+=16;return 1;}}
  g_edx=MEM32(g_esi+0x9c);g_eax=MEM32(g_esp+8);MEM32(g_eax)=g_edx;
  g_ecx=MEM32(g_esi+0x20);g_edx=MEM32(g_ecx+0x98);g_eax=MEM32(g_esp+12);MEM32(g_eax)=g_edx;
  g_ecx=MEM32(g_esi+0x20);g_edx=MEM32(g_ecx+0x94);g_eax=MEM32(g_esp+16);MEM32(g_eax)=g_edx;
  POP32(g_esp,g_esi);g_recomp_zf=zero;g_esp+=16;return 1;
 }
 if(operation==3){g_eax=MEM32(g_ecx+0x20);g_eax=MEM32(g_eax+0xc4);g_esp+=4;return 1;}
 if(operation==2){
  g_esp-=0x3c;g_eax=MEM32(g_ecx+0x20);
  g_xmm0=XMM_SCALAR(MEMF(g_eax+0x100));g_xmm1=XMM_SCALAR(MEMF(g_ecx+0x6c));g_xmm5=XMM_SCALAR(MEMF(g_ecx+0x7c));
  g_xmm2=XMM_SCALAR(MEMF(g_ecx+0x70));g_xmm6=XMM_SCALAR(MEMF(g_ecx+0x80));g_xmm3=XMM_SCALAR(MEMF(g_ecx+0x74));g_xmm4=XMM_SCALAR(MEMF(g_ecx+0x78));
  g_xmm1.f[0]-=g_xmm0.f[0];g_xmm2.f[0]-=g_xmm0.f[0];g_xmm5.f[0]+=g_xmm0.f[0];g_xmm6.f[0]+=g_xmm0.f[0];g_xmm3.f[0]-=g_xmm0.f[0];g_xmm7=g_xmm1;g_xmm4.f[0]+=g_xmm0.f[0];
  MEMF(g_esp+0x10)=g_xmm2.f[0];g_xmm1=g_xmm5;g_xmm1.f[0]+=g_xmm2.f[0];g_xmm2=g_xmm6;g_xmm2.f[0]+=g_xmm3.f[0];MEMF(g_esp+0x14)=g_xmm3.f[0];
  g_xmm3=XMM_SCALAR(MEMF(0x44c1f8));g_xmm2.f[0]*=g_xmm3.f[0];MEMF(g_esp+8)=g_xmm2.f[0];g_eax=MEM32(g_esp+8);MEMF(g_esp+0x20)=g_xmm6.f[0];
  g_xmm2=g_xmm6;g_xmm6=XMM_SCALAR(MEMF(g_esp+0x14));g_xmm0=g_xmm4;g_xmm0.f[0]+=g_xmm7.f[0];g_xmm0.f[0]*=g_xmm3.f[0];MEMF(g_esp)=g_xmm0.f[0];g_ecx=MEM32(g_esp);MEM32(g_esp+0x2c)=g_eax;
  g_xmm2.f[0]-=g_xmm6.f[0];g_xmm2.f[0]*=g_xmm3.f[0];MEMF(g_esp+8)=g_xmm2.f[0];g_eax=MEM32(g_esp+8);
  g_xmm0=g_xmm4;g_xmm0.f[0]-=g_xmm7.f[0];g_xmm0.f[0]*=g_xmm3.f[0];MEMF(g_esp)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x10));MEM32(g_esp+0x38)=g_eax;
  g_eax=MEM32(g_esp+0x40);MEMF(g_eax+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x20));MEMF(g_eax)=g_xmm7.f[0];MEMF(g_eax+8)=g_xmm6.f[0];MEMF(g_eax+0x14)=g_xmm0.f[0];
  g_xmm1.f[0]*=g_xmm3.f[0];MEMF(g_esp+4)=g_xmm1.f[0];g_edx=MEM32(g_esp+4);MEMF(g_eax+12)=g_xmm4.f[0];MEMF(g_eax+16)=g_xmm5.f[0];
  MEM32(g_esp+0x24)=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_esp+0x24));g_ecx=MEM32(g_esp);MEMF(g_eax+0x18)=g_xmm0.f[0];MEM32(g_esp+0x28)=g_edx;
  g_xmm0=XMM_SCALAR(MEMF(g_esp+0x28));MEMF(g_eax+0x1c)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x2c));MEMF(g_eax+0x20)=g_xmm0.f[0];
  g_xmm1=g_xmm5;g_xmm1.f[0]-=MEMF(g_esp+0x10);g_xmm1.f[0]*=g_xmm3.f[0];MEMF(g_esp+4)=g_xmm1.f[0];g_edx=MEM32(g_esp+4);MEM32(g_esp+0x30)=g_ecx;
  g_xmm0=XMM_SCALAR(MEMF(g_esp+0x30));MEMF(g_eax+0x24)=g_xmm0.f[0];MEM32(g_esp+0x34)=g_edx;g_xmm0=XMM_SCALAR(MEMF(g_esp+0x34));MEMF(g_eax+0x28)=g_xmm0.f[0];
  g_xmm0=XMM_SCALAR(MEMF(g_esp+0x38));MEMF(g_eax+0x2c)=g_xmm0.f[0];g_esp+=0x3c;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
 }
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-0x64;
 g_eax=MEM32(g_ecx+0x20);SET_LO8(g_edx,MEM8(g_eax+0x9c));g_xmm7=XMM_SCALAR(MEMF(g_eax+0x100));g_xmm7.f[0]*=MEMF(g_ecx+0xac);
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);int camera=LO8(g_edx)!=0;g_edx=MEM32(frame+0x10);PUSH32(g_esp,g_edi);if(!camera)g_edx=0x51e110;
 g_eax=MEM32(g_ecx+0x20);g_esi=MEM32(g_ecx+0xb0);g_xmm0=XMM_SCALAR(MEMF(g_eax+0xa0));g_xmm2=XMM_SCALAR(MEMF(g_esi));g_xmm1=XMM_SCALAR(MEMF(g_eax+0x3c));
 g_xmm3=XMM_SCALAR(MEMF(g_esi+4));g_xmm4=XMM_SCALAR(MEMF(g_esi+8));g_ebx=MEM32(frame+12);g_xmm6=XMM_SCALAR(MEMF(g_eax+0x44));
 g_xmm2.f[0]*=g_xmm0.f[0];g_xmm1.f[0]+=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_eax+0x40));g_xmm3.f[0]*=g_xmm0.f[0];g_xmm4.f[0]*=g_xmm0.f[0];g_xmm2.f[0]+=g_xmm3.f[0];g_xmm6.f[0]+=g_xmm4.f[0];
 MEMF(g_esp+0x2c)=g_xmm1.f[0];MEMF(g_esp+0x30)=g_xmm2.f[0];MEMF(g_esp+0x34)=g_xmm6.f[0];
 if((int32_t)g_ebx>0){
  g_eax=MEM32(frame+8);g_edi=MEM32(frame+0x18);g_esi=MEM32(frame+0x14);g_eax<<=5;MEM32(g_esp+0x14)=g_eax;MEM32(g_esp+0x18)=g_ebx;
  for(;;){
   g_ebx=MEM32(g_ecx+0x10);g_xmm0=XMM_SCALAR(MEMF(g_eax+g_ebx));g_xmm4=XMM_MEM(g_edx+0x20);g_xmm5=XMM_MEM(g_edx+0x10);g_eax+=g_ebx;
   MEMF(g_esp+0x50)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+4));MEMF(g_esp+0x54)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+8));MEMF(g_esp+0x58)=g_xmm0.f[0];
   g_xmm0=XMM_ZERO();MEMF(g_esp+0x5c)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+0x50);g_xmm3=g_xmm0;g_xmm3=XMM_SHUFFLE(g_xmm3,g_xmm0,0xaa);g_xmm4=XMM_MUL(g_xmm4,g_xmm3);
   g_xmm3=g_xmm0;g_xmm3=XMM_SHUFFLE(g_xmm3,g_xmm0,0x55);g_xmm5=XMM_MUL(g_xmm5,g_xmm3);g_xmm3=XMM_MEM(g_edx+0x30);g_xmm5=XMM_ADD(g_xmm5,g_xmm4);
   g_xmm4=g_xmm0;g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm0,0);g_xmm0=XMM_MEM(g_edx);g_xmm0=XMM_MUL(g_xmm0,g_xmm4);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);g_xmm0=XMM_ADD(g_xmm0,g_xmm5);XMM_STORE(g_esp+0x60,g_xmm0);
   g_xmm0=XMM_SCALAR(MEMF(g_eax+0x10));g_xmm0.f[0]+=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_eax+0x14));g_xmm1.f[0]+=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_eax+0x18));g_xmm2.f[0]+=g_xmm6.f[0];
   MEMF(g_esp+0x28)=g_xmm2.f[0];g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_esp+0x28);g_fp_stack[g_fp_top]*=MEMF(g_esp+0x28);
   MEMF(g_esp+0x24)=g_xmm1.f[0];g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_esp+0x24);MEMF(g_esp+0x20)=g_xmm0.f[0];g_fp_stack[g_fp_top]*=MEMF(g_esp+0x24);
   g_xmm3=XMM_SCALAR(MEMF(g_esp+0x60));g_xmm4=XMM_SCALAR(MEMF(g_esp+0x64));g_fp_stack[(g_fp_top+1)&7]+=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
   g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_esp+0x20);g_xmm5=XMM_SCALAR(MEMF(g_esp+0x68));g_fp_stack[g_fp_top]*=MEMF(g_esp+0x20);
   MEMF(g_esp+0x38)=g_xmm3.f[0];g_ebx=MEM32(g_esp+0x38);g_eax=g_esi;g_fp_stack[(g_fp_top+1)&7]+=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;MEM32(g_eax)=g_ebx;
   MEMF(g_esp+0x3c)=g_xmm4.f[0];g_fp_stack[g_fp_top]=sqrt(g_fp_stack[g_fp_top]);g_ebx=MEM32(g_esp+0x3c);MEM32(g_eax+4)=g_ebx;
   MEMF(g_esp+0x40)=g_xmm5.f[0];g_ebx=MEM32(g_esp+0x40);MEM32(g_eax+8)=g_ebx;g_eax=g_esi+12;g_fp_stack[g_fp_top]=MEMF(0x44bf94)/g_fp_stack[g_fp_top];MEMF(g_esp+0x1c)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
   g_xmm6=XMM_SCALAR(MEMF(g_esp+0x1c));g_xmm0.f[0]*=g_xmm6.f[0];g_xmm1.f[0]*=g_xmm6.f[0];g_xmm2.f[0]*=g_xmm6.f[0];g_xmm0.f[0]*=g_xmm7.f[0];g_xmm1.f[0]*=g_xmm7.f[0];g_xmm2.f[0]*=g_xmm7.f[0];
   g_xmm3.f[0]-=g_xmm0.f[0];MEMF(g_esp+0x44)=g_xmm3.f[0];g_ebx=MEM32(g_esp+0x44);g_xmm4.f[0]-=g_xmm1.f[0];g_xmm5.f[0]-=g_xmm2.f[0];MEMF(g_esp+0x48)=g_xmm4.f[0];MEMF(g_esp+0x4c)=g_xmm5.f[0];
   MEM32(g_eax)=g_ebx;g_ebx=MEM32(g_esp+0x48);MEM32(g_eax+4)=g_ebx;g_ebx=MEM32(g_esp+0x4c);MEM32(g_eax+8)=g_ebx;
   g_eax=MEM32(g_ecx+0x20);g_xmm0=XMM_MEM(g_eax+0x50);g_ebx=MEM32(g_esp+0x18);XMM_STORE(g_edi,g_xmm0);g_eax=MEM32(g_ecx+0x20);g_xmm0=XMM_MEM(g_eax+0x70);g_eax=MEM32(g_esp+0x14);XMM_STORE(g_edi+16,g_xmm0);
   g_eax+=32;g_esi+=24;g_edi+=32;g_ebx--;MEM32(g_esp+0x14)=g_eax;MEM32(g_esp+0x18)=g_ebx;if(!g_ebx)break;
   g_xmm2=XMM_SCALAR(MEMF(g_esp+0x30));g_xmm6=XMM_SCALAR(MEMF(g_esp+0x34));g_xmm1=XMM_SCALAR(MEMF(g_esp+0x2c));
  }
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=24;return 1;
}
