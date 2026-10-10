#include "migration_interface_extension.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>
extern void sub_00084C82(void),sub_00084CB3(void),sub_00080974(void),sub_000809DB(void);
#define EXT_CALL(fn,ret) do{PUSH32(g_esp,ret);fn();}while(0)
static int extension_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_EXTENSION");v=s&&!strcmp(s,"1");}return v;}
static int extension_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int extension_apart(uint64_t p,unsigned n,uint64_t s,unsigned m){return p+n<=s||s+m<=p;}
static void extension_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;}
static void extension_init(void){xbox_XcSHAInit((PXBOX_SHA_CONTEXT)XBOX_PTR(MEM32(g_esp+4)));g_eax=0;g_esp+=8;}
static void extension_update(void){xbox_XcSHAUpdate((PXBOX_SHA_CONTEXT)XBOX_PTR(MEM32(g_esp+4)),(const UCHAR*)XBOX_PTR(MEM32(g_esp+8)),MEM32(g_esp+12));g_eax=0;g_esp+=16;}
static void extension_final(void){xbox_XcSHAFinal((PXBOX_SHA_CONTEXT)XBOX_PTR(MEM32(g_esp+4)),(UCHAR*)XBOX_PTR(MEM32(g_esp+8)));g_eax=0;g_esp+=12;}
int migration_interface_extension(unsigned op){
 if(!extension_enabled()||op>2)return 0;
 uint32_t s=g_esp;if(s<0x10200||(s&3)||!extension_span(s-512,532)||!extension_span(0x519c50,4))return 0;
 uint32_t manager=MEM32(0x516b84);
 if(manager&&!extension_span(manager,0x724))return 0;
 if(op==0){uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;if(!extension_span(tls,4)||!extension_span((uint64_t)MEM32((uint32_t)tls)+8,4))return 0;}
 if(op&&manager&&MEM32(manager+0x720)){
  uint32_t object=MEM32(s+4),buffer=MEM32(s+8),length=op==1?MEM32(s+12):100;
  if(!extension_span(object,124)||!extension_apart(object,124,s-512,532)||(buffer&&(!extension_span(buffer,length)||!extension_apart(buffer,length,s-512,532)))||(op==1&&!buffer&&length))return 0;
  if(op==2&&!extension_span(MEM32(object+4),20))return 0;
 }
 if(op==0){
  g_eax=manager;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);unsigned error=0;
  if(!g_eax)error=21;else{g_esi=MEM32(g_eax+0x720);g_eax+=0x6bc;g_esi=g_esi?g_eax:0;if(!g_esi)error=21;}
  if(!error){PUSH32(g_esp,124);PUSH32(g_esp,0);EXT_CALL(sub_00084C82,0x3cf6a6);g_edi=g_eax;if(!g_edi)error=8;}
  if(error){PUSH32(g_esp,error);EXT_CALL(extension_error,0x3cf6b3);g_eax=UINT32_MAX;}
  else{g_eax=MEM32(g_esp+12);MEM32(g_edi)=g_eax;g_eax=g_edi+8;g_esi+=80;PUSH32(g_esp,g_eax);MEM32(g_edi+4)=g_esi;EXT_CALL(extension_init,0x3cf6cd);g_eax=g_edi;}
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;return 1;
 }
 if(op==1){
  g_eax=manager;int ready=0;if(g_eax){g_ecx=MEM32(g_eax+0x720);g_eax+=0x6bc;g_ecx=g_ecx?UINT32_MAX:0;ready=(g_eax&g_ecx)!=0;}
  if(!ready){PUSH32(g_esp,21);POP32(g_esp,g_eax);}else{PUSH32(g_esp,MEM32(g_esp+12));g_eax=MEM32(g_esp+8);PUSH32(g_esp,MEM32(g_esp+12));g_eax+=8;PUSH32(g_esp,g_eax);EXT_CALL(extension_update,0x3cf70a);g_eax=0;}g_esp+=16;return 1;
 }
 PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;g_esp-=20;g_eax=manager;
 if(!g_eax){PUSH32(g_esp,21);POP32(g_esp,g_eax);}
 else{
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax+0x720);g_eax+=0x6bc;g_esi=g_esi?g_eax:0;
  if(!g_esi){PUSH32(g_esp,21);POP32(g_esp,g_eax);}
  else{
   PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+8);PUSH32(g_esp,g_edi);g_edi=MEM32(frame+12);
   if(g_edi){PUSH32(g_esp,25);POP32(g_esp,g_ecx);g_eax=frame-20;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),100);g_esi+=100;g_edi+=100;g_ecx=0;
    PUSH32(g_esp,g_eax);g_esi=g_ebx+8;PUSH32(g_esp,g_esi);g_ebp=frame;EXT_CALL(extension_final,0x3cf75a);
    PUSH32(g_esp,g_esi);PUSH32(g_esp,20);PUSH32(g_esp,MEM32(g_ebx+4));g_ebp=frame;EXT_CALL(sub_00080974,0x3cf765);
    PUSH32(g_esp,20);g_eax=frame-20;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ebp=frame;EXT_CALL(extension_update,0x3cf771);
    g_eax=MEM32(frame+12)+80;PUSH32(g_esp,g_eax);PUSH32(g_esp,20);PUSH32(g_esp,MEM32(g_ebx+4));PUSH32(g_esp,g_esi);g_ebp=frame;EXT_CALL(sub_000809DB,0x3cf783);
   }
   PUSH32(g_esp,g_ebx);g_ebp=frame;EXT_CALL(sub_00084CB3,0x3cf789);POP32(g_esp,g_edi);g_eax=0;POP32(g_esp,g_ebx);
  }
  POP32(g_esp,g_esi);
 }
 g_esp=frame+16;return 1;
}
#undef EXT_CALL
