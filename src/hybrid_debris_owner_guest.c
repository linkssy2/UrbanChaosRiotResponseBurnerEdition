#include "hybrid_debris_owner_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
/* Legacy SSE selects the first NaN payload; these two operand orders are
 * byte-pinned at the original481B0 instructions. */
__declspec(noinline) float uc_hybrid_debris_ordered_add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
__declspec(noinline) float uc_hybrid_debris_ordered_mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
extern void sub_000481B0(void),sub_000484F0(void),sub_000489A0(void),sub_00048A20(void);
extern void sub_002D57FB(void),sub_002D589F(void),sub_002D5CCD(void),sub_002D631D(void);
extern void sub_002D3780(void),sub_00214230(void),sub_0022B210(void),sub_002E0E00(void),sub_002A7B50(void);
static int debris_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_DEBRIS_OWNER");flag=v&&!strcmp(v,"1");}return flag;}
static int debris_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int debris_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define DEBRIS_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define DEBRIS_COMPARE(a,b) do{double left=(a),right=(b);flags=(left!=left||right!=right)?0x47u:left<right?3u:left>right?2u:0x42u;}while(0)

/* Whole source-owned algorithms. Live guest fields are deliberately re-read
 * around callbacks and aliased stores. This is semantic migration, not a new
 * physics world, a visual replacement, or a measured optimization. */
static void debris_manager_update(void){
 uint32_t index=g_ebp;
 PUSH32(g_esp,index);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);index=0;
 if(g_eax){
  PUSH32(g_esp,g_ebx);g_ebx=0xfffffff8u;PUSH32(g_esp,g_edi);g_edi=g_esi+8;g_ebx-=g_esi;
  do{
   g_eax=MEM32(g_esp+0x14);g_ecx=MEM32(g_edi);PUSH32(g_esp,g_eax);DEBRIS_CALL(sub_000481B0,0x4816c);
   g_ecx=MEM32(g_edi);g_edx=MEM32(g_ecx);
   {uint32_t saved=g_esp,target=MEM32(g_edx+0x1c);PUSH32(g_esp,0x48173);RECOMP_ICALL_SAFE(target,saved);}
   if(LO8(g_eax)){
    g_ecx=MEM32(g_edi);
    if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x48183);RECOMP_ICALL_SAFE(target,saved);}
    g_eax=MEM32(g_esi+4);--g_eax;MEM32(g_esi+4)=g_eax;g_edx=MEM32(g_esi+g_eax*4+8);g_ecx=g_ebx+g_edi;MEM32(g_ecx+g_esi+8)=g_edx;
   }else{++index;g_edi+=4;}
  }while(index<MEM32(g_esi+4));
  POP32(g_esp,g_edi);POP32(g_esp,g_ebx);
 }
 POP32(g_esp,g_esi);POP32(g_esp,index);g_esp+=8;
}

