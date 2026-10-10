/* Whole decal ownership, with original material/device/world service boundaries.
 * Source-stage: no production connection or qualification is claimed yet. */
#include "hybrid_decal_owner_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
extern void sub_0009B8A0(void),sub_0009BA60(void),sub_0009BF50(void);
extern void sub_0009C3B0(void),sub_0009C430(void),sub_0009C340(void);
extern void sub_00095090(void),sub_000958B0(void),sub_00086F90(void),sub_0022B280(void);
extern void sub_002D589F(void),sub_002D631D(void),sub_002D5CCD(void),sub_002D57FB(void),sub_002D5990(void);
extern void sub_003EF5C0(void),sub_003EF600(void),sub_003EF700(void),sub_003EF740(void);
#define DEC_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define DEC_VCALL(target,pc,saved) do{uint32_t dec_target=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(dec_target,saved);}while(0)
#define DEC_PUSH(v) do{double dec_value=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=dec_value;}while(0)
#define DEC_POP() (g_fp_top=(g_fp_top+1u)&7u)
#define DEC_ST(i) g_fp_stack[(g_fp_top+(i))&7u]
static unsigned decal_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
static int decal_enabled(void){static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_DECAL_OWNER");enabled=p&&!strcmp(p,"1");}return enabled;}
static int decal_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram&&(uint64_t)p+n<=0x80000000ull;}
static int decal_apart(uint32_t p,uint64_t n,uint32_t q,uint64_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}

