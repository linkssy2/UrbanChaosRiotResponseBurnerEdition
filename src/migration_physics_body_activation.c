/* Original body wake request and deduplicated dirty-island queue insertion. */
#include "migration_physics_body_activation.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-128||(uint64_t)g_esp+32<=p);}
static int queue_valid(uint32_t world,uint32_t island){if(!safe(island,41))return 0;if(MEM16(island+34)!=0xffff)return 1;if(!safe(world,92))return 0;uint32_t n=MEM32(world+84),cap=MEM32(world+88)&0x3fffffff;return n<=65535&&cap<=65535&&n<=cap&&(!cap||safe(MEM32(world+80),cap*4));}
static void request_island(void){
 PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);MEM8(g_edi+40)=1;SET_LO8(g_eax,0);MEM8(g_edi+37)=(uint8_t)g_eax;MEM8(g_edi+36)=(uint8_t)g_eax;
 if(MEM16(g_edi+34)==0xffff){g_eax=MEM32(g_esp+8);SET_LO16(g_ecx,MEM16(g_eax+84));PUSH32(g_esp,g_esi);g_esi=g_eax+80;MEM16(g_edi+34)=(uint16_t)g_ecx;g_edx=MEM32(g_esi+8)&0x3fffffff;g_eax=MEM32(g_esi+4);if(g_eax==g_edx){PUSH32(g_esp,4);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2f6d01);sub_002E0150();g_esp+=8;}g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi);MEM32(g_ecx+g_eax*4)=g_edi;++MEM32(g_esi+4);POP32(g_esp,g_esi);}
 POP32(g_esp,g_edi);g_esp+=4;
}
static void request_body(void){g_eax=MEM32(g_ecx+84);int awake=0;if(g_eax){SET_LO8(g_edx,MEM8(g_eax+40));awake=(uint8_t)g_edx!=0;}if(!awake){SET_LO8(g_edx,MEM8(g_ecx+145));if(!(uint8_t)g_edx){g_ecx=MEM32(g_ecx+8);if(g_ecx){PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x2ecac6);request_island();g_esp+=8;}}}g_esp+=4;}
int uc_migration_physics_body_activation(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_ACTIVATION");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-128,160))return 0;
 if(entry==0x2f6cc0){if(!queue_valid(MEM32(g_esp+4),MEM32(g_esp+8)))return 0;request_island();return 1;}if(entry!=0x2ecaa0||!safe(g_ecx,146))return 0;uint32_t island=MEM32(g_ecx+84);if(island&&!safe(island,41))return 0;if((!island||!MEM8(island+40))&&!MEM8(g_ecx+145)&&MEM32(g_ecx+8)&&!queue_valid(MEM32(g_ecx+8),island))return 0;request_body();return 1;
}
