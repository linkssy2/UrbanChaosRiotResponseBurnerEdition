#include "hybrid_effect_properties_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_EFFECT_PROPERTIES");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int text_ok(uint32_t p,uint32_t s){
 /* ponytail: bounded authored names; unusual names over4096 bytes retain
  * the original path until asset evidence requires a larger bound. */
 for(unsigned i=0;i<4096;i++){if(!span(p+i,1)||!apart(p+i,1,s-64,84))return 0;if(!MEM8(p+i))return 1;}return 0;
}
static unsigned stream_size(uint32_t target){return target==0x1f8c01?12:target==0x1f8c14?8:target==0x1f8c51?0:4;}
static int binary_target(uint32_t t){return t==0x1f8bc8||t==0x1f8bdb||t==0x1f8bee||t==0x1f8c01||t==0x1f8c14||t==0x1f8c27||t==0x1f8c51;}
static int text_target(uint32_t t){return t==0x1f8cdc||t==0x1f8cf6||t==0x1f8d08||t==0x1f8d1a||t==0x1f8d2c||t==0x1f8d4c||t==0x1f8d64||t==0x1f8d76;}
static int string_slot_ok(uint32_t p,uint32_t index,uint32_t s){
 if(!span(p,0x6c)||!apart(p,0x6c,s-64,84))return 0;
 uint32_t list=MEM32(p+0x68);if(!list||(int32_t)index<0)return 1;
 if(!span(list,12)||!apart(list,12,s-64,84))return 0;uint32_t start=MEM32(list+4);if(!start)return 1;
 int32_t bytes=(int32_t)(MEM32(list+8)-start),count=bytes/28;
 if((int32_t)index>=count)return 1;
 uint64_t slot=(uint64_t)start+index*28ull;return slot<=0xffffffffu&&span((uint32_t)slot,28)&&apart((uint32_t)slot,28,s-64,84);
}
static void stream_read(unsigned bytes){
 g_eax=MEM32(g_esp+4);
 if(g_eax){g_edx=MEM32(g_ecx+0x60);if(bytes>4)PUSH32(g_esp,g_esi);
  if(bytes>4){for(unsigned i=0;i<bytes-4;i+=4){g_esi=MEM32(g_edx+i);MEM32(g_eax+i)=g_esi;}}
  g_edx=MEM32(g_edx+bytes-4);MEM32(g_eax+bytes-4)=g_edx;MEM32(g_ecx+0x60)+=bytes;
  if(bytes>4)POP32(g_esp,g_esi);
 }g_esp+=8;
}
static void string_slot(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_ecx+0x68);
 if(!g_edi)goto empty;
 g_esi=MEM32(g_esp+12);if((int32_t)g_esi<0)goto empty;
 g_eax=MEM32(g_edi+4);
 if(g_eax){g_ecx=MEM32(g_edi+8)-g_eax;int64_t product=(int64_t)(int32_t)0x92492493u*(int32_t)g_ecx;
  g_eax=(uint32_t)product;g_edx=(uint32_t)(product>>32);g_edx+=g_ecx;g_edx=(uint32_t)((int32_t)g_edx>>4);g_eax=(g_edx>>31)+g_edx;
 }
 if((int32_t)g_esi>=(int32_t)g_eax)goto empty;
 g_ecx=MEM32(g_edi+4);g_esi*=28;g_eax=MEM32(g_esi+g_ecx+24);g_esi+=g_ecx;
 {int z=g_eax==16;if(g_eax>=16)g_eax=MEM32(g_esi+4);else g_eax=g_esi+4;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=z;g_esp+=8;return;}
empty:
 POP32(g_esp,g_edi);g_eax=0x44d9b8;POP32(g_esp,g_esi);g_esp+=8;
}
static void binary_property(void){
 g_eax=MEM32(g_esp+4)-1;PUSH32(g_esp,g_esi);g_esi=g_ecx;
 if(g_eax>10){SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=12;return;}
 uint32_t target=MEM32(0x1f8c60+g_eax*4);int z=g_eax==10;
 if(target==0x1f8c51)goto done;
 if(target==0x1f8c27){
  g_ecx=MEM32(g_esi+16);g_edx=g_esp+8;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f8c34);stream_read(4);
  g_ecx=MEM32(g_esp+8);z=g_ecx==0xffffffffu;g_eax=0x44d9b8;
  if(!z){PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+16);PUSH32(g_esp,0x1f8c4b);string_slot();}
  g_ecx=MEM32(g_esp+12);MEM32(g_ecx)=g_eax;goto done;
 }
 if(target==0x1f8bc8||target==0x1f8c01){g_eax=MEM32(g_esp+12);g_ecx=MEM32(g_esi+16);PUSH32(g_esp,g_eax);}
 else if(target==0x1f8bee){g_edx=MEM32(g_esp+12);g_ecx=MEM32(g_esi+16);PUSH32(g_esp,g_edx);}
 else{g_ecx=MEM32(g_esp+12);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+16);}
 PUSH32(g_esp,target+13); /* All five original leaf-call return offsets are13. */
 stream_read(stream_size(target));SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_esp+=12;return;