static void debris_update(void){
 uint32_t offset=g_seh_ebp,flags;
 g_esp-=0x4c;g_xmm0=XMM_SCALAR(MEMF(g_ecx+0xc));g_xmm3=XMM_SCALAR(MEMF(g_esp+0x50));g_xmm0.f[0]=g_xmm0.f[0]-g_xmm3.f[0];
 DEBRIS_COMPARE(g_xmm0.f[0],MEMF(0x44c174));MEM32(g_esp)=g_ecx;MEMF(g_ecx+0xc)=g_xmm0.f[0];
 if(!(flags&0x41)){
  g_eax=MEM32(g_ecx+0x14);PUSH32(g_esp,g_ebx);g_xmm2=g_xmm3;g_xmm2.f[0] = uc_hybrid_debris_ordered_mul(g_xmm2.f[0], MEMF(0x450298));g_ebx=0;MEMF(g_esp+0x48)=g_xmm2.f[0];
  if(g_eax){
   PUSH32(g_esp,offset);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);offset=0;
   do{
    g_esi=MEM32(g_ecx+0x10);SET_LO8(g_eax,MEM8(g_esi+offset+0x1c));g_esi+=offset;
    if(!LO8(g_eax)){
     g_xmm1=XMM_SCALAR(MEMF(g_esi+0x10));g_xmm0=XMM_SCALAR(MEMF(g_esi+0xc));g_xmm4=XMM_SCALAR(MEMF(g_esi));PUSH32(g_esp,0);
     g_xmm1.f[0] = uc_hybrid_debris_ordered_add(g_xmm1.f[0], g_xmm2.f[0]);g_xmm2=XMM_SCALAR(MEMF(g_esi+0x14));g_edi=g_esi+0xc;PUSH32(g_esp,0xffff);PUSH32(g_esp,0xffff);MEMF(g_esp+0x50)=g_xmm4.f[0];g_xmm5=XMM_SCALAR(MEMF(g_esi+4));PUSH32(g_esp,1);g_eax=g_esp+0x30;
     MEMF(g_esp+0x24)=g_xmm0.f[0];MEMF(g_esp+0x28)=g_xmm1.f[0];MEMF(g_esp+0x2c)=g_xmm2.f[0];MEMF(g_esp+0x58)=g_xmm5.f[0];g_xmm6=XMM_SCALAR(MEMF(g_esi+8));PUSH32(g_esp,g_eax);g_ecx=g_esp+0x58;
     g_xmm0.f[0] = uc_hybrid_debris_ordered_mul(g_xmm0.f[0], g_xmm3.f[0]);g_xmm1.f[0] = uc_hybrid_debris_ordered_mul(g_xmm1.f[0], g_xmm3.f[0]);g_xmm2.f[0] = uc_hybrid_debris_ordered_mul(g_xmm2.f[0], g_xmm3.f[0]);g_xmm0.f[0] = uc_hybrid_debris_ordered_add(g_xmm0.f[0], g_xmm4.f[0]);g_xmm1.f[0] = uc_hybrid_debris_ordered_add(g_xmm1.f[0], g_xmm5.f[0]);g_xmm2.f[0] = uc_hybrid_debris_ordered_add(g_xmm2.f[0], g_xmm6.f[0]);PUSH32(g_esp,g_ecx);
     MEMF(g_esp+0x64)=g_xmm6.f[0];MEMF(g_esp+0x38)=g_xmm0.f[0];MEMF(g_esp+0x3c)=g_xmm1.f[0];MEMF(g_esp+0x40)=g_xmm2.f[0];DEBRIS_CALL(sub_00214230,0x482a0);g_esp+=0x18;
     if(g_eax){
      g_xmm0=XMM_SCALAR(MEMF(g_esp+0x1c));g_xmm7=XMM_SCALAR(MEMF(g_esp+0x18));g_xmm6=XMM_SCALAR(MEMF(g_esp+0x14));g_xmm1=g_xmm0;g_xmm1.f[0] = uc_hybrid_debris_ordered_mul(g_xmm1.f[0], g_xmm0.f[0]);g_xmm0=g_xmm7;g_xmm0.f[0] = uc_hybrid_debris_ordered_mul(g_xmm0.f[0], g_xmm7.f[0]);g_xmm1.f[0] = uc_hybrid_debris_ordered_add(g_xmm1.f[0], g_xmm0.f[0]);g_xmm0=g_xmm6;g_xmm0.f[0] = uc_hybrid_debris_ordered_mul(g_xmm0.f[0], g_xmm6.f[0]);g_xmm1.f[0] = uc_hybrid_debris_ordered_add(g_xmm1.f[0], g_xmm0.f[0]);g_xmm0=XMM_SCALAR(MEMF(0x44db94));DEBRIS_COMPARE(g_xmm0.f[0],g_xmm1.f[0]);
      if(!(flags&0x41)){
       MEM8(g_esi+0x1c)=1;g_edx=MEM32(g_eax);MEM32(g_esp+0x20)=g_edx;g_ecx=MEM32(g_eax+4);MEM32(g_esp+0x24)=g_ecx;g_edx=MEM32(g_eax+8);MEM32(g_esp+0x28)=g_edx;
      }else{
       g_xmm0=XMM_ZERO();g_xmm4=XMM_SCALAR(MEMF(g_eax+0x10));g_xmm5=XMM_SCALAR(MEMF(g_eax+0x14));g_xmm3=XMM_SCALAR(MEMF(g_eax+0xc));g_xmm2=g_xmm0;g_xmm1=g_xmm0;
       g_xmm2.f[0]=g_xmm2.f[0]-g_xmm7.f[0];g_xmm7=XMM_SCALAR(MEMF(g_esp+0x1c));g_xmm0.f[0]=g_xmm0.f[0]-g_xmm7.f[0];g_xmm0.f[0] = uc_hybrid_debris_ordered_mul(g_xmm0.f[0], g_xmm5.f[0]);g_xmm2.f[0] = uc_hybrid_debris_ordered_mul(g_xmm2.f[0], g_xmm4.f[0]);g_xmm0.f[0] = uc_hybrid_debris_ordered_add(g_xmm0.f[0], g_xmm2.f[0]);g_xmm1.f[0]=g_xmm1.f[0]-g_xmm6.f[0];g_xmm1.f[0] = uc_hybrid_debris_ordered_mul(g_xmm1.f[0], g_xmm3.f[0]);g_xmm0.f[0] = uc_hybrid_debris_ordered_add(g_xmm0.f[0], g_xmm1.f[0]);g_xmm0.f[0] = uc_hybrid_debris_ordered_mul(g_xmm0.f[0], MEMF(0x44C530));
       g_xmm3.f[0] = uc_hybrid_debris_ordered_mul(g_xmm3.f[0], g_xmm0.f[0]);g_xmm4.f[0] = uc_hybrid_debris_ordered_mul(g_xmm4.f[0], g_xmm0.f[0]);g_xmm4.f[0] = uc_hybrid_debris_ordered_add(g_xmm4.f[0], MEMF(g_esp + 0x18));g_xmm5.f[0] = uc_hybrid_debris_ordered_mul(g_xmm5.f[0], g_xmm0.f[0]);g_xmm3.f[0] = uc_hybrid_debris_ordered_add(g_xmm3.f[0], g_xmm6.f[0]);MEMF(g_esp+0x2c)=g_xmm3.f[0];g_ecx=MEM32(g_esp+0x2c);g_xmm3=XMM_SCALAR(MEMF(0x44c538));MEM32(g_esp+0x14)=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_esp+0x14));g_xmm0.f[0] = uc_hybrid_debris_ordered_mul(g_xmm0.f[0], g_xmm3.f[0]);g_xmm5.f[0] = uc_hybrid_debris_ordered_add(g_xmm5.f[0], g_xmm7.f[0]);
       MEMF(g_esp+0x30)=g_xmm4.f[0];g_edx=MEM32(g_esp+0x30);MEMF(g_esp+0x34)=g_xmm5.f[0];g_ecx=MEM32(g_esp+0x34);MEM32(g_esp+0x18)=g_edx;g_xmm1=XMM_SCALAR(MEMF(g_esp+0x18));MEM32(g_esp+0x1c)=g_ecx;g_xmm2=XMM_SCALAR(MEMF(g_esp+0x1c));g_xmm1.f[0] = uc_hybrid_debris_ordered_mul(g_xmm1.f[0], g_xmm3.f[0]);g_xmm2.f[0] = uc_hybrid_debris_ordered_mul(g_xmm2.f[0], g_xmm3.f[0]);g_xmm3=XMM_SCALAR(MEMF(g_esp+0x60));
       MEMF(g_esp+0x14)=g_xmm0.f[0];g_xmm0.f[0] = uc_hybrid_debris_ordered_mul(g_xmm0.f[0], g_xmm3.f[0]);MEMF(g_esp+0x18)=g_xmm1.f[0];g_xmm1.f[0] = uc_hybrid_debris_ordered_mul(g_xmm1.f[0], g_xmm3.f[0]);MEMF(g_esp+0x1c)=g_xmm2.f[0];g_xmm2.f[0] = uc_hybrid_debris_ordered_mul(g_xmm2.f[0], g_xmm3.f[0]);g_xmm3=XMM_SCALAR(MEMF(g_eax));g_xmm3.f[0] = uc_hybrid_debris_ordered_add(g_xmm3.f[0], g_xmm0.f[0]);g_xmm0=XMM_SCALAR(MEMF(g_eax+4));g_xmm0.f[0] = uc_hybrid_debris_ordered_add(g_xmm0.f[0], g_xmm1.f[0]);MEMF(g_esp+0x3c)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+8));g_eax=MEM32(g_esp+0x3c);g_xmm0.f[0] = uc_hybrid_debris_ordered_add(g_xmm0.f[0], g_xmm2.f[0]);
       MEMF(g_esp+0x38)=g_xmm3.f[0];g_edx=MEM32(g_esp+0x38);MEMF(g_esp+0x40)=g_xmm0.f[0];g_ecx=MEM32(g_esp+0x40);MEM32(g_esp+0x20)=g_edx;MEM32(g_esp+0x24)=g_eax;MEM32(g_esp+0x28)=g_ecx;
      }
     }
     g_edx=MEM32(g_esp+0x20);g_xmm2=XMM_SCALAR(MEMF(g_esp+0x54));g_xmm3=XMM_SCALAR(MEMF(g_esp+0x60));MEM32(g_esi)=g_edx;g_eax=MEM32(g_esp+0x24);g_edx=MEM32(g_esp+0x14);MEM32(g_esi+4)=g_eax;g_ecx=MEM32(g_esp+0x28);g_eax=MEM32(g_esp+0x18);MEM32(g_esi+8)=g_ecx;g_ecx=MEM32(g_esp+0x1c);MEM32(g_edi)=g_edx;MEM32(g_edi+4)=g_eax;MEM32(g_edi+8)=g_ecx;g_ecx=MEM32(g_esp+0x10);
    }
    g_eax=MEM32(g_ecx+0x14);++g_ebx;offset+=0x20;
   }while(g_ebx<g_eax);
   POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,offset);
  }
  POP32(g_esp,g_ebx);
 }
 g_esp+=0x4c;g_recomp_zf=g_esp==0;g_esp+=8;
}

