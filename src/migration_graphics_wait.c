#include "migration_graphics_wait.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E94A0(void),sub_003E9120(void),sub_003E92A0(void),sub_003E8FB0(void),sub_003E9320(void);
#define WAIT_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int wait_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_WAIT");flag=v&&!strcmp(v,"1");}return flag;}
static int wait_ordered(void){static int flag=-1;if(flag<0)flag=getenv("UCRR_ORDERED_GPU")!=NULL;return flag;}
static int wait_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void wait_progress(unsigned mode,uint32_t pc){PUSH32(g_esp,mode);PUSH32(g_esp,g_esi);WAIT_CALL(sub_003E92A0,pc);}
static void wait_distance(uint32_t frame,uint32_t pc){PUSH32(g_esp,g_eax);g_edx=frame;g_ecx=g_esi;WAIT_CALL(sub_003E8FB0,pc);}
static void wait_notify(uint32_t pc){uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);MEM32(g_esi+0x1a00)=g_edi;uint32_t target=MEM32(g_esi+0x19fc);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);g_esp+=4;}
int uc_migration_graphics_wait(uint32_t entry){
 if(!wait_enabled()||entry!=0x3e9560)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!wait_span(s-128,144)||!wait_span(0x51c178,1)||MEM8(0x51c178)||!wait_span(0x3f4238,4))return 0;
 uint32_t device=MEM32(0x3f4238);if(!wait_span(device,0x2480)||!wait_span(MEM32(device+48),4))return 0;
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f4238);g_eax=MEM32(g_esi+48);g_ecx=MEM32(g_eax);g_eax=MEM32(g_esi+44);g_edx=g_eax-g_ecx;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_ecx=g_eax-g_edi;
 if(g_ecx<g_edx){
  if(g_edi==g_eax){PUSH32(g_esp,0);WAIT_CALL(sub_003E94A0,0x3e958f);}
  PUSH32(g_esp,frame);g_eax=g_edi;WAIT_CALL(sub_003E9120,0x3e9597);frame=g_eax;
  if(frame){
   wait_progress(0,0x3e95a9);g_edx=MEM32(g_esi);if(g_eax<=g_edx)g_eax+=MEM32(g_esi+68);g_ecx=MEM32(frame+4);if(g_ecx<=g_edx)g_ecx+=MEM32(g_esi+68);g_ecx-=g_eax;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+24);
   int event_wait=0;
   /* Ordered D3D completion uses the RAM fence below, not an Xbox interrupt event. */
   if(!wait_ordered()&&(int32_t)g_ecx>=0&&g_ecx>=0x2400){
    g_eax=MEM32(g_esi+0x19fc);int prepare=1;
    if(g_eax){wait_notify(0x3e95ee);wait_progress(0,0x3e95f9);wait_distance(frame,0x3e9603);prepare=g_eax>=0x2000;}
    if(prepare){
     g_edi=MEM32(frame+4);g_eax=0;MEM32(g_esi+0x1dd0)=0;if(!(g_ebx&8)){MEM32(g_edi+16)=0x40110;MEM32(g_edi+20)=g_eax;}MEM32(g_edi+24)=0x40100;MEM32(g_edi+28)=5;WAIT_CALL(sub_003E9320,0x3e9637);
     wait_progress(1,0x3e963f);wait_distance(frame,0x3e9649);
     if(g_eax>=0x2000)event_wait=1;
     else{g_eax=0x40100;g_ecx=0;MEM32(g_edi+16)=g_eax;MEM32(g_edi+20)=g_ecx;MEM32(g_edi+24)=g_eax;MEM32(g_edi+28)=g_ecx;WAIT_CALL(sub_003E9320,0x3e9668);g_edi=MEM32(frame);}
    }
   }
   if(event_wait){
    g_edi=MEM32(0x44b268);g_esi+=0x1dcc;
    do{uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,6);PUSH32(g_esp,g_esi);uint32_t target=g_edi;PUSH32(g_esp,0x3e96bf);RECOMP_ICALL_SAFE(target,saved);}while(g_eax);
    g_recomp_zf=1;
   }else{
    g_eax=MEM32(g_esi+0x19fc);if(g_eax){g_ebx|=0x80000000;wait_notify(0x3e968a);}
    g_edx=MEM32(g_esi+48);g_esi=MEM32(g_esi+44);g_eax=g_esi-g_edi;
    do{RECOMP_MEMORY_READ_POLL(g_edx);g_ecx=MEM32(g_edx);g_edi=g_esi-g_ecx;}while(g_eax<g_edi);
   }
   POP32(g_esp,g_ebx);
  }
  POP32(g_esp,frame);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=12;return 1;
}
