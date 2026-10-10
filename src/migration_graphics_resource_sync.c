#include "migration_graphics_resource_sync.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_000825C8(void),sub_003E54F0(void),sub_003E9560(void);
#define CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_GRAPHICS_RESOURCE_SYNC");v=s&&!strcmp(s,"1");}return v;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}

/* Unlink before notification/free, then reload the predecessor's live link.
 * Callbacks may replace the remaining list; no host pointer cache survives them.
 * As in the original, remaining nodes and completing callbacks must stay valid. */
static void clear_private_data(void){
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f3134);PUSH32(g_esp,g_edi);g_edi=0x3f3134;
 if(g_esi){
  PUSH32(g_esp,frame);frame=MEM32(g_esp+16);
  do{
   if(MEM32(g_esi+4)==frame){
    g_eax=MEM32(g_esi);MEM32(g_edi)=g_eax;
    if(MEM8(g_esi+28)&1){
     g_eax=MEM32(g_esi+32);g_ecx=MEM32(g_eax);uint32_t saved=g_esp;
     PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_ecx+8);PUSH32(g_esp,0x3e542e);RECOMP_ICALL_SAFE(target,saved);
    }
    PUSH32(g_esp,0x24800000);PUSH32(g_esp,g_esi);CALL(sub_000825C8,0x3e5439);g_esi=MEM32(g_edi);
   }else{g_edi=g_esi;g_esi=MEM32(g_esi);}
  }while(g_esi);
  POP32(g_esp,frame);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;
}
static void resource_is_bound(void){
 g_ecx=MEM32(g_esp+4);g_eax=MEM32(g_ecx);
 if(g_eax&0x780000u){g_eax=1;g_esp+=8;return;}
 g_eax&=0x70000u;
 if(g_eax==0x50000u){g_eax=MEM32(g_ecx+20);if(g_eax&&(MEM32(g_eax)&0x780000u)){g_eax=1;g_esp+=8;return;}}
 g_eax=0;g_recomp_zf=1;g_esp+=8;
}
static void resource_wait(void){
 g_eax=MEM32(0x3f4238);
 if(!g_eax){g_esp+=8;return;}
 g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_eax)&0x70000u;PUSH32(g_esp,g_esi);
 if(g_ecx==0x50000u){
  g_esi=MEM32(g_eax+20);
  if(g_esi){
   PUSH32(g_esp,g_eax);CALL(sub_003E54F0,0x3e98bb);
   if(g_eax){
    g_edx=MEM32(0x3f4238);g_eax=MEM32(g_edx+44);PUSH32(g_esp,2);PUSH32(g_esp,g_eax);CALL(sub_003E9560,0x3e98d0);
    POP32(g_esp,g_esi);g_recomp_zf=0;g_esp+=8;return;
   }
   g_eax=g_esi;
  }
 }
 g_esi=MEM32(g_eax+8);PUSH32(g_esp,g_eax);CALL(sub_003E54F0,0x3e98df);
 if(g_eax){
  g_ecx=MEM32(0x3f4238);g_edx=MEM32(g_ecx+44);PUSH32(g_esp,2);PUSH32(g_esp,g_edx);CALL(sub_003E9560,0x3e98f4);
  POP32(g_esp,g_esi);g_recomp_zf=0;g_esp+=8;return;
 }
 if(g_esi){PUSH32(g_esp,16);PUSH32(g_esp,g_esi);CALL(sub_003E9560,0x3e9904);}
 POP32(g_esp,g_esi);g_esp+=8;
}
static void buffer_wait(void){
 g_edx=MEM32(0x3f4238);
 if(g_edx){
  g_ecx=MEM32(g_esp+4);g_eax=MEM32(g_ecx+8);
  if(MEM32(g_ecx)&0x780000u){
   g_eax=MEM32(g_edx+44);PUSH32(g_esp,2);PUSH32(g_esp,g_eax);CALL(sub_003E9560,0x3e9934);g_recomp_zf=0;g_esp+=8;return;
  }
  if(g_eax){PUSH32(g_esp,16);PUSH32(g_esp,g_eax);CALL(sub_003E9560,0x3e9943);}
 }
 g_esp+=8;
}
int uc_migration_graphics_resource_sync(uint32_t entry){
 if(!enabled()||(entry!=0x3e5400&&entry!=0x3e54f0&&entry!=0x3e9890&&entry!=0x3e9910))return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10100u||!span(s-128,140)||!span(0x51c178,1)||MEM8(0x51c178)||!span(0x3f4238,4)||!span(0x3f3134,4))return 0;
 if(entry==0x3e5400){uint32_t node=MEM32(0x3f3134);if(node&&!span(node,36))return 0;clear_private_data();}
 else{
  uint32_t device=MEM32(0x3f4238),p=MEM32(s+4);
  if(entry==0x3e54f0||device){
   if(!span(p,24))return 0;
   if(entry!=0x3e54f0&&!span(device,48))return 0;
   if((MEM32(p)&0x70000u)==0x50000u&&entry!=0x3e9910){uint32_t parent=MEM32(p+20);if(parent&&!span(parent,24))return 0;}
  }
  if(entry==0x3e54f0)resource_is_bound();else if(entry==0x3e9890)resource_wait();else buffer_wait();
 }
 return 1;
}
#undef CALL