static void debris_manager_add(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38a7bb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;
 if(MEM32(g_esi+4)!=8){
  PUSH32(g_esp,0x1c);DEBRIS_CALL(sub_002D57FB,0x48496);g_esp+=4;MEM32(g_esp+4)=g_eax;MEM32(g_esp+0x10)=0;
  if(g_eax){g_ecx=MEM32(g_esp+0x2c);g_edx=MEM32(g_esp+0x28);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+0x28);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+0x28);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+0x28);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+0x28);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;DEBRIS_CALL(sub_000484F0,0x484ce);}else g_eax=0;
  g_ecx=MEM32(g_esi+4);MEM32(g_esi+g_ecx*4+8)=g_eax;++MEM32(g_esi+4);
 }
 g_ecx=MEM32(g_esp+8);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=0x10;g_recomp_zf=g_esp==0;g_esp+=28;
}

static void debris_construct(void){
 uint32_t owner=g_seh_ebp;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38a7d8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x1c;PUSH32(g_esp,g_ebx);PUSH32(g_esp,owner);PUSH32(g_esp,g_esi);owner=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+0x1c)=owner;
 g_esi=MEM32(g_esp+0x3c);g_xmm0=XMM_SCALAR(MEMF(g_esp+0x48));g_eax=g_esi;g_eax<<=5;g_ebx=0;PUSH32(g_esp,g_eax);MEM32(g_esp+0x38)=g_ebx;MEM32(owner)=0x4502a0;MEMF(owner+8)=g_xmm0.f[0];MEMF(owner+0xc)=g_xmm0.f[0];MEM32(owner+0x14)=g_esi;DEBRIS_CALL(sub_002D589F,0x48541);g_esp+=4;MEM32(owner+0x10)=g_eax;
 if(g_esi>g_ebx){
  g_xmm0=XMM_SCALAR(MEMF(g_esp+0x4c));g_edi=MEM32(g_esp+0x4c);g_xmm1=g_xmm0;g_xmm1.f[0]=g_xmm1.f[0]*MEMF(0x45029c);MEMF(g_esp+0x10)=g_xmm1.f[0];g_xmm1=XMM_ZERO();g_xmm1.f[0]=g_xmm1.f[0]-g_xmm0.f[0];MEMF(g_esp+0x48)=g_xmm1.f[0];MEM32(g_esp+0x3c)=g_esi;
  do{
   g_esi=MEM32(owner+0x10);g_ecx=MEM32(g_esp+0x40);g_eax=MEM32(g_ecx);g_esi+=g_ebx;g_edx=g_esi;MEM32(g_edx)=g_eax;g_eax=MEM32(g_ecx+4);MEM32(g_edx+4)=g_eax;g_ecx=MEM32(g_ecx+8);MEM32(g_edx+8)=g_ecx;g_edx=MEM32(g_esp+0x48);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);DEBRIS_CALL(sub_002D3780,0x485a6);
   MEMF(g_esp+0x1c)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;g_eax=MEM32(g_esp+0x18);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);DEBRIS_CALL(sub_002D3780,0x485b5);
   MEMF(g_esp+0x5c)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;g_ecx=MEM32(g_esp+0x58);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);DEBRIS_CALL(sub_002D3780,0x485c4);
   MEMF(g_esp+0x38)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;g_edx=MEM32(g_esp+0x64);g_eax=MEM32(g_esp+0x2c);g_xmm0=XMM_SCALAR(MEMF(g_esp+0x5c));MEM32(g_esp+0x3c)=g_edx;g_edx=MEM32(g_esp+0x38);MEM32(g_esp+0x40)=g_eax;g_eax=MEM32(g_esp+0x3c);g_ecx=g_esi+0xc;MEM32(g_ecx)=g_edx;g_edx=MEM32(g_esp+0x40);MEM32(g_ecx+4)=g_eax;g_eax=MEM32(g_esp+0x54);g_esp+=0x18;g_ebx+=0x20;--g_eax;MEM32(g_ecx+8)=g_edx;MEMF(g_esi+0x18)=g_xmm0.f[0];MEM8(g_esi+0x1c)=0;MEM32(g_esp+0x3c)=g_eax;
  }while(g_eax);
 }
 g_eax=MEM32(g_esp+0x50);g_ecx=MEM32(0x5150b0);g_esi=MEM32(g_eax);g_edi=MEM32(g_ecx);DEBRIS_CALL(sub_0022B210,0x48626);g_ecx=g_eax;DEBRIS_CALL(sub_002E0E00,0x4862d);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ecx=g_esp+0x1c;DEBRIS_CALL(sub_002A7B50,0x48638);g_ecx=MEM32(0x5150b0);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edi+0x98);PUSH32(g_esp,0x48645);RECOMP_ICALL_SAFE(target,saved);}
 MEM32(owner+0x18)=g_eax;g_ecx=MEM32(0x5150b0);g_edx=MEM32(g_ecx);
 {uint32_t saved=g_esp;PUSH32(g_esp,owner);uint32_t target=MEM32(g_edx+0x70);PUSH32(g_esp,0x48654);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=MEM32(g_esp+0x2c);POP32(g_esp,g_edi);MEM32(owner+4)=g_eax;POP32(g_esp,g_esi);g_eax=owner;POP32(g_esp,owner);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=0x28;g_recomp_zf=g_esp==0;g_esp+=28;
}

