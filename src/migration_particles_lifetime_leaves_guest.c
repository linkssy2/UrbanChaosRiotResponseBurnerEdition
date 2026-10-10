#include "migration_particles_lifetime_leaves_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0020A480(void),sub_00218CE0(void);
static int lifetime_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_LIFETIME_LEAVES");f=p&&!strcmp(p,"1");}return f;}
static int lifetime_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int lifetime_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_migration_particles_lifetime_leaves(unsigned entry){
 if(!lifetime_enabled())return 0;
 if((g_esp&3)||!lifetime_span(g_esp,4)||!lifetime_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0xda5e0||entry==0x1d52f0){if(!lifetime_span(g_ecx,4))return 0;MEM32(g_ecx)=entry==0xda5e0?0x45c298:0x477880;g_esp+=4;return 1;}
 /* These particle-specific base adapters have only a tail transfer. Shared
  * destruction remains the existing service, on the unchanged caller stack. */
 if(entry==0xf9130||entry==0x13be30){if(!lifetime_span(g_ecx,4))return 0;if(entry==0xf9130)sub_0020A480();else sub_00218CE0();return 1;}
 if(entry==0x238c50){
  uint32_t s=g_esp;if(s<0x10008||!lifetime_span(s-8,16)||!lifetime_span(0x51cb88,4)||!lifetime_apart(0x51cb88,4,s-8,16))return 0;
  uint32_t pool=MEM32(0x51cb88);if(!lifetime_span(pool,4)||!lifetime_apart(pool,4,s-8,16))return 0;
  uint32_t table=MEM32(pool);if(!lifetime_span(table,12)||!lifetime_apart(table,12,s-8,16)||!lifetime_span(MEM32(table+8),1))return 0;
  g_ecx=MEM32(0x51cb88);g_edx=MEM32(g_esp+4);g_eax=MEM32(g_ecx);
  PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x238c60);RECOMP_ICALL_SAFE(target,s);g_esp+=4;return 1;
 }
 return 0;
}
