#include "migration_services_heap_metadata_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int metadata_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_services_heap_metadata(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_HEAP_METADATA");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!metadata_span(g_esp-16,36))return 0;
 if(entry==0x831bb){
  unsigned ptr=MEM32(g_esp+12);if(!metadata_span(ptr-24,24))return 0;g_ecx=ptr;SET_LO8(g_eax,MEM8(g_ecx-11));
  if(!(LO8(g_eax)&1))g_eax=0xffffffff;
  else if(LO8(g_eax)&8){g_edx=MEM16(g_ecx-16);g_eax=MEM32(g_ecx-24)-g_edx;}
  else{g_eax=MEM16(g_ecx-16);g_ecx=MEM8(g_ecx-10);g_eax=(g_eax<<4)-g_ecx;}
  g_esp+=16;return 1;
 }
 if(entry==0x82d12){
  unsigned ptr=MEM32(g_esp+4);if(!metadata_span(ptr,8))return 0;g_eax=ptr;
  if(MEM8(g_eax+5)&8)g_eax-=24;else{g_ecx=(unsigned)MEM16(g_eax)<<4;g_eax+=g_ecx-16;}
  g_esp+=8;return 1;
 }
 if(entry==0x84be4){
  if(!metadata_span(g_seh_ebp-44,16))return 0;int empty=!MEM8(g_seh_ebp-29);
  if(!empty){unsigned heap=MEM32(g_seh_ebp-44);if(!metadata_span(heap+0x580,4))return 0;g_eax=heap;unsigned saved=g_esp;PUSH32(g_esp,MEM32(g_eax+0x580));unsigned target=MEM32(0x44b3ec);PUSH32(g_esp,0x84bf9);RECOMP_ICALL_SAFE(target,saved);}
  g_recomp_zf=empty;g_esp+=4;return 1;
 }
 return 0;
}
