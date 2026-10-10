#include "migration_services_lists_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void),sub_002D5CCD(void),sub_00011EF0(void),sub_00012050(void),sub_002D619E(void);
#define LIST_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int list_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int list_valid(uint32_t owner){
 if(!list_span(owner,12))return 0;uint32_t sentinel=MEM32(owner+4);if(!list_span(sentinel,8))return 0;
 uint32_t node=MEM32(sentinel),prev=sentinel;unsigned n=0;
 while(node!=sentinel){if(++n>4096||!list_span(node,12)||MEM32(node+4)!=prev)return 0;prev=node;node=MEM32(node);}
 return MEM32(sentinel+4)==prev;
}
static void list_clear(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);MEM32(g_ecx)=g_ecx;g_ecx=MEM32(g_esi+4);MEM32(g_ecx+4)=g_ecx;MEM32(g_esi+8)=0;
 if(g_eax!=MEM32(g_esi+4)){PUSH32(g_esp,g_edi);do{g_edi=MEM32(g_eax);PUSH32(g_esp,g_eax);LIST_CALL(sub_002D5CCD,0x2012e8);g_ecx=MEM32(g_esi+4);g_esp+=4;g_eax=g_edi;}while(g_edi!=g_ecx);POP32(g_esp,g_edi);}POP32(g_esp,g_esi);g_esp+=4;
}
static void list_destroy(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x50948);list_clear();g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);LIST_CALL(sub_002D5CCD,0x50951);g_esp+=4;MEM32(g_esi+4)=0;POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=4;
}
static void list_sentinel(int large){
 PUSH32(g_esp,large?100:12);PUSH32(g_esp,large?0x22a647:0x2c3747);sub_002D57FB();g_esp+=4;if(g_eax)MEM32(g_eax)=g_eax;g_ecx=g_eax+4;if(g_ecx)MEM32(g_ecx)=g_eax;g_recomp_zf=!g_ecx;g_esp+=4;
}
static void list_node(void){
 PUSH32(g_esp,12);LIST_CALL(sub_002D57FB,0x2bcfa7);g_esp+=4;int missing=!g_eax;
 if(!missing){g_ecx=MEM32(g_esp+4);g_edx=MEM32(g_esp+8);MEM32(g_eax)=g_ecx;g_ecx=MEM32(g_esp+12);MEM32(g_eax+4)=g_edx;g_edx=MEM32(g_ecx);MEM32(g_eax+8)=g_edx;}
 g_recomp_zf=missing;g_esp+=16;
}
static void list_count(void){
 g_eax=MEM32(0);g_edx=MEM32(g_esp+4);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x389848);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_eax=MEM32(g_ecx+8);g_esp-=68;PUSH32(g_esp,g_esi);g_esi=0x3fffffffu-g_eax;int overflow=g_esi<g_edx;POP32(g_esp,g_esi);
 if(overflow){
  PUSH32(g_esp,16);PUSH32(g_esp,0x44c57c);g_ecx=g_esp+8;MEM32(g_esp+32)=15;MEM32(g_esp+28)=0;MEM8(g_esp+12)=0;LIST_CALL(sub_00011EF0,0x19801);
  g_eax=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esp+32;MEM32(g_esp+80)=0;LIST_CALL(sub_00012050,0x19816);
  PUSH32(g_esp,0x4c37ac);g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);MEM32(g_esp+36)=0x44e000;LIST_CALL(sub_002D619E,0x1982d);
 }
 g_eax+=g_edx;MEM32(g_ecx+8)=g_eax;g_ecx=MEM32(g_esp+68);MEM32(0)=g_ecx;g_esp+=80;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void list_append(void){
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+4);g_ecx=MEM32(g_edi+4);g_eax=g_esp+16;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x1f9289);list_node();
 PUSH32(g_esp,1);g_ecx=g_esi;g_ebx=g_eax;PUSH32(g_esp,0x1f9294);list_count();MEM32(g_edi+4)=g_ebx;g_edx=MEM32(g_ebx+4);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(g_edx)=g_ebx;POP32(g_esp,g_ebx);g_esp+=8;
}
static void list_create(int large){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,large?0x39db8b:0x39bbeb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);if(large)PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,large?20:12);g_edi=g_ecx;PUSH32(g_esp,large?0x22a692:0x1f9221);sub_002D57FB();
 g_esi=g_eax;g_esp+=4;MEM32(g_esp+(large?12:8))=g_esi;if(large)g_ebx=0;MEM32(g_esp+(large?24:20))=0;
 if(g_esi){g_ecx=g_esi;PUSH32(g_esp,large?0x22a6ac:0x1f923d);list_sentinel(large);MEM32(g_esi+4)=g_eax;MEM32(g_esi+8)=0;if(large){MEM32(g_esi+12)=0;MEM32(g_esi+16)=0;}}
 if(!large){g_ecx=MEM32(g_esp+12);MEM32(g_edi)=g_esi;MEM32(0x51c3a0)=g_edi;g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);}
 else{if(!g_esi)g_eax=0;MEM32(g_edi)=g_esi;g_eax=g_edi;g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);}
 MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_services_lists(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_LISTS");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!list_span(g_esp-256,284))return 0;uint32_t p=g_ecx;
 switch(entry){
 case 0x2012c0:case 0x50940:if(!list_valid(p))return 0;break;
 case 0x22a660:case 0x22a6f0:case 0x1f9270:
  if(!list_span(p,4))return 0;p=MEM32(p);if(p||entry!=0x22a6f0){if(!list_valid(p))return 0;}break;
 case 0x1f9200:case 0x22a670:if(!list_span(p,4))return 0;break;
 case 0x197b0:if(!list_span(p,12))return 0;break;
 case 0x2bcfa0:if(!list_span(MEM32(g_esp+12),4))return 0;break;
 case 0x22a640:case 0x2c3740:break;
 default:return 0;
 }
 switch(entry){
 case 0x2012c0:list_clear();break;
 case 0x50940:list_destroy();break;
 case 0x22a660:g_ecx=MEM32(g_ecx);list_clear();break;
 case 0x22a6f0:PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);if(g_esi){g_ecx=g_esi;PUSH32(g_esp,0x22a6fe);list_destroy();PUSH32(g_esp,g_esi);LIST_CALL(sub_002D5CCD,0x22a704);g_esp+=4;}POP32(g_esp,g_esi);g_esp+=4;break;
 case 0x22a640:case 0x2c3740:list_sentinel(entry==0x22a640);break;
 case 0x2bcfa0:list_node();break;
 case 0x197b0:list_count();break;
 case 0x1f9270:list_append();break;
 case 0x22a670:case 0x1f9200:list_create(entry==0x22a670);break;
 }
 return 1;
}
#undef LIST_CALL
