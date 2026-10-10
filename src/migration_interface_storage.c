#include "migration_interface_storage.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>
static int storage_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_STORAGE");v=s&&!strcmp(s,"1");}return v;}
static int storage_span(uint64_t p,uint32_t n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int storage_apart(uint64_t a,uint32_t n,uint64_t b,uint32_t m){return a+n<=b||b+m<=a;}
static int storage_import(uint32_t slot){uint32_t v=MEM32(slot);return v>=0xfe000000&&v<0xfe000000+XBOX_KERNEL_THUNK_TABLE_SIZE*4&&!(v&3);}
static void storage_call(uint32_t slot,uint32_t ret,uint32_t saved){uint32_t target=MEM32(slot);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void storage_set_error(void){
 unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;
}
static void storage_error(void){
 uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esp+4));storage_call(0x44b27c,0x820f9,saved);g_edx=g_eax;
 PUSH32(g_esp,g_edx);PUSH32(g_esp,0x82101);storage_set_error();g_eax=g_edx;g_esp+=8;
}
static void storage_fail(uint32_t frame,uint32_t ret){PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,ret);storage_error();g_eax=0;}
static void storage_transfer(int write){
 PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;g_esp-=16;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edi=MEM32(frame+20);g_ebx=0;if(g_edi)MEM32(g_edi)=0;g_esi=MEM32(frame+24);
 const uint32_t slot=write?0x44b2c4:0x44b2c0;
 int success=0,eof_error=0;
 if(g_esi){
  g_eax=MEM32(g_esi+8);MEM32(frame-16)=g_eax;g_eax=MEM32(g_esi+12);MEM32(frame-12)=g_eax;g_eax=MEM32(g_esi+16);g_ecx=frame-16;
  uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,MEM32(frame+16));g_ecx=g_eax;PUSH32(g_esp,MEM32(frame+12));PUSH32(g_esp,g_esi);
  g_edi=0x103;MEM32(g_esi)=g_edi;g_ecx=(g_eax&1)?0:g_esi;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));storage_call(slot,write?0x80e69:0x80d7c,saved);
  if(write){g_ecx=0xc0000000;g_edx=g_eax&g_ecx;success=g_edx!=g_ecx&&g_eax!=g_edi;}
  else success=(int32_t)g_eax>=(int32_t)g_ebx&&g_eax!=g_edi;
  if(success){g_eax=MEM32(frame+20);if(g_eax){g_ecx=MEM32(g_esi+4);MEM32(g_eax)=g_ecx;}}
  else if(!write){g_ecx=0xc0000011;if(g_eax==g_ecx){g_eax=MEM32(frame+20);if(g_eax)MEM32(g_eax)=g_ebx;eof_error=1;}}
 }else{
  uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(frame+16));g_eax=frame-8;PUSH32(g_esp,MEM32(frame+12));PUSH32(g_esp,g_eax);
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(frame+8));storage_call(slot,write?0x80ea2:0x80dc1,saved);
  int wait_failed=0;
  if(g_eax==0x103){saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(frame+8));storage_call(0x44b3b8,write?0x80eb4:0x80dd3,saved);wait_failed=(int32_t)g_eax<(int32_t)g_ebx;if(!wait_failed)g_eax=MEM32(frame-8);}
  success=!wait_failed&&(int32_t)g_eax>=(int32_t)g_ebx;
  if(success){g_eax=MEM32(frame-4);MEM32(g_edi)=g_eax;}
  else if(!write&&g_eax==0xc0000011){MEM32(g_edi)=g_ebx;success=1;}
  else{g_ecx=g_eax&0xc0000000;if(g_ecx==0x80000000){g_ecx=MEM32(frame-4);MEM32(g_edi)=g_ecx;}}
 }
 if(success)g_eax=1;else if(eof_error){PUSH32(g_esp,g_ecx);g_ebp=frame;PUSH32(g_esp,0x80e0b);storage_error();g_eax=0;}else storage_fail(frame,write?0x80ee1:0x80e0b);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+4+24;
}
static void storage_size64(void){
 uint32_t saved=g_esp;PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;g_esp-=64;
 PUSH32(g_esp,0x22);PUSH32(g_esp,0x38);g_eax=frame-64;PUSH32(g_esp,g_eax);g_eax=frame-8;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));storage_call(0x44b3bc,0x81172,saved);
 if((int32_t)g_eax<0)storage_fail(frame,0x8117c);
 else{g_eax=MEM32(frame+12);g_ecx=MEM32(frame-24);MEM32(g_eax)=g_ecx;g_ecx=MEM32(frame-20);MEM32(g_eax+4)=g_ecx;g_eax=1;}
 g_esp=frame+4+12;
}
static void storage_size32(void){
 PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);g_eax=frame-8;
 PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));PUSH32(g_esp,0x811f7);storage_size64();
 if(g_eax){g_eax=MEM32(frame+12);if(g_eax){g_ecx=MEM32(frame-4);MEM32(g_eax)=g_ecx;}if(MEM32(frame-8)==UINT32_MAX){PUSH32(g_esp,0);g_ebp=frame;PUSH32(g_esp,0x81214);storage_set_error();}}
 else MEM32(frame-8)=UINT32_MAX;
 g_eax=MEM32(frame-8);g_esp=frame+4+12;
}
static void storage_truncate(void){
 uint32_t saved=g_esp;PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;g_esp-=32;
 PUSH32(g_esp,14);PUSH32(g_esp,8);g_eax=frame-8;PUSH32(g_esp,g_eax);g_eax=frame-32;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));storage_call(0x44b3bc,0x80f05,saved);
 if((int32_t)g_eax<0)storage_fail(frame,0x80f0f);
 else{
  g_eax=MEM32(frame-8);saved=g_esp;PUSH32(g_esp,g_esi);g_esi=MEM32(0x44b34c);MEM32(frame-16)=g_eax;g_eax=MEM32(frame-4);MEM32(frame-12)=g_eax;
  PUSH32(g_esp,20);PUSH32(g_esp,8);g_eax=frame-16;PUSH32(g_esp,g_eax);g_eax=frame-32;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));PUSH32(g_esp,0x80f37);RECOMP_ICALL_SAFE(g_esi,saved);
  if((int32_t)g_eax>=0){g_eax=MEM32(frame-8);MEM32(frame-24)=g_eax;g_eax=MEM32(frame-4);saved=g_esp;PUSH32(g_esp,19);MEM32(frame-20)=g_eax;PUSH32(g_esp,8);g_eax=frame-24;PUSH32(g_esp,g_eax);g_eax=frame-32;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));PUSH32(g_esp,0x80f58);RECOMP_ICALL_SAFE(g_esi,saved);}
  if((int32_t)g_eax<0)storage_fail(frame,0x80f67);else g_eax=1;POP32(g_esp,g_esi);
 }
 g_esp=frame+4+8;
}
static void storage_open(void){
 PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;g_esp-=32;uint32_t creation=MEM32(frame+24);
 g_eax=creation>=1&&creation<=5?0:creation-5;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(frame+12);PUSH32(g_esp,1);POP32(g_esp,g_ebx);
 static const uint32_t disposition[]={0,2,5,1,3,4};
 if(creation>=1&&creation<=5)MEM32(frame+12)=disposition[creation];
 if(creation<1||creation>5||(creation==5&&!(g_esi&0x40000000))){PUSH32(g_esp,0xc000000d);PUSH32(g_esp,0x8129e);storage_error();g_eax=UINT32_MAX;}
 else{
  uint32_t saved=g_esp;PUSH32(g_esp,g_edi);g_edi=MEM32(frame+8);PUSH32(g_esp,g_edi);g_eax=frame-12;PUSH32(g_esp,g_eax);storage_call(0x44b2b8,0x812d0,saved);
  int slash=0;if(MEM16(frame-12)>LO16(g_ebx)){g_eax=MEM16(frame-12);slash=MEM8(g_eax+g_edi-1)==0x5c;}MEM32(frame+8)=slash?g_ebx:0;
  g_ecx=MEM32(frame+28);MEM32(frame-28)=frame-12;uint32_t flags=g_ecx;
  g_eax=((flags>>30)&2)|((flags>>26)&8)|((flags>>25)&4)|((flags>>17)&0x800)|((flags>>11)&0x4000)|((~flags>>25)&0x20);
  g_edx=(~flags>>25)&0x20;g_edi=flags&0x2000000;MEM32(frame-32)=0xfffffffd;MEM32(frame-24)=0x40;
  if(flags&0x4000000){g_eax|=0x1000;g_esi|=0x10000;}g_ebx=0;int directory=g_edi!=0;POP32(g_esp,g_edi);if(!directory)g_eax|=0x40;
  saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+12));g_ecx&=0x7fa7;PUSH32(g_esp,MEM32(frame+16));g_eax=frame-20;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);g_eax=frame-32;PUSH32(g_esp,g_eax);g_esi|=0x100080;PUSH32(g_esp,g_esi);g_eax=frame-4;PUSH32(g_esp,g_eax);storage_call(0x44b320,0x8138d,saved);
  g_esi=g_eax;
  if((int32_t)g_esi<(int32_t)g_ebx){
   PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x81399);storage_error();
   if(g_esi==0xc0000035||g_esi==0xc00000ba){PUSH32(g_esp,g_esi==0xc0000035?80:MEM32(frame+8)!=g_ebx?3:5);PUSH32(g_esp,0x813bd);storage_set_error();}g_eax=UINT32_MAX;
  }else{
   uint32_t requested=MEM32(frame+24),information=MEM32(frame-16);PUSH32(g_esp,((requested==2&&information==3)||(requested==4&&information==1))?183:g_ebx);g_ebp=frame;PUSH32(g_esp,0x813e7);storage_set_error();g_eax=MEM32(frame-4);
  }
 }
 POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+4+32;
}
int migration_interface_storage(unsigned op){
 if(!storage_enabled()||op>6)return 0;
 uint32_t s=g_esp;if(s<0x10100||(s&3)||!storage_span(s-256,292)||!storage_span(0x519c50,4))return 0;
 uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;
 if(!storage_span(tls,4)||!storage_span((uint64_t)MEM32((uint32_t)tls)+8,4)||!storage_apart(tls,4,s-256,284)||!storage_apart(MEM32((uint32_t)tls),12,s-256,284))return 0;
 if(!storage_import(0x44b27c))return 0;
 if(op==6){
  if(!storage_import(0x44b2b8)||!storage_import(0x44b320))return 0;
  uint32_t path=MEM32(s+4),length=0;while(length<260&&storage_span((uint64_t)path+length,1)&&MEM8(path+length))length++;
  if(length==260||!storage_span((uint64_t)path+length,1)||!storage_apart(path,length+1,s-256,292))return 0;
  storage_open();return 1;
 }
 if(op<2){
  uint32_t data=MEM32(s+8),n=MEM32(s+12),count=MEM32(s+16),overlap=MEM32(s+20);
  if((n&&(!storage_span(data,n)||!storage_apart(data,n,s-256,284)))||(count&&(!storage_span(count,4)||!storage_apart(count,4,s-256,284)))||(!overlap&&!count))return 0;
  if(overlap&&(!storage_span(overlap,20)||!storage_apart(overlap,20,s-256,284)))return 0;
  if(!storage_import(op?0x44b2c4:0x44b2c0)||!storage_import(0x44b3b8))return 0;storage_transfer(op==1);return 1;
 }
 if(op==5){if(!storage_import(0x44b2bc))return 0;PUSH32(g_esp,MEM32(s+4));storage_call(0x44b2bc,0x8122b,s);if((int32_t)g_eax<0){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x8123a);storage_error();g_eax=0;}else g_eax=1;g_esp+=8;return 1;}
 if(!storage_import(0x44b3bc))return 0;
 if(op==2){if(!storage_import(0x44b34c))return 0;storage_truncate();return 1;}
 uint32_t output=MEM32(s+8);unsigned bytes=op==3?8:4;
 if((op==3||output)&&(!storage_span(output,bytes)||!storage_apart(output,bytes,s-256,284)))return 0;
 if(op==3)storage_size64();else storage_size32();return 1;
}
