/* Clone original hinge/ragdoll motors and install the resulting joint. */
#include "migration_physics_joint_clone.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_0032A680(void);void sub_002F1230(void);void sub_0032A5B0(void);void sub_0032A5C0(void);void sub_0032A550(void);void sub_0032A560(void);void sub_0032A610(void);void sub_0032A620(void);void sub_002FE230(void);void sub_002F11E0(void);void sub_002FE260(void);void sub_002F0FC0(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-128||(uint64_t)g_esp+12<=p);}
static void clone_motor(void(*get)(void),void(*set)(void),unsigned first){
 g_ecx=g_esi;CALL(first,get);if(!g_eax)return;g_ecx=g_esi;CALL(first+11,get);if(MEM16(g_eax+4))++MEM16(g_eax+6);g_ecx=g_esi;CALL(first+29,get);g_edx=MEM32(g_eax);g_ecx=g_eax;uint32_t saved=g_esp,target=MEM32(g_edx+12);PUSH32(g_esp,first+36);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);g_ecx=g_edi;CALL(first+44,set);
}
static void clone_joint(void){
 uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);g_ebx=g_ecx;PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx+12);g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);g_ecx=g_esi;uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x2f12af);RECOMP_ICALL_SAFE(target,saved);g_eax-=4;
 if(!g_eax){
  g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,41);PUSH32(g_esp,160);target=MEM32(g_eax+16);PUSH32(g_esp,0x2f1395);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_eax;MEM16(g_eax+4)=160;CALL(0x2f13a2,sub_002FE230);g_edi=g_eax;PUSH32(g_esp,g_esi);g_ecx=g_edi;CALL(0x2f13ac,sub_002F11E0);g_eax=MEM32(g_esi+152);
  if(g_eax){if(MEM16(g_eax+4))++MEM16(g_eax+6);g_ecx=MEM32(g_esi+152);g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+12);PUSH32(g_esp,0x2f13cc);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);g_ecx=g_edi;CALL(0x2f13d4,sub_002FE260);}g_esi=g_edi;
 }else if(!--g_eax){
  g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,41);PUSH32(g_esp,224);target=MEM32(g_edx+16);PUSH32(g_esp,0x2f12d1);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_eax;MEM16(g_eax+4)=224;CALL(0x2f12de,sub_0032A680);g_edi=g_eax;PUSH32(g_esp,g_esi);g_ecx=g_edi;CALL(0x2f12e8,sub_002F1230);
  clone_motor(sub_0032A5B0,sub_0032A5C0,0x2f12ef);clone_motor(sub_0032A550,sub_0032A560,0x2f1322);clone_motor(sub_0032A610,sub_0032A620,0x2f1355);g_esi=g_edi;
 }
 g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,41);PUSH32(g_esp,40);target=MEM32(g_eax+16);PUSH32(g_esp,0x2f13e5);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esp+20);g_edx=MEM32(g_esp+16);PUSH32(g_esp,1);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;MEM16(g_eax+4)=40;CALL(0x2f13ff,sub_002F0FC0);g_edi=g_eax;
 if(g_esi!=MEM32(g_ebx+12)&&MEM16(g_esi+4)){--MEM16(g_esi+6);if(!MEM16(g_esi+6)){g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;target=MEM32(g_eax);PUSH32(g_esp,0x2f1420);RECOMP_ICALL_SAFE(target,saved);}}
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;
}
int uc_migration_physics_joint_clone(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_JOINT_CLONE");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2f12a0||!span(g_esp-128,144)||!safe(g_ecx,40)||!span(0x51e6f4,4))return 0;
 uint32_t data=MEM32(g_ecx+12),allocator=MEM32(0x51e6f4);if(!safe(data,16)||!safe(MEM32(data),16)||!safe(allocator,4)||!safe(MEM32(allocator),20))return 0;clone_joint();return 1;
}
#undef CALL
