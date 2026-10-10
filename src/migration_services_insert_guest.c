#include "migration_services_insert_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_SERVICES_INSERT");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
/* Split a released extent into representable heap entries and insert each
 * into its small bin or the size-ordered large bin. Guest metadata is the
 * only ownership state; no host allocation is involved. */
static void insert_extent(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);g_eax=MEM32(f+12);SET_LO8(g_edx,MEM8(g_eax+4));
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);SET_LO16(g_esi,MEM16(g_eax+2));g_ecx=LO8(g_edx);
 PUSH32(g_esp,g_edi);g_edi=MEM32(f+8);g_ecx=MEM32(g_edi+g_ecx*4+0x60);MEM32(f-8)=g_ecx;
 SET_LO8(g_ecx,MEM8(g_eax+5));MEM8(f+15)=LO8(g_ecx);g_ecx=MEM32(f+16);MEM32(g_edi+0x30)+=g_ecx;MEM8(f-1)=LO8(g_edx);
 while(g_ecx){
  if(g_ecx>0xff00){g_ebx=g_ecx==0xff01?0xfef0:0xff00;MEM8(g_eax+5)=0;}
  else{g_ebx=g_ecx;SET_LO8(g_ecx,MEM8(f+15));MEM8(g_eax+5)=LO8(g_ecx);}
  MEM8(g_eax+5)&=0xf8;MEM16(g_eax+2)=LO16(g_esi);MEM8(g_eax+4)=LO8(g_edx);MEM16(g_eax)=LO16(g_ebx);
  if(LO16(g_ebx)<128){
   g_ecx=LO16(g_ebx);g_esi=g_edi+g_ecx*8+0x180;
   if(MEM32(g_esi)==g_esi){g_edx=(g_ecx>>3)+g_edi+0x160;MEM32(f+8)=g_edx;g_ecx&=7;SET_LO8(g_edx,1u<<g_ecx);g_ecx=MEM32(f+8);MEM8(g_ecx)|=LO8(g_edx);}
  }else{
   g_ecx=g_edi+0x180;g_esi=MEM32(g_ecx);
   while(g_esi!=g_ecx&&LO16(g_ebx)>MEM16(g_esi-8))g_esi=MEM32(g_esi);
  }
  g_edx=MEM32(g_esi+4);g_ecx=g_eax+8;MEM32(g_ecx)=g_esi;MEM32(g_eax+12)=g_edx;MEM32(g_edx)=g_ecx;MEM32(g_esi+4)=g_ecx;
  g_ecx=LO16(g_ebx);MEM32(f+16)-=g_ecx;g_ecx*=16;g_eax+=g_ecx;g_ecx=MEM32(f-8);g_esi=g_ebx;
  if(g_eax>=MEM32(g_ecx+0x2c))goto done;
  if(!MEM32(f+16))break;
  SET_LO8(g_edx,MEM8(f-1));g_ecx=MEM32(f+16);
 }
 if(!(MEM8(f+15)&16))MEM16(g_eax+2)=LO16(g_esi);
done:
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f+20;
}
int uc_migration_services_insert(unsigned entry){
 if(!enabled()||entry!=0x82c18)return 0;
 uint32_t s=g_esp;if((s&3)||!span(s-24,44)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t heap=MEM32(s+4),block=MEM32(s+8),units=MEM32(s+12);
 if((heap&15)||(block&15)||!span(heap,0x580)||!span(block,16)||units>(g_xbox_total_ram-0x10000)/16)return 0;
 unsigned bytes=units*16+16;
 if(!span(block,bytes)||!apart(heap,0x580,block,bytes)||!apart(heap,0x580,s-24,44)||!apart(block,bytes,s-24,44))return 0;
 uint32_t segment=MEM32(heap+0x60+4*MEM8(block+4));
 if(!span(segment,0x44)||!apart(segment,0x44,block,bytes)||!apart(segment,0x44,s-24,44))return 0;
 if(MEM32(segment+0x2c)<block+units*16)return 0;
 /* ponytail: validate at most 4096 existing entries; corrupted or oversized
  * lists retain the original path instead of allocating a shadow index. */
 unsigned visited=0;
 for(unsigned bin=0;bin<128;bin++){
  uint32_t head=heap+0x180+bin*8,prev=head,node=MEM32(head);unsigned size=0;
  while(node!=head){
   if(++visited>4096||node<8||!span(node-8,16)||!apart(node-8,16,block,bytes)||!apart(node-8,16,s-24,44)||!apart(node-8,16,heap,0x580)||MEM32(node+4)!=prev)return 0;
   unsigned n=MEM16(node-8);if(bin?(n!=bin):(n<128||n<size))return 0;
   size=n;prev=node;node=MEM32(node);
  }
  if(MEM32(head+4)!=prev)return 0;
 }
 insert_extent();return 1;
}
