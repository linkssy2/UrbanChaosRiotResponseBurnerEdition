#include "migration_interface_create.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>
extern void sub_00081705(void),sub_00085C8A(void),sub_00081A63(void),sub_00081AE3(void),sub_00085CF6(void),sub_00081221(void),sub_00080E14(void),sub_00084CD1(void),sub_002D7EE5(void),sub_002D7774(void);
static int create_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int create_string(uint32_t p,unsigned width,unsigned limit){for(unsigned i=0;i<limit;i++){if(!create_span((uint64_t)p+i*width,width))return 0;if(width==1?!MEM8(p+i):!MEM16(p+i*2))return 1;}return 0;}
static void create_call(uint32_t slot,uint32_t ret,uint32_t saved){uint32_t target=MEM32(slot);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void create_get_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_eax=MEM32(g_eax+8);g_recomp_zf=level==2;g_esp+=4;}
static void create_copy(void){do{SET_LO8(g_ecx,MEM8(g_eax));MEM8(g_edx+g_eax)=LO8(g_ecx);g_eax++;}while(LO8(g_ecx));}
static void create_append(uint32_t frame){g_esi=frame+g_edi-296;g_eax=0x4555bc;g_edx=g_esi-g_eax;create_copy();}
int migration_interface_create(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_CREATE");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 uint32_t s=g_esp;if(s<0x10800||(s&3)||!create_span(s-2048,2080))return 0;
 uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;if(!create_span(tls,4)||!create_span((uint64_t)MEM32((uint32_t)tls)+8,4))return 0;
 if(!create_string(MEM32(s+4),1,220)||!create_string(MEM32(s+8),2,128)||!create_string(0x4555bc,1,14))return 0;
 uint32_t out=MEM32(s+20),cap=MEM32(s+24);if(out&&(!cap||cap>4096||!create_span(out,cap)))return 0;
 const uint32_t slots[]={0x44b2b8,0x44b320,0x44b27c};for(unsigned i=0;i<3;i++){uint32_t t=MEM32(slots[i]);if(t<0xfe000000||t>=0xfe000000+XBOX_KERNEL_THUNK_TABLE_SIZE*4||(t&3))return 0;}
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp-=600;g_eax=MEM32(frame+8);g_edx=frame-296-g_eax;create_copy();
 g_eax=frame-296;g_edx=g_eax+1;do{SET_LO8(g_ecx,MEM8(g_eax));g_eax++;}while(LO8(g_ecx));g_eax-=g_edx;g_ecx=247-g_eax;PUSH32(g_esp,g_ecx);g_eax=frame+g_eax-296;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+12));g_ebp=frame;PUSH32(g_esp,0x81b87);sub_00081705();
 g_eax=frame-296;g_ecx=g_eax+1;do{SET_LO8(g_edx,MEM8(g_eax));g_eax++;}while(LO8(g_edx));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_eax-=g_ecx;PUSH32(g_esp,g_edi);g_edi=g_eax;
 int success=0,remove=0;
 if(MEM32(frame+16)==3){create_append(frame);PUSH32(g_esp,MEM32(frame+12));g_eax=frame-296;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81bcd);sub_00081A63();MEM8(g_esi)=0;success=!g_eax;}
 else{
  PUSH32(g_esp,0);g_eax=frame-296;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81be8);sub_00085C8A();
  if(!g_eax){g_ebp=frame;PUSH32(g_esp,0x81bf1);create_get_error();MEM32(frame+20)=g_eax;g_ebx=0x491;int invalid=0;
   if(g_eax==183){create_append(frame);PUSH32(g_esp,MEM32(frame+12));g_eax=frame-296;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81c29);sub_00081A63();g_ebx=g_eax;MEM8(g_esi)=0;invalid=g_ebx!=0;
    if(!invalid){g_eax=frame-296;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81c3e);sub_00081AE3();}}
   if(invalid){PUSH32(g_esp,82);POP32(g_esp,g_eax);}else if(MEM32(frame+16)!=1&&!g_ebx)success=1;else g_eax=MEM32(frame+20);
  }else{
   create_append(frame);g_eax=frame-296;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=frame-28;PUSH32(g_esp,g_eax);create_call(0x44b2b8,0x81c86,saved);MEM32(frame-4)=0;
   saved=g_esp;PUSH32(g_esp,0x22);PUSH32(g_esp,3);PUSH32(g_esp,0);PUSH32(g_esp,4);g_eax=frame-28;MEM32(frame-16)=g_eax;g_eax=frame-8;PUSH32(g_esp,g_eax);g_eax=frame-36;PUSH32(g_esp,g_eax);g_eax=frame-20;PUSH32(g_esp,g_eax);g_ebx=0;PUSH32(g_esp,0xc0100000);g_eax=frame+8;g_ebx=1;PUSH32(g_esp,g_eax);MEM32(frame-20)=0xfffffffd;MEM32(frame-12)=0x40;MEM32(frame-8)=g_ebx;create_call(0x44b320,0x81cc7,saved);MEM8(g_esi)=0;
   if((int32_t)g_eax<0){saved=g_esp;PUSH32(g_esp,g_eax);create_call(0x44b27c,0x81cd5,saved);g_esi=g_eax;remove=1;}
   else{
    g_eax=(MEM8(frame+20)&(uint8_t)g_ebx)?0x4556bc:0x45569c;PUSH32(g_esp,0x45567c);PUSH32(g_esp,0x455674);PUSH32(g_esp,MEM32(frame+12));PUSH32(g_esp,61);PUSH32(g_esp,0x455624);PUSH32(g_esp,0xfeff);PUSH32(g_esp,g_eax);g_eax=frame-600;PUSH32(g_esp,152);PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81d16);sub_002D7EE5();g_eax=frame-600;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81d22);sub_002D7774();g_esi=g_eax;g_esp+=40;g_esi<<=1;
    while(g_esi){PUSH32(g_esp,0);g_eax=frame+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_eax=frame-600;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));g_ebp=frame;PUSH32(g_esp,0x81d41);sub_00080E14();if(!g_eax){remove=1;break;}g_esi-=MEM32(frame+12);}
    if(remove){g_ebp=frame;PUSH32(g_esp,0x81d85);create_get_error();PUSH32(g_esp,MEM32(frame+8));g_esi=g_eax;g_ebp=frame;PUSH32(g_esp,0x81d8f);sub_00081221();}
    else{PUSH32(g_esp,MEM32(frame+8));g_ebp=frame;PUSH32(g_esp,0x81d52);sub_00081221();success=1;}
   }
  }
 }
 if(remove){g_eax=frame-296;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81d9b);sub_00085CF6();g_eax=g_esi;}
 if(success){g_esi=MEM32(frame+24);if(g_esi){g_ebx=MEM32(frame+28);PUSH32(g_esp,g_ebx);g_eax=frame-296;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x81d6a);sub_00084CD1();g_ebx--;if(g_edi<g_ebx){g_eax=g_edi+g_esi;MEM8(g_eax)=92;MEM8(g_eax+1)=0;}}g_eax=0;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+32;return 1;
}
