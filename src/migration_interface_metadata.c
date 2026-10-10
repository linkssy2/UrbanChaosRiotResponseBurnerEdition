#include "migration_interface_metadata.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00080D27(void),sub_00080F6E(void);
extern void sub_002D7774(void),sub_002D7E55(void),sub_002D5990(void),sub_002D77A6(void);
#define META_CALL(fn,ret) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
static int metadata_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_METADATA");v=s&&!strcmp(s,"1");}return v;}
static int metadata_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int metadata_apart(uint64_t p,uint64_t n,uint64_t s,unsigned m){return p+n<=s||s+m<=p;}
int migration_interface_metadata(unsigned op){
 if(op||!metadata_enabled())return 0;
 uint32_t s=g_esp;if(s<0x10400||(s&3)||!metadata_span(s-1024,1048))return 0;
 uint32_t name=MEM32(s+8),output=MEM32(s+12),capacity=MEM32(s+16),length=0;
 if(!capacity||capacity>0x10000||!metadata_span(output,capacity*2)||!metadata_apart(output,capacity*2,s-1024,1048))return 0;
 while(length<128&&metadata_span((uint64_t)name+length*2,2)&&MEM16(name+length*2))length++;
 if(length==128||!metadata_span((uint64_t)name+length*2,2)||!metadata_apart(name,length*2+2,s-1024,1048)||!metadata_span(0x455674,6))return 0;
 if(MEM16(0x455674)!=13||MEM16(0x455676)!=10||MEM16(0x455678)!=0)return 0;
 PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp,buffer=frame-304;g_ebp=frame;g_esp-=304;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 PUSH32(g_esp,MEM32(frame+12));g_esi=0;MEM32(frame-16)=0;g_edi=buffer;MEM32(frame-4)=140;MEM32(frame-8)=0;META_CALL(sub_002D7774,0x81808);POP32(g_esp,g_ecx);MEM32(frame-20)=g_eax;MEM16(frame-24)=LO16(g_esi);
 for(;;){
  g_ebx=0;
  if(!MEM32(frame-8)){
   PUSH32(g_esp,g_ebx);g_eax=frame-12;PUSH32(g_esp,g_eax);g_eax=MEM32(frame-4)*2;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,MEM32(frame+8));META_CALL(sub_00080D27,0x8182b);
   if(!g_eax||!MEM32(frame-12))break;
  }
  MEM32(frame-8)=0;g_ecx=buffer;g_eax=g_edi-g_ecx;g_ecx=MEM32(frame-12);g_eax=(uint32_t)((int32_t)g_eax>>1);g_ebx=g_ecx+g_eax*2;g_eax=buffer;
  if(g_esi){
   PUSH32(g_esp,0x455674);PUSH32(g_esp,g_eax);META_CALL(sub_002D7E55,0x81867);POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);g_edi=buffer;MEM32(frame-4)=140;
   if(!g_eax)continue;
   g_ecx=g_edi;g_eax=g_eax-g_ecx+4;
   if(g_eax<g_ebx){g_ebx-=g_eax;g_esi=g_ebx;g_eax=buffer+g_eax;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_eax=g_edi;PUSH32(g_esp,g_eax);META_CALL(sub_002D5990,0x8189a);g_edi=buffer+g_esi;g_esi>>=1;g_eax=140;g_esp+=12;g_eax-=g_esi;MEM32(frame-4)=g_eax;}
   g_esi=0;continue;
  }
  PUSH32(g_esp,MEM32(frame+12));PUSH32(g_esp,g_eax);META_CALL(sub_002D7E55,0x818c0);POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
  int match=0;if(g_eax){g_ecx=MEM32(frame-20)*2;match=MEM16(g_ecx+g_eax)==61;}
  if(!match){g_eax=1;g_esi=g_eax;MEM32(frame-8)=g_eax;continue;}
  g_edi=g_eax+g_ecx+2;PUSH32(g_esp,0x455674);PUSH32(g_esp,g_edi);MEM32(frame-4)=g_edi;META_CALL(sub_002D7E55,0x818e4);g_esi=g_eax;POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
  if(!g_esi){g_esi=1;g_edi=buffer;MEM32(frame-4)=140;continue;}
  g_eax=g_esi-g_edi;g_edi=MEM32(frame+20);g_eax=(uint32_t)((int32_t)g_eax>>1);g_edi--;if((int32_t)g_edi>=(int32_t)g_eax)g_edi=g_eax;
  PUSH32(g_esp,g_edi);PUSH32(g_esp,MEM32(frame-4));PUSH32(g_esp,MEM32(frame+16));META_CALL(sub_002D77A6,0x81928);
  g_eax=MEM32(frame+16);MEM16(g_eax+g_edi*2)=0;g_eax=buffer;g_esi=g_esi-g_eax+4;g_esp+=12;
  if(g_esi<g_ebx){PUSH32(g_esp,1);PUSH32(g_esp,0);g_esi-=g_ebx;PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(frame+8));META_CALL(sub_00080F6E,0x81951);}
  MEM32(frame-16)=1;break;
 }
 g_eax=MEM32(frame-16);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+24;return 1;
}
#undef META_CALL
