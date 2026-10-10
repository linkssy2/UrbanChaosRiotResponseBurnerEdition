/* Exact shape extent, swept radius and allowed-penetration calculations. */
#include "migration_physics_body_bounds.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
/* Preserve ADDSS destination/source NaN selection across host register allocation. */
static __declspec(noinline) float add_scalar(float left,float right){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(left),_mm_set_ss(right)));}
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static void push_fp(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void pop_fp(void){g_fp_top=(g_fp_top+1)&7;}
static void status_ax(void){unsigned status=((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);SET_LO16(g_eax,status);}
static void penetration(void){
 PUSH32(g_esp,g_ecx);push_fp(MEMF(g_ecx+4));push_fp(MEMF(g_ecx));MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);pop_fp();status_ax();if(!RECOMP_PARITY8(((g_eax>>8)&255)&5)){pop_fp();push_fp(MEMF(g_esp));}
 g_eax=MEM32(g_ecx+8);MEM32(g_esp)=g_eax;g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(g_esp));status_ax();if(RECOMP_PARITY8(((g_eax>>8)&255)&5)){pop_fp();push_fp(MEMF(g_esp));}
 g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x44c1f8));status_ax();unsigned flags=((g_eax>>8)&255)&5;if(!RECOMP_PARITY8(flags)){g_fp_stack[g_fp_top]*=MEMF(0x44c5fc);MEMF(g_edx+32)=(float)g_fp_stack[g_fp_top];pop_fp();}else{pop_fp();MEM32(g_edx+32)=0x3dcccccd;}POP32(g_esp,g_ecx);g_recomp_zf=flags==0;g_esp+=4;
}
static void radius(void){
 uint32_t saved=g_esp,frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-64;g_eax=MEM32(g_ecx);g_edx=g_esp+32;PUSH32(g_esp,g_edx);PUSH32(g_esp,0);PUSH32(g_esp,0x505660);uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x2f175a);RECOMP_ICALL_SAFE(target,saved);
 g_xmm2=XMM_MEM(g_esp+48);g_xmm1=XMM_MEM(g_esp+32);g_eax=MEM32(frame+12);g_edx=MEM32(frame+8);g_xmm0=g_xmm2;g_xmm0=XMM_SUB(g_xmm0,g_xmm1);XMM_STORE(g_eax,g_xmm0);g_eax=MEM32(g_edx+80);g_xmm0=XMM_MUL(g_xmm0,g_xmm0);g_xmm3=g_xmm0;g_xmm3=XMM_SHUFFLE(g_xmm3,g_xmm0,85);g_xmm4=g_xmm0;g_xmm3.f[0]=add_scalar(g_xmm3.f[0],g_xmm0.f[0]);g_xmm4=XMM_SHUFFLE(g_xmm4,g_xmm0,170);g_xmm0=g_xmm4;g_xmm0.f[0]=add_scalar(g_xmm0.f[0],g_xmm3.f[0]);g_xmm3=XMM_MEM(g_eax+144);XMM_STORE(g_esp+16,g_xmm0);g_xmm0.f[0]=sqrtf(g_xmm0.f[0]);MEMF(g_esp+16)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+16);g_xmm1=XMM_ADD(g_xmm1,g_xmm2);g_ecx=g_esp+12;MEMF(g_ecx)=g_xmm0.f[0];push_fp(MEMF(g_esp+12));g_fp_stack[g_fp_top]*=MEMF(0x44c1f8);MEM32(g_esp+8)=0x3f000000;g_xmm0=XMM_SCALAR(MEMF(g_esp+8));g_xmm2=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm0=g_xmm2;g_xmm0=XMM_MUL(g_xmm0,g_xmm1);g_xmm0=XMM_SUB(g_xmm0,g_xmm3);g_xmm0=XMM_MUL(g_xmm0,g_xmm0);g_xmm1=g_xmm0;g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm0,85);g_xmm2=g_xmm0;g_xmm1.f[0]=add_scalar(g_xmm0.f[0],g_xmm1.f[0]);g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,170);g_xmm0=g_xmm2;g_xmm0.f[0]=add_scalar(g_xmm0.f[0],g_xmm1.f[0]);XMM_STORE(g_esp+16,g_xmm0);g_xmm0.f[0]=sqrtf(g_xmm0.f[0]);MEMF(g_esp+16)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+16);g_ecx=g_esp+8;MEMF(g_ecx)=g_xmm0.f[0];g_fp_stack[g_fp_top]+=MEMF(g_esp+8);MEMF(g_eax+176)=(float)g_fp_stack[g_fp_top];pop_fp();g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_physics_body_bounds(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_BOUNDS");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-256,272)||!span(0x44c1f8,4)||!span(0x44c5fc,4))return 0;if(entry==0x2f1830){if(!span(g_ecx,12)||!span(g_edx,36))return 0;penetration();return 1;}if(entry!=0x2f1740||!span(g_ecx,4)||!span(MEM32(g_ecx),16))return 0;uint32_t body=MEM32(g_esp+4),out=MEM32(g_esp+8);if(!span(body,84)||!span(MEM32(body+80),180)||!span(out,16)||!((uint64_t)out+16<=g_esp-256||(uint64_t)g_esp+12<=out))return 0;radius();return 1;
}
