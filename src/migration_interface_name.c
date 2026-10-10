#include "migration_interface_name.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D7740(void),sub_002D7DE0(void),sub_002D7DC0(void);
static int name_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void name_hex(void){g_eax=MEM32(g_esp+4);g_eax+=(int32_t)g_eax>9?0x37:0x30;g_esp+=8;}
int migration_interface_name(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_NAME");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;
 uint32_t s=g_esp;if(s<0x10100||(s&3)||!name_span(s-256,272))return 0;
 if(op){name_hex();return 1;}
 uint32_t input=MEM32(s+4),output=MEM32(s+8);unsigned n=0;
 for(;n<256;n++){if(!name_span((uint64_t)input+n*2,2))return 0;if(!MEM16(input+n*2))break;}
 if(n==256||!name_span(output,13)||!((uint64_t)output+13<=s-256||output>=s+16)||!((uint64_t)input+2*n+2<=s-256||input>=s+16))return 0;
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);MEM32(frame-8)=MEM32(frame-4)=0;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=input;g_eax=MEM16(input);PUSH32(g_esp,g_edi);
 if(g_eax){g_edi=g_eax;do{
  PUSH32(g_esp,0);PUSH32(g_esp,0x10000);PUSH32(g_esp,MEM32(frame-4));PUSH32(g_esp,MEM32(frame-8));g_ebp=frame;PUSH32(g_esp,0x81736);sub_002D7740();
  g_ecx=g_eax;g_eax=g_edi&0xffff;g_ebx=g_edx;g_edx=0;PUSH32(g_esp,0xffff);uint64_t sum=(uint64_t)g_ecx+g_eax;g_ecx=(uint32_t)sum;PUSH32(g_esp,0xffffffc5);g_ebx+=(uint32_t)(sum>>32);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);g_ebp=frame;PUSH32(g_esp,0x81750);sub_002D7DE0();
  g_esi+=2;g_edi=MEM16(g_esi);MEM32(frame-8)=g_eax;MEM32(frame-4)=g_edx;
 }while(g_edi);}
 g_edi=MEM32(frame+12);PUSH32(g_esp,11);POP32(g_esp,g_esi);g_ebx=0;
 do{g_eax=MEM32(frame-8);g_edx=MEM32(frame-4);g_ecx=g_ebx;g_ebp=frame;PUSH32(g_esp,0x81777);sub_002D7DC0();g_eax&=15;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81780);name_hex();MEM8(g_esi+g_edi)=(uint8_t)g_eax;g_ebx+=4;g_esi--;}while(g_ebx<=44);
 MEM8(g_edi+12)=0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+20;g_recomp_zf=0;return 1;
}