done:
 SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_recomp_zf=z;g_esp+=12;
}
static void textual_property(void){
 uint32_t frame=g_seh_ebp;g_edx=MEM32(g_ecx+28);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);frame=0;PUSH32(g_esp,g_edi);
 if((int32_t)g_edx<=0)goto missing;
 g_edi=g_ecx+32;
 for(;;){
  g_esi=MEM32(g_esp+24);g_eax=MEM32(g_edi+4);int equal=0;
  for(;;){
   SET_LO8(g_ebx,MEM8(g_eax));SET_LO8(g_ecx,LO8(g_ebx));unsigned other=MEM8(g_esi);
   if(LO8(g_ebx)!=other){g_eax=LO8(g_ebx)<other?0xffffffffu:1;break;}
   if(!LO8(g_ecx)){g_eax=0;equal=1;break;}
   SET_LO8(g_ebx,MEM8(g_eax+1));SET_LO8(g_ecx,LO8(g_ebx));other=MEM8(g_esi+1);
   if(LO8(g_ebx)!=other){g_eax=LO8(g_ebx)<other?0xffffffffu:1;break;}
   g_eax+=2;g_esi+=2;if(!LO8(g_ecx)){g_eax=0;equal=1;break;}
  }
  if(equal)break;
  ++frame;g_edi+=36;if((int32_t)frame>=(int32_t)g_edx)goto missing;
 }
 g_eax=MEM32(g_esp+20)-1;if(g_eax>10)goto missing;
 uint32_t t=MEM32(0x1f8d88+g_eax*4),out=MEM32(g_esp+28);int set_z=0;
 if(t==0x1f8cdc)goto missing;
 if(t==0x1f8cf6){g_eax=MEM32(g_edi+8);g_ecx=out;}
 else if(t==0x1f8d08||t==0x1f8d64){g_edx=MEM32(g_edi+(t==0x1f8d64?12:8));g_eax=out;}
 else if(t==0x1f8d1a){SET_LO8(g_ecx,MEM8(g_edi+8));g_edx=out;}
 else if(t==0x1f8d76){g_ecx=MEM32(g_edi+8);g_edx=out;}
 else if(t==0x1f8d2c){g_eax=out;g_edi+=16;g_ecx=MEM32(g_edi);MEM32(g_eax)=g_ecx;g_edx=MEM32(g_edi+4);MEM32(g_eax+4)=g_edx;g_ecx=MEM32(g_edi+8);set_z=1;}
 else {g_edx=MEM32(g_edi+28);g_eax=out;MEM32(g_eax)=g_edx;g_ecx=MEM32(g_edi+32);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);
 if(t==0x1f8cf6)MEM32(g_ecx)=g_eax;
 else if(t==0x1f8d08||t==0x1f8d64)MEM32(g_eax)=g_edx;
 else if(t==0x1f8d1a)MEM8(g_edx)=LO8(g_ecx);
 else if(t==0x1f8d76)MEM32(g_edx)=g_ecx;
 else MEM32(g_eax+(set_z?8:4))=g_ecx;
 SET_LO8(g_eax,1);POP32(g_esp,g_ebx);if(set_z)g_recomp_zf=g_edi==0;g_esp+=16;return;
missing:
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,0);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=16;
}
int uc_hybrid_effect_properties(unsigned entry){
 if(!enabled())return 0;uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10040u||!span(s-64,84)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x221250){if(!string_slot_ok(p,MEM32(s+4),s))return 0;string_slot();return 1;}
 if(entry==0x221140||entry==0x221160||entry==0x221190){
  uint32_t d=MEM32(s+4);unsigned n=entry==0x221140?4:entry==0x221160?12:8;
  if(d&&(!span(p,0x64)||!span(MEM32(p+0x60),n)||!span(d,n)||!apart(d,n,s-64,84)))return 0;
  stream_read(n);return 1;
 }
 if(entry!=0x1f8bb0&&entry!=0x1f8c90&&entry!=0x1f8dc0)return 0;
 if(!span(p,32)||!apart(p,32,s-64,84)||!span(0x1f8c60,44)||!span(0x1f8d88,44))return 0;
 int binary=entry==0x1f8bb0||(entry==0x1f8dc0&&MEM32(p+16));unsigned type=MEM32(s+4)-1;uint32_t out=MEM32(s+(entry==0x1f8bb0?8:12));
 if(binary){
  if(type<=10){uint32_t t=MEM32(0x1f8c60+type*4);if(!binary_target(t))return 0;unsigned n=stream_size(t);uint32_t reader=MEM32(p+16);
   if(n&&(out||t==0x1f8c27)){
    if(!span(reader,0x6c)||!apart(reader,0x6c,s-64,84))return 0;uint32_t data=MEM32(reader+0x60);
    if(!span(data,n)||!apart(data,n,s-64,84))return 0;
    if(t==0x1f8c27){if(!string_slot_ok(reader,MEM32(data),s))return 0;n=4;}
    if(!span(out,n)||!apart(out,n,s-64,84)||!apart(out,n,p,32)||!apart(out,n,reader,0x6c))return 0;
   }
  }
  if(entry==0x1f8dc0){g_eax=MEM32(s+12);g_edx=MEM32(s+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f8dd6);binary_property();g_recomp_zf=0;g_esp+=16;}
  else binary_property();return 1;
 }
 int32_t count=(int32_t)MEM32(p+28);if(count>4096)return 0;
 if(count>0){
  if(!span(p+32,(unsigned)count*36)||!apart(p+32,(unsigned)count*36,s-64,84)||!text_ok(MEM32(s+8),s))return 0;
  for(int i=0;i<count;i++)if(!text_ok(MEM32(p+36+i*36),s))return 0;
  if(type<=10&&!text_target(MEM32(0x1f8d88+type*4)))return 0;
  if(!span(out,12)||!apart(out,12,s-64,84))return 0;
 }
 if(entry==0x1f8dc0)g_eax=0;
 textual_property();return 1;
}
