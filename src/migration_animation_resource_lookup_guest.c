#include "migration_animation_resource_lookup_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0004FFE0(void),sub_002D76F3(void),sub_001F03D0(void),sub_001F4130(void),sub_002D631D(void);
static int resource_lookup_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int resource_lookup_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int resource_lookup_text(uint32_t p){for(unsigned i=0;i<=4096;i++){if(!resource_lookup_span(p+i,1))return 0;if(!MEM8(p+i))return 1;}return 0;}
/* Read-only structural validation. Execution reuses the existing indivisible
 * manual iterator, including its guest register effects. */
static int resource_lookup_next(uint32_t current,uint32_t *result){
 uint32_t next=MEM32(current+8);if(!resource_lookup_span(next,18))return 0;unsigned steps=0;
 if(!MEM8(next+17)){do{if(++steps>1024)return 0;current=next;next=MEM32(next);if(!resource_lookup_span(next,18))return 0;}while(!MEM8(next+17));*result=current;}
 else{next=MEM32(current+4);if(!resource_lookup_span(next,18))return 0;while(!MEM8(next+17)&&current==MEM32(next+8)){if(++steps>1024)return 0;current=next;next=MEM32(next+4);if(!resource_lookup_span(next,18))return 0;}*result=next;}
 return 1;
}
int uc_migration_animation_resource_lookup(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_RESOURCE_LOOKUP");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;if(!enabled||(s&3)||s<0x10400||!resource_lookup_span(s-1024,1032)||!resource_lookup_span(p,20)||!resource_lookup_apart(p,20,s-1024,1032)||!resource_lookup_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x1f08c0){
  uint32_t text=MEM32(s+4);if(text&&(!resource_lookup_text(text)||!resource_lookup_apart(text,4097,s-1024,1032)))return 0;if(!resource_lookup_span(MEM32(p+16),8))return 0;
  PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39b788);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=16;g_eax=MEM32(g_esp+32);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=0;g_esi=g_ecx;MEM32(g_esp+16)=2;
  if(!g_eax){MEM32(g_esp+8)=0;MEM32(g_esp+12)=0;MEM32(g_esp+20)=0;}else{PUSH32(g_esp,g_eax);g_ecx=g_esp+12;PUSH32(g_esp,0x1f0906);sub_001F03D0();}
  g_ecx=MEM32(g_esi+16);g_eax=g_esp+8;PUSH32(g_esp,g_eax);MEM32(g_esp+36)=g_edi;PUSH32(g_esp,0x1f0917);sub_001F4130();g_esi=g_eax;
  if(MEM32(g_esp+16)==1){g_ecx=MEM32(g_esp+8);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f092a);sub_002D631D();g_esp+=4;}
  g_ecx=MEM32(g_esp+24);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
 }
 if(entry!=0x1f4130)return 0;
 uint32_t query=MEM32(s+4),count=MEM32(p),buckets=MEM32(p+4);if(!count||count>4096||!resource_lookup_span(buckets,count*12)||!resource_lookup_apart(buckets,count*12,s-1024,1032)||!resource_lookup_span(query,16)||!resource_lookup_apart(query,16,s-1024,1032)||MEM32(query+4)>4096||(MEM32(query+4)&&!resource_lookup_text(MEM32(query))))return 0;
 uint32_t bucket=buckets+((count-1)&MEM32(query+12))*12,head=MEM32(bucket+4);if(!resource_lookup_span(head,18)||!resource_lookup_apart(head,18,s-1024,1032)||!MEM8(head+17))return 0;
 uint32_t node=MEM32(head),seen[1024];unsigned used=0;
 while(node!=head){if(used==1024||!resource_lookup_span(node,18)||MEM8(node+17)||!resource_lookup_apart(node,18,s-1024,1032))return 0;
  /* ponytail: a bounded linear duplicate scan; use a set if the cap grows. */
  for(unsigned i=0;i<used;i++)if(seen[i]==node)return 0;seen[used++]=node;uint32_t record=MEM32(node+12);if(!resource_lookup_span(record,16)||!resource_lookup_apart(record,16,s-1024,1032)||MEM32(record+4)>4096||(MEM32(record+4)&&!resource_lookup_text(MEM32(record))))return 0;if(!resource_lookup_next(node,&node))return 0;
 }
 PUSH32(g_esp,g_ecx);g_eax=MEM32(g_ecx);g_ecx=MEM32(g_ecx+4);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+16);g_edx=MEM32(g_esi+12);g_eax=(g_eax-1)&g_edx;g_eax+=g_eax*2;g_ebx=g_ecx+g_eax*4;g_eax=MEM32(g_ebx+4);PUSH32(g_esp,g_edi);g_edi=MEM32(g_eax);MEM32(g_esp+12)=g_edi;int found=0,active=g_edi!=g_eax;
 while(active){g_eax=MEM32(g_edi+12);g_edx=MEM32(g_eax+12);
  if(g_edx==MEM32(g_esi+12)){g_ecx=MEM32(g_eax+4);if(g_ecx==MEM32(g_esi+4)){if(!g_ecx)found=1;else{g_eax=MEM32(g_eax);g_ecx=MEM32(g_esi);if(g_eax==g_ecx)found=1;else{PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f417c);sub_002D76F3();g_esp+=8;int unequal=g_eax!=0;g_eax=0u-g_eax;SET_LO8(g_eax,unequal?0xff:0);SET_LO8(g_eax,(uint8_t)(LO8(g_eax)+1u));MEM8(g_esp+20)=LO8(g_eax);found=LO8(g_eax)!=0;}}}}
  if(found)break;g_ecx=g_esp+12;PUSH32(g_esp,0x1f4194);sub_0004FFE0();g_edi=MEM32(g_esp+12);active=g_edi!=MEM32(g_ebx+4);
 }
 if(found){g_eax=MEM32(g_edi+12);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);}else{POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=0;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=1;}g_esp+=8;return 1;
}
