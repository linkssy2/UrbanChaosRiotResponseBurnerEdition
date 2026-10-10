#include "migration_services_ranges_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static void vm_call(unsigned target,unsigned pc,unsigned saved){PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
static void return_node(void){
 g_ecx=MEM32(g_esp+4);g_eax=MEM32(g_ecx+24);g_edx=MEM32(g_eax+0x4c);g_eax=MEM32(g_esp+8);MEM32(g_eax)=g_edx;
 g_ecx=MEM32(g_ecx+24);MEM32(g_ecx+0x4c)=g_eax;MEM32(g_eax+4)=0;MEM32(g_eax+8)=0;g_esp+=12;
}
static void acquire_node(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;g_esp-=12;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(f+8);g_eax=MEM32(g_esi+24);g_ecx=g_eax+0x4c;g_ebx=0;PUSH32(g_esp,g_edi);
 if(!MEM32(g_ecx)){
  g_ecx=MEM32(g_eax+0x48);MEM32(f+8)=g_ecx;
  if(g_ecx&&MEM32(g_ecx+8)!=MEM32(g_ecx+4)){
   g_eax=MEM32(g_ecx+8);uint32_t saved=g_esp;PUSH32(g_esp,4);g_eax=0x1000;PUSH32(g_esp,g_eax);MEM32(f-4)=g_eax;g_edx=MEM32(g_ecx+8);g_eax=f-4;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);g_eax=f-12;g_edx+=g_ecx;PUSH32(g_esp,g_eax);MEM32(f-12)=g_edx;vm_call(MEM32(0x44b3d8),0x82632,saved);
   if((int32_t)g_eax<0){g_eax=0;goto done;}
   g_eax=MEM32(f+8);g_ecx=MEM32(f-4);MEM32(g_eax+8)+=g_ecx;g_eax=MEM32(f+8);
  }else{
   if(g_ecx)g_eax=MEM32(g_ecx+8);g_edi=MEM32(0x44b3d8);uint32_t saved=g_esp;
   PUSH32(g_esp,4);PUSH32(g_esp,0x2000);g_eax=f-8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);g_eax=f+8;PUSH32(g_esp,g_eax);MEM32(f-8)=0x10000;MEM32(f+8)=g_ebx;vm_call(g_edi,0x82670,saved);
   if((int32_t)g_eax<0){g_eax=0;goto done;}
   saved=g_esp;PUSH32(g_esp,4);g_eax=0x1000;PUSH32(g_esp,g_eax);MEM32(f-4)=g_eax;g_eax=f-4;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);g_eax=f+8;PUSH32(g_esp,g_eax);vm_call(g_edi,0x8268a,saved);
   if((int32_t)g_eax<0){saved=g_esp;PUSH32(g_esp,0x8000);g_eax=f-8;PUSH32(g_esp,g_eax);g_eax=f+8;PUSH32(g_esp,g_eax);vm_call(MEM32(0x44b3dc),0x826a1,saved);g_eax=0;goto done;}
   g_eax=MEM32(g_esi+24);g_eax=MEM32(g_eax+0x48);g_ecx=MEM32(f+8);MEM32(g_ecx)=g_eax;g_eax=MEM32(g_esi+24);g_ecx=MEM32(f+8);MEM32(g_eax+0x48)=g_ecx;
   g_eax=MEM32(f+8);g_ecx=MEM32(f-8);MEM32(g_eax+4)=g_ecx;g_eax=MEM32(f+8);g_ecx=MEM32(f-4);MEM32(g_eax+8)=g_ecx;g_eax=MEM32(f+8);g_ecx=g_eax+16;MEM32(f-12)=g_ecx;
  }
  g_ecx=MEM32(g_eax+8);g_edx=MEM32(f-12);g_ecx+=g_eax;g_eax=MEM32(g_esi+24)+0x4c;
  while(g_edx<g_ecx){MEM32(g_eax)=g_edx;g_eax=g_edx;g_edx+=16;}MEM32(g_eax)=g_ebx;g_ecx=MEM32(g_esi+24)+0x4c;
 }
 g_eax=MEM32(g_ecx);g_edx=MEM32(g_eax);MEM32(g_ecx)=g_edx;
