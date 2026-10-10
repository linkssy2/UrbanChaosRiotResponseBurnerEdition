#include "hybrid_explosion_factory_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void),sub_001CCC00(void),sub_001CCE00(void),sub_000EE4B0(void);
static int factory_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_EXPLOSION_FACTORY");flag=v&&!strcmp(v,"1");}return flag;}
int uc_hybrid_explosion_factory(unsigned kind){
 if(!factory_enabled())return 0;
 uint32_t stack=g_esp;
 if(kind>2||(stack&3)||stack<0x10400u||(uint64_t)stack+16>g_xbox_total_ram||g_xbox_total_ram<=0x51c178||MEM8(0x51c178))return 0;
 static const uint32_t pc[]={0x1ccda0,0x1ccfa0,0xee5f0},scope[]={0x399dcb,0x399e0b,0x38e4bb};
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,scope[kind]);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);
 PUSH32(g_esp,kind==2?0x240:0xd0);PUSH32(g_esp,pc[kind]+32);sub_002D57FB();g_esp+=4;MEM32(g_esp)=g_eax;MEM32(g_esp+12)=0;
 if(g_eax){
  g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,pc[kind]+62);
  if(kind==0)sub_001CCC00();else if(kind==1)sub_001CCE00();else sub_000EE4B0();
 }
 else g_eax=0;
 g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
