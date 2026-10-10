#include "migration_services_registry_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_services_registry(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_REGISTRY");enabled=p&&!strcmp(p,"1");}
 if(!enabled)return 0;
 uint32_t s=g_esp,p=g_ecx;if(!span(s-4,16)||!span(0x51c4dc,4)||MEM8(0x51c178))return 0;
 switch(entry){
 case 0x207c70:{
  if(!span(p,0x2004)||((uint64_t)p<s+12u&&(uint64_t)p+0x2004>s-4u))return 0;
  MEM32(s-4)=g_esi;uint32_t next=MEM32(p+0x2000);int found=0;
  for(unsigned i=0;i<1024;i++){
   g_eax=next+i;if(g_eax==0xffffffffu)continue;
   g_edx=g_eax&1023;
   if(MEM32(p+g_edx*8)!=0xffffffffu)continue;
   MEM32(p+g_edx*8)=g_eax;MEM32(p+g_edx*8+4)=MEM32(s+4);g_edx=g_eax+1;MEM32(p+0x2000)=g_edx;
   if((int32_t)g_edx>=0x7ffffbff){MEM32(p+0x2000)=0;g_recomp_zf=g_edx==0x7ffffbff;}
   found=1;break;
  }
  if(!found)g_eax=0xffffffffu;
  g_esp+=8;return 1;
 }
 case 0x207d70:{
  if(!span(p+4,4))return 0;uint32_t table=MEM32(p+4)+0x3c,id=MEM32(s+4);
  if(!span(table,0x2000))return 0;
  g_ecx=table;g_edx=id;g_eax=id!=0xffffffffu&&MEM32(table+(id&1023)*8)==id?MEM32(table+(id&1023)*8+4):0;
  g_recomp_zf=1;g_esp+=8;return 1;
 }
 case 0x1f9010:
  p=MEM32(0x51c4dc);if(!span(p,24))return 0;g_eax=p;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(p+20);g_esp+=4;return 1;
 case 0x1f9140:
  if(!span(p,4))return 0;MEM32(p)=0x44e3b0;g_esp+=4;return 1;
 case 0x1f92b0:
  p=MEM32(s+4);if(!span(p,4))return 0;g_eax=p;g_ecx=MEM32(s+8);MEM32(p)=g_ecx;g_esp+=4;return 1;
 case 0x208c50:
  if(!span(p,8))return 0;p=MEM32(p+4);if(!span(p,0x2074))return 0;g_eax=MEM32(p+0x2070);g_esp+=4;return 1;
 default:return 0;
 }
}