static void debris_fill(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_esp-=0x34;g_xmm0=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+0xc);PUSH32(g_esp,g_esi);g_eax=0;PUSH32(g_esp,g_edi);
 if((int32_t)g_ebx>=4){
  g_eax=MEM32(frame+0x14);g_esi=MEM32(frame+8);g_edx=MEM32(frame+0x18);g_ebx-=4;g_edi=g_eax+0x30;g_eax-=MEM32(frame+0x18);g_ebx>>=2;g_esi<<=5;g_edx+=0x10;++g_ebx;MEM32(g_esp+0x18)=g_eax;g_eax=g_ebx*4;MEMF(g_esp+0x30)=g_xmm0.f[0];MEMF(g_esp+0x34)=g_xmm0.f[0];MEMF(g_esp+0x38)=g_xmm0.f[0];MEM32(g_esp+0x1c)=g_eax;
  do{
   /* The four phases differ in EAX publication and aliased scratch reads. */
   for(unsigned phase=0;phase<4;++phase){
    g_eax=MEM32(g_ecx+0x10);g_xmm1=XMM_SCALAR(MEMF(g_eax+g_esi+phase*0x20));if(phase==3)g_eax+=g_esi+0x60;MEMF(g_esp+0x20)=g_xmm1.f[0];
    g_xmm1=XMM_SCALAR(MEMF(g_eax+(phase==3?0:g_esi+phase*0x20)+4));MEMF(g_esp+0x24)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_eax+(phase==3?0:g_esi+phase*0x20)+8));if(phase==2)g_eax+=g_esi+0x40;MEMF(g_esp+0x28)=g_xmm1.f[0];
    g_xmm1=XMM_SCALAR(MEMF(g_eax+(phase<2?g_esi+phase*0x20:0)+0x18));MEMF(g_esp+0x2c)=g_xmm1.f[0];g_xmm1=XMM_MEM(g_esp+0x20);
    if(phase==1){g_eax+=g_esi+0x20;g_eax=MEM32(g_esp+0x18);XMM_STORE(g_eax+g_edx,g_xmm1);}else XMM_STORE(g_edi+(phase==0?-48:phase==2?-16:0),g_xmm1);
    g_xmm1=XMM_SCALAR(MEMF(g_ecx+0xc));g_xmm1.f[0]=g_xmm1.f[0]/MEMF(g_ecx+8);if(!phase)g_eax+=g_esi;MEMF(g_esp+0x3c)=g_xmm1.f[0];g_xmm1=XMM_MEM(g_esp+0x30);XMM_STORE(g_edx+phase*0x10-0x10,g_xmm1);
   }
   g_esi+=0x80;g_edx+=0x40;g_edi+=0x40;--g_ebx;
  }while(g_ebx);
  g_ebx=MEM32(frame+0xc);g_eax=MEM32(g_esp+0x1c);
 }
 if((int32_t)g_eax<(int32_t)g_ebx){
  g_edi=MEM32(frame+0x18);g_edx=MEM32(frame+8);g_esi=g_eax;g_esi<<=4;g_esi+=g_edi;g_edi=g_eax+g_edx;g_edx=MEM32(frame+0x14);g_edx-=MEM32(frame+0x18);g_edi<<=5;MEMF(g_esp+0x30)=g_xmm0.f[0];MEMF(g_esp+0x34)=g_xmm0.f[0];MEMF(g_esp+0x38)=g_xmm0.f[0];g_ebx-=g_eax;
  do{
   g_eax=MEM32(g_ecx+0x10);g_xmm0=XMM_SCALAR(MEMF(g_eax+g_edi));g_eax+=g_edi;MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+4));MEMF(g_esp+0x24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+8));MEMF(g_esp+0x28)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+0x18));MEMF(g_esp+0x2c)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+0x20);XMM_STORE(g_esi+g_edx,g_xmm0);g_xmm0=XMM_SCALAR(MEMF(g_ecx+0xc));g_xmm0.f[0]=g_xmm0.f[0]/MEMF(g_ecx+8);MEMF(g_esp+0x3c)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+0x30);XMM_STORE(g_esi,g_xmm0);g_edi+=0x20;g_esi+=0x10;--g_ebx;
  }while(g_ebx);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=24;
}

