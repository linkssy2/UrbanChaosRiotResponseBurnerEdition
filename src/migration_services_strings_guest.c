#include "migration_services_strings_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00011C00(void),sub_00011C80(void),sub_00011E00(void),sub_002D581F(void),sub_002D585F(void),sub_002D5990(void);
static int strings_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static unsigned strings_data(unsigned obj){return MEM32(obj+24)<16?obj+4:MEM32(obj+4);}
static void strings_copy(unsigned n){
 g_ecx=n>>2;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),4*g_ecx);g_esi+=4*g_ecx;g_edi+=4*g_ecx;
 g_ecx=n&3;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx);g_esi+=g_ecx;g_edi+=g_ecx;g_ecx=0;
}
int uc_migration_services_strings(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_STRINGS");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!strings_span(g_esp-256,276)||!strings_span(g_ecx,28))return 0;
 if(entry!=0x11c00&&entry!=0x11e00&&entry!=0x11ef0)return 0;
 if(entry==0x11e00&&!strings_span(MEM32(g_esp+4),28))return 0;
 unsigned frame=entry==0x11c00?g_ebp:g_seh_ebp;
 if(entry==0x11c00){
  PUSH32(g_esp,frame);frame=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);
  if(MEM32(g_esi+20)<frame){PUSH32(g_esp,0x11c13);sub_002D581F();}
  g_eax=MEM32(g_esi+20)-frame;g_edi=MEM32(g_esp+20);if(g_eax<g_edi)g_edi=g_eax;
  if(g_edi){
   g_ecx=MEM32(g_esi+24);PUSH32(g_esp,g_ebx);g_ebx=g_esi+4;g_edx=g_ecx<16?g_ebx:MEM32(g_ebx);g_ecx=g_ecx<16?g_ebx:MEM32(g_ebx);
   g_eax-=g_edi;g_edx+=frame;PUSH32(g_esp,g_eax);g_edx+=g_edi;PUSH32(g_esp,g_edx);g_ecx+=frame;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x11c52);sub_002D5990();
   g_eax=MEM32(g_esi+20);g_ecx=MEM32(g_esi+24);g_eax-=g_edi;g_esp+=12;MEM32(g_esi+20)=g_eax;if(g_ecx>=16)g_ebx=MEM32(g_ebx);MEM8(g_ebx+g_eax)=0;POP32(g_esp,g_ebx);
  }
  POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=16;return 1;
 }
 if(entry==0x11ef0){
  PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_edx=MEM32(g_ebx+24);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=strings_data(g_ebx);g_esi=MEM32(g_esp+16);
  if(g_esi>=g_eax){
   g_eax=g_ebx+4;g_ecx=g_edx<16?g_eax:MEM32(g_eax);g_edi=MEM32(g_ebx+20)+g_ecx;
   if(g_edi>g_esi){
    if(g_edx>=16)g_eax=MEM32(g_eax);g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_esi-=g_eax;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);g_ecx=g_ebx;PUSH32(g_esp,0x11f3b);sub_00011E00();
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=!g_esi;g_esp+=12;return 1;
   }
  }
  PUSH32(g_esp,frame);frame=MEM32(g_esp+24);
  if(frame>0xfffffffe){g_ecx=g_ebx;PUSH32(g_esp,0x11f52);sub_002D585F();}
  g_eax=MEM32(g_ebx+24);int grown=g_eax<frame;
  if(grown){g_edx=MEM32(g_ebx+20);PUSH32(g_esp,g_edx);PUSH32(g_esp,frame);g_ecx=g_ebx;PUSH32(g_esp,0x11f65);sub_00011C80();}
  if(!frame&&!grown){int equal=g_eax==16;MEM32(g_ebx+20)=0;g_eax=strings_data(g_ebx);g_esp+=4;POP32(g_esp,g_edi);MEM8(g_eax)=0;POP32(g_esp,g_esi);g_eax=g_ebx;POP32(g_esp,g_ebx);g_recomp_zf=equal;g_esp+=12;return 1;}
  if(frame){g_edi=strings_data(g_ebx);g_eax=frame;strings_copy(frame);unsigned capacity=MEM32(g_ebx+24);MEM32(g_ebx+20)=frame;g_eax=strings_data(g_ebx);MEM8(g_eax+frame)=0;if(capacity>=16)g_recomp_zf=capacity==16;}
  g_esp+=4;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=g_ebx;POP32(g_esp,g_ebx);g_esp+=12;return 1;
 }
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+20);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+20);g_ebx=g_ecx;
 if(MEM32(g_edi+20)<g_esi){PUSH32(g_esp,0x11e18);sub_002D581F();}
 frame=MEM32(g_edi+20)-g_esi;g_eax=MEM32(g_esp+28);if(g_eax<frame)frame=g_eax;
 if(g_ebx==g_edi){
  PUSH32(g_esp,0xffffffff);frame+=g_esi;PUSH32(g_esp,frame);g_ecx=g_ebx;PUSH32(g_esp,0x11e36);sub_00011C00();
  PUSH32(g_esp,g_esi);PUSH32(g_esp,0);g_ecx=g_ebx;PUSH32(g_esp,0x11e40);sub_00011C00();
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=g_ebx;POP32(g_esp,g_ebx);g_recomp_zf=!frame;g_esp+=16;return 1;
 }
 if(frame>0xfffffffe){g_ecx=g_ebx;PUSH32(g_esp,0x11e55);sub_002D585F();}
 g_eax=MEM32(g_ebx+24);int grown=g_eax<frame;
 if(grown){g_eax=MEM32(g_ebx+20);PUSH32(g_esp,g_eax);PUSH32(g_esp,frame);g_ecx=g_ebx;PUSH32(g_esp,0x11e68);sub_00011C80();}
 if(!frame&&!grown){g_recomp_zf=g_eax==16;MEM32(g_ebx+20)=0;g_eax=strings_data(g_ebx);MEM8(g_eax)=0;}
 else if(frame){
  g_ecx=MEM32(g_edi+24);g_eax=16;g_edx=strings_data(g_edi);g_eax=g_ebx+4;g_edi=strings_data(g_ebx);g_esi+=g_edx;g_edx=frame;strings_copy(frame);
  MEM32(g_ebx+20)=frame;if(MEM32(g_ebx+24)>=16)g_eax=MEM32(g_eax);MEM8(g_eax+frame)=0;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;g_eax=g_ebx;POP32(g_esp,g_ebx);g_esp+=16;return 1;
}