static void decal_construct(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38cb73);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;
 MEM32(g_esi)=0x456a64;MEM32(g_esi+0x10)=0x456058;
 for(unsigned row=0;row<3;row++){g_xmm0=XMM_MEM(0x51e110+row*16);XMM_STORE(g_esi+0x20+row*16,g_xmm0);}
 g_xmm0=XMM_MEM(0x51e140);g_edi=0;SET_LO8(g_eax,1);XMM_STORE(g_esi+0x50,g_xmm0);MEM32(g_esp+0x14)=g_edi;MEM8(g_esi+0x60)=LO8(g_eax);
 g_ecx=MEM32(g_esp+0x1c);MEM8(g_esp+0x14)=LO8(g_eax);g_eax=0xaf286bcbu;
 {uint64_t product=(uint64_t)g_eax*g_ecx;g_eax=(uint32_t)product;g_edx=(uint32_t)(product>>32);}
 g_ecx-=g_edx;g_ecx>>=1;g_ecx+=g_edx;g_ecx>>=6;MEM32(g_esi+0x8c)=g_ecx;g_ecx*=76;
 PUSH32(g_esp,g_ecx);MEM32(g_esi)=0x456ae0;MEM32(g_esi+0x10)=0x456a78;DEC_CALL(sub_002D589F,0x9bf16);
 g_ecx=MEM32(g_esp+0x10);g_esp+=4;MEM32(g_esi+0x88)=g_edi;MEM32(g_esi+0x84)=g_edi;MEM32(g_esi+0x90)=g_edi;MEM32(g_esi+0x80)=g_eax;
 POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void decal_destroy(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38cb4f);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_edi=g_esi+16;MEM32(g_esp+12)=g_esi;
 MEM32(g_esi)=0x456ae0;MEM32(g_edi)=0x456a78;g_eax=MEM32(g_esi+0x80);PUSH32(g_esp,g_eax);MEM32(g_esp+0x1c)=1;DEC_CALL(sub_002D631D,0x9b8e3);
 g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);g_esp+=4;{uint32_t saved=g_esp;PUSH32(g_esp,g_edi);DEC_VCALL(MEM32(g_edx+0xdc),0x9b8f5,saved);}
 g_ecx=g_eax;DEC_CALL(sub_00095090,0x9b8fc);g_ecx=MEM32(g_esp+16);MEM32(g_edi)=0x455d50;MEM32(g_esi)=0x456a64;POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void decal_delete(unsigned adjust){
 if(adjust)g_ecx-=16;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;DEC_CALL(sub_0009B8A0,0x9bb98);
 if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);DEC_CALL(sub_002D5CCD,0x9bba5);g_esp+=4;}
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;
}
static void decal_reset(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=g_esi?g_esi+16:0;g_ecx=MEM32(0x519f60);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_ecx);DEC_VCALL(MEM32(g_eax+0xdc),0x9b94d,saved);}
 g_ecx=g_eax;DEC_CALL(sub_00095090,0x9b954);g_edx=MEM32(g_esi);g_ecx=g_esi;
 {uint32_t saved=g_esp;DEC_VCALL(MEM32(g_edx+8),0x9b95b,saved);}
 g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);
 {uint32_t saved=g_esp;PUSH32(g_esp,1);g_esi+=16;PUSH32(g_esp,g_esi);DEC_VCALL(MEM32(g_eax+0xdc),0x9b96f,saved);}
 g_ecx=g_eax;DEC_CALL(sub_000958B0,0x9b976);POP32(g_esp,g_esi);g_recomp_zf=g_esi==0;g_esp+=4;
}
static void decal_bounds(void){
 SET_LO8(g_ecx,MEM8(0x51a3a8));g_eax=1;
 if(!(LO8(g_eax)&LO8(g_ecx))){g_xmm0=XMM_ZERO();MEM32(0x51a3a8)|=g_eax;for(unsigned i=0;i<12;i++)MEMF(0x51a378+i*4)=g_xmm0.f[0];}
 g_eax=0x51a378;g_esp+=4;
}
static void decal_collect(void){
 SET_LO8(g_edx,MEM8(0x51a3b8));g_eax=1;
 if(!(LO8(g_eax)&LO8(g_edx))){g_edx=MEM32(0x51a3b8);g_xmm0=XMM_ZERO();g_edx|=g_eax;MEM32(0x51a3b8)=g_edx;MEM32(0x51a3ac)=2;MEMF(0x51a3b4)=g_xmm0.f[0];MEM32(0x51a3b0)=0x30;}
 g_eax=g_ecx-16;g_eax=g_eax?0xffffffffu:0;
 {uint32_t saved=g_esp;PUSH32(g_esp,0);g_eax&=g_ecx;g_ecx=MEM32(g_esp+8);g_edx=MEM32(g_ecx);PUSH32(g_esp,0x51a3ac);PUSH32(g_esp,g_eax);DEC_VCALL(MEM32(g_edx+4),0x9ba56,saved);}
 g_recomp_zf=g_eax==0;g_esp+=12;
}
static void decal_reserve(void){
 uint32_t frame=g_seh_ebp;g_edx=MEM32(g_ecx+0x88);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_ecx+0x8c);frame=g_edx;g_ebx=0;
 for(;;){
  g_edx++;MEM32(g_ecx+0x88)=g_edx;if((int32_t)g_edx>=(int32_t)g_edi)MEM32(g_ecx+0x88)=g_ebx;
  g_edx=MEM32(g_ecx+0x88);g_esi=MEM32(g_ecx+0x84);if(g_edx!=g_esi)break;
  g_esi++;MEM32(g_ecx+0x84)=g_esi;if((int32_t)g_esi>=(int32_t)g_edi)MEM32(g_ecx+0x84)=g_ebx;
  g_esi=MEM32(g_ecx+0x80);g_eax=g_edx;g_eax*=76;if(!(MEM8(g_eax+g_esi+0x40)&1))break;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=frame;POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=4;
}
static void decal_tick(void){
 g_xmm1=XMM_ZERO();g_xmm3=XMM_SCALAR(MEMF(g_esp+4));unsigned flags=decal_compare(g_xmm1.f[0],g_xmm3.f[0]);g_edx=MEM32(g_ecx+0x84);
 if((flags&1)&&g_edx!=MEM32(g_ecx+0x88)){
  g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm4=XMM_SCALAR(MEMF(0x47df9c));g_xmm2=XMM_SCALAR(MEMF(0x456af0));PUSH32(g_esp,g_esi);
  do{
   g_esi=MEM32(g_ecx+0x80);g_eax=g_edx;g_eax*=76;g_xmm5=XMM_SCALAR(MEMF(g_eax+g_esi+0x44));g_eax+=g_esi;flags=decal_compare(g_xmm5.f[0],g_xmm1.f[0]);
   if(!(flags&0x41)){
    flags=decal_compare(g_xmm0.f[0],MEMF(g_eax+0x48));if(!(flags&0x41)){g_xmm5.f[0]=g_xmm5.f[0]*g_xmm3.f[0];g_xmm5.f[0]=g_xmm5.f[0]+MEMF(g_eax+0x48);MEMF(g_eax+0x48)=g_xmm5.f[0];}
    g_xmm5=XMM_SCALAR(MEMF(g_eax+0x48));flags=decal_compare(g_xmm5.f[0],g_xmm4.f[0]);
    if(!(flags&1)){g_xmm5=XMM_SCALAR(MEMF(g_eax+0x44));flags=decal_compare(g_xmm5.f[0],g_xmm2.f[0]);if(!(flags&0x41)){g_xmm6=XMM_SCALAR(MEMF(g_eax+0x44));g_xmm5.f[0]=g_xmm5.f[0]*g_xmm2.f[0];g_xmm6.f[0]=g_xmm6.f[0]-g_xmm5.f[0];MEMF(g_eax+0x44)=g_xmm6.f[0];}}
    g_xmm5=XMM_SCALAR(MEMF(g_eax+0x48));flags=decal_compare(g_xmm5.f[0],g_xmm0.f[0]);if(!(flags&1)){MEMF(g_eax+0x48)=g_xmm0.f[0];MEMF(g_eax+0x44)=g_xmm1.f[0];}
   }
   g_eax=MEM32(g_ecx+0x8c);g_edx++;if((int32_t)g_edx>=(int32_t)g_eax)g_edx=0;
  }while(g_edx!=MEM32(g_ecx+0x88));POP32(g_esp,g_esi);
 }
 g_esp+=8;
}
static void decal_count(void){
 g_edx=MEM32(g_ecx+0x78);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+0x74);
 if((int32_t)g_edx>=(int32_t)g_esi){g_edx-=g_esi;g_eax=g_edx+g_edx;POP32(g_esp,g_esi);g_recomp_zf=g_edx==0;}
 else{g_eax=MEM32(g_ecx+0x7c);g_eax-=g_esi;g_eax+=g_edx;g_eax+=g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;}g_esp+=4;
}

