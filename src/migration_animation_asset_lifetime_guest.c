#include "migration_animation_asset_lifetime_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F4AA0(void);
extern void sub_002D631D(void);
static int asset_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int asset_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int asset_gate(unsigned depth){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_ASSET_LIFETIME");enabled=v&&!strcmp(v,"1");}
 return enabled&&!(g_esp&3)&&g_esp>=0x10000+depth&&asset_span(g_esp-depth,depth+8)&&asset_span(0x51c178,1)&&!MEM8(0x51c178);
}
static int asset_allocator(unsigned slot){
 if(!asset_span(0x4fc704,4))return 0;uint32_t p=MEM32(0x4fc704);if(!asset_span(p,4))return 0;
 return asset_span(MEM32(p),slot+4);
}
/* Allocator callbacks inherit their existing synchronous lifetime/ABI contract.
 * Fields are deliberately reread after each callback; no replay on failure. */
int uc_migration_animation_asset_allocate(void){
 if(!asset_gate(32)||!asset_allocator(4))return 0;
 g_ecx=MEM32(0x4fc704);g_edx=MEM32(g_esp+4);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x1f4a92);RECOMP_ICALL_SAFE(target,saved);g_esp+=4;return 1;
}
int uc_migration_animation_asset_free(void){
 if(!asset_gate(32)||!asset_allocator(8))return 0;
 g_ecx=MEM32(0x4fc704);g_edx=MEM32(g_esp+4);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x1f4ab0);RECOMP_ICALL_SAFE(target,saved);g_esp+=4;return 1;
}
int uc_migration_animation_asset_destroy(void){
 if(!asset_gate(128)||!asset_span(g_ecx,120)||!asset_apart(g_ecx,120,g_esp-128,136))return 0;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b8d3);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;g_eax=MEM32(g_esi+112);MEM32(g_esp+16)=1;
 if((int32_t)g_eax>0){g_eax=MEM32(g_esi+116);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f2c15);sub_001F4AA0();g_esp+=4;}
 g_eax=MEM32(g_esi+104);if((int32_t)g_eax>0){g_ecx=MEM32(g_esi+108);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f2c28);sub_001F4AA0();g_esp+=4;}
 g_eax=MEM32(g_esi+96);if((int32_t)g_eax>0){g_edx=MEM32(g_esi+100);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f2c3b);sub_001F4AA0();g_esp+=4;}
 g_eax=MEM32(g_esi+88);if((int32_t)g_eax>0){g_eax=MEM32(g_esi+92);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f2c4e);sub_001F4AA0();g_esp+=4;}
 g_ecx=MEM32(g_esi+44);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f2c5a);sub_001F4AA0();g_edx=MEM32(g_esi+48);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f2c63);sub_001F4AA0();g_eax=MEM32(g_esi+40);g_esp+=8;
 if(g_eax){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f2c73);sub_001F4AA0();g_esp+=4;}
 g_eax=MEM32(g_esi+36);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f2c7f);sub_001F4AA0();g_ecx=MEM32(g_esi+32);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f2c88);sub_001F4AA0();g_edx=MEM32(g_esi+24);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f2c91);sub_001F4AA0();g_eax=MEM32(g_esi+80);g_esp+=12;
 if((int32_t)g_eax<0){g_eax=MEM32(g_esi+76);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f2ca4);sub_002D631D();g_esp+=4;}
 if(MEM32(g_esi+8)==1){g_ecx=MEM32(g_esi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f2cb5);sub_002D631D();g_esp+=4;}
 g_ecx=MEM32(g_esp+8);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
