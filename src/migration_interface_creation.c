#include "migration_interface_creation.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>
extern void sub_00081265(void),sub_00081221(void),sub_00081797(void),sub_000817DF(void),sub_002D7EB3(void);
static int creation_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void creation_call(uint32_t slot,uint32_t ret,uint32_t saved){uint32_t target=MEM32(slot);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void creation_get_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_eax=MEM32(g_eax+8);g_recomp_zf=level==2;g_esp+=4;}
static void creation_set_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;}
static void creation_error(void){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esp+4));creation_call(0x44b27c,0x820f9,saved);g_edx=g_eax;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x82101);creation_set_error();g_eax=g_edx;g_esp+=8;}
int migration_interface_creation(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_CREATION");enabled=s&&!strcmp(s,"1");}if(!enabled||op>3)return 0;
 uint32_t s=g_esp;if(s<0x10400||(s&3)||!creation_span(s-1024,1040))return 0;
 uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;if(!creation_span(tls,4)||!creation_span((uint64_t)MEM32((uint32_t)tls)+8,4))return 0;
 const uint32_t slots[]={0x44b27c,0x44b2b8,0x44b2b4,0x44b2bc,0x44b320,0x44b34c,0x44b2dc};for(unsigned i=0;i<7;i++){uint32_t t=MEM32(slots[i]);if(t<0xfe000000||t>=0xfe000000+XBOX_KERNEL_THUNK_TABLE_SIZE*4||(t&3))return 0;}
 uint32_t path=MEM32(s+4);unsigned n=0;for(;n<512;n++){if(!creation_span((uint64_t)path+n,1))return 0;if(!MEM8(path+n))break;}if(n==512)return 0;
 if(!op){uint32_t p=MEM32(s+8);for(n=0;n<128;n++){if(!creation_span((uint64_t)p+n*2,2))return 0;if(!MEM16(p+n*2))break;}if(n==128)return 0;}
 uint32_t saved=s;PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;
 if(op==0){
  g_esp-=256;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=0;PUSH32(g_esp,0);PUSH32(g_esp,4);PUSH32(g_esp,4);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x80100000);PUSH32(g_esp,MEM32(frame+8));PUSH32(g_esp,0x81a84);sub_00081265();g_edi=g_eax;
  if(g_edi!=UINT32_MAX){PUSH32(g_esp,1);PUSH32(g_esp,g_edi);g_ebp=frame;PUSH32(g_esp,0x81a93);sub_00081797();int valid=g_eax!=0;
   if(valid){PUSH32(g_esp,128);g_eax=frame-256;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x455624);PUSH32(g_esp,g_edi);g_ebp=frame;PUSH32(g_esp,0x81aae);sub_000817DF();valid=g_eax!=0;
    if(valid){PUSH32(g_esp,MEM32(frame+12));g_eax=frame-256;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81ac1);sub_002D7EB3();valid=!g_eax;POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);}}
   if(!valid)g_esi=0x491;PUSH32(g_esp,g_edi);g_ebp=frame;PUSH32(g_esp,0x81ad2);sub_00081221();
  }else{g_ebp=frame;PUSH32(g_esp,0x81ad9);creation_get_error();g_esi=g_eax;}
  POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_esp=frame+16;return 1;
 }
 if(op==1){
  g_esp-=48;PUSH32(g_esp,g_esi);PUSH32(g_esp,0);PUSH32(g_esp,0x2000000);PUSH32(g_esp,3);PUSH32(g_esp,0);PUSH32(g_esp,3);PUSH32(g_esp,0xc0000000);PUSH32(g_esp,MEM32(frame+8));PUSH32(g_esp,0x81b04);sub_00081265();g_esi=g_eax;
  if(g_esi!=UINT32_MAX){PUSH32(g_esp,g_edi);PUSH32(g_esp,10);POP32(g_esp,g_ecx);g_eax=0;g_edi=frame-48;for(unsigned i=0;i<10;i++)MEM32(g_edi+i*4)=0;g_edi+=40;g_ecx=0;g_eax=frame-32;saved=g_esp;PUSH32(g_esp,g_eax);creation_call(0x44b2dc,0x81b20,saved);
   saved=g_esp;PUSH32(g_esp,4);PUSH32(g_esp,40);g_eax=frame-48;PUSH32(g_esp,g_eax);g_eax=frame-8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);creation_call(0x44b34c,0x81b33,saved);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x81b39);sub_00081221();POP32(g_esp,g_edi);}
  POP32(g_esp,g_esi);g_esp=frame+12;return 1;
 }
 unsigned create=op==2;g_esp-=create?28:32;PUSH32(g_esp,MEM32(frame+8));g_eax=frame-(create?8:12);PUSH32(g_esp,g_eax);creation_call(0x44b2b8,create?0x85c9d:0x85d09,saved);
 saved=g_esp;PUSH32(g_esp,0x4021);
 if(create){PUSH32(g_esp,2);PUSH32(g_esp,3);PUSH32(g_esp,0x80);g_eax=frame-8;MEM32(frame-24)=g_eax;PUSH32(g_esp,0);g_eax=frame-16;PUSH32(g_esp,g_eax);g_eax=frame-28;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x100001);g_eax=frame+8;PUSH32(g_esp,g_eax);MEM32(frame-28)=0xfffffffd;MEM32(frame-20)=0x40;creation_call(0x44b320,0x85cd8,saved);}
 else{g_eax=frame-12;MEM32(frame-28)=g_eax;PUSH32(g_esp,7);g_eax=frame-20;PUSH32(g_esp,g_eax);g_eax=frame-32;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x110000);g_eax=frame-4;PUSH32(g_esp,g_eax);MEM32(frame-32)=0xfffffffd;MEM32(frame-24)=0x40;creation_call(0x44b2b4,0x85d3b,saved);}
 if((int32_t)g_eax<0){PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,create?0x85cf0:0x85d45);creation_error();g_eax=0;}
 else if(create){saved=g_esp;PUSH32(g_esp,MEM32(frame+8));creation_call(0x44b2bc,0x85ce5,saved);g_eax=1;}
 else{saved=g_esp;PUSH32(g_esp,g_esi);PUSH32(g_esp,13);PUSH32(g_esp,1);g_eax=frame+11;PUSH32(g_esp,g_eax);g_eax=frame-20;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame-4));MEM8(frame+11)=1;creation_call(0x44b34c,0x85d63,saved);saved=g_esp;PUSH32(g_esp,MEM32(frame-4));g_esi=g_eax;creation_call(0x44b2bc,0x85d6e,saved);
  if((int32_t)g_esi<0){PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x85d7d);creation_error();g_eax=0;}else g_eax=1;POP32(g_esp,g_esi);}
 g_esp=frame+(create?16:12);return 1;
}
