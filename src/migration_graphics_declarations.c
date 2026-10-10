#include "migration_graphics_declarations.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int declarations_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_DECLARATIONS");flag=v&&!strcmp(v,"1");}return flag;}
static int declarations_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Decode FVF position, weights, normal, colors and up to four texture sets.
 * The live format table controls texture widths; retain argument-slot scratch. */
static void declaration_format(void){
 uint32_t frame=g_seh_ebp;g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_eax+28;MEM32(g_eax+4)=0;MEM32(g_eax+16)=0;g_edx=g_edi;g_esi=16;
 do{MEM32(g_edx)=2;g_edx+=16;g_esi--;}while(g_esi);
 g_ebx=MEM32(g_esp+20);g_esi=g_ebx&14;g_edx=0;
 if(g_esi==4){MEM32(g_eax+4)|=2;MEM32(g_edi)=0x42;g_edx=16;}
 else if(g_esi){MEM32(g_edi)=0x32;g_edx=12;if(g_esi>=6){g_edx=((g_esi-6)>>1)+1;g_ecx=(g_edx<<4)|2;MEM32(g_eax+40)=12;MEM32(g_eax+44)=g_ecx;g_edx=g_edx*4+12;}}
 if(g_ebx&16){MEM32(g_eax+56)=g_edx;MEM32(g_eax+60)=0x32;g_edx+=12;}
 g_ecx=64;if(g_ebx&64){g_edi=MEM32(g_eax+4)|0x400;MEM32(g_eax+72)=g_edx;MEM32(g_eax+76)=g_ecx;MEM32(g_eax+4)=g_edi;g_edx+=4;}
 if(g_ebx&128){g_esi=MEM32(g_eax+4)|0x800;MEM32(g_eax+88)=g_edx;MEM32(g_eax+92)=g_ecx;MEM32(g_eax+4)=g_esi;g_edx+=4;}
 g_esi=(g_ebx>>8)&15;g_ebx>>=16;MEM32(g_esp+16)=g_ebx;
 if(g_esi){
  g_ecx=0;g_edi=g_eax+0xac;MEM32(g_esp+20)=g_esi;PUSH32(g_esp,frame);
  do{
   g_ebx&=3;g_esi=MEM32(0x3f22c8+g_ebx*4);g_ebx=(g_esi<<4)|2;MEM32(g_edi)=g_ebx;g_ebx=g_esi<<LO8(g_ecx);MEM32(g_edi-4)=g_edx;frame=MEM32(g_eax+16);g_edx+=g_esi*4;g_esi=MEM32(g_esp+24);frame|=g_ebx;g_ebx=MEM32(g_esp+20)>>2;g_edi+=16;g_ecx+=8;g_esi--;MEM32(g_eax+16)=frame;MEM32(g_esp+20)=g_ebx;MEM32(g_esp+24)=g_esi;
  }while(g_esi);
  POP32(g_esp,frame);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;
}
/* Replay the original terminated declaration payload into constant/program
 * caches. Header count1 selects a program destination; other counts copy data. */
static void declaration_cache(void){
 uint32_t frame=g_seh_ebp;g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_esp+8);g_edx=MEM32(0x3f4238);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);g_eax+=0x114;g_ecx<<=4;PUSH32(g_esp,g_esi);g_ecx+=g_edx+0x10a8;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+20);MEM32(g_esp+24)=g_ecx;
 do{
  g_ecx=MEM32(g_eax);g_edx=g_ecx>>18;g_eax+=4;
  if(g_edx==1){g_edi=(MEM32(g_eax)<<4)+0x3f3180;MEM32(g_esp+20)=g_edi;}
  else{
   g_ecx&=0xffff;g_ebx=g_edx*4;int constants=g_ecx==0xb00;g_ecx=g_ebx;g_esi=g_eax;frame=g_ecx;if(constants)g_edi=MEM32(g_esp+24);
   g_ecx>>=2;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=frame&3;
   memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx);g_esi+=g_ecx;g_edi+=g_ecx;g_ecx=0;
   if(constants){g_ecx=MEM32(g_esp+24);g_edi=MEM32(g_esp+20);g_ecx+=g_ebx;MEM32(g_esp+24)=g_ecx;}
   else{g_edi=MEM32(g_esp+20)+g_ebx;MEM32(g_esp+20)=g_edi;}
  }
  g_ecx=MEM32(g_eax+g_edx*4);g_eax+=g_edx*4;
 }while(g_ecx);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=12;
}
int uc_migration_graphics_declarations(uint32_t entry){
 if(!declarations_enabled())return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10100||!declarations_span(s-128,144)||!declarations_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x3e62f0){if(!declarations_span(MEM32(s+4),0x11c)||!declarations_span(0x3f22c8,16)||((MEM32(s+8)>>8)&15)>4)return 0;declaration_format();}
 else if(entry==0x3e6430){if(!declarations_span(MEM32(s+4),0x118)||!declarations_span(0x3f4238,4)||!declarations_span(MEM32(0x3f4238),0x10a8))return 0;declaration_cache();}else return 0;
 return 1;
}
