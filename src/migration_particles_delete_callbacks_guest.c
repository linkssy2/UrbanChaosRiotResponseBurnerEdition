#include "migration_particles_delete_callbacks_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00175200(void),sub_00175C20(void),sub_001867D0(void),sub_00187530(void),sub_0023EE00(void),sub_002D5CCD(void);
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_DELETE_CALLBACKS");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Each row is an original deleting callback: table restoration or a qualified
 * owner destructor, followed by the original optional shared heap release. */
typedef struct{unsigned entry,table,destroy_pc,free_pc;void(*destroy)(void);}DeleteCallback;
static const DeleteCallback callbacks[]={
 {0x175240,0,0x175248,0x175255,sub_00175200},
 {0x1867b0,0x472cbc,0,0x1867c6,0},
 {0x1868f0,0,0x1868f8,0x186905,sub_001867D0},
 {0x187570,0,0x187578,0x187585,sub_00187530},
 {0x1a8400,0x477880,0,0x1a8416,0},
 {0x218da0,0x480ca8,0,0x218db6,0},
 {0x238c30,0x482f44,0,0x238c46,0},
 {0x238cc0,0x482f80,0,0x238cd6,0},
 {0x23ee80,0,0x23ee88,0x23ee95,sub_0023EE00}
};
int uc_migration_particles_delete_callbacks(unsigned entry){
 if(!enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10020||!span(s-32,48)||!span(p,4)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x175c00){if(p<0x10080||!span(p-128,4))return 0;g_ecx-=128;sub_00175C20();return 1;}
 const DeleteCallback*r=0;for(unsigned i=0;i<sizeof callbacks/sizeof*callbacks;i++)if(callbacks[i].entry==entry){r=callbacks+i;break;}
 if(!r)return 0;
 if((uint64_t)p<s+16&&(uint64_t)p+4>s-32)return 0;
 unsigned release=MEM8(s+4)&1;
 PUSH32(g_esp,g_esi);g_esi=p;
 if(r->destroy){PUSH32(g_esp,r->destroy_pc);r->destroy();release=MEM8(g_esp+8)&1;}
 else MEM32(g_esi)=r->table;
 if(release){PUSH32(g_esp,g_esi);PUSH32(g_esp,r->free_pc);sub_002D5CCD();g_esp+=4;}
 g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
}
