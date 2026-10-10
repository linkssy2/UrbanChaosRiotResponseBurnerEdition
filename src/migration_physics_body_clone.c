/* Original body clone lifetime and ordered property-array copying. */
#include "migration_physics_body_clone.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002F2F90(void);void sub_002F1E90(void);void sub_002F1C40(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-512||(uint64_t)g_esp+32<=p);}
static void clone_body(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-196;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_ecx=g_esp+16;g_ebp=frame;CALL(0x2f1ffa,sub_002F2F90);g_eax=g_esp+16;PUSH32(g_esp,g_eax);g_ecx=g_esi;g_ebp=frame;CALL(0x2f2006,sub_002F1E90);
 g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,42);PUSH32(g_esp,196);uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x2f2018);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_esp+16;PUSH32(g_esp,g_ecx);g_ecx=g_eax;MEM16(g_eax+4)=196;g_ebp=frame;CALL(0x2f202a,sub_002F1C40);
 g_edx=MEM32(g_esi+16);g_edi=g_eax;MEM32(g_edi+16)=g_edx;g_eax=MEM32(g_esi+12);MEM32(g_edi+12)=g_eax;g_ecx=MEM32(g_edi+76);g_edx=MEM32(g_esi+72);g_eax=g_ecx&0x3fffffff;
 if((int32_t)g_eax<(int32_t)g_edx){if((int32_t)g_ecx>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,20);g_eax<<=4;PUSH32(g_esp,g_eax);g_eax=MEM32(g_edi+68);PUSH32(g_esp,g_eax);target=MEM32(g_edx+20);PUSH32(g_esp,0x2f2062);RECOMP_ICALL_SAFE(target,saved);}
  g_eax=MEM32(g_esi+72);g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,20);g_eax<<=4;PUSH32(g_esp,g_eax);target=MEM32(g_edx+16);PUSH32(g_esp,0x2f2076);RECOMP_ICALL_SAFE(target,saved);MEM32(g_edi+68)=g_eax;g_eax=MEM32(g_edi+76)&0x40000000;g_ecx=MEM32(g_esi+72);g_eax|=g_ecx;MEM32(g_edi+76)=g_eax;
 }
 g_ecx=MEM32(g_esi+72);g_eax=MEM32(g_edi+68);MEM32(g_edi+72)=g_ecx;g_edx=MEM32(g_esi+68);if((int32_t)g_ecx>0){g_edx-=g_eax;MEM32(g_esp+12)=g_ecx;do{g_ecx=g_edx+g_eax;g_ebx=MEM32(g_ecx);g_esi=g_eax;MEM32(g_esi)=g_ebx;g_ebx=MEM32(g_ecx+4);MEM32(g_esi+4)=g_ebx;g_ebx=MEM32(g_ecx+8);MEM32(g_esi+8)=g_ebx;g_ecx=MEM32(g_ecx+12);MEM32(g_esi+12)=g_ecx;g_ecx=MEM32(g_esp+12);g_eax+=16;--g_ecx;MEM32(g_esp+12)=g_ecx;}while(g_ecx);}
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=4;
}
int uc_migration_physics_body_clone(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_CLONE");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2f1fe0||!span(g_esp-512,544)||!safe(g_ecx,196)||!span(0x51e6f4,4))return 0;uint32_t n=MEM32(g_ecx+72),allocator=MEM32(0x51e6f4);if(n>65535||(n&&!safe(MEM32(g_ecx+68),n*16))||!safe(allocator,4)||!safe(MEM32(allocator),24))return 0;clone_body();return 1;
}
#undef CALL
