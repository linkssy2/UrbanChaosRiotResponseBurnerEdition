#include "migration_interface_listlife.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D5CCD(void),sub_002742A0(void),sub_002668E0(void);
static int listlife_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int listlife_child(unsigned p){return !p||(listlife_span(p,4)&&listlife_span(MEM32(p),4));}
static void listlife_call(unsigned target,unsigned ret,unsigned saved){PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void listlife_clear(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_edi);g_edi=MEM32(g_eax);if(g_edi!=g_eax){do{g_ecx=MEM32(g_edi+8);if(g_ecx){g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,1);listlife_call(MEM32(g_eax),0x2742bd,saved);}g_edi=MEM32(g_edi);}while(g_edi!=MEM32(g_esi+4));}
 g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);MEM32(g_ecx)=g_ecx;g_ecx=MEM32(g_esi+4);MEM32(g_ecx+4)=g_ecx;MEM32(g_esi+8)=0;if(g_eax!=MEM32(g_esi+4)){do{g_edi=MEM32(g_eax);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2742e8);sub_002D5CCD();g_ecx=MEM32(g_esi+4);g_esp+=4;g_eax=g_edi;}while(g_edi!=g_ecx);}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;
}
static void listlife_destroy(void){
 unsigned frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39f80e);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=8;PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;MEM32(g_esp+12)=g_edi;MEM32(g_edi)=0x4877b4;g_eax=MEM32(g_edi+88);g_esi=0;MEM32(g_esp+28)=1;
 if(g_eax){frame=g_edi+8;do{g_ecx=MEM32(frame);if(g_ecx){g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,1);listlife_call(MEM32(g_eax),0x26692d,saved);}g_eax=MEM32(g_edi+88);g_esi++;frame+=4;}while(g_esi<g_eax);}
 SET_LO8(g_eax,MEM8(g_edi+176));if(LO8(g_eax)){g_ecx=MEM32(g_edi+4);MEM32(g_ecx)=0;}g_ecx=MEM32(g_edi+196);if(g_ecx){g_edx=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,1);listlife_call(MEM32(g_edx),0x26695b,saved);}g_esi=g_edi+220;MEM32(g_esp+16)=g_esi;g_ecx=g_esi;MEM8(g_esp+28)=2;PUSH32(g_esp,0x266971);sub_002742A0();
 g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);MEM32(g_ecx)=g_ecx;g_ecx=MEM32(g_esi+4);MEM32(g_ecx+4)=g_ecx;MEM32(g_esi+8)=0;if(g_eax!=MEM32(g_esi+4)){do{frame=MEM32(g_eax);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x266998);sub_002D5CCD();g_ecx=MEM32(g_esi+4);g_esp+=4;g_eax=frame;}while(frame!=g_ecx);}g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2669ad);sub_002D5CCD();g_ecx=MEM32(g_esp+24);g_esp+=4;MEM32(g_esi+4)=0;MEM32(g_edi)=0x450708;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(0)=g_ecx;g_esp+=20;g_recomp_zf=!g_esp;g_esp+=4;
}
int migration_interface_listlife(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_LISTLIFE");enabled=s&&!strcmp(s,"1");}if(!enabled||op>5)return 0;unsigned s=g_esp,obj=g_ecx;
 if(s<0x10400||(s&3)||!listlife_span(s-1024,1036)||!listlife_span(obj,op==0?2320:op==1||op==3?12:232))return 0;
 if(op==2||op>3){unsigned n=MEM32(obj+88);if(n>20)return 0;for(unsigned i=0;i<n;i++)if(!listlife_child(MEM32(obj+8+i*4)))return 0;}
 if(op==2){unsigned n=MEM32(s+8);if(n>20||(n&&!listlife_span(MEM32(s+4),n*4)))return 0;}
 if(op>=3){unsigned list=op==3?obj:obj+220,head=MEM32(list+4);if(!listlife_span(head,12))return 0;unsigned p=MEM32(head),n=0;while(p!=head){if(++n>4096||!listlife_span(p,12)||!listlife_child(MEM32(p+8)))return 0;p=MEM32(p);}if(op>3&&((MEM8(obj+176)&&!listlife_span(MEM32(obj+4),4))||!listlife_child(MEM32(obj+196))))return 0;}
 if(op<2){g_eax=MEM32(g_ecx+(op?8:2316));g_esp+=4;return 1;}if(op==3){listlife_clear();return 1;}if(op==4){listlife_destroy();return 1;}
 if(op==5){PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x2669d8);sub_002668E0();if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2669e5);sub_002D5CCD();g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+88);PUSH32(g_esp,g_edi);g_edi=0;if(g_eax){PUSH32(g_esp,g_ebx);g_ebx=g_esi+8;do{g_ecx=MEM32(g_ebx);if(g_ecx){g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,1);listlife_call(MEM32(g_eax),0x263ffd,saved);}g_eax=MEM32(g_esi+88);g_edi++;g_ebx+=4;}while(g_edi<g_eax);POP32(g_esp,g_ebx);}g_ecx=MEM32(g_esp+16);g_eax=0;MEM32(g_esi+88)=g_ecx;if(g_ecx){g_edx=MEM32(g_esp+12);g_ecx=g_esi+8;do{g_edi=MEM32(g_edx+g_eax*4);MEM32(g_ecx)=g_edi;g_edi=MEM32(g_esi+88);g_eax++;g_ecx+=4;}while(g_eax<g_edi);}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=12;return 1;
}
