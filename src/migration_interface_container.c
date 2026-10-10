#include "migration_interface_container.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00085B2C(void),sub_00085C39(void),sub_00081265(void),sub_00081221(void),sub_00081797(void),sub_000817DF(void),sub_00082528(void),sub_000825C8(void),sub_00084CD1(void);
#define CONTAINER_CALL(fn,ret) do{PUSH32(g_esp,ret);fn();}while(0)
static int container_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_CONTAINER");v=s&&!strcmp(s,"1");}return v;}
static int container_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int container_apart(uint64_t p,unsigned n,uint64_t s,unsigned m){return p+n<=s||s+m<=p;}
static int container_string(uint32_t p,unsigned max){unsigned n=0;while(n<max&&container_span((uint64_t)p+n,1)&&MEM8(p+n))n++;return n<max&&container_span((uint64_t)p+n,1)?(int)n:-1;}
static void container_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;}
static void container_validate(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;PUSH32(g_esp,g_ecx);MEM32(frame-4)=0;PUSH32(g_esp,g_esi);g_esi=MEM32(frame+8);
 if(!(MEM32(g_esi)&16))g_eax=0;
 else{
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);g_edi=g_esi+44;g_eax=g_edi;g_ecx=g_eax+1;
  do{SET_LO8(g_edx,MEM8(g_eax));g_eax++;}while(LO8(g_edx));g_eax-=g_ecx;g_ecx=MEM32(frame+12);g_edx=g_esi+320-g_ecx;
  do{SET_LO8(g_ebx,MEM8(g_ecx));MEM8(g_edx+g_ecx)=LO8(g_ebx);g_ecx++;}while(LO8(g_ebx));
  g_edx=MEM32(frame+16);g_ecx=g_esi+g_edx+320-g_edi;
  do{SET_LO8(g_ebx,MEM8(g_edi));MEM8(g_ecx+g_edi)=LO8(g_ebx);g_edi++;}while(LO8(g_ebx));
  g_edi=g_eax+g_edx;
  if(g_edi<259){g_ecx=g_eax+g_esi+g_edx;MEM8(g_ecx+320)=92;MEM8(g_ecx+321)=LO8(g_ebx);}
  if((int32_t)g_edi<247){
   g_eax+=g_esi;g_ebx=g_eax+g_edx;g_ecx=0x4555bc;g_eax=g_ebx+320-g_ecx;
   do{SET_LO8(g_edx,MEM8(g_ecx));MEM8(g_eax+g_ecx)=LO8(g_edx);g_ecx++;}while(LO8(g_edx));
   PUSH32(g_esp,0);g_edi=128;PUSH32(g_esp,g_edi);PUSH32(g_esp,3);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,0x80000000);g_eax=g_esi+320;PUSH32(g_esp,g_eax);g_ebp=frame;CONTAINER_CALL(sub_00081265,0x81a1b);MEM32(frame+8)=g_eax;
   if(g_eax!=UINT32_MAX){
    PUSH32(g_esp,1);PUSH32(g_esp,g_eax);g_ebp=frame;CONTAINER_CALL(sub_00081797,0x81a2b);MEM32(frame-4)=g_eax;
    if(g_eax){PUSH32(g_esp,g_edi);g_esi+=580;PUSH32(g_esp,g_esi);PUSH32(g_esp,0x455624);PUSH32(g_esp,MEM32(frame+8));g_ebp=frame;CONTAINER_CALL(sub_000817DF,0x81a47);MEM32(frame-4)=g_eax;}
    PUSH32(g_esp,MEM32(frame+8));g_ebp=frame;CONTAINER_CALL(sub_00081221,0x81a52);
   }
   MEM8(g_ebx+321)=0;
  }
  g_eax=MEM32(frame-4);POP32(g_esp,g_edi);POP32(g_esp,g_ebx);
 }
 POP32(g_esp,g_esi);g_esp=frame+20;
}
static void container_first(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp-=260;g_eax=MEM32(frame+8);g_edx=frame-260-g_eax;
 do{SET_LO8(g_ecx,MEM8(g_eax));MEM8(g_edx+g_eax)=LO8(g_ecx);g_eax++;}while(LO8(g_ecx));
 g_eax=frame-260;g_edx=g_eax+1;do{SET_LO8(g_ecx,MEM8(g_eax));g_eax++;}while(LO8(g_ecx));g_eax-=g_edx;
 PUSH32(g_esp,g_ebx);g_ebx=g_eax;PUSH32(g_esp,g_esi);g_eax=0x455644;g_ecx=frame+g_ebx-260;PUSH32(g_esp,g_edi);g_ecx-=g_eax;
 do{SET_LO8(g_edx,MEM8(g_eax));MEM8(g_ecx+g_eax)=LO8(g_edx);g_eax++;}while(LO8(g_edx));
 g_esi=MEM32(frame+12);PUSH32(g_esp,g_esi);g_eax=frame-260;PUSH32(g_esp,g_eax);g_ebp=frame;CONTAINER_CALL(sub_00085B2C,0x81dff);g_edi=g_eax;
 if(g_edi!=UINT32_MAX){
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(frame+8));PUSH32(g_esp,g_esi);g_ebp=frame;CONTAINER_CALL(container_validate,0x81e14);
  if(!g_eax){
   PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_ebp=frame;CONTAINER_CALL(sub_00085C39,0x81e1f);MEM32(frame+12)=g_eax;
   while(g_eax){
    PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(frame+8));PUSH32(g_esp,g_esi);g_ebp=frame;CONTAINER_CALL(container_validate,0x81e30);if(g_eax)break;
    PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_ebp=frame;CONTAINER_CALL(sub_00085C39,0x81e3b);MEM32(frame+12)=g_eax;
   }
   if(!MEM32(frame+12)){PUSH32(g_esp,g_edi);g_ebp=frame;CONTAINER_CALL(sub_00081221,0x81e4e);PUSH32(g_esp,18);g_edi=UINT32_MAX;g_ebp=frame;CONTAINER_CALL(container_error,0x81e58);}
  }
  if(g_edi!=UINT32_MAX){
   PUSH32(g_esp,0x24830000);PUSH32(g_esp,272);g_ebp=frame;CONTAINER_CALL(sub_00082528,0x81e6c);g_esi=g_eax;
   if(g_esi){PUSH32(g_esp,260);PUSH32(g_esp,MEM32(frame+8));g_eax=g_esi+12;PUSH32(g_esp,g_eax);MEM32(g_esi)=0x53425645;MEM32(g_esi+4)=g_edi;MEM32(g_esi+8)=g_ebx;g_ebp=frame;CONTAINER_CALL(sub_00084CD1,0x81e8f);g_edi=g_esi;}
   else{PUSH32(g_esp,g_edi);g_ebp=frame;CONTAINER_CALL(sub_00081221,0x81e99);PUSH32(g_esp,8);g_edi=UINT32_MAX;g_ebp=frame;CONTAINER_CALL(container_error,0x81ea3);}
  }
 }
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+16;
}
static void container_next(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);PUSH32(g_esp,MEM32(g_esp+16));PUSH32(g_esp,MEM32(g_esi+4));CONTAINER_CALL(sub_00085C39,0x81ebe);g_edi=g_eax;
 if(g_edi){PUSH32(g_esp,g_ebx);g_ebx=g_esi+12;
  for(;;){PUSH32(g_esp,MEM32(g_esi+8));PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(g_esp+28));CONTAINER_CALL(container_validate,0x81ed5);if(g_eax)break;PUSH32(g_esp,MEM32(g_esp+20));PUSH32(g_esp,MEM32(g_esi+4));CONTAINER_CALL(sub_00085C39,0x81ee5);g_edi=g_eax;if(!g_edi)break;}
  POP32(g_esp,g_ebx);
 }
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=12;
}
static void container_close(void){
 g_ecx=MEM32(g_esp+4);g_edx=MEM32(g_ecx);g_eax=0;PUSH32(g_esp,g_esi);
 if(g_edx==0x4e49434b){PUSH32(g_esp,0x24830000);PUSH32(g_esp,g_ecx);CONTAINER_CALL(sub_000825C8,0x81f2c);g_eax=1;}
 else if(g_edx==0x434f4e54||g_edx==0x53425645||g_edx==0xbe4bea00){g_esi=MEM32(g_ecx+(g_edx==0xbe4bea00?12:4));PUSH32(g_esp,0x24830000);PUSH32(g_esp,g_ecx);CONTAINER_CALL(sub_000825C8,0x81f3f);PUSH32(g_esp,g_esi);CONTAINER_CALL(sub_00081221,0x81f45);}
 POP32(g_esp,g_esi);g_esp+=8;
}
int migration_interface_container(unsigned op){
 if(!container_enabled()||op>3)return 0;uint32_t s=g_esp;if(s<0x10800||(s&3)||!container_span(s-2048,2068)||!container_span(0x519c50,4))return 0;
 uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;if(!container_span(tls,4)||!container_span((uint64_t)MEM32((uint32_t)tls)+8,4))return 0;
 uint32_t input=MEM32(s+4),output=MEM32(s+8);
 if(op==2){if(!container_span(input,16)||!container_apart(input,16,s-2048,2068))return 0;container_close();return 1;}
 if(op==3){if(!container_span(input,836)||!container_apart(input,836,s-2048,2068)||MEM32(s+12)>240||container_string(output,241)<0||!container_apart(output,241,input,836)||container_string(input+44,261)<0)return 0;}
 else{if(!container_span(output,836)||!container_apart(output,836,s-2048,2068))return 0;
  if(op==0){if(container_string(input,241)<0||!container_apart(input,241,s-2048,2068))return 0;}
  else if(!container_span(input,272)||!container_apart(input,272,s-2048,2068)||MEM32(input+8)>240||container_string(input+12,241)<0)return 0;
 }
 if(container_string(0x4555bc,14)<0||container_string(0x455644,8)<0||!container_span(0x455624,16))return 0;
 if(op==0)container_first();else if(op==1)container_next();else container_validate();return 1;
}
#undef CONTAINER_CALL