static void debris_destroy(void){
 uint32_t saved=g_esp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38a7f8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x4502a0;g_ecx=MEM32(0x5150b0);g_edx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edx);MEM32(g_esp+0x14)=0;
 {uint32_t target=MEM32(g_eax+0x74);PUSH32(g_esp,0x489da);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_esi+0x10);PUSH32(g_esp,g_eax);DEBRIS_CALL(sub_002D631D,0x489e3);g_ecx=MEM32(g_esp+0xc);g_esp+=4;MEM32(g_esi)=0x4502c0;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=0x10;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void debris_manager_clear(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38a818);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+8)=g_esi;MEM32(g_esi)=0x45028c;g_eax=MEM32(g_esi+4);g_ebx=0;MEM32(g_esp+0x14)=0;
 if(g_eax){
  PUSH32(g_esp,g_edi);g_edi=g_esi+8;
  do{g_ecx=MEM32(g_edi);if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x48a6c);RECOMP_ICALL_SAFE(target,saved);}g_eax=MEM32(g_esi+4);++g_ebx;g_edi+=4;}while(g_ebx<g_eax);
  POP32(g_esp,g_edi);
 }
 g_ecx=MEM32(g_esp+0xc);MEM32(g_esi)=0x450280;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=0x10;g_recomp_zf=g_esp==0;g_esp+=4;
}

