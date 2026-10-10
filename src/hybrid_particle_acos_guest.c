#include "hybrid_particle_acos_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int particle_acos_enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_ACOS");value=p&&!strcmp(p,"1");}return value;}
static int particle_acos_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int particle_acos_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define acos_push(v) do{double value_=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value_;}while(0)
#define acos_pop() (g_fp_top=(g_fp_top+1u)&7u)
#define acos_top() g_fp_stack[g_fp_top]
#define acos_st(i) g_fp_stack[(g_fp_top+(i))&7u]

int uc_hybrid_particle_acos(void){
 if(!particle_acos_enabled())return 0;
 uint32_t s=g_esp;uint64_t bits;
 if((s&3)||s<0x10018u||!particle_acos_span(s-24,28)||MEM32(s)!=0x239918||
    (unsigned)g_fp_top>7||!particle_acos_span(0x51c178,1)||MEM8(0x51c178)||
    !particle_acos_span(0x51e4d4,4)||!particle_acos_apart(s-24,28,0x51c178,1)||
    !particle_acos_apart(s-24,28,0x51e4d4,4)||(_mm_getcsr()&0x1f80u)!=0x1f80u)return 0;
 memcpy(&bits,&g_fp_stack[g_fp_top],sizeof bits);
 if((bits&UINT64_C(0x7fffffffffffffff))>UINT64_C(0x3ff0000000000000))return 0;

 /* Complete finite 2D681C service at the original particle birth call only.
  * Keep CONTROL's sqrt/atan2 sequence, virtual x87/control state and retired
  * frames. No host acos, exceptional-service substitution or late fallback.
  * Worker-disabled qualification is not a synchronization primitive. */
 g_esp-=12;MEMD(g_esp)=acos_top();PUSH32(g_esp,0x2d6827);
 g_eax=MEM32(g_esp+8)&0x7ff00000u;g_recomp_zf=0;g_esp+=4;
 PUSH32(g_esp,0x2d682c);PUSH32(g_esp,g_edx);MEM16(g_esp)=g_fp_control_word;
 if(MEM16(g_esp)!=0x27f){
  PUSH32(g_esp,0x2d684d);g_edx=MEM32(g_esp+4);g_edx=(g_edx&0x300u)|0x7fu;
  MEM16(g_esp+6)=LO16(g_edx);g_fp_control_word=MEM16(g_esp+6);g_recomp_zf=g_edx==0;g_esp+=4;
 }
 if(g_eax<0x3ff00000u){
  acos_push(1.0);acos_top()=acos_top()+acos_st(1);
  acos_push(1.0);acos_top()=acos_top()-acos_st(2);
  acos_st(1)=acos_st(1)*acos_top();acos_pop();acos_top()=sqrt(acos_top());
  {double t=acos_top();acos_top()=acos_st(1);acos_st(1)=t;}
  acos_st(1)=atan2(acos_st(1),acos_top());acos_pop();
 }else{
  /* Admission proves the two exact endpoints; retain their integer residue. */
  g_eax=MEM32(g_esp+12);g_ecx=g_eax;g_eax&=0xfffffu;g_eax|=MEM32(g_esp+8);
  g_ecx&=0x80000000u;acos_pop();
  if(g_ecx)acos_push(3.14159265358979323846);else acos_push(0.0);
 }
 if(MEM32(0x51e4d4)){
  if(MEM16(g_esp)!=0x27f)g_fp_control_word=MEM16(g_esp);
  g_recomp_zf=MEM16(g_esp)==0x27f;POP32(g_esp,g_edx);g_esp+=4;
 }else{
  g_edx=13;g_ecx=0x5048a0;SET_LO16(g_eax,MEM16(g_esp));
  if(LO16(g_eax)!=0x27f){
   SET_LO16(g_eax,LO16(g_eax)&0x20u);
   if(LO16(g_eax)){
    g_eax=(g_eax&0xffff0000u)|(uint32_t)(uint16_t)(((g_fp_top&7u)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700u):g_fp_cmp==2?0x4500u:g_fp_cmp<0?0x0100u:g_fp_cmp>0?0:0x4000u));
    SET_LO16(g_eax,LO16(g_eax)&0x20u);
   }
   /* The original status expression contains no bit 0x20, so the source's
    * precision-exception service cannot be entered in this CONTROL lift. */
   g_fp_control_word=MEM16(g_esp);
  }
  POP32(g_esp,g_edx);g_esp+=4;
 }
 g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
#undef acos_push
#undef acos_pop
#undef acos_top
#undef acos_st
