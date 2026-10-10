/* The original five-slot ragdoll pool, including unsigned ID reuse and wrap. */
#include "migration_physics_ragdoll_pool.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include "hybrid_guest_cpu.h"
#include <windows.h>
#include <math.h>
void sub_002A5F00(void);void sub_002EEFF0(void);void sub_002D57FB(void);void sub_0029E4F0(void);void sub_002A6130(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+64<=p);}
typedef struct { volatile uint32_t *extra; uint32_t bytes,limit; } Pool;
static volatile uint32_t *word(Pool *p,uint32_t offset){return offset<40?&MEM32(0x51da44+offset):p->extra+(offset-40)/4;}
static void lookup(Pool *p){g_edx=MEM32(g_esp+4);g_eax=0;if(g_edx!=0xffffffff){g_ecx=0x51da44;while(*word(p,g_ecx-0x51da44)!=g_edx){g_eax+=8;g_ecx+=8;if(g_eax>=p->bytes){g_eax=0;g_esp+=4;return;}}g_eax=*word(p,g_ecx-0x51da44+4);g_recomp_zf=1;}g_esp+=4;}
static void destroy_pool(Pool *p){
 SET_LO8(g_eax,MEM8(0x51da6c));if((uint8_t)g_eax){PUSH32(g_esp,g_esi);g_esi=0;do{g_ecx=*word(p,g_esi+4);if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2a5050);RECOMP_ICALL_SAFE(target,saved);}*word(p,g_esi+4)=0;g_esi+=8;}while(g_esi<p->bytes);MEM8(0x51da6c)=0;POP32(g_esp,g_esi);}g_esp+=4;
}
static void update_pool(Pool *p){
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_ebx=0;do{if(*word(p,g_ebx)!=0xffffffff){g_esi=*word(p,g_ebx+4);g_eax=MEM32(g_esi+0x74);g_edi=0;while((int32_t)g_edi<(int32_t)g_eax){g_eax=MEM32(g_esi+0x70);g_ecx=MEM32(g_eax+g_edi*4);PUSH32(g_esp,1);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+0x88);CALL(0x2a5d86,sub_002EEFF0);g_eax=MEM32(g_esi+0x74);++g_edi;}}g_ebx+=8;}while(g_ebx<p->bytes);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=4;
}
static void acquire(Pool *p){
 if(p->extra){PUSH32(g_esp,g_esi);g_esi=0;for(unsigned slot=0;slot<p->limit/8;slot++){uint32_t id=*word(p,slot*8);if(id==0xffffffff){g_esi=slot;break;}if(id<*word(p,g_esi*8))g_esi=slot;}}
 else{ g_eax=0;g_ecx=0xffffffff;PUSH32(g_esp,g_esi);while(g_eax<5&&MEM32(g_eax*8+0x51da44)!=g_ecx)++g_eax;
 if(g_eax<5)g_esi=g_eax;else{g_eax=MEM32(0x51da4c);g_ecx=MEM32(0x51da44);g_esi=g_eax<g_ecx?1:0;g_ecx=MEM32(0x51da54);if(g_ecx<*word(p,g_esi*8))g_esi=2;g_edx=MEM32(0x51da5c);if(g_edx<*word(p,g_esi*8))g_esi=3;g_eax=MEM32(0x51da64);if(g_eax<*word(p,g_esi*8))g_esi=4;}
}
 g_eax=MEM32(0x51da70);g_ecx=MEM32(g_esp+24);g_edx=MEM32(g_esp+28);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+20);*word(p,g_esi*8)=g_eax;++g_eax;uint32_t next=g_eax;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+20);MEM32(0x51da70)=g_eax;g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=*word(p,g_esi*8+4);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);CALL(0x2a683a,sub_002A5F00);g_eax=*word(p,g_esi*8);POP32(g_esp,g_esi);g_recomp_zf=next==0;g_esp+=4;
}
static void release(Pool *p){
 PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+8);if(g_edi!=0xffffffff){PUSH32(g_esp,g_esi);g_esi=0;do{if(*word(p,g_esi)==g_edi){g_ecx=*word(p,g_esi+4);for(unsigned i=0;i<6;i++)PUSH32(g_esp,0);*word(p,g_esi)=0xffffffff;CALL(0x2a6889,sub_002A5F00);}g_esi+=8;}while(g_esi<p->bytes);POP32(g_esp,g_esi);}POP32(g_esp,g_edi);g_esp+=4;
}
static void construct_pool(Pool *p){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x3a179b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+32);PUSH32(g_esp,g_esi);g_esi=0;PUSH32(g_esp,g_edi);g_ebx=0xffffffff;
 do{PUSH32(g_esp,0xa0);*word(p,g_esi)=g_ebx;CALL(0x2a6ab0,sub_002D57FB);g_edi=g_eax;g_esp+=4;MEM32(g_esp+16)=g_edi;g_eax=0;MEM32(g_esp+28)=0;if(g_edi){g_eax=MEM32(g_esp+36);PUSH32(g_esp,frame);PUSH32(g_esp,g_eax);CALL(0x2a6ace,sub_0029E4F0);PUSH32(g_esp,g_eax);g_ecx=g_edi;CALL(0x2a6ad6,sub_002A6130);}*word(p,g_esi+4)=g_eax;g_esi+=8;MEM32(g_esp+28)=g_ebx;}while(g_esi<p->bytes);
 g_ecx=MEM32(g_esp+20);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM8(0x51da6c)=1;POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
