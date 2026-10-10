/* Original endpoint quicksort: equal-key exchanges and guest recursion preserved. */
#include "migration_physics_broadphase_sort.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static void sort_endpoints(uint32_t ret){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=~15u;PUSH32(g_esp,g_ecx);g_edx=MEM32(frame+12);PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+16);PUSH32(g_esp,g_esi);g_esi=MEM32(frame+8);PUSH32(g_esp,g_edi);
 for(;;){g_ecx=(uint32_t)((int32_t)(g_edx+g_ebx)>>1);g_ecx=MEM32(g_esi+g_ecx*4);g_eax=g_ebx;g_edi=g_edx;
  for(;;){while(MEM16(g_esi+g_edi*4)<(uint16_t)g_ecx)++g_edi;while((uint16_t)g_ecx<MEM16(g_esi+g_eax*4))--g_eax;if((int32_t)g_eax<(int32_t)g_edi)break;if(g_eax!=g_edi){g_ebx=MEM32(g_esi+g_edi*4);g_edx=MEM32(g_esi+g_eax*4);MEM32(g_esi+g_eax*4)=g_ebx;g_ebx=MEM32(frame+16);MEM32(g_esi+g_edi*4)=g_edx;g_edx=MEM32(frame+12);}--g_eax;++g_edi;if((int32_t)g_edi>(int32_t)g_eax)break;}
  if((int32_t)g_edx<(int32_t)g_eax){g_ecx=MEM32(frame+20);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,ret);sort_endpoints(ret);g_esp+=16;}
  if((int32_t)g_edi>=(int32_t)g_ebx)break;g_edx=g_edi;MEM32(frame+12)=g_edx;
 }
 g_recomp_zf=g_edi==g_ebx;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=4;
}
int uc_migration_physics_broadphase_sort(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE_SORT");enabled=v&&!strcmp(v,"1");}if(!enabled||(entry!=0x324470&&entry!=0x3244f0)||!span(g_esp,20))return 0;
 uint32_t data=MEM32(g_esp+4),lo=MEM32(g_esp+8),hi=MEM32(g_esp+12);if(hi>=65535||lo>hi)return 0;uint32_t bytes=(hi+1)*4,depth=(hi-lo+1)*64;
 if(!span(g_esp-depth,depth+20)||!span(data,bytes)||!((uint64_t)data+bytes<=g_esp-depth||(uint64_t)g_esp+20<=data))return 0;
 sort_endpoints(entry==0x324470?0x3244d8:0x324558);return 1;
}