/* Reserve first, then resolve material and publish each field in source order.
 * Corner aliases are intentional: this is not an out-of-place quad conversion. */
static void decal_insert(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_esp-=0x44;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_ebp=frame;DEC_CALL(sub_0009BA60,0x9bbc1);g_edi=MEM32(g_ecx+0x80);g_esi=g_eax;g_eax=MEM32(frame+8);g_esi*=76;g_ecx=MEM32(g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_eax+4);g_esi+=g_edi;g_ebp=frame;DEC_CALL(sub_0022B280,0x9bbdc);
 if(g_eax)g_eax-=4;else g_eax=0;
 g_edx=MEM32(frame+16);MEM32(g_esi)=g_eax;g_eax=MEM32(frame+12);MEM32(g_esi+0x40)=g_edx;
 DEC_PUSH(MEMF(g_eax));DEC_ST(0)-=MEMF(g_eax+0x18);g_xmm0=XMM_ZERO();DEC_PUSH(MEMF(g_eax+4));DEC_ST(0)-=MEMF(g_eax+0x1c);DEC_PUSH(MEMF(g_eax+8));DEC_ST(0)-=MEMF(g_eax+0x20);
 DEC_PUSH(DEC_ST(0));DEC_ST(0)*=DEC_ST(1);DEC_PUSH(DEC_ST(2));DEC_ST(0)*=DEC_ST(3);DEC_ST(1)+=DEC_ST(0);DEC_POP();DEC_PUSH(DEC_ST(3));DEC_ST(0)*=DEC_ST(4);DEC_ST(1)+=DEC_ST(0);DEC_POP();DEC_ST(0)=sqrt(DEC_ST(0));DEC_ST(3)=DEC_ST(0);DEC_POP();DEC_POP();DEC_POP();MEMF(g_esp+0x18)=(float)DEC_ST(0);DEC_POP();
 DEC_PUSH(MEMF(0x456b00));DEC_PUSH(MEMF(g_esp+0x18));g_fp_cmp=RECOMP_FCMP(DEC_ST(0),DEC_ST(1));DEC_POP();DEC_POP();
 if(g_fp_cmp>0){g_xmm1=XMM_SCALAR(MEMF(frame+0x14));g_xmm1.f[0]=g_xmm1.f[0]/MEMF(g_esp+0x18);MEMF(g_esi+0x44)=g_xmm1.f[0];}else MEMF(g_esi+0x44)=g_xmm0.f[0];
 g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_edx=g_esi+16;MEMF(g_esi+0x48)=g_xmm0.f[0];MEMF(g_esp+0x40)=g_xmm0.f[0];MEMF(g_esp+0x44)=g_xmm0.f[0];MEMF(g_esp+0x48)=g_xmm0.f[0];MEMF(g_esp+0x4c)=g_xmm1.f[0];g_xmm0=XMM_MEM(g_esp+0x40);g_ecx=g_eax;MEM32(g_esp+0x18)=g_edx;MEM32(g_esp+0x1c)=4;
 do{
  g_edi=MEM32(g_esp+0x18);g_edx=g_ecx;g_ebx=MEM32(g_edx);MEM32(g_edi)=g_ebx;g_ebx=MEM32(g_edx+4);MEM32(g_edi+4)=g_ebx;g_edx=MEM32(g_edx+8);MEM32(g_edi+8)=g_edx;
  g_xmm2=XMM_SCALAR(MEMF(g_ecx));g_edx=MEM32(g_esp+0x1c);MEMF(g_esp+0x30)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_ecx+4));MEMF(g_esp+0x34)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_ecx+8));g_ebx=g_edi;g_ebx+=12;g_ecx+=12;MEMF(g_esp+0x38)=g_xmm2.f[0];MEMF(g_esp+0x3c)=g_xmm1.f[0];g_xmm2=XMM_MEM(g_esp+0x30);--g_edx;g_xmm0=XMM_ADD(g_xmm0,g_xmm2);MEM32(g_esp+0x18)=g_ebx;MEM32(g_esp+0x1c)=g_edx;
 }while(g_edx);
 /* The original cross product and normalization leave all eight SIMD lanes
  * and virtual x87 slots observable at the lighting service. */
 g_xmm1=XMM_SCALAR(MEMF(g_eax+0x20));g_xmm1.f[0]=g_xmm1.f[0]-MEMF(g_eax+8);g_xmm3=XMM_SCALAR(MEMF(g_eax+0x1c));g_xmm3.f[0]=g_xmm3.f[0]-MEMF(g_eax+4);g_xmm4=XMM_SCALAR(MEMF(g_eax+12));g_xmm4.f[0]=g_xmm4.f[0]-MEMF(g_eax);g_xmm2=XMM_SCALAR(MEMF(g_eax+0x14));g_xmm2.f[0]=g_xmm2.f[0]-MEMF(g_eax+8);g_xmm5=XMM_SCALAR(MEMF(g_eax+0x18));g_xmm5.f[0]=g_xmm5.f[0]-MEMF(g_eax);XMM_STORE(g_esp+0x40,g_xmm0);g_xmm0=XMM_SCALAR(MEMF(g_eax+16));g_xmm0.f[0]=g_xmm0.f[0]-MEMF(g_eax+4);
 g_xmm7=g_xmm1;g_xmm7.f[0]=g_xmm7.f[0]*g_xmm0.f[0];g_xmm0.f[0]=g_xmm0.f[0]*g_xmm5.f[0];g_xmm6=g_xmm3;g_xmm6.f[0]=g_xmm6.f[0]*g_xmm2.f[0];g_xmm1.f[0]=g_xmm1.f[0]*g_xmm4.f[0];g_xmm3.f[0]=g_xmm3.f[0]*g_xmm4.f[0];g_xmm0.f[0]=g_xmm0.f[0]-g_xmm3.f[0];MEMF(g_esp+0x38)=g_xmm0.f[0];g_xmm0=XMM_ZERO();g_xmm2.f[0]=g_xmm2.f[0]*g_xmm5.f[0];g_xmm1.f[0]=g_xmm1.f[0]-g_xmm2.f[0];MEMF(g_esp+0x34)=g_xmm1.f[0];MEMF(g_esp+0x3c)=g_xmm0.f[0];g_xmm6.f[0]=g_xmm6.f[0]-g_xmm7.f[0];MEMF(g_esp+0x30)=g_xmm6.f[0];g_xmm0=XMM_MEM(g_esp+0x30);XMM_STORE(g_esp+0x30,g_xmm0);
 DEC_PUSH(MEMF(g_esp+0x3c));DEC_ST(0)*=MEMF(g_esp+0x3c);g_xmm0=XMM_SCALAR(MEMF(0x44ec38));DEC_PUSH(MEMF(g_esp+0x38));g_xmm1=XMM_MEM(g_esp+0x40);DEC_ST(0)*=MEMF(g_esp+0x38);g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);DEC_ST(1)+=DEC_ST(0);DEC_POP();DEC_PUSH(MEMF(g_esp+0x34));g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);DEC_ST(0)*=MEMF(g_esp+0x34);g_xmm1=XMM_DIV(g_xmm1,g_xmm0);DEC_ST(1)+=DEC_ST(0);DEC_POP();DEC_PUSH(MEMF(g_esp+0x30));DEC_ST(0)*=MEMF(g_esp+0x30);DEC_ST(1)+=DEC_ST(0);DEC_POP();DEC_ST(0)=sqrt(DEC_ST(0));DEC_ST(0)=MEMF(0x44bf94)/DEC_ST(0);
 for(unsigned component=0;component<4;component++){DEC_PUSH(MEMF(g_esp+0x30+component*4));DEC_ST(0)*=DEC_ST(1);MEMF(g_esp+0x20+component*4)=(float)DEC_ST(0);DEC_POP();}
 g_xmm2=XMM_MEM(g_esp+0x20);g_xmm1=XMM_ADD(g_xmm1,g_xmm2);DEC_POP();XMM_STORE(g_esp+0x40,g_xmm1);
 {uint32_t saved=g_esp;DEC_VCALL(MEM32(g_eax+0xc4),0x9bdf4,saved);}
 g_xmm0=XMM_SCALAR(MEMF(g_esp+0x40));MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x44));MEMF(g_esp+0x24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x48));g_ecx=g_esp+0x30;MEMF(g_esp+0x28)=g_xmm0.f[0];
 {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=MEM32(g_eax);g_ecx=g_esp+0x24;PUSH32(g_esp,g_ecx);g_ecx=g_esp+0x24;PUSH32(g_esp,g_ecx);g_ecx=g_eax;DEC_VCALL(MEM32(g_edx+0x28),0x9be2e,saved);}
 g_ecx=g_eax;g_ebp=frame;DEC_CALL(sub_00086F90,0x9be35);g_edx=MEM32(g_eax);g_esi+=4;MEM32(g_esi)=g_edx;g_ecx=MEM32(g_eax+4);MEM32(g_esi+4)=g_ecx;g_edx=MEM32(g_eax+8);POP32(g_esp,g_edi);MEM32(g_esi+8)=g_edx;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esi==0;g_esp+=20;
}

