#include "migration_services_crt_heap_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00083B5D(void),sub_000842D8(void),sub_002D58E3(void),sub_002D9814(void),sub_002D9762(void);
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void new_handler(void){
 g_eax=MEM32(0x51e4f0);
 if(g_eax){uint32_t saved=g_esp,target=g_eax;PUSH32(g_esp,MEM32(g_esp+4));PUSH32(g_esp,0x2d86f7);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_ecx);}
 g_eax=g_eax!=0;g_recomp_zf=!g_eax;g_esp+=4;
}
static void allocate_one(void){
 g_eax=MEM32(g_esp+4);if(!g_eax)g_eax=1;if(MEM32(0x520a40)!=1)g_eax=(g_eax+15)&~15u;
 PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0x2d83c5);g_eax=MEM32(0x5206c4);g_esp+=4;
 PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2d83cb);sub_00083B5D();g_esp+=4;
}
static void allocate_retry(void){
 if(MEM32(g_esp+4)>0xffffffe0u){g_eax=0;g_esp+=4;return;}
 for(;;){
  PUSH32(g_esp,MEM32(g_esp+4));PUSH32(g_esp,0x2d83dc);allocate_one();unsigned ok=g_eax;POP32(g_esp,g_ecx);if(ok||MEM32(g_esp+8)==g_eax)break;
  PUSH32(g_esp,MEM32(g_esp+4));PUSH32(g_esp,0x2d83f0);new_handler();ok=g_eax;POP32(g_esp,g_ecx);if(!ok){g_eax=0;break;}
 }
 g_esp+=4;
}
static void allocate_default(void){
 PUSH32(g_esp,MEM32(0x51e6cc));PUSH32(g_esp,MEM32(g_esp+8));PUSH32(g_esp,0x2d8407);allocate_retry();POP32(g_esp,g_ecx);POP32(g_esp,g_ecx);g_esp+=4;
}
static void allocate_throwing(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);
 for(;;){
  PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2d5818);allocate_default();unsigned ok=g_eax;POP32(g_esp,g_ecx);if(ok)break;
  PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2d5808);new_handler();ok=g_eax;POP32(g_esp,g_ecx);
  if(!ok){PUSH32(g_esp,0x2d5812);sub_002D58E3();}
 }
 POP32(g_esp,g_esi);g_recomp_zf=0;g_esp+=4;
}
static void free_bridge(void){
 PUSH32(g_esp,MEM32(g_esp+12));PUSH32(g_esp,MEM32(g_esp+12));PUSH32(g_esp,MEM32(g_esp+12));PUSH32(g_esp,0x84c76);sub_000842D8();g_eax=LO8(g_eax);g_esp+=16;
}
static void release_crt(void){
 uint32_t f=g_esp-4,old=g_seh_ebp;PUSH32(g_esp,12);PUSH32(g_esp,0x496fc8);PUSH32(g_esp,0x2d73b0);PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(f)=old;g_esp-=12;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(f-24)=g_esp;MEM32(g_esp-4)=0x2d73b0;MEM32(f-4)=0xffffffffu;MEM32(f-8)=0x496fc8;g_eax=f-16;MEM32(0)=g_eax;g_seh_ebp=f;g_recomp_zf=0;g_esi=0;
 if(MEM32(f+8)){
  if(MEM32(0x520a40)==3){
   PUSH32(g_esp,4);PUSH32(g_esp,0x2d73c7);sub_002D9814();POP32(g_esp,g_ecx);MEM32(f-4)=g_esi;MEM32(f-28)=g_esi;MEM32(f-4)=0xffffffffu;
   PUSH32(g_esp,0x2d73d7);PUSH32(g_esp,4);PUSH32(g_esp,0x2d73fa);sub_002D9762();POP32(g_esp,g_ecx);g_esp+=4;
   if(MEM32(f-28)!=g_esi)goto done;
  }
  PUSH32(g_esp,MEM32(f+8));PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2d73e5);g_eax=MEM32(0x5206c4);g_esp+=4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2d73eb);free_bridge();
 }
done:g_seh_ebp=f;PUSH32(g_esp,0x2d73f0);MEM32(0)=MEM32(f-16);POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);MEM32(g_esp-4)=g_ecx;g_esp+=4;
}
int uc_migration_services_crt_heap(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_CRT_HEAP");enabled=p&&!strcmp(p,"1");}
 if(!enabled||!span(g_esp-256,276)||(g_esp&3)||MEM8(0x51c178))return 0;
 switch(entry){
 case 0x84c7c:g_eax=MEM32(0x5206c4);g_esp+=4;break;
 case 0x84c65:free_bridge();break;
 case 0x2d83a5:allocate_one();break;
 case 0x2d83cc:allocate_retry();break;
 case 0x2d83f8:allocate_default();break;
 case 0x2d86e8:new_handler();break;
 case 0x2d57fb:case 0x2d589f:allocate_throwing();break;
 case 0x2d73a4:case 0x2d5ccd:case 0x2d631d:release_crt();break;
 default:return 0;
 }
 return 1;
}
