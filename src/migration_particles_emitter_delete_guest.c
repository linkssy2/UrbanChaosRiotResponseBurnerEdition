#include "migration_particles_emitter_delete_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D5CCD(void);
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_EMITTER_DELETE");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_particles_emitter_delete(unsigned entry){
 if(entry!=0xda5f0||!enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10020||!span(s-32,48)||!span(p,4)||!span(0x51c178,1)||MEM8(0x51c178)||((uint64_t)p<s+16&&(uint64_t)p+4>s-32))return 0;
 unsigned release=MEM8(s+4)&1;PUSH32(g_esp,g_esi);g_esi=p;MEM32(g_esi)=0x45c298;
 if(release){PUSH32(g_esp,g_esi);PUSH32(g_esp,0xda606);sub_002D5CCD();g_esp+=4;}
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
}