static void decal_selection_destroy(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi)=0;g_eax=MEM32(g_esi+8);if(g_eax){PUSH32(g_esp,g_eax);DEC_CALL(sub_002D5CCD,0x9c2a6);g_esp+=4;}
 MEM32(g_esi+8)=0;MEM32(g_esi+12)=0;MEM32(g_esi+16)=0;POP32(g_esp,g_esi);g_esp+=4;
}
static void decal_selection_resize(void){
 uint32_t bits=g_ebp;PUSH32(g_esp,bits);bits=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+8);PUSH32(g_esp,g_edi);g_edi=(bits+31u)>>5;
 if(g_ecx){
  g_eax=MEM32(g_esi+12);g_eax-=g_ecx;g_eax=(uint32_t)((int32_t)g_eax>>2);
  if(g_edi<g_eax){g_eax=g_ecx;g_ecx=MEM32(g_esi+12);g_edx=g_eax+g_edi*4;if(g_edx!=g_ecx){g_eax=g_ecx;g_eax-=g_ecx;PUSH32(g_esp,g_ebx);g_eax=(uint32_t)((int32_t)g_eax>>2);g_ebx=g_eax*4;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);DEC_CALL(sub_002D5990,0x9c3ff);g_esp+=12;g_eax+=g_ebx;MEM32(g_esi+12)=g_eax;POP32(g_esp,g_ebx);}}
 }
 MEM32(g_esi)=bits;bits&=31;
 if(bits){g_ecx=MEM32(g_esi+8);g_eax=g_ecx+g_edi*4-4;g_ecx=bits;g_edx=1;g_edx<<=LO8(g_ecx);g_ecx=MEM32(g_eax);--g_edx;g_ecx&=g_edx;MEM32(g_eax)=g_ecx;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,bits);g_esp+=8;
}
static void decal_selection_construct(void){
 uint32_t value=g_ebp;PUSH32(g_esp,value);PUSH32(g_esp,g_esi);value=0;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);g_esi=g_ecx;MEM32(g_esi+4)=value;MEM32(g_esi+8)=value;MEM32(g_esi+12)=value;
 if(g_edi){
  if(g_edi>0x3fffffffu){DEC_CALL(sub_0009C340,0x9c455);}
  PUSH32(g_esp,g_ebx);g_ebx=g_edi*4;PUSH32(g_esp,g_ebx);DEC_CALL(sub_002D57FB,0x9c463);g_ecx=g_ebx+g_eax;g_esp+=4;MEM32(g_esi+12)=g_ecx;MEM32(g_esi+4)=g_eax;MEM32(g_esi+8)=g_eax;g_ecx=g_eax;
  if(g_edi>value){g_edx=MEM32(g_esp+0x18);do{value=MEM32(g_edx);MEM32(g_ecx)=value;g_ecx+=4;--g_edi;}while(g_edi);}
  g_ebx+=g_eax;MEM32(g_esi+8)=g_ebx;POP32(g_esp,g_ebx);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,value);g_esp+=12;
}

