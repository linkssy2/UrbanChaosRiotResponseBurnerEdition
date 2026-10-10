#include "migration_interface_directory.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>
static int directory_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_DIRECTORY");v=s&&!strcmp(s,"1");}return v;}
static int directory_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int directory_apart(uint64_t a,unsigned n,uint64_t b,unsigned m){return a+n<=b||b+m<=a;}
static void directory_call(uint32_t slot,uint32_t ret,uint32_t saved){uint32_t target=MEM32(slot);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void directory_set_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;}
static void directory_error(void){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esp+4));directory_call(0x44b27c,0x820f9,saved);g_edx=g_eax;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x82101);directory_set_error();g_eax=g_edx;g_esp+=8;}
static void directory_convert(void){
 g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_eax+56);g_edx=MEM32(g_esp+8);MEM32(g_edx)=g_ecx;
 static const unsigned fields[]={8,12,16,20,24,28,44,40};for(unsigned i=0;i<8;i++){g_ecx=MEM32(g_eax+fields[i]);MEM32(g_edx+4+i*4)=g_ecx;}
 g_ecx=MEM32(g_eax+60);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=g_ecx;PUSH32(g_esp,g_edi);g_ecx>>=2;g_esi=g_eax+64;g_edi=g_edx+44;
 memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=g_ebx&3;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx);g_esi+=g_ecx;g_edi+=g_ecx;g_ecx=0;g_eax=MEM32(g_eax+60);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM8(g_eax+g_edx+44)=0;MEM8(g_edx+304)=0;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=12;
}
static void directory_next(void){
 uint32_t saved=g_esp;PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp-=336;PUSH32(g_esp,g_esi);g_esi=0;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_esi);PUSH32(g_esp,1);PUSH32(g_esp,328);g_eax=frame-336;PUSH32(g_esp,g_eax);g_eax=frame-8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(frame+8));directory_call(0x44b404,0x85c65,saved);
 int failed=(int32_t)g_eax<(int32_t)g_esi;POP32(g_esp,g_esi);
 if(failed){PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x85c70);directory_error();g_eax=0;}
 else{PUSH32(g_esp,MEM32(frame+12));g_eax=frame-336;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x85c83);directory_convert();g_eax=1;}
 g_esp=frame+16;
}
static void directory_first(void){
 uint32_t saved=g_esp;PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp-=364;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,MEM32(frame+8));g_eax=frame-8;PUSH32(g_esp,g_eax);directory_call(0x44b2b8,0x85b44,saved);
 g_edx=MEM32(frame-4);g_eax=frame-8;MEM32(frame-24)=g_eax;g_eax=MEM16(frame-8);g_edi=0;MEM32(frame-28)=0xfffffffd;MEM32(frame-20)=0x40;
 while(g_eax&&MEM8(g_edx+g_eax-1)!=92)g_eax--;
 g_ecx=MEM32(frame-8)-g_eax;g_edx+=g_eax;MEM16(frame-16)=LO16(g_ecx);MEM16(frame-14)=LO16(g_ecx);MEM32(frame-12)=g_edx;MEM16(frame-8)=LO16(g_eax);MEM16(frame-6)=LO16(g_eax);
 if(!LO16(g_eax)||!LO16(g_ecx)){PUSH32(g_esp,87);g_ebp=frame;PUSH32(g_esp,0x85c30);directory_set_error();g_eax=UINT32_MAX;}
 else{
  if(LO16(g_ecx)==3&&MEM8(g_edx)==42&&MEM8(g_edx+1)==46&&MEM8(g_edx+2)==42)MEM16(frame-16)=LO16(g_edi);
  saved=g_esp;PUSH32(g_esp,0x4021);PUSH32(g_esp,3);g_eax=frame-36;PUSH32(g_esp,g_eax);g_eax=frame-28;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x100001);g_eax=frame+8;PUSH32(g_esp,g_eax);directory_call(0x44b2b4,0x85bd4,saved);
  int failed=(int32_t)g_eax<(int32_t)g_edi;
  if(failed)PUSH32(g_esp,g_eax);
  else{
   saved=g_esp;PUSH32(g_esp,g_edi);g_eax=frame-16;PUSH32(g_esp,g_eax);PUSH32(g_esp,1);PUSH32(g_esp,328);g_eax=frame-364;PUSH32(g_esp,g_eax);g_eax=frame-36;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edi);PUSH32(g_esp,MEM32(frame+8));directory_call(0x44b404,0x85bfe,saved);g_esi=g_eax;failed=(int32_t)g_esi<(int32_t)g_edi;
   if(failed){saved=g_esp;PUSH32(g_esp,MEM32(frame+8));directory_call(0x44b2bc,0x85c0d,saved);PUSH32(g_esp,g_esi);}
  }
  if(failed){g_ebp=frame;PUSH32(g_esp,0x85c13);directory_error();g_eax=UINT32_MAX;}
  else{PUSH32(g_esp,MEM32(frame+12));g_eax=frame-364;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x85c24);directory_convert();g_eax=MEM32(frame+8);}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp=frame+16;
}
int migration_interface_directory(unsigned op){
 if(!directory_enabled()||op>2)return 0;uint32_t s=g_esp;
 if(s<0x10300||(s&3)||!directory_span(s-768,784))return 0;
 uint32_t output=MEM32(s+8);if(!directory_span(output,305)||!directory_apart(output,305,s-768,784))return 0;
 if(!op){uint32_t input=MEM32(s+4);if(!directory_span(input,64))return 0;uint32_t n=MEM32(input+60);if(n>260||!directory_span(input,64+n)||!directory_apart(input,64+n,output,305)||!directory_apart(input,64+n,s-768,784))return 0;directory_convert();return 1;}
 if(!directory_span(0x519c50,4))return 0;uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;if(!directory_span(tls,4)||!directory_span((uint64_t)MEM32((uint32_t)tls)+8,4))return 0;
 const uint32_t slots[]={0x44b27c,0x44b404,0x44b2b8,0x44b2b4,0x44b2bc};for(unsigned i=0;i<(op==1?2u:5u);i++){uint32_t target=MEM32(slots[i]);if(target<0xfe000000||target>=0xfe000000+XBOX_KERNEL_THUNK_TABLE_SIZE*4||(target&3))return 0;}
 if(op==2){uint32_t path=MEM32(s+4),n=0;while(n<260&&directory_span((uint64_t)path+n,1)&&MEM8(path+n))n++;if(n==260||!directory_span((uint64_t)path+n,1)||!directory_apart(path,n+1,s-768,784))return 0;directory_first();}
 else directory_next();return 1;
}
