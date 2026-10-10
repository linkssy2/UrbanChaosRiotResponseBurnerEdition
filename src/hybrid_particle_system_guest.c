#include "hybrid_particle_system_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00239510(void);
extern void sub_002D6780(void);
extern void sub_00239330(void);
static int system_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_SYSTEM");flag=v&&!strcmp(v,"1");}return flag;}
/* Complete239110 on the title-owned system. All services and numeric inner
 * owners remain original/current; the retained callbacks may change live state. */
static int system_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int system_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static unsigned system_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
int uc_hybrid_particle_system(void){
 if(!system_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx,frame=g_ebp;
 if((s&3)||s<0x10400u||!system_span(s-1024,1032)||!system_span(p,0x94)||
    !system_apart(p,0x94,s-1024,1032)||(unsigned)g_fp_top>7||
    !system_span(0x51c178,1)||MEM8(0x51c178)||!system_apart(p,0x94,0x51c178,1)||
    !system_apart(s-1024,1032,0x51c178,1))return 0;
 uint32_t capacity=MEM32(p),count=MEM32(p+0x90),pool=MEM32(p+4);
 if(count>capacity||capacity>4096||(capacity&&(!system_span(pool,capacity*32)||
    !system_apart(pool,capacity*32,p,0x94)||!system_apart(pool,capacity*32,s-1024,1032)||
    !system_apart(pool,capacity*32,0x51c178,1))))return 0;
 /* No rejection after the first write. As with the source, completing callbacks
  * own the validity/lifetime of pointers and may replace any live state. */
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-0x64;
 g_xmm0=XMM_ZERO();unsigned flags=system_compare(g_xmm0.f[0],MEMF(frame+8));
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;
 if(!(flags&0x41u))MEMF(frame+8)=g_xmm0.f[0];
 g_ecx=MEM32(g_esi+12);g_eax=MEM32(g_ecx);
 {uint32_t saved=g_esp,target=MEM32(g_eax+12);PUSH32(g_esp,0x239134);RECOMP_ICALL_SAFE(target,saved);}
 g_ebx=MEM32(frame+8);
 if(!LO8(g_eax)){
  g_ecx=MEM32(g_esi+16);g_edx=MEM32(g_ecx);
  {uint32_t saved=g_esp,target=MEM32(g_edx+4);PUSH32(g_esp,0x239147);RECOMP_ICALL_SAFE(target,saved);}
  if(!LO8(g_eax)){
   g_ecx=MEM32(g_esi+16);g_eax=MEM32(g_ecx);g_edx=g_esi+0x60;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=g_esp+0x34;PUSH32(g_esp,g_edx);
    uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x239160);RECOMP_ICALL_SAFE(target,saved);}
   g_ecx=MEM32(g_esi+12);g_eax=MEM32(g_ecx);
   {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x239169);RECOMP_ICALL_SAFE(target,saved);}
   g_edi=g_eax-1u;
   if((int32_t)g_edi>=0)do{
    g_eax=MEM32(g_esi+0x90);if(g_eax==MEM32(g_esi))break;
    g_edx=MEM32(g_esi+4);g_ecx=MEM32(g_esi+20);g_eax=(g_eax<<5)+g_edx;g_edx=MEM32(g_ecx+0x88);
    PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);g_eax=g_esi+0x20;PUSH32(g_esp,g_eax);
    g_edx=g_esp+0x3c;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);g_ebp=frame;
    PUSH32(g_esp,0x23919c);sub_00239510();
    g_ecx=MEM32(g_esi+0x90);g_esp+=20;++g_ecx;--g_edi;MEM32(g_esi+0x90)=g_ecx;
   }while((int32_t)g_edi>=0);
   /* Preserve the four ordered load/store pairs, including their alias effects. */
   g_xmm0=XMM_MEM(g_esp+0x30);XMM_STORE(g_esi+0x20,g_xmm0);
   g_xmm0=XMM_MEM(g_esp+0x40);XMM_STORE(g_esi+0x30,g_xmm0);
   g_xmm0=XMM_MEM(g_esp+0x50);XMM_STORE(g_esi+0x40,g_xmm0);
   g_xmm0=XMM_MEM(g_esp+0x60);XMM_STORE(g_esi+0x50,g_xmm0);
  }
 }
 g_edi=MEM32(g_esi+20);g_xmm0=XMM_SCALAR(MEMF(frame+8));g_xmm0.f[0]/=MEMF(g_edi+0x90);
 g_xmm1=XMM_SCALAR(MEMF(0x455a5c));MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0.f[0]*=g_xmm1.f[0];
 flags=system_compare(g_xmm1.f[0],g_xmm0.f[0]);MEMF(g_esp+12)=g_xmm0.f[0];
 if(flags&0x41u)MEMF(g_esp+12)=g_xmm1.f[0];
 {double value=MEMF(g_esp+12);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value;}
 g_ebp=frame;PUSH32(g_esp,0x23920f);sub_002D6780();
 g_xmm0=XMM_SCALAR(MEMF(frame+8));flags=system_compare(g_xmm0.f[0],MEMF(0x44d024));
 if(!(flags&1u)&&!LO16(g_eax))g_eax=1;
 g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_ecx=MEM32(g_esi+24);g_xmm3=g_xmm1;g_xmm2=g_xmm0;
 g_xmm2.f[0]+=g_xmm1.f[0];g_xmm3.f[0]/=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_edi+0x34));
 g_xmm1.f[0]-=g_xmm3.f[0];g_xmm1.f[0]*=MEMF(g_edi+0x48);g_xmm3=XMM_SCALAR(MEMF(g_edi+0x30));
 MEMF(g_esp+16)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_edi+0x8c));g_xmm1.f[0]-=MEMF(g_edi+0x88);
 MEMF(g_esp+12)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_edi+0x38));g_edx=MEM32(g_esp+12);
 g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+0x2c)=g_xmm1.f[0];g_xmm2.f[0]*=g_xmm0.f[0];MEMF(g_esp+0x28)=g_xmm2.f[0];
 g_xmm3.f[0]*=g_xmm0.f[0];MEMF(g_esp+0x24)=g_xmm3.f[0];
 g_xmm5=XMM_SCALAR(MEMF(g_ecx));g_xmm1=XMM_SCALAR(MEMF(g_ecx+4));g_xmm6=XMM_SCALAR(MEMF(g_ecx+8));
 g_xmm4=XMM_SCALAR(MEMF(g_edi+0xa0));g_xmm2=XMM_SCALAR(MEMF(g_edi+0x3c));g_xmm7=XMM_SCALAR(MEMF(g_edi+0x40));g_xmm3=XMM_SCALAR(MEMF(g_edi+0x44));
 g_ecx=g_esi+0x60;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);
 g_edx=g_esp+0x24;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+0x24);g_ecx=g_esp+0x34;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);
 g_xmm1.f[0]*=g_xmm4.f[0];g_xmm5.f[0]*=g_xmm4.f[0];g_xmm6.f[0]*=g_xmm4.f[0];PUSH32(g_esp,g_edx);
 g_xmm2.f[0]+=g_xmm5.f[0];g_xmm1.f[0]+=g_xmm7.f[0];g_xmm3.f[0]+=g_xmm6.f[0];
 g_xmm2.f[0]*=g_xmm0.f[0];g_xmm1.f[0]*=g_xmm0.f[0];g_xmm3.f[0]*=g_xmm0.f[0];PUSH32(g_esp,g_ebx);g_ecx=g_esi;
 MEMF(g_esp+0x38)=g_xmm2.f[0];MEMF(g_esp+0x3c)=g_xmm1.f[0];MEMF(g_esp+0x40)=g_xmm3.f[0];
 g_ebp=frame;PUSH32(g_esp,0x239313);sub_00239330();
 POP32(g_esp,g_edi);MEM32(g_esi+0x90)=g_eax;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
 g_esp=frame;POP32(g_esp,frame);g_esp+=8;return 1;
}