done:
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f+12;
}
static void insert_range(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(f+8);PUSH32(g_esp,g_edi);g_edi=MEM32(f+16);g_ebx=g_esi+0x38;
 for(;;){
  g_eax=MEM32(g_ebx);if(!g_eax)break;g_edx=MEM32(g_eax+4);g_ecx=MEM32(f+12);
  if(g_edx>g_ecx){
   g_edx=g_ecx+g_edi;
   if(g_edx==MEM32(g_eax+4)){MEM32(g_eax+8)+=g_edi;MEM32(g_eax+4)=g_ecx;g_eax=MEM32(g_eax+8);if(g_eax>MEM32(g_esi+0x1c))MEM32(g_esi+0x1c)=g_eax;goto done;}
   break;
  }
  g_ecx=MEM32(g_eax+8)+g_edx;
  if(g_ecx==MEM32(f+12)){
   g_edi+=MEM32(g_eax+8);g_ecx=g_edx;MEM32(f+12)=g_ecx;g_ecx=MEM32(g_eax);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);MEM32(g_ebx)=g_ecx;g_ebp=f;PUSH32(g_esp,0x8275b);return_node();MEM32(g_esi+0x34)--;
   if(g_edi>MEM32(g_esi+0x1c))MEM32(g_esi+0x1c)=g_edi;
  }else g_ebx=g_eax;
 }
 PUSH32(g_esp,g_esi);g_ebp=f;PUSH32(g_esp,0x82776);acquire_node();
 if(g_eax){g_ecx=MEM32(f+12);MEM32(g_eax+4)=g_ecx;MEM32(g_eax+8)=g_edi;g_ecx=MEM32(g_ebx);MEM32(g_eax)=g_ecx;MEM32(g_ebx)=g_eax;MEM32(g_esi+0x34)++;if(g_edi>=MEM32(g_esi+0x1c))MEM32(g_esi+0x1c)=g_edi;}
done:
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=20;
}
int uc_migration_services_ranges(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_RANGES");enabled=p&&!strcmp(p,"1");}
 if(!enabled||(entry!=0x825e2&&entry!=0x82702&&entry!=0x82723))return 0;
 uint32_t s=g_esp;if((s&3)||!span(s-128,148)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t seg=MEM32(s+4);if(!span(seg,0x44)||!apart(seg,0x44,s-128,148))return 0;
 uint32_t heap=MEM32(seg+24);if(!span(heap,0x50)||!apart(heap,0x50,s-128,148))return 0;
 if(entry==0x82702){uint32_t node=MEM32(s+8);if(!span(node,12)||!apart(node,12,s-128,148))return 0;return_node();return 1;}
 uint32_t node=MEM32(heap+0x4c);unsigned seen=0;
 while(node){if(++seen>4096||!span(node,16)||!apart(node,16,s-128,148)||!apart(node,16,heap,0x50)||!apart(node,16,seg,0x44))return 0;node=MEM32(node);}
 uint32_t pool=MEM32(heap+0x48);
 if(pool&&(!span(pool,16)||!span(pool,MEM32(pool+4))||MEM32(pool+8)<16||MEM32(pool+8)>MEM32(pool+4)||!apart(pool,MEM32(pool+4),s-128,148)))return 0;
 if(entry==0x825e2){acquire_node();return 1;}
 node=MEM32(seg+0x38);seen=0;uint32_t previous=0;
 while(node){if(++seen>4096||!span(node,12)||!apart(node,12,s-128,148)||!apart(node,12,heap,0x50)||!apart(node,12,seg,0x44)||MEM32(node+4)<previous)return 0;previous=MEM32(node+4);node=MEM32(node);}
 insert_range();return 1;
}
