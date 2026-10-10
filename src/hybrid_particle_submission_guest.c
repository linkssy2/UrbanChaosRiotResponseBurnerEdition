#include "hybrid_particle_submission_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_000ACC50(void);
static int submission_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_SUBMISSION");flag=v&&!strcmp(v,"1");}return flag;}
static int submission_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int submission_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int submission_admit(void){
 uint32_t s=g_esp,o=g_ecx;
 return submission_enabled()&&!(s&3)&&s>=0x10400&&submission_span(s-1024,1040)&&submission_span(o,8)&&
  submission_apart(o,8,s-1024,1040)&&submission_span(0x51c178,1)&&!MEM8(0x51c178)&&
  submission_apart(s-1024,1040,0x51c178,1)&&submission_apart(o,8,0x51c178,1);
}
#define SUBMIT_CALL(target,ret,saved) do{uint32_t t_=(target);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(t_,saved);}while(0)
int uc_hybrid_particle_submission(void){
 if(!submission_admit()||(unsigned)g_fp_top>7)return 0;
 uint32_t frame=g_seh_ebp;g_esp-=0x3c;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edi);SUBMIT_CALL(MEM32(g_eax+0x50),0x99d02,saved);}
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);g_edi=g_eax;g_eax=g_esp+0x14;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);SUBMIT_CALL(MEM32(g_edx+0x14),0x99d11,saved);}
 g_edx=MEM32(g_edi);{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;SUBMIT_CALL(MEM32(g_edx+0x18),0x99d19,saved);}
 PUSH32(g_esp,g_eax);PUSH32(g_esp,0x99d1f);sub_000ACC50();g_esp+=8;
 if(g_eax){
  g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);
  {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);SUBMIT_CALL(MEM32(g_eax+0xd4),0x99d35,saved);}
  g_ebx=g_eax;
  if(g_ebx){
   g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);
   {uint32_t saved=g_esp;PUSH32(g_esp,frame);frame=MEM32(g_ebx);g_eax=g_esp+0x1c;PUSH32(g_esp,g_eax);SUBMIT_CALL(MEM32(g_edx+0x14),0x99d4b,saved);}
   {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_ebx;SUBMIT_CALL(MEM32(frame+0x20),0x99d51,saved);}
   unsigned accepted=LO8(g_eax)!=0;POP32(g_esp,frame);if(!accepted)goto reject_bounds;
  }
  g_ecx=MEM32(g_esi+4);g_ebx=MEM32(g_ecx);g_edx=MEM32(g_edi);g_ecx=g_edi;
  {uint32_t saved=g_esp;SUBMIT_CALL(MEM32(g_edx+8),0x99d62,saved);}
  g_ecx=MEM32(g_esi+4);{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);SUBMIT_CALL(MEM32(g_ebx+4),0x99d69,saved);}
  g_fp_stack[g_fp_top]+=MEMF(0x4f8770);g_ecx=MEM32(g_esp+0x4c);g_eax=MEM32(g_ecx);
  {uint32_t saved=g_esp;PUSH32(g_esp,0);g_fp_stack[g_fp_top]=-g_fp_stack[g_fp_top];g_edx=g_esp+0x10;
   MEMF(g_esp+0x18)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);
   MEM32(g_esp+0x18)=5;MEM32(g_esp+0x1c)=0x110;SUBMIT_CALL(MEM32(g_eax+4),0x99d96,saved);}
 reject_bounds:POP32(g_esp,g_ebx);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=0x3c;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
int uc_hybrid_particle_bounds_copy(unsigned entry){
 uint32_t output,ret;
 switch(entry){case 0x99da0:output=0x51a284;ret=0x99db2;break;case 0x9a840:output=0x51a2b4;ret=0x9a852;break;
  case 0x9a870:output=0x51a2e4;ret=0x9a882;break;case 0x9b0f0:output=0x51a314;ret=0x9b102;break;
  case 0x9b120:output=0x51a348;ret=0x9b132;break;default:return 0;}
 if(!submission_admit()||!submission_span(output,48)||!submission_apart(output,48,g_esp-1024,1040))return 0;
 g_ecx=MEM32(g_ecx+4);g_eax=MEM32(g_ecx);g_esp-=48;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edx=g_esp+8;PUSH32(g_esp,g_edx);SUBMIT_CALL(MEM32(g_eax+0x14),ret,saved);}
 g_esi=g_eax;g_ecx=12;g_edi=output;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
 POP32(g_esp,g_edi);g_eax=output;POP32(g_esp,g_esi);g_esp+=48;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
#undef SUBMIT_CALL
