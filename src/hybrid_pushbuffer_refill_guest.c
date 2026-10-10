#include "hybrid_pushbuffer_refill_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9220(void),sub_003E9380(void),sub_003E94A0(void),sub_003E9560(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PUSHBUFFER_REFILL");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Valid live device/ring and completing progress/wait services are inherited
 * source preconditions. No speculative GPU work, new lock, or late fallback. */
int uc_hybrid_pushbuffer_refill(uint32_t entry){
 if(!enabled())return 0;
 uint32_t s=g_esp;
 if((entry!=0x3e9220&&entry!=0x3e96d0)||(s&3)||s<0x10100u||!span(s-128,140)||!span(0x51c178,1)||MEM8(0x51c178)||!span(0x3f4238,4)||!span(0x3f69d4,4))return 0;
 uint32_t device=entry==0x3e9220?g_ecx:MEM32(0x3f4238);
 if(!span(device,0x1c24))return 0;
 if(entry==0x3e9220){
  g_eax=MEM32(g_ecx+0x1c20);g_eax=MEM32(g_eax+0x44);g_edx=MEM32(g_ecx+0x24);g_eax|=0x80000000u;
  PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
  if(g_eax>=g_edx&&g_eax<MEM32(g_ecx+0x28)){
   g_edi=MEM32(g_ecx);g_esi=MEM32(g_ecx+0x40);g_edx=g_edi<g_eax;g_esi-=g_edx;
   POP32(g_esp,g_edi);MEM32(g_ecx+0x60)=g_esi;MEM32(g_ecx+0x5c)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_esi==0;g_esp+=4;return 1;
  }
  g_esi=MEM32(g_ecx+0x40);g_eax=MEM32(g_ecx+0x934);RECOMP_GPU_PROGRESS_READ(g_eax+0x400b10);g_eax=MEM32(g_eax+0x400b10);g_edx=g_esi;g_edx-=g_eax;g_eax>>=5;
  PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_ecx+0x60);g_edi=g_esi;g_eax&=0x7fffffc;g_edi-=g_ebx;g_edx&=3;g_eax|=0x80000000u;POP32(g_esp,g_ebx);
  if(g_edi>=g_edx&&(g_edi!=g_edx||MEM32(g_ecx+0x5c)<=g_eax)){
   g_esi-=g_edx;POP32(g_esp,g_edi);MEM32(g_ecx+0x60)=g_esi;MEM32(g_ecx+0x5c)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_esi==0;g_esp+=4;return 1;
  }
  g_eax=MEM32(g_ecx+0x5c);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;return 1;
 }
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f4238);int recording=(MEM8(g_esi+8)&4)!=0;PUSH32(g_esp,g_edi);
 if(recording){
  g_eax=MEM32(g_esi+0x764);g_edi=MEM32(g_eax+4);g_ecx=MEM32(g_esi);g_edx=MEM32(g_esi+0x768);g_ecx-=g_edi;g_edx+=g_ecx;MEM32(g_esi+0x768)=g_edx;g_eax=MEM32(g_eax+4);
  POP32(g_esp,g_edi);MEM32(g_esi)=g_eax;POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=g_edx==0;g_esp+=12;return 1;
 }
 g_edi=MEM32(g_esi);g_edx=MEM32(g_esi+0x30);g_eax=MEM32(g_edx);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+24);PUSH32(g_esp,frame);frame=g_edi+g_ebx;g_ecx=g_esi;MEM32(g_esp+28)=frame;MEM32(g_esp+16)=g_eax;
 PUSH32(g_esp,0x3e9724);sub_003E9220();
 g_ecx=MEM32(g_esi+0x28);frame+=0x4000;
 if(frame>=g_ecx){
  g_edx=MEM32(g_esp+24);g_edx+=g_edi;
  if(g_edx>g_ecx){
   g_ecx=MEM32(g_esi+0x40);g_edx=MEM32(g_esi);++g_ecx;MEM32(g_esi+0x40)=g_ecx;g_ecx=MEM32(g_esi+0x24);g_edx-=g_ecx;g_ecx&=0xfffffff;MEM32(g_esi+0x44)=g_edx;g_edx=MEM32(g_esi);++g_ecx;MEM32(g_edx)=g_ecx;
   if(g_eax>MEM32(g_esi)){g_eax=MEM32(g_esi+0x24);g_eax+=4;}
   g_edi=MEM32(g_esi+0x24);if(g_eax==g_edi)g_eax=g_edi+4;g_ecx=g_edi+g_ebx;MEM32(g_esi)=g_edi;
  }
  MEM32(g_esp+28)=g_ecx;
 }
 if(g_edi<g_eax&&g_eax<=MEM32(g_esp+28)){
  g_ebx=MEM32(0x3f69d4);g_eax=MEM32(g_esi+0x34);g_edx=MEM32(g_esi+0x48);frame=MEM32(g_esi+0x38);g_ebx>>=1;g_ebx+=g_edi;
  for(;;){
   g_ecx=MEM32(g_edx+g_eax*8);--g_eax;g_eax&=frame;MEM32(g_esp+24)=g_ecx;g_ecx=MEM32(g_edx+g_eax*8+4);if(g_ecx<g_edi)g_ecx+=MEM32(g_esi+0x44);
   if(g_ecx<=g_ebx||g_eax==MEM32(g_esi+0x34))break;
   g_ecx=MEM32(g_esp+16);if(MEM32(g_edx+g_eax*8)<=g_ecx)break;
  }
  g_edx=MEM32(g_esp+24);PUSH32(g_esp,8);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x3e97c4);sub_003E9560();
 }
 g_eax=MEM32(g_esp+28);g_eax-=0x204;MEM32(g_esi+4)=g_eax;g_eax=MEM32(g_esi+8);int begun=(g_eax&0x800)!=0;POP32(g_esp,frame);POP32(g_esp,g_ebx);
 if(begun){g_eax|=0x1000;g_ecx=g_esi;MEM32(g_esi+8)=g_eax;PUSH32(g_esp,0x3e97e9);sub_003E9380();g_eax=MEM32(g_esi);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=g_eax==0;g_esp+=12;return 1;}
 PUSH32(g_esp,3);PUSH32(g_esp,0x3e97f8);sub_003E94A0();g_ecx=g_esi;PUSH32(g_esp,0x3e97ff);sub_003E9380();g_eax=MEM32(g_esi);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=12;return 1;
}
