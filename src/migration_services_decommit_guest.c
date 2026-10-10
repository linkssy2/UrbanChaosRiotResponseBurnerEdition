#include "migration_services_decommit_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_000825E2(void),sub_00082702(void),sub_00082723(void),sub_00082C18(void);
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static void decommit_pages(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;g_esp-=36;PUSH32(g_esp,g_ebx);g_ebx=MEM32(f+8);
 if(MEM32(g_ebx+0x584)){PUSH32(g_esp,MEM32(f+16));PUSH32(g_esp,MEM32(f+12));PUSH32(g_esp,g_ebx);g_ebp=f;PUSH32(g_esp,0x834ae);sub_00082C18();goto done;}
 MEM32(f-16)=0;PUSH32(g_esp,g_esi);g_esi=MEM32(f+12);g_eax=MEM8(g_esi+4);g_eax=MEM32(g_ebx+g_eax*4+0x60);MEM32(f+8)=g_eax;
 g_eax=(g_esi+0xfff)&~0xfffu;g_edx=0xfffff000;g_ecx=(uint32_t)((int32_t)(g_eax-g_esi)>>4);PUSH32(g_esp,g_edi);MEM32(f-4)=g_eax;MEM32(f-8)=g_ecx;
 if(LO16(g_ecx)==1){g_eax+=0x1000;MEM32(f-4)=g_eax;MEM32(f-8)=0x101;}
 else{SET_LO16(g_ecx,MEM16(g_esi+2));if(LO16(g_ecx)&&g_eax==g_esi){g_ecx=LO16(g_ecx)<<4;g_edi=g_ecx;g_ecx=g_esi-g_edi;MEM32(f-16)=g_ecx;}}
 g_edi=MEM32(f+16);MEM32(f-24)=0;g_edi=(g_edi<<4)+g_esi;g_ecx=g_edi&g_edx;MEM32(f+12)=g_ecx;g_ecx=(uint32_t)((int32_t)(g_edi-MEM32(f+12))>>4);MEM32(f-12)=g_ecx;
 if(LO16(g_ecx)==1){MEM32(f+12)-=0x1000;MEM32(f-12)=0x101;g_ecx=MEM32(f-12);}
 else if(!LO16(g_ecx)&&!(MEM8(g_esi+5)&16))MEM32(f-24)=g_edi;
 g_ecx=LO16(g_ecx);MEM32(f-20)=g_ecx;g_ecx<<=4;g_edi-=g_ecx;MEM32(f-36)=g_ecx;
 MEM32(f+12)=MEM32(f+12)>g_eax?MEM32(f+12)-g_eax:0;
 if(!MEM32(f+12))goto insert_whole;
 PUSH32(g_esp,MEM32(f+8));g_ebp=f;PUSH32(g_esp,0x83587);sub_000825E2();MEM32(f-28)=g_eax;if(!g_eax)goto insert_whole;
 {uint32_t saved=g_esp;PUSH32(g_esp,0x4000);g_eax=f+12;PUSH32(g_esp,g_eax);g_eax=f-4;PUSH32(g_esp,g_eax);uint32_t target=MEM32(0x44b3dc);PUSH32(g_esp,0x835a5);RECOMP_ICALL_SAFE(target,saved);}
 PUSH32(g_esp,MEM32(f-28));MEM32(f-32)=g_eax;PUSH32(g_esp,MEM32(f+8));g_ebp=f;PUSH32(g_esp,0x835b3);sub_00082702();if((int32_t)MEM32(f-32)<0)goto insert_whole;
 PUSH32(g_esp,MEM32(f+12));PUSH32(g_esp,MEM32(f-4));PUSH32(g_esp,MEM32(f+8));g_ebp=f;PUSH32(g_esp,0x835cb);sub_00082723();
 g_eax=MEM32(f+12);g_edx=MEM32(f+8);g_ecx=MEM32(f-8);g_eax>>=12;MEM32(g_edx+0x30)+=g_eax;
 if(LO16(g_ecx)){
  g_eax=LO16(g_ecx);MEM8(g_esi+5)=16;MEM16(g_esi)=LO16(g_ecx);MEM32(g_ebx+0x30)+=g_eax;MEM32(g_edx+0x40)=g_esi;MEM8(g_esi+5)&=0xf8;
  if(LO16(g_ecx)<128){
   g_eax=g_ebx+g_eax*8+0x180;MEM32(f+8)=g_eax;
   if(MEM32(g_eax)==g_eax){g_ecx=MEM16(g_esi);g_eax=(g_ecx>>3)+g_ebx+0x160;MEM32(f-32)=g_eax;g_ecx&=7;SET_LO8(g_eax,1u<<LO8(g_ecx));SET_LO8(g_ecx,LO8(g_eax));g_eax=MEM32(f-32);MEM8(g_eax)|=LO8(g_ecx);g_eax=MEM32(f+8);}
  }else{
   g_ecx=g_ebx+0x180;g_eax=MEM32(g_ecx);
   while(g_ecx!=g_eax){SET_LO16(g_ecx,MEM16(f-8));if(LO16(g_ecx)<=MEM16(g_eax-8))break;g_eax=MEM32(g_eax);g_ecx=g_ebx+0x180;}
   MEM32(f+8)=g_eax;g_eax=MEM32(f+8);
  }
  g_ecx=MEM32(g_eax+4);MEM32(f+16)=g_ecx;g_ecx=g_esi+8;MEM32(g_ecx)=g_eax;MEM32(f+8)=g_ecx;g_ecx=MEM32(f+16);MEM32(g_esi+12)=g_ecx;g_esi=MEM32(f+8);MEM32(g_ecx)=g_esi;MEM32(g_eax+4)=g_esi;
 }else{
  g_eax=MEM32(f-16);
  if(g_eax){MEM8(g_eax+5)|=16;MEM32(g_edx+0x40)=g_eax;}
  else{g_eax=MEM32(g_edx+0x40);g_ecx=MEM32(f-4);if(g_eax>=g_ecx){g_esi=MEM32(f+12)+g_ecx;if(g_eax<g_esi){g_eax=MEM32(g_edx+0x28);MEM32(g_edx+0x40)=g_eax;}}}
 }
 g_eax=MEM32(f-12);
 if(LO16(g_eax)){
  MEM16(g_edi+2)=0;SET_LO8(g_ecx,MEM8(g_edx+4));MEM8(g_edi+4)=LO8(g_ecx);g_ecx=MEM32(f-36);MEM8(g_edi+5)=0;MEM16(g_edi)=LO16(g_eax);MEM16(g_ecx+g_edi+2)=LO16(g_eax);
  if(LO16(g_eax)<128){
   g_eax=MEM32(f-20);g_esi=g_ebx+g_eax*8+0x180;
   if(MEM32(g_esi)==g_esi){g_ecx=MEM16(g_edi);g_eax=g_ecx;g_ecx&=7;SET_LO8(g_edx,1u<<LO8(g_ecx));g_eax=(g_eax>>3)+g_ebx+0x160;MEM8(g_eax)|=LO8(g_edx);}
  }else{g_ecx=g_ebx+0x180;g_esi=MEM32(g_ecx);while(g_ecx!=g_esi&&LO16(g_eax)>MEM16(g_esi-8))g_esi=MEM32(g_esi);}
  g_ecx=MEM32(g_esi+4);g_eax=g_edi+8;MEM32(g_eax)=g_esi;MEM32(g_edi+12)=g_ecx;MEM32(g_ecx)=g_eax;MEM32(g_esi+4)=g_eax;g_eax=MEM32(f-20);MEM32(g_ebx+0x30)+=g_eax;
 }else{g_eax=MEM32(f-24);if(g_eax)MEM16(g_eax+2)=0;}
 goto restore;
insert_whole:PUSH32(g_esp,MEM32(f+16));PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);g_ebp=f;PUSH32(g_esp,0x83732);sub_00082C18();
restore:POP32(g_esp,g_edi);POP32(g_esp,g_esi);
done:POP32(g_esp,g_ebx);g_esp=f+20;
}
int uc_migration_services_decommit(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_DECOMMIT");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x8348f)return 0;
 uint32_t s=g_esp;if((s&3)||!span(s-256,276)||MEM8(0x51c178))return 0;
 uint32_t heap=MEM32(s+4),block=MEM32(s+8),units=MEM32(s+12);
 if((heap&15)||(block&15)||!span(heap,0x588)||!units||units>0xff00||!span(block,units*16+16)||!apart(heap,0x588,block,units*16+16)||!apart(block,units*16+16,s-256,276))return 0;
 uint32_t seg=MEM32(heap+0x60+4*MEM8(block+4));if(!span(seg,0x44)||MEM32(seg+24)!=heap||MEM32(seg+0x2c)<block+units*16)return 0;
 if(MEM16(block+2)&&!span(block-MEM16(block+2)*16,16))return 0;
 unsigned count=0;
 for(unsigned i=0;i<128;i++){uint32_t head=heap+0x180+i*8,p=head,n=MEM32(head);while(n!=head){if(++count>4096||n<8||!span(n-8,16)||MEM32(n+4)!=p)return 0;p=n;n=MEM32(n);}if(MEM32(head+4)!=p)return 0;}
 decommit_pages();return 1;
}
