/* Original 73-bone name lookup and one-time parent index table. */
#include "migration_physics_ragdoll_names.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int string_valid(uint32_t p){for(unsigned i=0;i<256;i++){if(!span(p+i,1))return 0;if(!MEM8(p+i))return 1;}return 0;}
static void lookup(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);frame=0;g_edi=0;int found=0;
 do{
  g_ecx=MEM32(0x503848+g_edi);g_esi=g_eax;int comparison=0;
  for(;;){SET_LO8(g_ebx,MEM8(g_ecx));SET_LO8(g_edx,(uint8_t)g_ebx);unsigned a=(uint8_t)g_ebx,b=MEM8(g_esi);if(a!=b){comparison=a<b?-1:1;break;}if(!(uint8_t)g_edx)break;SET_LO8(g_ebx,MEM8(g_ecx+1));SET_LO8(g_edx,(uint8_t)g_ebx);a=(uint8_t)g_ebx;b=MEM8(g_esi+1);if(a!=b){comparison=a<b?-1:1;break;}g_ecx+=2;g_esi+=2;if(!(uint8_t)g_edx)break;}
  g_ecx=(uint32_t)comparison;if(!comparison){found=1;break;}g_edi+=12;++frame;
 }while(g_edi<876);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=found?frame:0xffffffff;POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=found;g_esp+=4;
}
static void initialize(void){
 SET_LO8(g_eax,MEM8(0x51dad0));if(!(uint8_t)g_eax){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=0x51d920;g_esi=0;do{g_eax=MEM32(g_esi+0x50384c);PUSH32(g_esp,0x2a500d);lookup();MEM32(g_edi)=g_eax;g_esi+=12;g_edi+=4;}while(g_esi<876);POP32(g_esp,g_edi);MEM8(0x51dad0)=1;POP32(g_esp,g_esi);}g_esp+=4;
}
int uc_migration_physics_ragdoll_names(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_RAGDOLL_NAMES");enabled=v&&!strcmp(v,"1");}if(!enabled||(entry!=0x2a4f90&&entry!=0x2a4ff0)||!span(g_esp-64,96)||!span(0x503848,876)||!span(0x51d920,433))return 0;
 if(entry==0x2a4ff0&&MEM8(0x51dad0)){initialize();return 1;}if(entry==0x2a4f90&&!string_valid(g_eax))return 0;for(unsigned i=0;i<73;i++){if(!string_valid(MEM32(0x503848+i*12))||(entry==0x2a4ff0&&!string_valid(MEM32(0x50384c+i*12))))return 0;}
 if(entry==0x2a4f90)lookup();else initialize();return 1;
}