int uc_hybrid_debris_owner(uint32_t entry){
 if(!debris_enabled())return 0;
 unsigned size;
 switch(entry){case 0x48110:case 0x48120:case 0x48140:case 0x48470:case 0x48a00:case 0x48a20:size=0x28;break;case 0x481b0:case 0x484f0:case 0x48670:case 0x48680:case 0x486a0:case 0x486c0:case 0x48700:case 0x48980:case 0x489a0:size=0x1c;break;case 0x48a90:size=0;break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x12000u||!debris_span(s-8192,8224)||(unsigned)g_fp_top>7||!debris_span(0x51c178,1)||MEM8(0x51c178)||!debris_apart(s-8192,8224,0x51c178,1))return 0;
 if(size&&(!debris_span(p,size)||!debris_apart(p,size,s-8192,8224)||!debris_apart(p,size,0x51c178,1)))return 0;
 switch(entry){
 case 0x48110:MEM32(g_ecx)=0x450280;g_esp+=4;break;
 case 0x48670:MEM32(g_ecx)=0x4502c0;g_esp+=4;break;
 case 0x48120:case 0x48680:{int deleting=MEM8(g_esp+4)&1;PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi)=entry==0x48120?0x450280:0x4502c0;if(deleting){PUSH32(g_esp,g_esi);DEBRIS_CALL(sub_002D5CCD,entry==0x48120?0x48136:0x48696);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;break;}
 case 0x48140:debris_manager_update();break;
 case 0x481b0:debris_update();break;
 case 0x48470:debris_manager_add();break;
 case 0x484f0:debris_construct();break;
 case 0x486a0:{uint32_t flags;g_xmm0=XMM_ZERO();DEBRIS_COMPARE(g_xmm0.f[0],MEMF(g_ecx+0xc));if(flags&1){g_eax=0;g_recomp_zf=1;}else{g_eax=1;g_recomp_zf=!!(flags&0x40);}g_esp+=4;break;}
 case 0x486c0:{uint32_t flags;g_xmm0=XMM_SCALAR(MEMF(g_ecx+0xc));DEBRIS_COMPARE(g_xmm0.f[0],MEMF(0x44c174));if(!(flags&0x41)){g_eax=MEM32(g_ecx+0x14);g_edx=MEM32(g_esp+4);MEM32(g_edx)=g_eax;g_eax=MEM32(g_esp+8);g_edx=MEM32(g_esp+0xc);MEM32(g_eax)=2;g_ecx=MEM32(g_ecx+0x18);MEM32(g_edx)=g_ecx;}else{g_eax=MEM32(g_esp+4);MEM32(g_eax)=0;}g_recomp_zf=!!(flags&0x40);g_esp+=16;break;}
 case 0x48700:debris_fill();break;
 case 0x489a0:debris_destroy();break;
 case 0x48a20:debris_manager_clear();break;
 case 0x48980:case 0x48a00:PUSH32(g_esp,g_esi);g_esi=g_ecx;if(entry==0x48980){DEBRIS_CALL(sub_000489A0,0x48988);}else{DEBRIS_CALL(sub_00048A20,0x48a08);}if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);DEBRIS_CALL(sub_002D5CCD,entry==0x48980?0x48995:0x48a15);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;break;
 case 0x48a90:PUSH32(g_esp,0x28);DEBRIS_CALL(sub_002D57FB,0x48a97);g_esp+=4;{int empty=g_eax==0;if(!empty){MEM32(g_eax)=0x45028c;MEM32(g_eax+4)=0;}else g_eax=0;g_recomp_zf=empty;}g_esp+=4;break;
 }
 return 1;
}
#undef DEBRIS_CALL
#undef DEBRIS_COMPARE
