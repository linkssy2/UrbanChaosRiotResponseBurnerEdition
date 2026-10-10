#include "migration_services_lifecycle_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00218550(void),sub_002183C0(void),sub_0020A140(void),sub_00218680(void),sub_00218180(void);
extern void sub_002D57FB(void),sub_002D5CCD(void),sub_0022B770(void),sub_001FB790(void),sub_001FBA40(void);
#define LIFE_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static int lifecycle_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void lifecycle_enter(uint32_t handler){PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,handler);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);}
static void lifecycle_leave(int has_edi,unsigned pop){g_ecx=MEM32(g_esp+(has_edi?12:8));if(has_edi)POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=pop;}
static void coroutine_context(void){
 g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;SET_LO8(g_ecx,MEM8(g_esp+16));g_ebx=0;
 MEM32(g_esi+0x28)=g_eax;MEM32(g_esi+0x20)=0;MEM32(g_esi+0x24)=0;MEM8(g_esi+0x2d)=0;MEM8(g_esi+0x2c)=LO8(g_ecx);
 if(!MEM32(0x51c3dc)){
  PUSH32(g_esp,g_edi);g_edi=MEM32(0x51519c)+0x10000;PUSH32(g_esp,0x10000);MEM32(0x51519c)=g_edi;LIFE_CALL(sub_002D57FB,0x1fb9a2);
  g_edi=g_eax;g_esp+=4;MEM32(0x51c3dc)=g_edi;g_ecx=0x4000;g_eax=0;memset((void*)XBOX_PTR(g_edi),0,0x10000);g_edi+=0x10000;g_ecx=0;POP32(g_esp,g_edi);
 }
 uint32_t count=++MEM32(0x51c3d8);MEM8(g_esi+0x2e)=LO8(g_ebx);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=count==0;g_esp+=12;
}
static void coroutine_owner(void){
 lifecycle_enter(0x39bd4b);PUSH32(g_esp,0x30);g_esi=g_ecx;LIFE_CALL(sub_002D57FB,0x1fb9f0);g_esp+=4;MEM32(g_esp+4)=g_eax;MEM32(g_esp+16)=0;
 if(g_eax){g_ecx=MEM32(g_esp+28);g_edx=MEM32(g_esp+24);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;PUSH32(g_esp,0x1fba14);coroutine_context();}
 else g_eax=0;MEM32(g_esi)=g_eax;lifecycle_leave(0,12);
}
static void coroutine_resume(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi);SET_LO8(g_eax,MEM8(g_ecx+0x2e));int pending=!LO8(g_eax);
 if(pending){LIFE_CALL(sub_001FB790,0x1fb921);g_ecx=MEM32(g_esi);MEM8(g_ecx+0x2e)=LO8(g_eax);}POP32(g_esp,g_esi);g_recomp_zf=pending;g_esp+=4;
}
static void coroutine_release(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);if(g_esi){
  g_eax=MEM32(g_esi+0x20);if(g_eax){g_ecx=MEM32(0x4e8410);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+0x24);PUSH32(g_esp,g_eax);LIFE_CALL(sub_0022B770,0x1fba5e);}
  g_eax=MEM32(0x51c3d8)-1;MEM32(0x51c3d8)=g_eax;if((int32_t)g_eax<=0){g_ecx=MEM32(0x51c3dc);PUSH32(g_esp,g_ecx);LIFE_CALL(sub_002D5CCD,0x1fba79);g_esp+=4;MEM32(0x51c3dc)=0;}
  PUSH32(g_esp,g_esi);LIFE_CALL(sub_002D5CCD,0x1fba8c);g_esp+=4;
 }POP32(g_esp,g_esi);g_esp+=4;
}
static void lifecycle_clear_records(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=0;g_edx=0;
 for(;;){g_eax=MEM32(g_ecx+0x64);g_esi=g_eax?MEM32(g_eax+8):0;if((int32_t)g_edi>=(int32_t)g_esi)break;g_eax=MEM32(g_eax+12);MEM8(g_eax+g_edx+20)=0;g_edi++;g_edx+=28;}
 int equal=g_edi==g_esi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=equal;g_esp+=4;
}
static void lifecycle_release_attachment(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+0xc0);int missing=!g_ecx;
 if(!missing){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x216f93);RECOMP_ICALL_SAFE(target,saved);}
 MEM32(g_esi+0xc0)=0;POP32(g_esp,g_esi);g_recomp_zf=missing;g_esp+=4;
}
static void lifecycle_run(unsigned entry){
 switch(entry){
 case 0x1fb780:g_eax=g_ecx;MEM32(g_eax)=0;g_esp+=4;break;
 case 0x1fb910:coroutine_resume();break;
 case 0x1fb960:coroutine_context();break;
 case 0x1fb9d0:coroutine_owner();break;
 case 0x1fba40:coroutine_release();break;
 case 0x208ba0:g_eax=MEM32(g_ecx+4);g_eax=MEM32(g_eax+0x2040);g_esp+=4;break;
 case 0x2102a0:lifecycle_clear_records();break;
 case 0x216f80:lifecycle_release_attachment();break;
 case 0x218a60:
  PUSH32(g_esp,g_esi);g_esi=g_ecx;LIFE_CALL(sub_00218550,0x218a68);g_eax=0;g_ecx=0xffffffffu;MEM32(g_esi)=0x480bf8;
  MEM32(g_esi+0xc4)=0;MEM32(g_esi+0xb0)=g_ecx;MEM32(g_esi+0xb4)=g_ecx;MEM32(g_esi+0xbc)=0;MEM32(g_esi+0xc0)=0;MEM32(g_esi+0xc8)=0;MEM8(g_esi+0xcc)=0;MEM8(g_esi+0xcd)=0;MEM8(g_esi+0xce)=0;
  g_ecx=MEM32(MEM32(0x51c3e0)+16);if(g_ecx){g_edx=MEM32(g_ecx+0x64);if(g_edx&&MEM32(g_edx+8)){MEM8(g_esi+0xce)=1;PUSH32(g_esp,0x218ace);lifecycle_clear_records();}}
  g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=4;break;
 case 0x218b30:
  PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x218b38);lifecycle_release_attachment();g_ecx=g_esi;LIFE_CALL(sub_002183C0,0x218b3f);PUSH32(g_esp,0x80000);g_ecx=g_esi;LIFE_CALL(sub_0020A140,0x218b4b);POP32(g_esp,g_esi);g_esp+=4;break;
 case 0x218c40:
  lifecycle_enter(0x39d2fb);g_esi=g_ecx;SET_LO8(g_eax,MEM8(g_esi+0xcc));if(!LO8(g_eax))SET_LO8(g_eax,MEM8(g_esi+0xcd));if(!LO8(g_eax))SET_LO8(g_eax,MEM8(g_esi+0xce));
  if(LO8(g_eax)){
   g_eax=MEM32(g_esi+0xc8);if(!g_eax){PUSH32(g_esp,4);LIFE_CALL(sub_002D57FB,0x218c88);g_esp+=4;MEM32(g_esp+4)=g_eax;MEM32(g_esp+16)=0;
    if(g_eax){g_ecx=g_eax;PUSH32(g_esp,0x218ca2);lifecycle_run(0x1fb780);}else g_eax=0;
    PUSH32(g_esp,1);PUSH32(g_esp,g_esi);g_ecx=g_eax;MEM32(g_esp+24)=0xffffffffu;MEM32(g_esi+0xc8)=g_eax;PUSH32(g_esp,0x218cbe);coroutine_owner();
   }g_ecx=MEM32(g_esi+0xc8);PUSH32(g_esp,0x218cc9);coroutine_resume();
  }g_ecx=g_esi;LIFE_CALL(sub_00218680,0x218cd0);lifecycle_leave(0,4);break;
 case 0x218ce0:{
  lifecycle_enter(0x39d318);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;MEM32(g_esi)=0x480bf8;g_ecx=MEM32(0x51c4dc);MEM32(g_esp+20)=0;
  PUSH32(g_esp,0x218d17);lifecycle_run(0x208ba0);g_ecx=MEM32(g_esi+0xbc);g_edx=MEM32(g_eax);uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_eax;uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x218d25);RECOMP_ICALL_SAFE(target,saved);
  g_edi=MEM32(g_esi+0xc8);if(g_edi){g_ecx=g_edi;LIFE_CALL(sub_001FBA40,0x218d36);PUSH32(g_esp,g_edi);LIFE_CALL(sub_002D5CCD,0x218d3c);g_esp+=4;}
  g_ecx=g_esi;MEM32(g_esp+20)=0xffffffffu;MEM32(g_esi)=0x4644a0;LIFE_CALL(sub_00218180,0x218d54);lifecycle_leave(1,4);break;
 }
 }
}
int uc_migration_services_lifecycle(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_LIFECYCLE");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!lifecycle_span(g_esp-256,280)||!lifecycle_span(g_ecx,0xd0))return 0;
 switch(entry){case 0x1fb780:case 0x1fb910:case 0x1fb960:case 0x1fb9d0:case 0x1fba40:case 0x208ba0:case 0x216f80:case 0x218a60:case 0x218b30:case 0x218c40:case 0x218ce0:break;
 case 0x2102a0:{uint32_t records=MEM32(g_ecx+0x64);if(records){if(!lifecycle_span(records,16))return 0;int32_t n=MEM32(records+8);if(n>4096||(n>0&&!lifecycle_span(MEM32(records+12),(unsigned)n*28)))return 0;}break;}
 default:return 0;}
 lifecycle_run(entry);return 1;
}
#undef LIFE_CALL
