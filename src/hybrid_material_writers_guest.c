#include "hybrid_material_writers_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void),sub_003EE550(void),sub_003E94A0(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_MATERIAL_WRITERS");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}

/* Complete public packet writers. Original growth, dirty-state preparation and
 * flush services remain synchronous boundaries, including their live feedback.
 * No no-wrap assumption, redundant-state suppression, or rollback after writes. */
int uc_hybrid_material_writers(unsigned operation){
 if(operation>6||!enabled())return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10100u||!span(s-128,156)||!span(0x3f41c8,128)||!span(0x51c178,1)||MEM8(0x51c178)||!apart(s-128,156,0x3f41c8,128)||!apart(s-128,156,0x51c178,1))return 0;
 uint32_t device=MEM32(0x3f4238),packet;
 if(operation==0)packet=MEM32(0x3f4240);
 else {if(!span(device,12)||!apart(device,12,s-128,156))return 0;packet=MEM32(device);}
 if(!span(packet,20)||!apart(packet,20,s-128,156))return 0;
 if(operation==0){
  for(;;){
   g_eax=MEM32(0x3f4240)+8;uint32_t limit=MEM32(0x3f4244);
   if(g_eax<limit){MEM32(0x3f4240)=g_eax;MEM32(g_eax-8)=g_ecx;MEM32(g_eax-4)=g_edx;g_recomp_zf=g_eax==limit;g_esp+=4;return 1;}
   PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x3e70a3);sub_003E9810();POP32(g_esp,g_ecx);POP32(g_esp,g_edx);
  }
 }
 PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f4238);
 if(operation==5){PUSH32(g_esp,0x3ef70c);sub_003EE550();}
 g_eax=MEM32(g_esi);int grow=g_eax>=MEM32(g_esi+4);
 if(operation==1||operation==4)PUSH32(g_esp,g_edi);
 if(grow){
  static const uint32_t returns[]={0,0x3e7414,0x3e7463,0x3ef613,0x3ef6c4,0x3ef718,0x3ef753};
  PUSH32(g_esp,returns[operation]);sub_003E9810();
 }
 switch(operation){
 case 1: /* Fog color: preserve the unswizzled cached value. */
  g_ecx=MEM32(g_esp+12);g_edx=MEM8(g_esp+14);g_edi=(g_ecx&255)<<16;g_edx|=g_edi;g_edi=g_ecx&0xff00ff00;
  MEM32(g_eax)=0x402a8;g_edx|=g_edi;MEM32(g_eax+4)=g_edx;g_eax+=8;MEM32(g_esi)=g_eax;POP32(g_esp,g_edi);MEM32(0x3f41c8)=g_ecx;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;break;
 case 2: /* Cull enable and face selection. */
  g_ecx=MEM32(g_esp+8);MEM32(g_eax)=0x40308;
  if(!g_ecx){MEM32(g_eax+4)=g_ecx;g_eax+=8;MEM32(g_esi)=g_eax;MEM32(0x3f41ec)=g_ecx;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;}
  else {
   MEM32(g_eax+4)=1;PUSH32(g_esp,g_edi);g_edi=MEM32(0x3f41e8);g_edx=g_ecx!=g_edi;MEM32(g_eax+8)=0x4039c;g_eax+=16;POP32(g_esp,g_edi);g_edx+=0x404;MEM32(g_eax-4)=g_edx;MEM32(g_esi)=g_eax;MEM32(0x3f41ec)=g_ecx;POP32(g_esp,g_esi);g_recomp_zf=g_edx==0;
  }
  g_esp+=8;break;
 case 3: /* Four component immediate attribute. Trace entry stays in caller. */
  g_ecx=MEM32(g_esp+8);g_ecx=g_ecx==0xffffffffu?0x1518:(g_ecx+0x1a0)<<4;
  g_edx=MEM32(g_esp+16);g_ecx+=0x100000;MEM32(g_eax)=g_ecx;g_ecx=MEM32(g_esp+12);MEM32(g_eax+4)=g_ecx;g_ecx=MEM32(g_esp+20);MEM32(g_eax+8)=g_edx;g_edx=MEM32(g_esp+24);MEM32(g_eax+12)=g_ecx;MEM32(g_eax+16)=g_edx;
  g_eax+=20;MEM32(g_esi)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;RECOMP_TRACE_EXIT("sub_003EF600",0x003ef600);g_esp+=24;break;
 case 4: /* Packed immediate color. */
  g_ecx=MEM32(g_esp+12);g_edx=g_ecx*4+0x41940;g_ecx=MEM32(g_esp+16);MEM32(g_eax)=g_edx;g_edx=MEM8(g_esp+18);g_edi=(g_ecx&255)<<16;g_edx|=g_edi;g_ecx&=0xff00ff00;g_edx|=g_ecx;MEM32(g_eax+4)=g_edx;
  g_eax+=8;POP32(g_esp,g_edi);MEM32(g_esi)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=12;break;
 case 5: /* Begin primitive; dirty preparation precedes command allocation. */
  g_ecx=MEM32(g_esp+8);MEM32(g_eax)=0x417fc;MEM32(g_eax+4)=g_ecx;g_eax+=8;MEM32(g_esi)=g_eax;MEM32(g_esi+8)|=0x800;POP32(g_esp,g_esi);g_esp+=8;break;
 case 6: { /* End primitive; flush sees the cleared state and restored ESI. */
  MEM32(g_eax)=0x417fc;MEM32(g_eax+4)=0;g_eax+=8;MEM32(g_esi)=g_eax;g_eax=MEM32(g_esi+8);g_ecx=g_eax&0xffffe7ff;int no_flush=!(g_eax&0x1000);
  MEM32(g_esi+8)=g_ecx;POP32(g_esp,g_esi);if(!no_flush){PUSH32(g_esp,1);PUSH32(g_esp,0x3ef780);sub_003E94A0();}g_recomp_zf=no_flush;g_esp+=4;break;
 }
 }
 return 1;
}
