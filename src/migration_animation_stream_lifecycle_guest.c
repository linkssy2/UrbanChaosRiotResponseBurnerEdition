#include "migration_animation_stream_lifecycle_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void),sub_002D5CCD(void),sub_002D68FB(void);
static int streamlife_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int streamlife_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int streamlife_gate(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_STREAM_LIFECYCLE");enabled=v&&!strcmp(v,"1");}return enabled&&!(g_esp&3)&&g_esp>=0x10400&&streamlife_span(g_esp-1024,1032)&&streamlife_span(0x51c178,1)&&!MEM8(0x51c178);}
static int streamlife_holder(uint32_t p){if(!streamlife_span(p,4)||!streamlife_apart(p,4,g_esp-1024,1032))return 0;uint32_t h=MEM32(p);if(!streamlife_span(h,4)||!streamlife_apart(h,4,g_esp-1024,1032))return 0;uint32_t object=MEM32(h);return !object||(streamlife_span(object,4)&&streamlife_span(MEM32(object),4));}
int uc_migration_animation_stream_create(void){
 if(!streamlife_gate()||!streamlife_span(g_ecx,4)||!streamlife_apart(g_ecx,4,g_esp-1024,1032))return 0;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,4);g_esi=g_ecx;PUSH32(g_esp,0x1f0aca);sub_002D57FB();g_esp+=4;int allocated=g_eax!=0;
 if(allocated){MEM32(g_eax)=0;MEM32(g_esi)=g_eax;g_eax=g_esi;}else{g_eax=0;MEM32(g_esi)=g_eax;g_eax=g_esi;}
 POP32(g_esp,g_esi);g_recomp_zf=allocated?0:g_eax==0;g_esp+=4;return 1;
}
static void streamlife_close_call(uint32_t pc){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
int uc_migration_animation_stream_close(void){
 if(!streamlife_gate()||!streamlife_holder(MEM32(g_esp+4)))return 0;
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi);g_ecx=MEM32(g_eax);int occupied=g_ecx!=0;
 if(occupied){streamlife_close_call(0x1f0b03);g_eax=MEM32(g_esi);MEM32(g_eax)=0;}else{g_ecx=MEM32(g_esi);MEM32(g_ecx)=0;}
 POP32(g_esp,g_esi);g_recomp_zf=!occupied;g_esp+=4;return 1;
}
int uc_migration_animation_stream_destroy(void){
 if(!streamlife_gate()||!streamlife_holder(g_ecx))return 0;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);g_ecx=MEM32(g_eax);if(g_ecx)streamlife_close_call(0x1f0bd1);g_eax=MEM32(g_esi);MEM32(g_eax)=0;g_ecx=MEM32(g_esi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f0be1);sub_002D5CCD();g_esp+=4;POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
int uc_migration_animation_stream_pool(void){
 if(!streamlife_gate()||!streamlife_span(0x51c114,4))return 0;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b7cb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);g_eax=MEM32(0x51c114);PUSH32(g_esp,g_esi);g_esi=0;
 if(!g_eax){PUSH32(g_esp,68);PUSH32(g_esp,0x1f0c19);sub_002D57FB();g_esp+=4;MEM32(g_esp+4)=g_eax;MEM32(g_esp+16)=g_esi;if(g_eax!=g_esi){PUSH32(g_esp,0x1f0bc0);PUSH32(g_esp,0x1f0ac0);PUSH32(g_esp,16);g_esi=g_eax+4;PUSH32(g_esp,4);PUSH32(g_esp,g_esi);MEM32(g_eax)=16;PUSH32(g_esp,0x1f0c45);sub_002D68FB();}MEM32(0x51c114)=g_esi;}
 g_ecx=MEM32(g_esp+8);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
int uc_migration_animation_stream_open(void){
 if(!streamlife_gate()||!streamlife_span(0x51c114,4)||!streamlife_span(0x5150b0,4))return 0;
 uint32_t pool=MEM32(0x51c114);int available=!pool;
 if(pool){if(!streamlife_span(pool,64)||!streamlife_apart(pool,64,g_esp-1024,1032))return 0;for(unsigned i=0;i<16;i++){uint32_t holder=MEM32(pool+i*4);if(!streamlife_span(holder,4)||!streamlife_apart(holder,4,g_esp-1024,1032))return 0;if(!MEM32(holder))available=1;}}
 if(available){uint32_t service=MEM32(0x5150b0);if(!streamlife_span(service,4)||!streamlife_span(MEM32(service),64))return 0;}
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b7cb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);g_edx=MEM32(0x51c114);PUSH32(g_esp,g_esi);g_esi=0;PUSH32(g_esp,g_edi);
 if(!g_edx){
  PUSH32(g_esp,68);PUSH32(g_esp,0x1f0c8b);sub_002D57FB();g_esp+=4;MEM32(g_esp+8)=g_eax;MEM32(g_esp+20)=g_esi;
  if(g_eax){PUSH32(g_esp,0x1f0bc0);PUSH32(g_esp,0x1f0ac0);PUSH32(g_esp,16);g_esi=g_eax+4;PUSH32(g_esp,4);PUSH32(g_esp,g_esi);MEM32(g_eax)=16;PUSH32(g_esp,0x1f0cb7);sub_002D68FB();}else g_esi=0;
  MEM32(g_esp+20)=0xffffffffu;MEM32(0x51c114)=g_esi;
 }else{
  g_ecx=0;g_eax=g_edx+8;int found=0;
  while(g_ecx<16){for(unsigned slot=0;slot<4;slot++){g_edi=MEM32(g_eax-8+slot*4);if(!MEM32(g_edi)){g_ecx+=slot;found=1;break;}}if(found)break;g_ecx+=4;g_eax+=16;}
  if(g_ecx<16)g_esi=g_edx+g_ecx*4;
 }
 if(g_esi){
  g_ecx=MEM32(0x5150b0);g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+60);PUSH32(g_esp,0x1f0d3f);RECOMP_ICALL_SAFE(target,saved);
  g_ecx=MEM32(g_esp+28);g_edx=MEM32(g_eax);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=g_eax;target=MEM32(g_edx+16);PUSH32(g_esp,0x1f0d4d);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_esi);MEM32(g_edx)=g_eax;g_eax=MEM32(g_esi);
  if(!MEM32(g_eax)){g_ecx=MEM32(g_esp+12);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=0;MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;}
 }
 POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
