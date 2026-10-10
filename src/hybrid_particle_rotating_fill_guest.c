#include "hybrid_particle_rotating_fill_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_0023A010(void);
/* Scalar SSE preserves the source destination operand when both inputs are NaNs. */
static float rotating_fill_mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
static float rotating_fill_add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}

static int rotating_fill_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_ROTATING_FILL");enabled=v&&!strcmp(v,"1");}return enabled;}
static int rotating_fill_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}

int uc_hybrid_particle_rotating_fill(void){
 if(!rotating_fill_enabled())return 0;
 uint32_t start=g_esp;
 if((start&3)||start<0x10100u||!rotating_fill_span(start-256,288)||!rotating_fill_span(g_ecx,0x124)||
    !rotating_fill_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-0x64;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x20);SET_LO8(g_ecx,MEM8(g_eax+0x9c));
 int camera=LO8(g_ecx)!=0;PUSH32(g_esp,g_edi);MEM32(g_esp+0x24)=g_eax;
 if(camera){
  g_eax=MEM32(frame+12);MEM32(g_esp+0x18)=0;
  if((int32_t)g_eax>0){
   g_edx=MEM32(frame+0x1c);g_ebx=MEM32(frame+8);g_eax=MEM32(frame+0x14);g_edi=MEM32(frame+16);g_ebx<<=5;g_eax-=g_edx;MEM32(g_esp+0x1c)=g_edx;MEM32(g_esp+0x20)=g_eax;
   for(;;){
    g_eax=MEM32(g_esi+16);g_xmm3=XMM_SCALAR(MEMF(g_esi+0xc4));g_ecx=g_ebx+g_eax;g_eax=MEM16(g_ecx+0x1c);g_xmm0.f[0]=(float)(int32_t)g_eax;g_eax=MEM16(g_ecx+0x1e);g_eax&=0x8000000fu;
    g_xmm0.f[0]=rotating_fill_mul(g_xmm0.f[0],MEMF(0x482fbc));MEMF(g_esp+0x28)=g_xmm0.f[0];g_xmm3.f[0]=rotating_fill_mul(g_xmm3.f[0],g_xmm0.f[0]);
    /* MEM16 is zero-extended, so the source signed-remainder repair cannot fire. */
    g_xmm2=XMM_SCALAR(MEMF(g_eax*4+0x51ca98));g_eax=MEM32(g_esi+16);g_xmm0=XMM_SCALAR(MEMF(g_esi+0xb8));g_xmm1=XMM_SCALAR(MEMF(g_esi+0xc0));g_xmm5=XMM_MEM(g_edi+32);g_xmm6=XMM_MEM(g_edi+16);
    g_xmm0.f[0]=rotating_fill_mul(g_xmm0.f[0],g_xmm2.f[0]);g_xmm1.f[0]=rotating_fill_mul(g_xmm1.f[0],g_xmm2.f[0]);g_xmm2=XMM_SCALAR(MEMF(g_eax+g_ebx));MEMF(g_esp+0x30)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_eax+g_ebx+4));MEMF(g_esp+0x34)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_eax+g_ebx+8));g_eax+=g_ebx;MEMF(g_esp+0x38)=g_xmm2.f[0];
    g_xmm2=XMM_ZERO();g_xmm0.f[0]=rotating_fill_add(g_xmm0.f[0],MEMF(g_esi+0xb4));MEMF(g_esp+0x3c)=g_xmm2.f[0];g_xmm2=XMM_MEM(g_esp+0x30);g_xmm1.f[0]=rotating_fill_add(g_xmm1.f[0],MEMF(g_esi+0xbc));g_xmm4=g_xmm2;g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm2,0xaa);g_eax=MEM32(frame+0x18);g_xmm1.f[0]=rotating_fill_mul(g_xmm1.f[0],MEMF(0x44d95c));
    g_xmm5=XMM_MUL(g_xmm5,g_xmm4);g_xmm4=g_xmm2;g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm2,0x55);g_xmm6=XMM_MUL(g_xmm6,g_xmm4);g_xmm4=XMM_MEM(g_edi+48);g_xmm6=XMM_ADD(g_xmm6,g_xmm5);g_xmm5=g_xmm2;g_xmm5=XMM_SHUFFLE(g_xmm5,g_xmm2,0);g_xmm2=XMM_MEM(g_edi);g_xmm2=XMM_MUL(g_xmm2,g_xmm5);g_xmm2=XMM_ADD(g_xmm2,g_xmm4);g_xmm2=XMM_ADD(g_xmm2,g_xmm6);XMM_STORE(g_esp+0x50,g_xmm2);
    g_xmm2=XMM_SCALAR(MEMF(g_esp+0x50));MEMF(g_esp+0x40)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_esp+0x54));MEMF(g_esp+0x44)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_esp+0x58));MEMF(g_esp+0x48)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_ecx+12));g_ecx=MEM32(g_esp+0x20);MEMF(g_esp+0x4c)=g_xmm2.f[0];g_xmm2=XMM_MEM(g_esp+0x40);XMM_STORE(g_ecx+g_edx,g_xmm2);
    g_ecx=MEM32(g_esp+0x18);g_xmm0.f[0]=rotating_fill_mul(g_xmm0.f[0],g_xmm3.f[0]);g_xmm0.f[0]=rotating_fill_add(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_eax+g_ecx*4)=g_xmm0.f[0];g_eax=MEM32(g_esp+0x24);SET_LO8(g_ecx,MEM8(g_eax+0xa6));
    int thermal=LO8(g_ecx)!=0;if(thermal){g_ecx=MEM32(g_esi+0xac);thermal=MEM8(g_ecx+12)!=0;}
    if(thermal){g_xmm0=XMM_MEM(g_eax+0xb0);XMM_STORE(g_edx,g_xmm0);}
    else{g_edx=MEM32(g_esp+0x28);PUSH32(g_esp,g_edx);g_eax=g_esp+0x64;PUSH32(g_esp,g_eax);g_ecx=g_esi+0xcc;g_ebp=frame;PUSH32(g_esp,0x23b86d);sub_0023A010();g_ecx=MEM32(g_esp+0x1c);g_xmm0=XMM_MEM(g_eax);XMM_STORE(g_ecx,g_xmm0);g_edx=g_ecx;}
    g_eax=MEM32(g_esp+0x18);g_ecx=MEM32(frame+12);g_eax++;g_edx+=16;g_ebx+=32;int equal=g_eax==g_ecx;MEM32(g_esp+0x18)=g_eax;MEM32(g_esp+0x1c)=g_edx;
    if((int32_t)g_eax<(int32_t)g_ecx)continue;
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=equal;g_esp+=28;return 1;
   }
  }
 }else{
  g_ecx=MEM32(frame+16);g_xmm1=XMM_MEM(g_ecx+32);g_xmm0=XMM_MEM(g_ecx+48);g_xmm2=XMM_MEM(0x51e4a0);SET_LO8(g_ecx,MEM8(0x4fe7c4));
  g_xmm2=XMM_XOR(g_xmm2,g_xmm0);g_xmm0=g_xmm1;g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_xmm2=g_xmm0;g_xmm2=XMM_MOVE_HIGH_TO_LOW(g_xmm2,g_xmm0);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);g_xmm2=g_xmm0;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,1);g_xmm0.f[0]=rotating_fill_add(g_xmm0.f[0],g_xmm2.f[0]);g_xmm2=g_xmm1;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm1,0x27);g_xmm3=g_xmm2;g_xmm3.f[0]=g_xmm0.f[0];g_xmm3=XMM_SHUFFLE(g_xmm3,g_xmm3,0x27);XMM_STORE(g_esp+0x60,g_xmm3);
  if(LO8(g_ecx)){SET_LO8(g_ecx,MEM8(g_eax+0xc8));MEM8(g_esp+0x17)=1;if(LO8(g_ecx))MEM8(g_esp+0x17)=0;}else MEM8(g_esp+0x17)=0;
  g_eax=MEM32(frame+12);g_ecx=0;MEM32(g_esp+0x18)=g_ecx;
  if((int32_t)g_eax>0){
   g_edi=MEM32(frame+8);g_ebx=MEM32(frame+0x1c);g_eax=MEM32(frame+0x14);g_edi<<=5;g_eax-=g_ebx;MEM32(g_esp+0x20)=g_eax;
   do{
    g_edx=MEM32(g_esi+16);g_eax=g_edx+g_edi;g_edx=MEM16(g_eax+0x1c);g_xmm1.f[0]=(float)(int32_t)g_edx;g_edx=MEM16(g_eax+0x1e);g_edx&=0x8000000fu;g_xmm1.f[0]=rotating_fill_mul(g_xmm1.f[0],MEMF(0x482fbc));MEMF(g_esp+0x2c)=g_xmm1.f[0];
    g_xmm2=XMM_SCALAR(MEMF(g_edx*4+0x51ca98));g_xmm0=XMM_SCALAR(MEMF(g_esi+0xb8));g_xmm4=XMM_SCALAR(MEMF(g_esi+0xc4));g_xmm4.f[0]=rotating_fill_mul(g_xmm4.f[0],g_xmm1.f[0]);g_xmm1=XMM_SCALAR(MEMF(g_esi+0xc0));g_xmm0.f[0]=rotating_fill_mul(g_xmm0.f[0],g_xmm2.f[0]);g_xmm0.f[0]=rotating_fill_add(g_xmm0.f[0],MEMF(g_esi+0xb4));g_xmm1.f[0]=rotating_fill_mul(g_xmm1.f[0],g_xmm2.f[0]);g_xmm1.f[0]=rotating_fill_add(g_xmm1.f[0],MEMF(g_esi+0xbc));g_xmm1.f[0]=rotating_fill_mul(g_xmm1.f[0],MEMF(0x44d95c));g_xmm0.f[0]=rotating_fill_mul(g_xmm0.f[0],g_xmm4.f[0]);g_xmm4=XMM_SCALAR(MEMF(g_eax+12));g_eax=MEM32(g_esi+16);g_eax+=g_edi;g_xmm0.f[0]=rotating_fill_add(g_xmm0.f[0],g_xmm1.f[0]);
    g_xmm1=XMM_SCALAR(MEMF(g_eax));MEMF(g_esp+0x40)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_eax+4));MEMF(g_esp+0x44)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_eax+8));SET_LO8(g_eax,MEM8(g_esp+0x17));MEMF(g_esp+0x48)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(0x44bf94));MEMF(g_esp+0x4c)=g_xmm1.f[0];g_xmm2=XMM_MEM(g_esp+0x40);XMM_STORE(g_esp+0x30,g_xmm2);MEMF(g_esp+0x1c)=g_xmm1.f[0];
    if(LO8(g_eax)){
     g_xmm1=g_xmm3;g_xmm1=XMM_MUL(g_xmm1,g_xmm2);g_xmm5=g_xmm1;g_xmm5=XMM_SHUFFLE(g_xmm5,g_xmm1,2);g_xmm2=g_xmm3;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm3,3);g_xmm5.f[0]=rotating_fill_add(g_xmm5.f[0],g_xmm2.f[0]);g_xmm2=g_xmm1;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm1,1);g_xmm1.f[0]=rotating_fill_add(g_xmm1.f[0],g_xmm2.f[0]);g_xmm2=XMM_SCALAR(MEMF(0x482f40));g_xmm1.f[0]=rotating_fill_add(g_xmm1.f[0],g_xmm5.f[0]);g_eax=g_esp+0x28;MEMF(g_eax)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_esp+0x28));double a=g_xmm2.f[0];double b=g_xmm1.f[0];unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;
     if(!(flags&0x41u)){g_xmm1.f[0]=rotating_fill_mul(g_xmm1.f[0],MEMF(0x4fe7d0));g_xmm1.f[0]=rotating_fill_mul(g_xmm1.f[0],g_xmm1.f[0]);MEMF(g_esp+0x1c)=g_xmm1.f[0];}
     g_xmm1=XMM_SCALAR(MEMF(g_esp+0x1c));g_xmm1.f[0]=rotating_fill_mul(g_xmm1.f[0],g_xmm4.f[0]);g_xmm4=g_xmm1;
    }
    g_edx=MEM32(g_esp+0x20);g_eax=MEM32(frame+0x18);MEMF(g_esp+0x3c)=g_xmm4.f[0];g_xmm1=XMM_MEM(g_esp+0x30);XMM_STORE(g_edx+g_ebx,g_xmm1);MEMF(g_eax+g_ecx*4)=g_xmm0.f[0];g_eax=MEM32(g_esp+0x24);SET_LO8(g_edx,MEM8(g_eax+0xa6));
    int thermal=LO8(g_edx)!=0;if(thermal){g_edx=MEM32(g_esi+0xac);thermal=MEM8(g_edx+12)!=0;}
    if(thermal)g_xmm0=XMM_MEM(g_eax+0xb0);
    else{g_eax=MEM32(g_esp+0x2c);PUSH32(g_esp,g_eax);g_ecx=g_esp+0x54;PUSH32(g_esp,g_ecx);g_ecx=g_esi+0xcc;g_ebp=frame;PUSH32(g_esp,0x23baae);sub_0023A010();g_xmm0=XMM_SCALAR(MEMF(g_esp+0x5c));g_xmm0.f[0]=rotating_fill_mul(g_xmm0.f[0],MEMF(g_esp+0x1c));g_xmm3=XMM_MEM(g_esp+0x60);g_ecx=MEM32(g_esp+0x18);MEMF(g_esp+0x5c)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+0x50);}
    g_eax=MEM32(frame+12);g_ecx++;XMM_STORE(g_ebx,g_xmm0);g_edi+=32;g_ebx+=16;MEM32(g_esp+0x18)=g_ecx;
   }while((int32_t)g_ecx<(int32_t)g_eax);
  }
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=28;return 1;
}
