#include "migration_animation_cache_lifecycle_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void);
extern void sub_002D589F(void);
extern void sub_002D631D(void);
extern void sub_002D5CCD(void);
extern void sub_001F3520(void);
extern void sub_001F3580(void);
extern void sub_001F3B30(void);
static int lifecycle_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int lifecycle_admit(unsigned bytes){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_CACHE_LIFECYCLE");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;return enabled&&!(s&3)&&s>=0x10400&&lifecycle_span(s-1024,1048)&&lifecycle_span(p,bytes)&&((uint64_t)p+bytes<=s-1024||(uint64_t)s+24<=p)&&lifecycle_span(0x51c178,1)&&!MEM8(0x51c178);
}
/* Allocation/free remain synchronous shared services. Publish each returned
 * buffer before the next call; callbacks may observe the live descriptor. */
int uc_migration_animation_cache_construct(void){
 if(!lifecycle_admit(16)||MEM32(g_esp+4)>4096)return 0;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);g_eax=g_edi*8;g_esi=g_ecx;PUSH32(g_esp,g_eax);MEM32(g_esi)=0x80000001;PUSH32(g_esp,0x1f353c);sub_002D589F();
 g_ebx=(g_edi+g_edi*2)<<1;PUSH32(g_esp,g_ebx);MEM32(g_esi+4)=g_eax;PUSH32(g_esp,0x1f354a);sub_002D589F();PUSH32(g_esp,g_ebx);MEM32(g_esi+8)=g_eax;PUSH32(g_esp,0x1f3553);sub_002D589F();
 MEM32(g_esi+12)=g_eax;g_edx=0;g_esp+=12;g_eax=0;
 if((int32_t)g_edi>0){g_ecx=MEM32(g_esp+20);do{g_ebx=MEM32(g_esi+4);MEM32(g_ebx+g_eax*8)=g_ecx;g_ebx=MEM32(g_esi+4);MEM32(g_ebx+g_eax*8+4)=g_edx;++g_eax;}while((int32_t)g_eax<(int32_t)g_edi);}
 POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;return 1;
}
int uc_migration_animation_cache_destroy(void){
 if(!lifecycle_admit(16))return 0;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f358c);sub_002D631D();g_ecx=MEM32(g_esi+8);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f3595);sub_002D631D();g_edx=MEM32(g_esi+12);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f359e);sub_002D631D();g_esp+=12;POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
int uc_migration_animation_cache_create(void){
 if(!lifecycle_admit(88)||((MEM8(g_ecx+72)&8)&&MEM32(g_ecx+20)>4096))return 0;
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39b8eb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;
 if(MEM8(g_esi+72)&8){
  PUSH32(g_esp,16);PUSH32(g_esp,0x1f3b56);sub_002D57FB();g_esp+=4;MEM32(g_esp+4)=g_eax;MEM32(g_esp+16)=0;
  if(g_eax){g_ecx=MEM32(g_esi+28);g_edx=MEM32(g_esi+20);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;PUSH32(g_esp,0x1f3b78);sub_001F3520();POP32(g_esp,g_esi);g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;}
 }
 g_ecx=MEM32(g_esp+8);g_eax=0;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
int uc_migration_animation_clip_bind(void){
 if(!lifecycle_admit(56)||!lifecycle_span(MEM32(g_esp+4),88)||!lifecycle_span(0x44bf94,4))return 0;
 uint32_t clip=MEM32(g_esp+4);if((MEM8(clip+72)&8)&&MEM32(clip+20)>4096)return 0;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esp+8);MEM32(g_esi+48)=g_ecx;PUSH32(g_esp,0x1f225f);sub_001F3B30();g_xmm0=XMM_ZERO();MEM32(g_esi+52)=g_eax;g_eax=MEM32(g_esi+48);MEMF(g_esi+16)=g_xmm0.f[0];MEMF(g_esi+20)=g_xmm0.f[0];MEMF(g_esi+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf94));MEM32(g_esi+28)=0;MEMF(g_esi+32)=g_xmm0.f[0];g_ecx=MEM32(g_eax+56);MEM32(g_esi+36)=g_ecx;g_edx=MEM32(g_eax+60);g_eax=MEM32(g_esp+12);MEM32(g_esi+44)=g_eax;MEM32(g_esi+40)=g_edx;g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=12;return 1;
}
int uc_migration_animation_clip_release(void){
 if(!lifecycle_admit(56))return 0;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_esi=MEM32(g_edi+52);
 if(g_esi){g_ecx=g_esi;PUSH32(g_esp,0x1f2392);sub_001F3580();PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1f2398);sub_002D5CCD();g_esp+=4;}
 MEM32(g_edi+52)=0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;return 1;
}