/* Each material pass owns a temporary selection bitset. It is published only
 * while the original material callback is allowed to consume it. */
static void decal_render(void){
 uint32_t count=g_seh_ebp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38cbb8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x28;g_eax=MEM32(g_esp+0x38);int draw=MEM32(g_eax)==2;MEM32(g_esp)=g_ecx;
 if(draw){
  g_eax=MEM32(g_ecx+0x78);PUSH32(g_esp,count);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx-16;g_ecx=MEM32(g_edi+0x84);
  if((int32_t)g_eax>=(int32_t)g_ecx){g_eax-=g_ecx;count=g_eax;}else{count=MEM32(g_edi+0x8c);count-=g_ecx;count+=g_eax;}
  g_ecx=g_esp+0x44;PUSH32(g_esp,g_ecx);g_edx=(count+31u)>>5;g_esi=0;PUSH32(g_esp,g_edx);g_ecx=g_esp+0x2c;MEM32(g_esp+0x28)=g_esi;MEM32(g_esp+0x4c)=g_esi;DEC_CALL(sub_0009C430,0x9c50a);
  PUSH32(g_esp,count);g_ecx=g_esp+0x24;MEM32(g_esp+0x40)=g_esi;DEC_CALL(sub_0009C3B0,0x9c518);g_ecx=MEM32(g_esp+12);g_eax=g_esp+0x20;MEM32(g_ecx+0x80)=g_eax;g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);MEM32(g_esp+0x3c)=1;
  {uint32_t saved=g_esp;DEC_VCALL(MEM32(g_edx+0x60),0x9c539,saved);}g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;DEC_VCALL(MEM32(g_edx+0x74),0x9c540,saved);}MEM8(g_esp+0x44)=LO8(g_eax);
  if((int32_t)count>0)do{
   SET_LO8(g_eax,MEM8(g_esp+0x44));
   if(!LO8(g_eax)){
    g_eax=MEM32(g_edi+0x84);g_ecx=MEM32(g_edi+0x8c);g_eax+=g_esi;if((int32_t)g_eax>=(int32_t)g_ecx)g_eax-=g_ecx;g_ecx=MEM32(g_edi+0x80);g_eax*=76;
    if(MEM8(g_ecx+g_eax+0x40)&2){g_ecx=MEM32(g_esp+0x28);g_eax=g_esi;g_edx=g_eax;g_edx>>=5;g_edx=g_ecx+g_edx*4;g_eax&=31;g_ecx=1;MEM32(g_esp+16)=g_ecx;g_ecx=g_eax;g_eax=MEM32(g_esp+16);g_eax<<=LO8(g_ecx);MEM32(g_edx)|=g_eax;}
   }
   g_ecx=MEM32(g_esp+0x28);g_eax=g_esi;g_edx=g_eax;g_edx>>=5;g_edx=g_ecx+g_edx*4;g_eax&=31;g_ecx=1;MEM32(g_esp+16)=g_ecx;g_ecx=g_eax;g_eax=MEM32(g_esp+16);g_eax<<=LO8(g_ecx);
   if(!(MEM32(g_edx)&g_eax)){
    g_eax=MEM32(g_edi+0x84);g_ecx=MEM32(g_edi+0x8c);g_eax+=g_esi;if((int32_t)g_eax>=(int32_t)g_ecx)g_eax-=g_ecx;g_ecx=MEM32(g_edi+0x80);g_eax*=76;g_eax+=g_ecx;
    MEM32(g_esp+0x14)=0x456af4;MEM32(g_esp+0x18)=g_esi;MEM32(g_esp+0x1c)=g_edi;g_ecx=MEM32(g_eax);g_eax=MEM32(g_ecx);
    {uint32_t saved=g_esp;PUSH32(g_esp,0);g_edx=g_esp+0x18;PUSH32(g_esp,g_edx);MEM8(g_esp+0x44)=2;DEC_VCALL(MEM32(g_eax+0x18),0x9c60c,saved);}
    MEM8(g_esp+0x3c)=1;MEM32(g_esp+0x14)=0x45688c;
   }
   ++g_esi;
  }while((int32_t)g_esi<(int32_t)count);
  g_eax=MEM32(g_esp+12);g_ecx=0;POP32(g_esp,g_edi);MEM32(g_eax+0x80)=g_ecx;g_eax=MEM32(g_esp+0x24);int empty=g_eax==g_ecx;POP32(g_esp,g_esi);MEM32(g_esp+0x18)=g_ecx;POP32(g_esp,count);
  if(!empty){PUSH32(g_esp,g_eax);DEC_CALL(sub_002D5CCD,0x9c643);g_esp+=4;}
 }
 g_ecx=MEM32(g_esp+0x28);MEM32(0)=g_ecx;g_esp+=0x34;g_recomp_zf=g_esp==0;g_esp+=8;
}

