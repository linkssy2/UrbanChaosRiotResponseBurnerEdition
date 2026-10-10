#include "hybrid_animation_lifecycle_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F19F0(void);
extern void sub_002D5CCD(void);
extern void sub_002D631D(void);
extern void sub_002D69BB(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_ANIMATION_LIFECYCLE");flag=v&&!strcmp(v,"1");}return flag;}
static int lifecycle_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Original synchronous ownership and callback lifetime remain authoritative.
 * Worker-zero qualifies title configuration, not synchronization. No list
 * prewalk, copied graph, new clock, post-effect decline or replay is used. */
/* Whole lifecycle owners; original callbacks keep all live guest feedback. */
static int lifecycle_admit(void){
 uint32_t s=g_esp,p=g_ecx;
 return !(s&3)&&s>=0x10400u&&lifecycle_span(s-1024,1048)&&lifecycle_span(p,0x74)&&
 ((uint64_t)p+0x74<=s-1024||(uint64_t)s+24<=p)&&lifecycle_span(0x51c178,1)&&!MEM8(0x51c178);
}
int uc_hybrid_animation_insert(void){
 if(!enabled())return 0;
 if(!lifecycle_admit())return 0;
 uint32_t frame=g_seh_ebp,first_saved=g_esp;int set_zf=0;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edi=MEM32(g_esp+24);g_eax=MEM32(g_edi);g_ebx=g_ecx;g_esi=MEM32(g_ebx+40);g_ecx=g_edi;MEM32(g_esp+16)=g_ebx;
 {uint32_t target=MEM32(g_eax+24);PUSH32(g_esp,0x1f1689);RECOMP_ICALL_SAFE(target,first_saved);}
 g_edx=MEM32(g_edi);g_ecx=g_edi;frame=g_eax;
 {uint32_t saved=g_esp,target=MEM32(g_edx+40);PUSH32(g_esp,0x1f1692);RECOMP_ICALL_SAFE(target,saved);}
 if(!LO8(g_eax)){
  g_eax=MEM32(g_edi);g_ecx=g_edi;
  {uint32_t saved=g_esp,target=MEM32(g_eax+52);PUSH32(g_esp,0x1f169d);RECOMP_ICALL_SAFE(target,saved);}
  if(g_esi){
   g_edx=MEM32(g_esi);g_ecx=g_esi;
   {uint32_t saved=g_esp,target=MEM32(g_edx+24);PUSH32(g_esp,0x1f16a8);RECOMP_ICALL_SAFE(target,saved);}
   if((int32_t)g_eax>(int32_t)frame){
    do{
     g_eax=MEM32(g_esi+4);
     if(!g_eax){MEM32(g_esi+4)=g_edi;MEM32(g_edi+4)=0;set_zf=2;goto insert_return;}
     g_ebx=g_esi;g_esi=g_eax;g_eax=MEM32(g_esi);g_ecx=g_esi;
     {uint32_t saved=g_esp,target=MEM32(g_eax+24);PUSH32(g_esp,0x1f16c2);RECOMP_ICALL_SAFE(target,saved);}
    }while((int32_t)g_eax>(int32_t)frame);
    if(g_ebx){MEM32(g_edi+4)=g_esi;MEM32(g_ebx+4)=g_edi;set_zf=1;goto insert_return;}
    g_ebx=MEM32(g_esp+16);
   }
  }
  g_ecx=MEM32(g_ebx+40);MEM32(g_edi+4)=g_ecx;MEM32(g_ebx+40)=g_edi;
 }
insert_return:
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);
 if(set_zf)g_recomp_zf=set_zf==2;g_esp+=8;return 1;
}
int uc_hybrid_animation_clear(void){
 if(!enabled())return 0;
 if(!lifecycle_admit())return 0;
 PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+40);
 if(g_eax){
  PUSH32(g_esp,g_esi);
  do{
   g_esi=MEM32(g_edi+40);g_eax=MEM32(g_esi+4);MEM32(g_edi+40)=g_eax;g_edx=MEM32(g_esi);g_ecx=g_esi;
   {uint32_t saved=g_esp,target=MEM32(g_edx+48);PUSH32(g_esp,0x1f1a10);RECOMP_ICALL_SAFE(target,saved);}
   g_eax=MEM32(g_esi+8);
   if(g_eax&&!(MEM8(g_esi+12)&16)){
    g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);
    {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x1f1a26);RECOMP_ICALL_SAFE(target,saved);}
   }else if(MEM8(g_esi+12)&4){
    g_edx=MEM32(g_esi);
    {uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x1f1a36);RECOMP_ICALL_SAFE(target,saved);}
   }
   g_eax=MEM32(g_edi+40);
  }while(g_eax);
  POP32(g_esp,g_esi);
 }
 POP32(g_esp,g_edi);g_recomp_zf=1;g_esp+=4;return 1;
}
int uc_hybrid_animation_destroy(void){
 if(!enabled())return 0;
 if(!lifecycle_admit()||!lifecycle_span(0x51c138,12))return 0;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b82b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esp+16)=0;
 PUSH32(g_esp,0x1f1aea);sub_001F19F0();g_eax=MEM32(g_esi+112);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f1af3);sub_002D5CCD();
 g_eax=MEM32(g_esi+32);g_esp+=4;
 if(g_eax){
  g_ecx=MEM32(g_eax-16);PUSH32(g_esp,g_edi);g_edi=g_eax-16;PUSH32(g_esp,0x11030);PUSH32(g_esp,g_ecx);PUSH32(g_esp,48);PUSH32(g_esp,g_eax);
  PUSH32(g_esp,0x1f1b12);sub_002D69BB();PUSH32(g_esp,g_edi);PUSH32(g_esp,0x1f1b18);sub_002D631D();g_esp+=4;POP32(g_esp,g_edi);
 }
 g_eax=MEM32(0x51c140)-1;MEM32(0x51c140)=g_eax;
 if((int32_t)g_eax<=0){g_edx=MEM32(0x51c138);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f1b37);sub_002D631D();g_esp+=4;MEM32(0x51c138)=0;}
 g_eax=MEM32(g_esi+20);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f1b4d);sub_002D631D();g_ecx=MEM32(g_esi+16);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f1b56);sub_002D631D();
 g_edx=MEM32(g_esi+12);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f1b5f);sub_002D631D();g_ecx=MEM32(g_esp+20);g_esp+=12;POP32(g_esp,g_esi);MEM32(0)=g_ecx;
 g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
