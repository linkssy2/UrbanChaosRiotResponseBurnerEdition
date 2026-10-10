#include "migration_services_entity_support_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0020BDC0(void),sub_0022A670(void),sub_0022A660(void),sub_0022A6F0(void),sub_00207D70(void),sub_00208C10(void),sub_0020A140(void);
extern void sub_0021AC70(void),sub_0021ACE0(void),sub_0020A620(void),sub_0020A480(void),sub_002A1AB0(void),sub_0008A050(void),sub_000BBCA0(void);
#define SUPPORT_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define SUPPORT_FRAME_CALL(fn,pc) do{g_ebp=frame;SUPPORT_CALL(fn,pc);}while(0)
static int support_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void support_enter(void){PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39d2e6);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;}
static void support_leave(void){g_ecx=MEM32(g_esp+12);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;}
static void support_attachment(void){SET_LO8(g_eax,MEM8(g_ecx+0x39));int absent=!LO8(g_eax);g_eax=absent?0:MEM32(g_ecx+0x34);g_recomp_zf=absent;g_esp+=4;}
static void support_construct(void){
 support_enter();SUPPORT_CALL(sub_0020BDC0,0x218573);g_edi=g_esi+0x84;g_ecx=g_edi;MEM32(g_esp+20)=0;MEM32(g_esi)=0x480b60;SUPPORT_CALL(sub_0022A670,0x21858e);
 g_eax=MEM32(0x51c3e0);g_ecx=MEM32(g_eax+16);int placement=g_ecx!=0;g_ecx=MEM32(0x51c4dc);MEM8(g_esp+20)=1;int inherited=0;
 if(!placement){g_eax=MEM32(g_esi+0x58);PUSH32(g_esp,g_eax);SUPPORT_CALL(sub_00207D70,0x2185ae);if(g_eax&&(MEM32(g_eax+0x5c)&0x8000000)){g_ecx=MEM32(g_eax+0x74);if(g_ecx){SET_LO16(g_ecx,MEM16(g_eax+0x94));MEM16(g_esi+0x94)=LO16(g_ecx);inherited=1;}}if(!inherited)g_ecx=MEM32(0x51c4dc);}
 if(!inherited){SUPPORT_CALL(sub_00208C10,0x2185dd);MEM16(g_esi+0x94)=LO16(g_eax);}
 g_xmm0=XMM_ZERO();g_ecx=g_edi;MEM32(g_esi+0x98)=0xffff;MEMF(g_esi+0xa0)=0;MEMF(g_esi+0x9c)=0;MEMF(g_esi+0xa4)=0;MEM32(g_esi+0x90)=16;SUPPORT_CALL(sub_0022A660,0x21861a);
 g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMD(0x480be8);g_fp_stack[g_fp_top]=sqrt(g_fp_stack[g_fp_top]);g_xmm0=XMM_SCALAR(MEMF(0x480b5c));PUSH32(g_esp,0x8000000);g_ecx=g_esi;MEMF(g_esi+0x88)=g_xmm0.f[0];MEMF(g_esi+0x8c)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;SUPPORT_CALL(sub_0020A140,0x218644);
 g_ecx=MEM32(g_esp+12);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void support_clear(void){
 uint32_t inherited=g_seh_ebp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+0x74);if(g_ecx){SUPPORT_CALL(sub_0021AC70,0x2183cf);g_ecx=MEM32(g_esi+0x74);SUPPORT_CALL(sub_0021ACE0,0x2183d7);MEM32(g_esi+0x5c)&=~0x800u;}
 g_ecx=g_esi+0x84;SUPPORT_CALL(sub_0022A660,0x2183e9);g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=inherited;sub_0020A620();
}
static void support_destroy(void){
 support_enter();MEM32(g_esi)=0x480b60;g_eax=MEM32(g_esi+0x5c);MEM32(g_esp+20)=1;if(g_eax&0x800){g_ecx=MEM32(g_esi+0x74);if(g_ecx){SUPPORT_CALL(sub_0021AC70,0x2181c0);g_ecx=MEM32(g_esi+0x74);SUPPORT_CALL(sub_0021ACE0,0x2181c8);}}
 g_edi=g_esi+0x84;g_ecx=g_edi;SUPPORT_CALL(sub_0022A660,0x2181d5);g_ecx=g_edi;MEM8(g_esp+20)=0;SUPPORT_CALL(sub_0022A6F0,0x2181e1);g_ecx=g_esi;MEM32(g_esp+20)=0xffffffffu;SUPPORT_CALL(sub_0020A480,0x2181f0);support_leave();
}
static void support_sync(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-0x8c;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=g_esi+0x84;SUPPORT_FRAME_CALL(sub_0022A660,0x21869a);g_ecx=MEM32(g_esi+0x74);
 if(g_ecx){g_eax=MEM32(g_esi+0x5c);if(g_eax&0x800){
  for(unsigned row=0;row<4;row++){g_xmm0=XMM_MEM(g_esi+16+row*16);XMM_STORE(g_esp+16+row*16,g_xmm0);}g_ebp=frame;PUSH32(g_esp,0x2186da);support_attachment();
  if(g_eax){g_ecx=MEM32(g_esi+0x74);g_ebp=frame;PUSH32(g_esp,0x2186ea);support_attachment();if(MEM32(g_eax+0xa8)==1){
   g_ecx=MEM32(g_esi+0x74);g_eax=g_esp+80;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x218700);support_attachment();g_ecx=g_eax;SUPPORT_FRAME_CALL(sub_002A1AB0,0x218707);
   for(unsigned row=0;row<3;row++){g_xmm0=XMM_MEM(g_eax+row*16);XMM_STORE(g_esp+16+row*16,g_xmm0);}g_ecx=g_esp+16;g_xmm0=XMM_MEM(g_eax+48);PUSH32(g_esp,g_ecx);g_ecx=g_esi;XMM_STORE(g_esp+68,g_xmm0);SUPPORT_FRAME_CALL(sub_0008A050,0x218736);
   g_xmm0=XMM_SCALAR(MEMF(g_esp+64));MEMF(g_esp+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+68));g_edx=g_esp+4;MEMF(g_esp+8)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+72));PUSH32(g_esp,g_edx);g_ecx=g_esi;MEMF(g_esp+16)=g_xmm0.f[0];SUPPORT_FRAME_CALL(sub_000BBCA0,0x218766);
  }}
  g_eax=MEM32(g_esi);g_ecx=g_esp+16;uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_esi;uint32_t target=MEM32(g_eax+0x80);PUSH32(g_esp,0x218775);RECOMP_ICALL_SAFE(target,saved);
  g_ecx=MEM32(g_esi+0x5c);g_eax=MEM32(g_esi+0x90);g_ecx&=~0x80u;g_eax&=~2u;MEM32(g_esi+0x5c)=g_ecx;MEM32(g_esi+0x90)=g_eax;
 }}POP32(g_esp,g_esi);g_esp=frame+8;
}
int uc_migration_services_entity_support(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_ENTITY_SUPPORT");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!support_span(g_esp-256,272)||!support_span(g_ecx,0xa8))return 0;
 switch(entry){case 0x218550:support_construct();break;case 0x2183c0:support_clear();break;case 0x218180:support_destroy();break;case 0x218680:support_sync();break;case 0x21ab20:support_attachment();break;default:return 0;}return 1;
}
#undef SUPPORT_CALL
#undef SUPPORT_FRAME_CALL
