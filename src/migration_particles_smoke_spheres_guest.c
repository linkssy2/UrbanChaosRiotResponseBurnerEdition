#include "migration_particles_smoke_spheres_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void),sub_00011EF0(void),sub_00012050(void),sub_002D619E(void);
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_SMOKE_SPHERES");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int list_ok(uint32_t owner,uint32_t stack){
 uint32_t head=MEM32(owner+4);
 if(!span(head,8)||!apart(head,8,stack-256,320)||!apart(head,8,owner,12)||!apart(head,8,0x51c178,1))return 0;
 uint32_t node=MEM32(head),previous=head;
 /* ponytail: validate at most 4096 registered volumes before effects. Larger
  * lists keep the original; raise the bound with corresponding fixtures. */
 for(unsigned i=0;i<=4096;i++){
  if(node==head)return MEM32(head+4)==previous;
  if(i==4096||!span(node,0x54)||!apart(node,0x54,stack-256,320)||!apart(node,0x54,owner,12)||!apart(node,0x54,head,8)||!apart(node,0x54,0x51c178,1)||MEM32(node+4)!=previous)return 0;
  previous=node;node=MEM32(node);
 }return 0;
}
static void update_sphere(void){
 g_ecx=MEM32(g_ecx+4);g_eax=MEM32(g_ecx);g_esp-=12;
 if(g_eax!=g_ecx){
  g_edx=MEM32(g_esp+16);
  while(MEM32(g_eax+8)!=g_edx){g_eax=MEM32(g_eax);if(g_eax==g_ecx)break;}
  if(g_eax!=g_ecx){
   /* Scratch and node spans are disjoint. Snapshot the three scalar words,
    * publish the position, then the radius without floating arithmetic. */
   for(unsigned i=0;i<3;i++){g_xmm0=XMM_SCALAR(MEMF(g_esp+20+4*i));MEMF(g_esp+4*i)=g_xmm0.f[0];}
   g_ecx=g_eax+12;g_xmm0=XMM_SCALAR(MEMF(g_esp+32));
   for(unsigned i=0;i<3;i++){g_edx=MEM32(g_esp+4*i);MEM32(g_ecx+4*i)=g_edx;}
   MEMF(g_eax+24)=g_xmm0.f[0];
  }
 }
 g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=24;
}
static void copy_volume_record(void){
 /* Forward, interleaved reads and writes preserve partially aliased records. */
 g_eax=g_ecx;g_ecx=MEM32(g_esp+4);
 for(unsigned i=0;i<18;i++){g_edx=MEM32(g_ecx+4*i);MEM32(g_eax+4*i)=g_edx;}
 g_ecx=MEM32(g_ecx+72);MEM32(g_eax+72)=g_ecx;g_esp+=8;
}
static void allocate_node(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,0x54);CALL(sub_002D57FB,0x4fb48);g_esi=g_eax;g_esp+=4;unsigned allocated=g_esi!=0;
 if(allocated){g_ecx=MEM32(g_esp+12);g_edx=MEM32(g_esp+16);g_eax=MEM32(g_esp+8);MEM32(g_esi+4)=g_ecx;PUSH32(g_esp,g_edx);g_ecx=g_esi+8;MEM32(g_esi)=g_eax;CALL(copy_volume_record,0x4fb6b);}
 g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=!allocated;g_esp+=16;
}
static void increment_count(void){
 g_eax=MEM32(0);g_edx=MEM32(g_esp+4);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x38a988);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_eax=MEM32(g_ecx+8);g_esp-=0x44;PUSH32(g_esp,g_esi);g_esi=0x35e50d7u-g_eax;
 unsigned overflow=g_esi<g_edx;POP32(g_esp,g_esi);
 if(overflow){
  PUSH32(g_esp,16);PUSH32(g_esp,0x44c57c);g_ecx=g_esp+8;MEM32(g_esp+32)=15;MEM32(g_esp+28)=0;MEM8(g_esp+12)=0;CALL(sub_00011EF0,0x4fbd1);
  g_eax=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esp+32;MEM32(g_esp+80)=0;CALL(sub_00012050,0x4fbe6);
  PUSH32(g_esp,0x4c37ac);g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);MEM32(g_esp+36)=0x44e000;CALL(sub_002D619E,0x4fbfd);
 }
 g_eax+=g_edx;MEM32(g_ecx+8)=g_eax;g_ecx=MEM32(g_esp+68);MEM32(0)=g_ecx;g_esp+=80;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void append_sphere(void){
 g_esp-=76;g_eax=MEM32(g_esp+80);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;g_edi=MEM32(g_esi+4);
 uint32_t record=g_esp+12,extra=MEM32(g_esp+112);
 MEM32(record)=g_eax;
 /* Box-only words +20..+64 deliberately retain old guest stack bytes. */
 for(unsigned i=0;i<4;i++){g_xmm0=XMM_SCALAR(MEMF(g_esp+96+4*i));MEMF(record+4+4*i)=g_xmm0.f[0];}
 MEM32(record+68)=extra;MEM32(record+72)=0;g_edx=record;g_eax=MEM32(g_edi+4);
 PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);g_ecx=g_esi;CALL(allocate_node,0x4f9e4);
 PUSH32(g_esp,1);g_ecx=g_esi;g_ebx=g_eax;CALL(increment_count,0x4f9ef);
 MEM32(g_edi+4)=g_ebx;g_ecx=MEM32(g_ebx+4);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(g_ecx)=g_ebx;POP32(g_esp,g_ebx);g_esp+=76;g_recomp_zf=g_esp==0;g_esp+=28;
}
int uc_migration_particles_smoke_spheres(unsigned entry){
 if(!enabled()||(entry!=0x4f320&&entry!=0x4f980))return 0;
 uint32_t s=g_esp,o=g_ecx;
 if((s&3)||s<0x10200u||!span(s-256,320)||!span(o,12)||!apart(o,12,s-256,320)||!span(0x51c178,1)||MEM8(0x51c178)||!apart(s-256,320,0x51c178,1)||!apart(o,12,0x51c178,1)||!list_ok(o,s))return 0;
 if(entry==0x4f980&&MEM32(o+8)>4096)return 0;
 if(entry==0x4f320)update_sphere();else append_sphere();return 1;
}
#undef CALL
