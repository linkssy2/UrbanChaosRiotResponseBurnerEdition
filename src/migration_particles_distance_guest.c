#include "migration_particles_distance_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_DISTANCE");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_particles_distance(unsigned entry){
 if(entry!=0x23bc80||!enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10200||!span(s-512,528)||!span(p,36)||!span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
 if(!span(MEM32(s+4),64)||!span(MEM32(p),24)||!span(MEM32(p+28),4)||!span(MEM32(p+32),48)||!span(0x51e420,32))return 0;
 uint32_t source=MEM32(p+28);if(!span(MEM32(source),12))return 0;
 if((uint64_t)p<s+16&&(uint64_t)p+36>s-512)return 0;
 uint32_t frame=g_ebp,saved=s;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-132;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+28);g_ebx=MEM32(g_edi);
 g_ecx=g_esp+96;PUSH32(g_esp,g_ecx);g_ecx=g_esi;
 {uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0x23bca2);RECOMP_ICALL_SAFE(target,saved);}
 saved=g_esp;PUSH32(g_esp,g_eax);g_edx=g_esp+36;PUSH32(g_esp,g_edx);g_ecx=g_edi;
 {uint32_t target=MEM32(g_ebx+8);PUSH32(g_esp,0x23bcad);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(frame+8);g_xmm1=XMM_MEM(g_eax+48);g_xmm0=XMM_MEM(g_esp+80);g_xmm3=XMM_MEM(0x51e420);g_edx=MEM32(g_esi+32);
 g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=(double)SMEM32(g_edx+44);
 /* Original squared distance reduction and reciprocal-root refinement. Keep
  * its rounding sequence, including the retired vector scratch and x87 slot. */
 g_xmm0=XMM_SUB(g_xmm0,g_xmm1);g_xmm0=XMM_MUL(g_xmm0,g_xmm0);g_xmm1=g_xmm0;g_xmm1=XMM_MOVE_HIGH_TO_LOW(g_xmm1,g_xmm0);
 g_xmm0=XMM_ADD(g_xmm0,g_xmm1);g_xmm1=g_xmm0;g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm0,1);g_xmm0.f[0]+=g_xmm1.f[0];XMM_STORE(g_esp+16,g_xmm0);
 g_xmm1.f[0]=1.0f/sqrtf(g_xmm0.f[0]);MEMF(g_esp+16)=g_xmm1.f[0];g_xmm1=XMM_MEM(g_esp+16);
 g_xmm2=g_xmm1;g_xmm2.f[0]*=g_xmm0.f[0];g_xmm2.f[0]*=g_xmm1.f[0];g_xmm3.f[0]-=g_xmm2.f[0];g_xmm2=XMM_MEM(0x51e430);
 POP32(g_esp,g_edi);g_xmm2.f[0]*=g_xmm1.f[0];g_xmm2.f[0]*=g_xmm3.f[0];g_ecx=g_esp+8;g_xmm0.f[0]*=g_xmm2.f[0];POP32(g_esp,g_esi);
 MEMF(g_ecx)=g_xmm0.f[0];g_fp_stack[g_fp_top]=MEMF(g_esp+4)-g_fp_stack[g_fp_top];POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
