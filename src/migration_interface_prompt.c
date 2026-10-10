#include "migration_interface_prompt.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00017D00(void),sub_002D6E2C(void),sub_002730B0(void),sub_00273100(void),sub_00273190(void),sub_00216B70(void);
#define PROMPT_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
static int prompt_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void prompt_start(void){
 g_ecx=MEM32(0x51d2b0);g_esp-=12;if(!g_ecx){g_xmm0=XMM_ZERO();g_eax=g_esp;PUSH32(g_esp,g_eax);MEMF(g_esp+4)=g_xmm0.f[0];MEMF(g_esp+8)=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm0.f[0];PROMPT_CALL(00216B70,0x2731bb);MEM32(0x51d2b0)=g_eax;g_edx=MEM32(g_eax);g_esp+=4;g_ecx=g_eax;unsigned saved=g_esp,target=MEM32(g_edx+56);PUSH32(g_esp,0x2731ca);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x51d2b0);}
 if(g_ecx){g_eax=MEM32(g_esp+20);if(g_eax){g_edx=MEM32(g_esp+16);unsigned saved=g_esp,target;if(g_edx){PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);target=MEM32(g_esi+4);PUSH32(g_esp,0x2731ee);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);}else{g_edx=MEM32(g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,0x4759d8);PUSH32(g_esp,g_eax);target=MEM32(g_edx+4);PUSH32(g_esp,0x273200);RECOMP_ICALL_SAFE(target,saved);}}}g_esp+=12;g_recomp_zf=!g_esp;g_esp+=4;
}
static void prompt_activate(void){
 PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_eax=g_esp+4;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);MEM8(0x501340)=1;PROMPT_CALL(00017D00,0x275c9e);g_esi=g_eax;if(!g_esi)goto no;
 SET_LO8(g_eax,MEM8(0x51d2ec));if(LO8(g_eax)){g_eax=MEM32(g_esp+4);if(g_eax){PUSH32(g_esp,g_eax);PROMPT_CALL(002D6E2C,0x275cbf);g_esp+=4;if((int32_t)g_eax<10)goto yes;g_edx=MEM32(g_esp+4);PUSH32(g_esp,g_edx);PROMPT_CALL(002D6E2C,0x275cd5);g_esp+=4;if((int32_t)g_eax>15)goto yes;}}
 SET_LO8(g_eax,MEM8(g_esp+16));if(LO8(g_eax)){PROMPT_CALL(002730B0,0x275cea);int ready=0;if(g_eax){g_edx=MEM32(g_eax);unsigned saved=g_esp;PUSH32(g_esp,0x51d2dc);PUSH32(g_esp,g_esi);g_ecx=g_eax;unsigned target=MEM32(g_edx+32);PUSH32(g_esp,0x275cfb);RECOMP_ICALL_SAFE(target,saved);ready=LO8(g_eax)!=0;}if(ready){PUSH32(g_esp,0x51d2dc);PUSH32(g_esp,g_esi);PROMPT_CALL(00273100,0x275d13);g_esp+=8;}else MEM8(0x51d2dc)=0;}
 PROMPT_CALL(002730B0,0x275d1b);if(g_eax){g_edx=MEM32(g_eax);unsigned saved=g_esp;PUSH32(g_esp,0x51d2dc);PUSH32(g_esp,g_esi);g_ecx=g_eax;unsigned target=MEM32(g_edx+32);PUSH32(g_esp,0x275d2c);RECOMP_ICALL_SAFE(target,saved);if(LO8(g_eax))goto no;}
 g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);MEM8(0x51d2dc)=0;PROMPT_CALL(00273190,0x275d42);g_eax=MEM32(g_esp+12);g_edx=0x51d2dc;g_esp+=8;g_edx-=g_eax;do{SET_LO8(g_ecx,MEM8(g_eax));MEM8(g_edx+g_eax)=LO8(g_ecx);g_eax++;}while(LO8(g_ecx));
yes: SET_LO8(g_eax,1);POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_esp+=4;return;
no: SET_LO8(g_eax,0);POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=4;
}
int migration_interface_prompt(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_PROMPT");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation>3||(g_esp&3)||!prompt_span((uint64_t)g_esp-1024,1040))return 0;
 unsigned service=MEM32(0x51d2b0);if(operation!=1&&service&&(!prompt_span(service,4)||!prompt_span(MEM32(service),60)))return 0;
 if(operation==1){g_eax=MEM32(0x51d2b0);g_esp+=4;}else if(operation==2){g_ecx=MEM32(0x51d2b0);int absent=!g_ecx;if(g_ecx){g_edx=MEM32(g_esp+8);g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+8);PUSH32(g_esp,g_edx);unsigned target=MEM32(g_eax+20);PUSH32(g_esp,0x273119);RECOMP_ICALL_SAFE(target,saved);}g_recomp_zf=absent;g_esp+=4;}else if(operation==3)prompt_start();else prompt_activate();return 1;
}