/* Batch matching records through the original immediate writers. Device,
 * exposure and material callbacks may change live guest state between marks. */
static void decal_draw_batch(void){
 uint32_t y=g_seh_ebp;g_esp-=0x34;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);MEM32(g_esp+16)=g_esi;DEC_VCALL(MEM32(g_eax+0xa4),0x9bf68,saved);}
 g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;DEC_VCALL(MEM32(g_edx+0x1c),0x9bf6f,saved);}
 g_eax=MEM32(g_esi+0x88);g_edx=MEM32(g_esi+0x84);
 if((int32_t)g_eax>=(int32_t)g_edx){g_eax-=g_edx;MEM32(g_esp+4)=g_eax;}else{g_ecx=MEM32(g_esi+0x8c);g_ecx-=g_edx;g_ecx+=g_eax;MEM32(g_esp+4)=g_ecx;}
 PUSH32(g_esp,8);DEC_CALL(sub_003EF700,0x9bf9c);
 {static const uint32_t attribute[3]={1,5,6},returns[3]={0x9bfae,0x9bfc0,0x9bfd2};for(unsigned i=0;i<3;i++){PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,attribute[i]);DEC_CALL(sub_003EF600,returns[i]);}}
 PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,4);DEC_CALL(sub_003EF5C0,0x9bfdd);
 g_eax=MEM32(g_esi+0x84);g_edx=MEM32(g_esp+0x3c);g_ecx=MEM32(g_esi+0x8c);g_eax+=g_edx;if((int32_t)g_eax>=(int32_t)g_ecx)g_eax-=g_ecx;g_ecx=MEM32(g_esi+0x80);g_eax*=76;g_eax=MEM32(g_eax+g_ecx);MEM32(g_esp+8)=g_eax;int draw=(int32_t)g_edx<(int32_t)MEM32(g_esp+4);MEM32(g_esp+0x3c)=g_edx;
 if(draw){
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,y);PUSH32(g_esp,g_edi);
  do{
   g_ebx=MEM32(g_esi+0x84);g_eax=MEM32(g_esi+0x8c);g_ebx+=g_edx;if((int32_t)g_ebx>=(int32_t)g_eax)g_ebx-=g_eax;g_ecx=MEM32(g_esi+0x80);g_ebx*=76;g_eax=MEM32(g_ebx+g_ecx);g_ebx+=g_ecx;g_ecx=MEM32(g_esp+0x14);
   if(g_eax==g_ecx){
    g_ecx=MEM32(g_esi+0x90);g_ecx=MEM32(g_ecx+8);g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm1=XMM_ZERO();g_eax=g_edx;g_edx=g_eax;g_edx>>=5;g_edx=g_ecx+g_edx*4;g_eax&=31;g_ecx=g_eax;SET_LO8(g_eax,MEM8(0x51a3dc));g_esi=1;g_esi<<=LO8(g_ecx);g_ecx=MEM32(g_edx);g_ecx|=g_esi;int initialized=LO8(g_eax)&1;MEM32(g_edx)=g_ecx;
    if(!initialized){MEM32(0x51a3dc)|=1;MEMF(0x51a3bc)=g_xmm1.f[0];MEMF(0x51a3c0)=g_xmm1.f[0];MEMF(0x51a3c4)=g_xmm0.f[0];MEMF(0x51a3c8)=g_xmm1.f[0];MEMF(0x51a3cc)=g_xmm0.f[0];MEMF(0x51a3d0)=g_xmm0.f[0];MEMF(0x51a3d4)=g_xmm1.f[0];MEMF(0x51a3d8)=g_xmm0.f[0];}
    g_xmm2=XMM_SCALAR(MEMF(g_ebx+0x44));unsigned flags=decal_compare(g_xmm2.f[0],g_xmm1.f[0]);
    if(!(flags&0x41)){g_xmm0.f[0]=g_xmm0.f[0]-MEMF(g_ebx+0x48);g_xmm0.f[0]=g_xmm0.f[0]*MEMF(0x44c1f8);MEMF(g_esp+0x18)=g_xmm0.f[0];}
    g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);{uint32_t saved=g_esp;DEC_VCALL(MEM32(g_eax+0x40),0x9c0f8,saved);}g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;DEC_VCALL(MEM32(g_edx+0x10),0x9c0ff,saved);}
    DEC_ST(0)=MEMF(0x44bf94)/DEC_ST(0);PUSH32(g_esp,0x3f800000);g_esp-=12;DEC_PUSH(DEC_ST(0));DEC_ST(0)*=MEMF(g_ebx+12);MEMF(g_esp+8)=(float)DEC_ST(0);DEC_POP();DEC_PUSH(DEC_ST(0));DEC_ST(0)*=MEMF(g_ebx+8);MEMF(g_esp+4)=(float)DEC_ST(0);DEC_POP();DEC_ST(0)*=MEMF(g_ebx+4);MEMF(g_esp)=(float)DEC_ST(0);DEC_POP();PUSH32(g_esp,2);DEC_CALL(sub_003EF600,0x9c12c);
    g_esi=0;g_edi=g_ebx+0x18;
    do{
     g_xmm3=XMM_SCALAR(MEMF(g_ebx+0x44));flags=decal_compare(g_xmm3.f[0],MEMF(0x44c174));g_xmm0=XMM_SCALAR(MEMF(g_edi-8));g_xmm1=XMM_SCALAR(MEMF(g_edi-4));g_xmm2=XMM_SCALAR(MEMF(g_edi));MEMF(g_esp+0x20)=g_xmm0.f[0];MEMF(g_esp+0x24)=g_xmm1.f[0];MEMF(g_esp+0x28)=g_xmm2.f[0];
     if(!(flags&0x41)){
      g_eax=(g_esi+2)&0x80000003u;if((int32_t)g_eax<0){--g_eax;g_eax|=0xfffffffcu;++g_eax;}g_xmm3=XMM_SCALAR(MEMF(g_esp+0x18));g_eax+=g_eax*2;
      g_xmm0.f[0]=g_xmm0.f[0]-MEMF(g_ebx+g_eax*4+16);g_xmm1.f[0]=g_xmm1.f[0]-MEMF(g_ebx+g_eax*4+20);g_xmm2.f[0]=g_xmm2.f[0]-MEMF(g_ebx+g_eax*4+24);g_eax=g_ebx+g_eax*4+16;
      MEMF(g_esp+0x2c)=g_xmm0.f[0];g_ecx=MEM32(g_esp+0x2c);MEMF(g_esp+0x30)=g_xmm1.f[0];g_edx=MEM32(g_esp+0x30);MEMF(g_esp+0x34)=g_xmm2.f[0];g_eax=MEM32(g_esp+0x34);MEM32(g_esp+0x20)=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_esp+0x20));g_xmm0.f[0]=g_xmm0.f[0]*g_xmm3.f[0];MEM32(g_esp+0x24)=g_edx;g_xmm1=XMM_SCALAR(MEMF(g_esp+0x24));MEM32(g_esp+0x28)=g_eax;g_xmm2=XMM_SCALAR(MEMF(g_esp+0x28));g_xmm1.f[0]=g_xmm1.f[0]*g_xmm3.f[0];g_xmm2.f[0]=g_xmm2.f[0]*g_xmm3.f[0];
      g_xmm3=XMM_SCALAR(MEMF(g_edi-8));g_xmm3.f[0]=g_xmm3.f[0]-g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_edi-4));g_xmm0.f[0]=g_xmm0.f[0]-g_xmm1.f[0];MEMF(g_esp+0x3c)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_edi));y=MEM32(g_esp+0x3c);g_xmm0.f[0]=g_xmm0.f[0]-g_xmm2.f[0];MEMF(g_esp+0x38)=g_xmm3.f[0];g_ecx=MEM32(g_esp+0x38);MEMF(g_esp+0x40)=g_xmm0.f[0];g_edx=MEM32(g_esp+0x40);MEM32(g_esp+0x20)=g_ecx;MEM32(g_esp+0x28)=g_edx;
     }else y=MEM32(g_esp+0x24);
     g_eax=MEM32(g_esi*8+0x51a3c0);g_ecx=MEM32(g_esi*8+0x51a3bc);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,3);DEC_CALL(sub_003EF5C0,0x9c236);
     g_edx=MEM32(g_esp+0x28);g_eax=MEM32(g_esp+0x20);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);PUSH32(g_esp,y);PUSH32(g_esp,g_eax);PUSH32(g_esp,0xffffffffu);DEC_CALL(sub_003EF600,0x9c24d);++g_esi;g_edi+=12;
    }while((int32_t)g_esi<4);
    g_esi=MEM32(g_esp+0x1c);g_edx=MEM32(g_esp+0x48);
   }
   g_eax=MEM32(g_esp+16);++g_edx;int more=(int32_t)g_edx<(int32_t)g_eax;MEM32(g_esp+0x48)=g_edx;if(!more)break;
  }while(1);
  POP32(g_esp,g_edi);POP32(g_esp,y);POP32(g_esp,g_ebx);
 }
 DEC_CALL(sub_003EF740,0x9c27b);POP32(g_esp,g_esi);g_esp+=0x34;g_recomp_zf=g_esp==0;g_esp+=8;
}