/* PC retention owns only pool bookkeeping; original constructors, simulation,
 * explicit release and mission destruction retain their ownership. */
static volatile LONG retention_mode;
static int retention_configured;
static Pool retained={NULL,40,40};
unsigned uc_ragdoll_retention_get(void){return (unsigned)InterlockedCompareExchange(&retention_mode,0,0);}
void uc_ragdoll_retention_set(unsigned mode){if(mode<=2){InterlockedExchange(&retention_mode,(LONG)mode);retention_configured=1;}}
double uc_ragdoll_retention_seconds(void){return !retention_configured?120.0:uc_ragdoll_retention_get()==2?INFINITY:uc_ragdoll_retention_get()==1?120.0:30.0;}
static int grow_retained(void){
 uint32_t source=*word(&retained,4),count=retained.bytes/8;
 if(!safe(source,0xa0))return 0;
 for(unsigned offset=0x70;offset<=0x7c;offset+=12){uint32_t n=MEM32(source+offset+4);if(n>65535||(n&&!safe(MEM32(source+offset),n*4)))return 0;}
 if(count>=UINT32_MAX/8-1)return 0;
 volatile uint32_t *extra=realloc((void*)retained.extra,(size_t)(count-4)*8);
 if(!extra)return 0;
 retained.extra=extra;
 HybridCPU cpu;save_cpu(&cpu);
 PUSH32(g_esp,0xa0);CALL(0x2a6ab0,sub_002D57FB);g_esp+=4;
 uint32_t object=g_eax;
 if(object){
  /* Setup asset lists are borrowed; clone the live pool-owned bodies/joints. */
  PUSH32(g_esp,source+0x7c);PUSH32(g_esp,source+0x70);CALL(0x2a6ace,sub_0029E4F0);
  PUSH32(g_esp,g_eax);g_ecx=object;CALL(0x2a6ad6,sub_002A6130);object=g_eax;
 }
 restore_cpu(&cpu);
 if(!object)return 0;
 *word(&retained,count*8)=UINT32_MAX;*word(&retained,count*8+4)=object;retained.bytes+=8;return 1;
}
static int run(unsigned entry,volatile uint32_t *extra){
 Pool pool={extra,extra?256u:40u,extra?256u:40u};Pool *p=&pool;
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_RAGDOLL_POOL");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-256,320)||!safe(0x51da44,48))return 0;
 if(retention_configured){
  p=&retained;
  if(entry==0x2a6790){
   unsigned mode=uc_ragdoll_retention_get();p->limit=mode==0?40:mode==1?256:p->bytes+8;
   unsigned slot=0;while(slot<p->bytes/8&&slot<p->limit/8&&*word(p,slot*8)!=UINT32_MAX)++slot;
   if(slot==p->bytes/8&&p->bytes<p->limit&&!grow_retained()){
    /* Allocation failure must not evict an existing Forever body. */
    g_eax=UINT32_MAX;g_esp+=4;return 1;
   }
   if(p->limit>p->bytes)p->limit=p->bytes;
  }
 }
 if(entry==0x2a5070){lookup(p);return 1;}if(entry==0x2a6a70){construct_pool(p);return 1;}
 if(entry!=0x2a5030&&entry!=0x2a5d50&&entry!=0x2a6790&&entry!=0x2a6850)return 0;
 for(unsigned i=0;i<p->bytes/8;i++){uint32_t id=*word(p,i*8),object=*word(p,i*8+4);if(entry==0x2a5030){if(MEM8(0x51da6c)&&object&&(!safe(object,4)||!safe(MEM32(object),4)))return 0;}
  else if(entry==0x2a6790||entry==0x2a6850&&id==MEM32(g_esp+4)||entry==0x2a5d50&&id!=0xffffffff){if(!safe(object,0xa0))return 0;if(entry==0x2a5d50){uint32_t n=MEM32(object+0x74);if(n>65535||(n&&!safe(MEM32(object+0x70),n*4)))return 0;}}
 }
 if(entry==0x2a5030){destroy_pool(p);if(retention_configured){free((void*)retained.extra);retained=(Pool){NULL,40,40};}}else if(entry==0x2a5d50)update_pool(p);else if(entry==0x2a6790)acquire(p);else release(p);return 1;
}
int uc_migration_physics_ragdoll_pool(unsigned entry){return run(entry,NULL);}
int uc_migration_physics_ragdoll_pool_retained(unsigned entry,volatile unsigned *extra){return extra?run(entry,extra):0;}
#undef CALL
