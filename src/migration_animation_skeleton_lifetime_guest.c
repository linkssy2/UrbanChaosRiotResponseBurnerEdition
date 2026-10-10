#include "migration_animation_skeleton_lifetime_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F4A10(void),sub_001F4AA0(void),sub_002D5CCD(void);
static int skeleton_lifetime_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int skeleton_lifetime_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static void skeleton_lifetime_destroy(void){
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39b7b3);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;MEM32(g_esi)=0x47fa48;g_edi=MEM32(g_esi+16);MEM32(g_esp+20)=1;
 if(g_edi){g_ecx=g_edi;PUSH32(g_esp,0x1f086a);sub_001F4A10();PUSH32(g_esp,g_edi);PUSH32(g_esp,0x1f0870);sub_002D5CCD();g_esp+=4;}
 g_eax=MEM32(g_esi+12);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f087c);sub_001F4AA0();g_ecx=MEM32(g_esi+20);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f0885);sub_001F4AA0();g_eax=MEM32(g_esi+32);g_esp+=8;
 if((int32_t)g_eax<0){g_eax=MEM32(g_esi+28);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f0898);sub_002D5CCD();g_esp+=4;}
 g_ecx=g_esi;MEM32(g_esp+20)=0xffffffff;PUSH32(g_esp,0x1f08aa);
 /* The base destructor only installs its vtable; fuse that private call. */
 MEM32(g_ecx)=0x48d93c;g_esp+=4;
 g_ecx=MEM32(g_esp+12);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_animation_skeleton_lifetime(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_SKELETON_LIFETIME");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,o=g_ecx;unsigned args=entry==0x1f09c0?8:4;
 if(!enabled||(entry!=0x1f0830&&entry!=0x1f09c0)||(s&3)||s<0x11000||!skeleton_lifetime_span(s-4096,4096+args)||!skeleton_lifetime_span(o,36)||!skeleton_lifetime_apart(o,36,s-4096,4096+args)||!skeleton_lifetime_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(!skeleton_lifetime_span(0x4fc704,4))return 0;uint32_t allocator=MEM32(0x4fc704);if(!skeleton_lifetime_span(allocator,4)||!skeleton_lifetime_span(MEM32(allocator),12))return 0;
 uint32_t table=MEM32(o+16);if(table&&!skeleton_lifetime_span(table,8))return 0;
 if(entry==0x1f0830)skeleton_lifetime_destroy();else{PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x1f09c8);skeleton_lifetime_destroy();if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1f09d5);sub_002D5CCD();g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;}
 return 1;
}