int uc_hybrid_decal_owner(uint32_t entry){
 if(!decal_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;unsigned size=0;
 switch(entry){
 case 0x9b8a0:case 0x9b920:case 0x9b930:case 0x9ba60:case 0x9bac0:case 0x9bb90:case 0x9bbb0:case 0x9be80:case 0x9bf50:size=0x94;break;
 case 0x9bb80:case 0x9ba00:case 0x9be60:case 0x9c4a0:if(p<16)return 0;p-=16;size=0x94;break;
 case 0x9c290:case 0x9c3b0:case 0x9c430:size=20;break;
 case 0x9c2c0:size=12;break;
 case 0x9b980:break;
 default:return 0;
 }
 if((s&3)||s<0x12000u||!decal_span(s-8192,8256)||(unsigned)g_fp_top>7||!decal_span(0x51c178,1)||MEM8(0x51c178)||!decal_apart(s-8192,8256,0x51c178,1))return 0;
 if(size&&(!decal_span(p,size)||!decal_apart(p,size,0x51c178,1)))return 0;
 /* The selection object and callback adapter are deliberately stack-resident.
  * Main owners exclude the prologue span; retained descendants keep the same
  * completing-service and live allocation contract as the original. */
 if(size==0x94&&!decal_apart(p,size,s-8192,8256))return 0;
 switch(entry){
 case 0x9b8a0:decal_destroy();break;
 case 0x9b920:g_eax=MEM32(g_ecx+0x88);MEM32(g_ecx+0x84)=g_eax;g_esp+=4;break;
 case 0x9b930:decal_reset();break;
 case 0x9b980:decal_bounds();break;
 case 0x9ba00:decal_collect();break;
 case 0x9ba60:decal_reserve();break;
 case 0x9bac0:decal_tick();break;
 case 0x9bb80:decal_delete(1);break;
 case 0x9bb90:decal_delete(0);break;
 case 0x9bbb0:decal_insert();break;
 case 0x9be60:decal_count();break;
 case 0x9be80:decal_construct();break;
 case 0x9bf50:decal_draw_batch();break;
 case 0x9c290:decal_selection_destroy();break;
 case 0x9c2c0:g_eax=MEM32(g_ecx+4);g_ecx=MEM32(g_ecx+8);PUSH32(g_esp,g_eax);DEC_CALL(sub_0009BF50,0x9c2cc);g_esp+=4;break;
 case 0x9c3b0:decal_selection_resize();break;
 case 0x9c430:decal_selection_construct();break;
 case 0x9c4a0:decal_render();break;
 }
 return 1;
}
#undef DEC_CALL
#undef DEC_VCALL
#undef DEC_PUSH
#undef DEC_POP
#undef DEC_ST
