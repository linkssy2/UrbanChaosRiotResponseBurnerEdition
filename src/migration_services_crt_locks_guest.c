#include "migration_services_crt_locks_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D83F8(void),sub_002D73A4(void),sub_002DB935(void),sub_002DB134(void),sub_002D60F2(void);
static uint32_t lock_frame(unsigned locals,unsigned scope,unsigned pc){
 uint32_t f=g_esp-4,old=g_seh_ebp;PUSH32(g_esp,locals);PUSH32(g_esp,scope);PUSH32(g_esp,pc);PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);g_eax=locals;MEM32(f)=old;g_esp-=locals;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=MEM32(f-8);MEM32(f-24)=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(f-4);MEM32(f-4)=0xffffffff;MEM32(f-8)=g_eax;g_eax=f-16;MEM32(0)=g_eax;g_recomp_zf=!g_esp;g_seh_ebp=f;g_esp+=4;return f;
}
static void lock_leave(uint32_t f,unsigned pc){
 g_seh_ebp=f;PUSH32(g_esp,pc);g_ecx=MEM32(f-16);MEM32(0)=g_ecx;POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);PUSH32(g_esp,g_ecx);g_esp+=8;
}
static void release_lock(void){g_eax=MEM32(g_esp+4);uint32_t saved=g_esp;PUSH32(g_esp,MEM32(0x504910+8*g_eax));uint32_t target=MEM32(0x44b3ec);PUSH32(g_esp,0x2d9773);RECOMP_ICALL_SAFE(target,saved);g_esp+=4;}
static void release_guard(void){PUSH32(g_esp,10);PUSH32(g_esp,0x2d9812);release_lock();POP32(g_esp,g_ecx);g_esp+=4;}
static void default_init(void){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esp+4));uint32_t target=MEM32(0x44b3f8);PUSH32(g_esp,0x2de29f);RECOMP_ICALL_SAFE(target,saved);g_eax=1;g_recomp_zf=0;g_esp+=12;}
static void initialize_lock(void){
 uint32_t f=lock_frame(16,0x497800,0x2de2b1);if(!MEM32(0x51e6d4))MEM32(0x51e6d4)=0x2de295;MEM32(f-4)=0;
 uint32_t saved=g_esp;PUSH32(g_esp,MEM32(f+12));PUSH32(g_esp,MEM32(f+8));uint32_t target=MEM32(0x51e6d4);PUSH32(g_esp,0x2de2d4);if(target==0x2de295)default_init();else RECOMP_ICALL_SAFE(target,saved);
 MEM32(f-32)=g_eax;MEM32(f-4)=0xffffffff;lock_leave(f,0x2de305);
}
static void create_lock(void);
static void acquire_lock(void){
 g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_esi=0x504910+8*g_eax;
 if(!MEM32(g_esi)){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2d982b);create_lock();int ok=!!g_eax;POP32(g_esp,g_ecx);if(!ok){PUSH32(g_esp,17);PUSH32(g_esp,0x2d9837);sub_002DB134();POP32(g_esp,g_ecx);}}
 uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esi));uint32_t target=MEM32(0x44b3f0);PUSH32(g_esp,0x2d9840);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);g_esp+=4;
}
static void create_lock(void){
 uint32_t f=lock_frame(8,0x497080,0x2d9780);g_esi=0x504910+8*MEM32(f+8);g_ebx=0;
 if(MEM32(g_esi)!=g_ebx)g_eax=1;
 else{
  PUSH32(g_esp,28);PUSH32(g_esp,0x2d9797);sub_002D83F8();POP32(g_esp,g_ecx);g_edi=g_eax;
  if(g_edi==g_ebx){PUSH32(g_esp,0x2d97a3);sub_002DB935();MEM32(g_eax)=12;g_eax=0;}
  else{
   PUSH32(g_esp,10);PUSH32(g_esp,0x2d97b2);acquire_lock();POP32(g_esp,g_ecx);MEM32(f-4)=g_ebx;
   int failed=0;
   if(MEM32(g_esi)==g_ebx){
    PUSH32(g_esp,4000);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x2d97c5);initialize_lock();POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);
    if(g_eax)MEM32(g_esi)=g_edi;
    else{
     PUSH32(g_esp,g_edi);PUSH32(g_esp,0x2d97d1);sub_002D73A4();PUSH32(g_esp,0x2d97d6);sub_002DB935();MEM32(g_eax)=12;PUSH32(g_esp,0xffffffff);g_eax=f-16;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2d97e7);sub_002D60F2();g_esp+=12;g_eax=0;failed=1;
    }
   }else{PUSH32(g_esp,g_edi);PUSH32(g_esp,0x2d97f8);sub_002D73A4();POP32(g_esp,g_ecx);}
   if(!failed){MEM32(f-4)=0xffffffff;PUSH32(g_esp,0x2d9802);release_guard();g_eax=1;}
  }
 }
 lock_leave(f,0x2d980a);
}
int uc_migration_services_crt_locks(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_CRT_LOCKS");enabled=p&&!strcmp(p,"1");}
 uint32_t s=g_esp;if(!enabled||MEM8(0x51c178)||s<0x10200||s>=0x80000000u||(uint64_t)s+16>g_xbox_total_ram)return 0;
 if((entry==0x2d9774||entry==0x2d9814||entry==0x2d9762)&&MEM32(s+4)>=32)return 0;
 switch(entry){case 0x2d9774:create_lock();break;case 0x2d9814:acquire_lock();break;case 0x2d9762:release_lock();break;case 0x2d980b:release_guard();break;case 0x2de2a5:initialize_lock();break;case 0x2de295:default_init();break;default:return 0;}return 1;
}
