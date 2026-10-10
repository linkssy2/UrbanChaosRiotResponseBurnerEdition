#include "migration_interface_signing.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>
extern void sub_00082528(void),sub_000825C8(void),sub_00080974(void),sub_000809DB(void);
#define SIGN_CALL(fn,ret) do{PUSH32(g_esp,ret);fn();}while(0)
static int signing_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_SIGNING");v=s&&!strcmp(s,"1");}return v;}
static int signing_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int signing_apart(uint64_t p,unsigned n,uint64_t s,unsigned m){return p+n<=s||s+m<=p;}
static void signing_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;}
static void signing_last_error(void){g_ecx=MEM32(4);g_eax=MEM32(g_ecx+MEM32(0x519c50)*4);g_eax=MEM32(g_eax+8);g_recomp_zf=MEM8(0x24)==2;g_esp+=4;}
static void signing_sha_init(void){xbox_XcSHAInit((PXBOX_SHA_CONTEXT)XBOX_PTR(MEM32(g_esp+4)));g_eax=0;g_esp+=8;}
static void signing_sha_update(void){xbox_XcSHAUpdate((PXBOX_SHA_CONTEXT)XBOX_PTR(MEM32(g_esp+4)),(const UCHAR*)XBOX_PTR(MEM32(g_esp+8)),MEM32(g_esp+12));g_eax=0;g_esp+=16;}
static void signing_sha_final(void){xbox_XcSHAFinal((PXBOX_SHA_CONTEXT)XBOX_PTR(MEM32(g_esp+4)),(UCHAR*)XBOX_PTR(MEM32(g_esp+8)));g_eax=0;g_esp+=12;}
static void signing_platform_key(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;uint32_t saved=g_esp;
 for(unsigned offset=24;offset>=8;offset-=4)PUSH32(g_esp,MEM32(frame+offset));
 uint32_t target=MEM32(0x44b28c);PUSH32(g_esp,0x85da3);RECOMP_ICALL_SAFE(target,saved);
 saved=g_esp;PUSH32(g_esp,g_eax);target=MEM32(0x44b27c);PUSH32(g_esp,0x85daa);RECOMP_ICALL_SAFE(target,saved);g_esp=frame+28;
}
static void signing_begin_hmac(void){
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(0x10118);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x24830000);PUSH32(g_esp,124);SIGN_CALL(sub_00082528,0x80ad7);g_edi=g_eax;
 int error=0;
 if(!g_edi)error=8;
 else{
  g_eax=MEM32(g_esp+16);g_ebx=MEM32(g_esp+20);MEM32(g_edi)=g_eax;
  if(MEM32(g_esi+8)==g_ebx)g_eax=MEM32(0x44b3b0);
  else{
   g_eax=0;g_ecx=g_esi+0x5c;
   for(;;){g_edx=MEM32(g_ecx);if(!g_edx){error=5;break;}if(g_ebx==g_edx)break;g_eax++;g_ecx+=4;if(g_eax>=16)break;}
   if(!error)g_eax=(g_eax<<4)+MEM32(0x44b3b4);
  }
  if(!error){MEM32(g_edi+4)=g_eax;g_eax=g_edi+8;PUSH32(g_esp,g_eax);PUSH32(g_esp,16);PUSH32(g_esp,MEM32(g_edi+4));SIGN_CALL(sub_00080974,0x80b3a);g_eax=g_edi;}
 }
 if(error){PUSH32(g_esp,(uint32_t)error);SIGN_CALL(signing_error,0x80b14);g_eax=UINT32_MAX;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;
}
static void signing_begin_sha(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,0x24830000);PUSH32(g_esp,124);SIGN_CALL(sub_00082528,0x80bb1);g_esi=g_eax;
 if(!g_esi){PUSH32(g_esp,8);SIGN_CALL(signing_error,0x80bbe);g_eax=UINT32_MAX;g_recomp_zf=0;}
 else{MEM32(g_esi+4)=0;g_eax=g_esi+8;PUSH32(g_esp,g_eax);MEM32(g_esi)=2;SIGN_CALL(signing_sha_init,0x80bd6);g_eax=g_esi;}
 POP32(g_esp,g_esi);g_esp+=4;
}
static void signing_update(void){PUSH32(g_esp,MEM32(g_esp+12));g_eax=MEM32(g_esp+8);PUSH32(g_esp,MEM32(g_esp+12));g_eax+=8;PUSH32(g_esp,g_eax);SIGN_CALL(signing_sha_update,0x80bef);g_eax=0;g_recomp_zf=1;g_esp+=16;}
static void signing_finish_sha(void){
 uint32_t digest=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);
 if(digest){PUSH32(g_esp,MEM32(g_esp+12));g_eax=g_esi+8;PUSH32(g_esp,g_eax);SIGN_CALL(signing_sha_final,0x80c0d);}
 PUSH32(g_esp,0x24830000);PUSH32(g_esp,g_esi);SIGN_CALL(sub_000825C8,0x80c18);g_eax=0;POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=12;
}
static void signing_secondary(void){
 PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;g_esp-=264;PUSH32(g_esp,g_esi);g_esi=MEM32(0x44b2a8);SET_LO8(g_eax,0);PUSH32(g_esp,g_edi);
 for(g_ecx=0;g_ecx<16;g_ecx++)SET_LO8(g_eax,LO8(g_eax)|MEM8(g_ecx+g_esi));
 if(!LO8(g_eax)){g_eax=frame-8;PUSH32(g_esp,g_eax);PUSH32(g_esp,256);g_eax=frame-264;PUSH32(g_esp,g_eax);g_eax=frame-4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0xffff);g_ebp=frame;SIGN_CALL(signing_platform_key,0x80a8d);g_esi=frame-210;}
 g_edi=MEM32(frame+8)+8;PUSH32(g_esp,g_edi);PUSH32(g_esp,16);PUSH32(g_esp,g_esi);g_ebp=frame;SIGN_CALL(sub_00080974,0x80aa2);
 PUSH32(g_esp,20);PUSH32(g_esp,MEM32(frame+12));PUSH32(g_esp,g_edi);g_ebp=frame;SIGN_CALL(signing_sha_update,0x80aad);
 PUSH32(g_esp,MEM32(frame+16));PUSH32(g_esp,16);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_ebp=frame;SIGN_CALL(sub_000809DB,0x80ab9);g_eax=1;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp=frame+20;g_recomp_zf=0;
}
static void signing_finish_hmac(void){
 PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;g_esp-=20;g_eax=MEM32(frame+12);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(frame+8);g_esi=MEM32(g_edi)&1;g_ebx=1;
 if(g_eax){if(g_esi)g_eax=frame-20;PUSH32(g_esp,g_eax);PUSH32(g_esp,16);PUSH32(g_esp,MEM32(g_edi+4));g_eax=g_edi+8;PUSH32(g_esp,g_eax);g_ebp=frame;SIGN_CALL(sub_000809DB,0x80b72);
  if(g_esi){PUSH32(g_esp,MEM32(frame+12));g_eax=frame-20;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);g_ebp=frame;SIGN_CALL(signing_secondary,0x80b83);g_ebx=g_eax;}}
 PUSH32(g_esp,0x24830000);PUSH32(g_esp,g_edi);g_ebp=frame;SIGN_CALL(sub_000825C8,0x80b90);POP32(g_esp,g_edi);POP32(g_esp,g_esi);uint32_t success=g_ebx;POP32(g_esp,g_ebx);
 if(success)g_eax=0;else{g_ebp=frame;SIGN_CALL(signing_last_error,0x80ba0);}g_esp=frame+16;
}
static void signing_begin(void){
 g_eax=MEM32(g_esp+4);
 if(g_eax<=1){PUSH32(g_esp,MEM32(g_esp+8));PUSH32(g_esp,g_eax);SIGN_CALL(signing_begin_hmac,0x80c71);}
 else if(g_eax==2||g_eax==4)SIGN_CALL(signing_begin_sha,0x80c65);
 else if(g_eax==8){uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(0x516c34);PUSH32(g_esp,0x80c5e);RECOMP_ICALL_SAFE(target,saved);}
 else{PUSH32(g_esp,50);SIGN_CALL(signing_error,0x80c7a);g_eax=UINT32_MAX;}g_esp+=12;
}
int migration_interface_signing(unsigned op){
 if(!signing_enabled()||op>9)return 0;
 uint32_t s=g_esp;if(s<0x10400||(s&3)||!signing_span(s-1024,1044)||!signing_span(0x519c50,4))return 0;
 uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;
 if(!signing_span(tls,4)||!signing_span((uint64_t)MEM32((uint32_t)tls)+8,4))return 0;
 if(op==7||op==8||op==9){const uint32_t slots[]={0x44b28c,0x44b27c};for(unsigned i=0;i<2;i++){uint32_t target=MEM32(slots[i]);if(target<0xfe000000||target>=0xfe000000+XBOX_KERNEL_THUNK_TABLE_SIZE*4||(target&3))return 0;}}
 if(op==0||op==4||op==5){if(!signing_span(MEM32(0x10118),0xa0)||!signing_span(MEM32(0x44b3b0),16)||!signing_span(MEM32(0x44b3b4),272))return 0;}
 if(op==2||op==3||op>=6){
  uint32_t object=MEM32(s+4),buffer=MEM32(s+8),length=(op==2||op==6)?MEM32(s+12):20;
  if(!signing_span(object,124)||!signing_apart(object,124,s-1024,1044)||(buffer&&(!signing_span(buffer,length)||!signing_apart(buffer,length,s-1024,1044))))return 0;
  if((op==2||op==6)&&length&&!buffer)return 0;
  if((op==7||op==8||op==9)&&!signing_span(MEM32(0x44b2a8),16))return 0;
  if(op==9&&(!signing_span(MEM32(s+12),20)||!signing_apart(MEM32(s+12),20,s-1024,1044)))return 0;
  if((op==7||op==8)&&!signing_span(MEM32(object+4),16))return 0;
 }
 switch(op){
 case 0:signing_begin_hmac();break;case 1:signing_begin_sha();break;case 2:signing_update();break;case 3:signing_finish_sha();break;case 4:signing_begin();break;
 case 5:g_eax=MEM32(0x10118);PUSH32(g_esp,MEM32(g_eax+8));PUSH32(g_esp,MEM32(g_esp+8));SIGN_CALL(signing_begin,0x80c91);g_esp+=8;break;
 case 6:{PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_ecx=MEM32(frame+8);g_eax=MEM32(g_ecx);uint32_t type=g_eax;
  if(type<=2||type==4||type==8){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(frame+16));PUSH32(g_esp,MEM32(frame+12));PUSH32(g_esp,g_ecx);
   if(type==8){uint32_t target=MEM32(0x516c38);PUSH32(g_esp,0x80cbd);RECOMP_ICALL_SAFE(target,saved);}else{g_ebp=frame;PUSH32(g_esp,type<=1?0x80cd9:0x80ccb);signing_update();}}
  else{PUSH32(g_esp,50);POP32(g_esp,g_eax);}g_esp=frame+20;break;}
 case 7:{g_ecx=MEM32(g_esp+4);g_eax=MEM32(g_ecx);uint32_t type=g_eax;
  if(type<=2||type==4||type==8){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esp+8));PUSH32(g_esp,g_ecx);
   if(type==8){uint32_t target=MEM32(0x516c3c);PUSH32(g_esp,0x80d07);RECOMP_ICALL_SAFE(target,saved);}else if(type<=1)SIGN_CALL(signing_finish_hmac,0x80d1f);else SIGN_CALL(signing_finish_sha,0x80d13);}
  else{PUSH32(g_esp,50);POP32(g_esp,g_eax);}g_esp+=12;break;}
 case 8:signing_finish_hmac();break;default:signing_secondary();break;
 }
 return 1;
}
#undef SIGN_CALL
