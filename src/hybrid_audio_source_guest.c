#include "hybrid_audio_source_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_00240870(void);
extern void sub_00243F60(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_AUDIO_SOURCE");flag=v&&!strcmp(v,"1");}return flag;}
/* Worker-zero is the pinned title configuration, not a synchronization lock.
 * Guest objects and callbacks own lifetime. Admission is complete before the
 * first effect; after it succeeds this function never declines or replays. */
/* Complete quiescent source destructor at the original deleting-wrapper call.
 * Guest objects remain authoritative; callbacks/free routines are never replayed.
 * Reuses the ownership domain of audio_source_retirement.c, not its incompatible
 * native-pointer layout. No second graph, device or allocation is constructed. */
static int source_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int source_apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return(uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static int source_object(uint32_t p,unsigned n,uint32_t stack){return source_span(p,n)&&source_apart(p,n,stack-1024,1048)&&source_apart(p,n,0x51c178,1)&&source_apart(p,n,0x51cb90,4)&&source_apart(p,n,0x483a00,0x300);}
static int source_link(uint32_t p,uint32_t stack){
 if(!source_object(p,8,stack))return 0;
 uint32_t a=MEM32(p),b=MEM32(p+4);
 return source_object(a,8,stack)&&source_object(b,8,stack)&&MEM32(a+4)==p&&MEM32(b)==p;
}
static int source_admit(void){
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10400u||!source_span(s-1024,1048)||!source_object(p,0x44,s)||!source_span(0x51c178,1)||MEM8(0x51c178)||!source_span(0x51cb90,4)||!source_span(0x483ae4,8)||MEM32(p)!=0x483ae4||MEM32(0x483ae8)!=0x247050||MEM32(p+0x1c)!=0||MEM32(s)!=0x247058||g_esi!=p)return 0;
 uint32_t e=MEM32(0x51cb90),buffers[64],count=0;
 if(!source_object(e,0x84,s)||!source_apart(e,0x84,p,0x44)||!source_link(p+4,s)||MEM32(p+0x14)!=p+0x14||MEM32(p+0x18)!=p+0x14||!source_link(p+12,s)||!source_link(e+0x28,s))return 0;
 /* Unregister executes before the empty-pending branch. Its reciprocal-node
  * writes must not change the source's pending header through an alias. */
 for(unsigned j=0;j<2;j++){uint32_t node=MEM32(p+4+j*4);if(node!=p+4&&!source_apart(node,8,p,0x44))return 0;}
 for(uint32_t node=MEM32(p+12);node!=p+12;node=MEM32(node)){
  if(count==64||node<32)return 0;uint32_t b=node-32;
  if(!source_object(b,0x54,s)||!source_apart(b,0x54,p,0x44)||!source_apart(b,0x54,e,0x84)||!source_link(node,s)||MEM32(b)!=0x483ac0||MEM32(b+4)!=0x483ab4||!MEM32(b+0x30)||(MEM8(b+0x44)&8)||MEM32(b+0x38)||MEM32(b+0x40))return 0;
  for(unsigned j=0;j<count;j++)if(!source_apart(b,0x54,buffers[j],0x54))return 0;
  for(unsigned j=0;j<5;j++)if(!source_link(b+8+8*j,s))return 0;
  for(unsigned j=1;j<5;j++)if(j!=3&&MEM32(b+8+8*j)!=b+8+8*j)return 0;
  uint32_t group=MEM32(b+0x34),device=MEM32(b+0x3c);
  if(group&&(!source_object(group,0x24,s)||!source_apart(group,0x24,p,0x44)||!source_apart(group,0x24,b,0x54)||MEM32(group+0x20)<=64))return 0;
  if(device&&(!source_object(device,0x30,s)||!source_apart(device,0x30,p,0x44)||!source_apart(device,0x30,b,0x54)||MEM32(device)!=0x48453c||MEM32(device+4)!=0x48452c||!MEM32(device+0x18)||MEM32(device+0x1c)!=0x12||!source_link(device+8,s)||MEM32(device+0x10)!=device+0x10||MEM32(device+0x14)!=device+0x10))return 0;
  buffers[count++]=b;
 }
 unsigned instances=0;
 for(uint32_t node=MEM32(e+0x28);node!=e+0x28;node=MEM32(node)){
  if(++instances>256||node<24)return 0;uint32_t i=node-24;
  if(!source_object(i,0x50,s)||!source_link(node,s)||MEM32(i)!=0x483c10||!source_apart(i,0x50,p,0x44))return 0;
  for(unsigned j=0;j<count;j++)if(MEM32(i+0x44)==buffers[j]||!source_apart(i,0x50,buffers[j],0x54))return 0;
 }
 return 1;
}
int uc_hybrid_audio_source_destroy(void){
 if(!enabled()||!source_admit())return 0;
 /* Full246510, with its proved-empty pending ring. Exact guest call frontiers
  * and retired SEH/frame writes remain observable to the retained callbacks. */
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e718);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 MEM32(g_esp+12)=g_ebx;MEM32(g_ebx)=0x483ae4;PUSH32(g_esp,0);MEM32(g_esp+28)=0;
 PUSH32(g_esp,0x246544);sub_00241880();g_esp+=4;
 if(g_eax){PUSH32(g_esp,g_ebx);g_ecx=g_eax;PUSH32(g_esp,0x246553);sub_00240870();}
 g_eax=MEM32(g_ebx+20);g_edi=g_ebx+20;
 g_eax=MEM32(g_ebx+12);g_edi=g_ebx+12;
 while(MEM32(g_edi)!=g_edi){
  g_eax=MEM32(g_edi);PUSH32(g_esp,g_eax);g_esi=g_eax-32;PUSH32(g_esp,0x2465bb);sub_00243F60();
  g_edx=MEM32(g_esi);g_esp+=4;
  {uint32_t saved=g_esp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0);g_ecx=g_esi;uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x2465c9);RECOMP_ICALL_SAFE(target,saved);}
  g_eax=MEM32(g_esi);g_ecx=g_esi;
  {uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x2465d0);RECOMP_ICALL_SAFE(target,saved);}
 }
 g_ecx=g_ebx+4;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x2465dd);sub_00243F60();
 g_ecx=MEM32(g_esp+20);g_esp+=4;POP32(g_esp,g_edi);MEM32(g_ebx)=0x483a80;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;
 g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
