#include "migration_graphics_material_vector.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00090860(void),sub_000907F0(void),sub_002D57FB(void),sub_002D5990(void),sub_000A71F0(void),sub_002D5CCD(void),sub_0008EBF0(void),sub_000BE430(void),sub_000A71C0(void);
static int vector_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MATERIAL_VECTOR");flag=v&&!strcmp(v,"1");}return flag;}
static int vector_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define VCALL(fn,pc) do{PUSH32(esp,pc);fn();}while(0)
static uint32_t vector_length(uint32_t end,uint32_t begin){return (uint32_t)((int32_t)(end-begin)>>2);}
int uc_migration_graphics_material_vector(uint32_t entry){
 if(!vector_enabled()||(entry!=0x90860&&entry!=0x90a40))return 0;
 uint32_t s=esp;if((s&3)||s<0x10100||!vector_span(s-128,148)||!vector_span(ecx,16)||!vector_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t begin=MEM32(ecx+4),end=MEM32(ecx+8),limit=MEM32(ecx+12),amount=MEM32(s+(entry==0x90860?8:4));
 if(amount>4096||(begin&&((begin|end|limit)&3||end<begin||limit<end||!vector_span(begin,(uint64_t)limit-begin))))return 0;
 if(entry==0x90860&&(!vector_span(MEM32(s+12),4)||MEM32(s+4)<begin||MEM32(s+4)>end))return 0;
 if(entry==0x90a40){
  PUSH32(esp,esi);esi=ecx;edx=MEM32(esi+4);PUSH32(esp,edi);eax=edx?vector_length(MEM32(esi+8),edx):0;edi=MEM32(esp+12);
  if(eax<edi){eax=edx?vector_length(MEM32(esi+8),edx):0;ecx=MEM32(esi+8);edx=esp+16;PUSH32(esp,edx);edi-=eax;PUSH32(esp,edi);PUSH32(esp,ecx);ecx=esi;VCALL(sub_00090860,0x90a82);POP32(esp,edi);POP32(esp,esi);g_recomp_zf=edi==0;esp+=12;return 1;}
  if(edx){eax=MEM32(esi+8);ecx=vector_length(eax,edx);if(edi<ecx){edx+=edi*4;if(edx!=eax){ecx=0;edi=ecx*4;PUSH32(esp,edi);PUSH32(esp,eax);PUSH32(esp,edx);VCALL(sub_002D5990,0x90ab6);esp+=12;eax+=edi;MEM32(esi+8)=eax;}}}
  POP32(esp,edi);POP32(esp,esi);esp+=12;return 1;
 }
 uint32_t frame=g_seh_ebp;PUSH32(esp,ecx);eax=MEM32(esp+16);PUSH32(esp,esi);esi=ecx;edx=MEM32(esi+4);ecx=MEM32(eax);PUSH32(esp,edi);MEM32(esp+24)=ecx;eax=edx?vector_length(MEM32(esi+12),edx):0;edi=MEM32(esp+20);
 if(edi){
  ecx=edx?vector_length(MEM32(esi+8),edx):0;PUSH32(esp,ebx);ebx=0x3fffffff-ecx;if(ebx<edi){ecx=esi;VCALL(sub_000907F0,0x908b1);}
  ecx=edx?vector_length(MEM32(esi+8),edx):0;ecx+=edi;PUSH32(esp,frame);
  if(eax<ecx){
   ecx=eax>>1;ebx=0x3fffffff-ecx;if(ebx<eax)eax=0;else eax+=ecx;
   ecx=edx?vector_length(MEM32(esi+8),edx):0;ecx+=edi;if(eax<ecx){eax=edx?vector_length(MEM32(esi+8),edx):0;eax+=edi;}
   eax<<=2;PUSH32(esp,eax);MEM32(esp+32)=eax;VCALL(sub_002D57FB,0x90916);
   ecx=MEM32(esi+4);ebx=MEM32(esp+28);edx=eax;eax=vector_length(ebx,ecx);frame=eax*4;PUSH32(esp,frame);PUSH32(esp,ecx);PUSH32(esp,edx);MEM32(esp+32)=edx;VCALL(sub_002D5990,0x90939);
   esp+=16;edx=esp+32;PUSH32(esp,edx);eax+=frame;PUSH32(esp,edi);PUSH32(esp,eax);ecx=esi;VCALL(sub_000A71F0,0x9094c);
   ecx=vector_length(MEM32(esi+8),ebx);ecx<<=2;PUSH32(esp,ecx);PUSH32(esp,ebx);PUSH32(esp,eax);VCALL(sub_002D5990,0x9095f);
   eax=MEM32(esi+4);esp+=12;ecx=eax?vector_length(MEM32(esi+8),eax):0;edi+=ecx;if(eax){PUSH32(esp,eax);VCALL(sub_002D5CCD,0x90981);esp+=4;}
   eax=MEM32(esp+16);edx=MEM32(esp+28);POP32(esp,frame);POP32(esp,ebx);ecx=eax+edi*4;edx+=eax;POP32(esp,edi);MEM32(esi+12)=edx;MEM32(esi+8)=ecx;MEM32(esi+4)=eax;POP32(esp,esi);POP32(esp,ecx);g_recomp_zf=edx==0;esp+=16;return 1;
  }
  frame=MEM32(esi+8);ebx=MEM32(esp+24);edx=vector_length(frame,ebx);eax=edi*4;ecx=esi;MEM32(esp+24)=eax;
  if(edx<edi){
   eax+=ebx;PUSH32(esp,eax);PUSH32(esp,frame);PUSH32(esp,ebx);VCALL(sub_0008EBF0,0x909cb);
   eax=MEM32(esi+8);edx=eax-ebx;ecx=esp+32;PUSH32(esp,ecx);edx=(uint32_t)((int32_t)edx>>2);edi-=edx;PUSH32(esp,edi);PUSH32(esp,eax);ecx=esi;VCALL(sub_000A71F0,0x909e5);
   ecx=MEM32(esi+8);eax=MEM32(esp+24);ecx+=eax;MEM32(esi+8)=ecx;esi=ecx;ecx=esp+32;PUSH32(esp,ecx);esi-=eax;PUSH32(esp,esi);PUSH32(esp,ebx);VCALL(sub_000BE430,0x90a01);
   esp+=12;POP32(esp,frame);POP32(esp,ebx);POP32(esp,edi);POP32(esp,esi);POP32(esp,ecx);g_recomp_zf=esp==0;esp+=16;return 1;
  }
  PUSH32(esp,frame);edi=frame-eax;PUSH32(esp,frame);PUSH32(esp,edi);VCALL(sub_0008EBF0,0x90a18);
  PUSH32(esp,frame);PUSH32(esp,edi);PUSH32(esp,ebx);MEM32(esi+8)=eax;VCALL(sub_000A71C0,0x90a23);
  eax=MEM32(esp+36);edx=esp+44;PUSH32(esp,edx);eax+=ebx;PUSH32(esp,eax);PUSH32(esp,ebx);VCALL(sub_000BE430,0x90a35);esp+=24;POP32(esp,frame);POP32(esp,ebx);
 }
 POP32(esp,edi);POP32(esp,esi);POP32(esp,ecx);esp+=16;return 1;
}
#undef VCALL
