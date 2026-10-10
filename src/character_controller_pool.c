/* Retained objects can exhaust several original fixed-size pools. */
#include "character_controller_pool.h"
#include "hybrid_guest_cpu.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D83F8(void),sub_002D73A4(void);
typedef struct Spill {struct Spill *next;uint32_t owner,object;} Spill;
static Spill *spills;
static int span(uint32_t p,uint32_t n){return p>=0x10000&&(uint64_t)p+n<=g_xbox_total_ram;}
static void release_object(uint32_t object){
 HybridCPU cpu;save_cpu(&cpu);
 PUSH32(g_esp,object);PUSH32(g_esp,0x238502);sub_002D73A4();g_esp+=4;
 restore_cpu(&cpu);
}
int uc_character_controller_pool(unsigned entry){
 uint32_t owner=g_ecx;
 if(entry==0x238440||entry==0x2384f0){
  if(!span(g_esp,8))return 0;
  Spill **link=&spills;
  while(*link){
   Spill *s=*link;
   if(s->owner==owner&&(entry==0x2384f0||s->object==MEM32(g_esp+4))){
    *link=s->next;release_object(s->object);free(s);
    if(entry==0x238440){g_esp+=8;return 1;}
   }else link=&s->next;
  }
  return 0; /* Original teardown still owns its contiguous pool storage. */
 }
 if(entry!=0x238430||!span(owner,28)||!span(g_esp-512,520)||
    MEM32(owner)!=0x482e94||!MEM32(owner+8)||MEM32(owner+8)>g_xbox_total_ram||MEM32(owner+0x14))return 0;
 Spill *s=malloc(sizeof(*s));uint32_t object=0;
 if(s){
  HybridCPU cpu;save_cpu(&cpu);
  PUSH32(g_esp,MEM32(owner+8));PUSH32(g_esp,0x2383e8);sub_002D83F8();g_esp+=4;object=g_eax;
  restore_cpu(&cpu);
  if(object){s->owner=owner;s->object=object;s->next=spills;spills=s;}else free(s);
 }
 /* Leave the exhausted free-list at zero; never read slot minus one. */
 g_eax=object;g_recomp_zf=!object;g_esp+=4;return 1;
}
