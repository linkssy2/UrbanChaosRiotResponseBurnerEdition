#include "hybrid_animation_owner_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F27F0(void);
extern void sub_001F2610(void);
extern void sub_002D5CCD(void);
extern void sub_001F1CE0(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_ANIMATION_OWNER");flag=v&&!strcmp(v,"1");}return flag;}
/* Integer owner only: real update, mask allocation, removal and event calls
 * remain synchronous and keep live guest links authoritative. Well-formed
 * callback lifetime is inherited from the original owner, not prevalidated by
 * a second list walk. Worker-zero qualifies configuration; it is not a lock.
 * Every decline precedes all guest effects; committed calls never replay. */
static int owner_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_animation_owner(void){
 if(!enabled())return 0;
 uint32_t stack=g_esp,owner=g_ecx,frame=g_seh_ebp;
 if((stack&3)||stack<0x10400u||!owner_span(stack-1024,1048)||!owner_span(owner,0x74)||
    !((uint64_t)owner+0x74<=stack-1024||(uint64_t)stack+24<=owner)||
    !owner_span(0x51c178,1)||MEM8(0x51c178))return 0;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b859);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 g_esp-=20;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);g_edi=g_ecx;MEM32(g_esp+8)=0;g_ebx=g_edi+40;MEM32(g_edi+44)=0;
 g_ecx=MEM32(g_ebx);MEM32(g_esp+16)=g_edi;MEM32(g_esp+12)=g_ebx;
 if(g_ecx){
  PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);
  do{
   g_edx=MEM32(g_esp+52);g_eax=MEM32(g_ecx);
   {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x1f1dad);RECOMP_ICALL_SAFE(target,saved);}
   if(--g_eax==0){
    g_esi=MEM32(g_ebx);g_ecx=MEM32(g_esi+4);MEM32(g_ebx)=g_ecx;g_edx=MEM32(g_esi);g_ecx=g_esi;
    {uint32_t saved=g_esp,target=MEM32(g_edx+48);PUSH32(g_esp,0x1f1ec6);RECOMP_ICALL_SAFE(target,saved);}
    g_eax=MEM32(g_esi+8);
    if(g_eax&&!(MEM8(g_esi+12)&16)){
     g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);
     {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x1f1edc);RECOMP_ICALL_SAFE(target,saved);}
    }else if(MEM8(g_esi+12)&4){
     g_edx=MEM32(g_esi);
     {uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x1f1eec);RECOMP_ICALL_SAFE(target,saved);}
    }
   }else if(--g_eax==0){
    g_ecx=MEM32(g_ebx);g_edx=MEM32(g_ecx);
    {uint32_t saved=g_esp,target=MEM32(g_edx+28);PUSH32(g_esp,0x1f1dce);RECOMP_ICALL_SAFE(target,saved);}
    g_edi=MEM32(g_ebx);g_ecx=MEM32(g_edi+4);g_edi+=4;frame=g_eax;
    if(g_ecx){
     do{
      g_eax=MEM32(g_esp+24);g_edx=MEM32(g_eax+4);g_eax=MEM32(g_ecx);g_edx+=28;
      {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+28);PUSH32(g_esp,0x1f1df0);RECOMP_ICALL_SAFE(target,saved);}
      PUSH32(g_esp,g_eax);g_ecx=g_esp+36;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f1dfb);sub_001F27F0();
      g_esp+=12;g_esi=MEM32(g_esp+16);g_esi|=1;PUSH32(g_esp,g_eax);g_ecx=frame;MEM32(g_esp+48)=0;MEM32(g_esp+20)=g_esi;
      PUSH32(g_esp,0x1f1e19);sub_001F2610();
      if(LO8(g_eax)){g_edx=MEM32(g_edi);SET_LO8(g_ebx,(MEM8(g_edx+12)&1)?0:1);}else SET_LO8(g_ebx,0);
      MEM32(g_esp+44)=0xffffffffu;
      if(MEM8(g_esp+16)&1){
       g_ecx=MEM32(g_esp+16);g_eax=MEM32(g_esp+32);g_ecx&=~1u;MEM32(g_esp+16)=g_ecx;
       if((int32_t)g_eax<0){g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f1e57);sub_002D5CCD();g_esp+=4;}
      }
      if(LO8(g_ebx)){
       g_esi=MEM32(g_edi);g_ecx=MEM32(g_esi+4);MEM32(g_edi)=g_ecx;g_edx=MEM32(g_esi);g_ecx=g_esi;
       {uint32_t saved=g_esp,target=MEM32(g_edx+48);PUSH32(g_esp,0x1f1e6c);RECOMP_ICALL_SAFE(target,saved);}
       g_eax=MEM32(g_esi+8);
       if(g_eax&&!(MEM8(g_esi+12)&16)){
        g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);
        {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x1f1e82);RECOMP_ICALL_SAFE(target,saved);}
       }else if(MEM8(g_esi+12)&4){
        g_edx=MEM32(g_esi);
        {uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x1f1e92);RECOMP_ICALL_SAFE(target,saved);}
       }
      }else{g_edi=MEM32(g_edi);g_edi+=4;}
      g_ecx=MEM32(g_edi);
     }while(g_ecx);
     g_ebx=MEM32(g_esp+20);
    }
    g_eax=MEM32(g_ebx);g_edi=MEM32(g_esp+24);g_eax+=4;MEM32(g_esp+20)=g_eax;g_ebx=g_eax;
   }else{g_eax=MEM32(g_ebx);g_eax+=4;MEM32(g_esp+20)=g_eax;g_ebx=g_eax;}
   g_ecx=MEM32(g_ebx);
  }while(g_ecx);
  POP32(g_esp,g_esi);POP32(g_esp,frame);
 }
 g_ecx=g_edi;PUSH32(g_esp,0x1f1eff);sub_001F1CE0();g_ecx=MEM32(g_esp+28);POP32(g_esp,g_edi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;
 g_esp+=32;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
