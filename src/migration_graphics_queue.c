#include "migration_graphics_queue.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int queue_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_QUEUE");flag=v&&!strcmp(v,"1");}return flag;}
static int queue_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static uint32_t queue_unwrap(uint32_t value,uint32_t base,uint32_t size){return value>base?value:value+size;}
/* Return the earliest still-monotonic fence record from the short or history
 * ring. Preserve the original scratch record and published register results. */
static void queue_record(void){
 uint32_t frame=g_seh_ebp;g_edx=MEM32(0x3f4238);g_esp-=8;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_edx+44);PUSH32(g_esp,g_edi);g_edi=g_eax;g_eax=g_esi-g_edi;
 int history=0;
 if(g_eax<=128){g_ecx=MEM32(g_esp+20);g_edi=(g_edi>>1)&63;}
 else{
  g_ecx=MEM32(g_edx+52);g_edi=MEM32(g_edx+72);frame=MEM32(g_edi+g_ecx*8);g_ebx=g_esi-frame;
  if(g_ebx>g_eax)g_edi=(g_esi>>1)&63;
  else{g_ebx=MEM32(g_edx+56);do{g_ecx=(g_ecx+1)&g_ebx;frame=g_esi-MEM32(g_edi+g_ecx*8);}while(frame>g_eax);history=1;}
 }
 if(!history){
  frame=MEM32(g_edx+g_edi*8+104);g_ebx=MEM32(g_edx);g_eax=g_edx+g_edi*8+100;frame=queue_unwrap(frame,g_ebx,MEM32(g_edx+68));
  g_esi=(g_esi>>1)&63;g_ecx=(g_edi+1)&63;
  while(g_ecx!=g_esi){g_edi=queue_unwrap(MEM32(g_edx+g_ecx*8+104),g_ebx,MEM32(g_edx+68));if(g_edi<frame){g_eax=0;break;}g_ecx=(g_ecx+1)&63;}
 }else{
  frame=MEM32(g_edx+72);g_esi=MEM32(frame+g_ecx*8+4);g_eax=frame+g_ecx*8;MEM32(g_esp+20)=g_eax;MEM32(g_esp+16)=g_esi;
  if(g_esi<=MEM32(g_edx)){g_esi+=MEM32(g_edx+68);MEM32(g_esp+16)=g_esi;}
  g_edi=MEM32(g_edx+52);g_ebx=MEM32(g_edx+56);g_edi=(g_edi+1)&g_ebx;g_ecx=(g_ecx+1)&g_ebx;
  while(g_ecx!=g_edi){g_esi=queue_unwrap(MEM32(frame+g_ecx*8+4),MEM32(g_edx),MEM32(g_edx+68));if(g_esi<MEM32(g_esp+16)){g_eax=0;break;}g_ecx=(g_ecx+1)&g_ebx;}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_graphics_queue(uint32_t entry){
 if(!queue_enabled()||(entry!=0x3e8fb0&&entry!=0x3e9120))return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!queue_span(s-32,44)||!queue_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x3e8fb0){
  if(!queue_span(g_ecx,72)||!queue_span(g_edx,8))return 0;
  g_eax=MEM32(s+4);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_ecx);g_esi=queue_unwrap(g_eax,g_edi,MEM32(g_ecx+68));g_eax=queue_unwrap(MEM32(g_edx+4),g_edi,MEM32(g_ecx+68))-g_esi;g_ecx=(g_eax>>31)-1;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax&=g_ecx;g_recomp_zf=g_eax==0;g_esp+=8;return 1;
 }
 if(!queue_span(0x3f4238,4))return 0;uint32_t d=MEM32(0x3f4238);
 if(!queue_span(d,616)||!((uint64_t)d+616<=s-32||d>=s+12))return 0;
 uint32_t sequence=MEM32(d+44),delta=sequence-g_eax;
 if(delta>128){
  uint32_t mask=MEM32(d+56),index=MEM32(d+52),records=MEM32(d+72);
  if(mask>4095||(mask&(mask+1))||index>mask||!queue_span(records,((uint64_t)mask+1)*8)||!((uint64_t)records+(mask+1)*8<=s-32||records>=s+12))return 0;
  if(sequence-MEM32(records+index*8)<=delta){unsigned count=0;do{index=(index+1)&mask;if(++count>mask+1)return 0;}while(sequence-MEM32(records+index*8)>delta);}
 }
 queue_record();return 1;
}
