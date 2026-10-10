#include "migration_services_crt_resize_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D83F8(void),sub_002D73A4(void),sub_002D9814(void),sub_002D9762(void),sub_002D86E8(void),sub_00084C7C(void),sub_000844CC(void),sub_000831BB(void);
static int resize_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static unsigned resize_frame(unsigned scope,unsigned pc){
 unsigned f=g_esp-4,old=g_seh_ebp;PUSH32(g_esp,16);PUSH32(g_esp,scope);PUSH32(g_esp,pc);PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(f)=old;g_esp-=16;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(f-24)=g_esp;MEM32(g_esp-4)=pc;MEM32(f-4)=0xffffffff;MEM32(f-8)=scope;g_eax=f-16;MEM32(0)=g_eax;g_seh_ebp=f;g_recomp_zf=0;return f;
}
static void resize_leave(unsigned f,unsigned pc){
 g_seh_ebp=f;PUSH32(g_esp,pc);g_ecx=MEM32(f-16);MEM32(0)=g_ecx;POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);PUSH32(g_esp,g_ecx);g_esp+=8;
}
static void resize_unlock(unsigned pc){PUSH32(g_esp,4);PUSH32(g_esp,pc);sub_002D9762();POP32(g_esp,g_ecx);g_esp+=4;}
static void resize_general(unsigned f){
 for(;;){
  g_eax=0;
  if(g_esi<=0xffffffe0){if(g_esi==g_edi)g_esi=1;PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(f+8));PUSH32(g_esp,g_edi);PUSH32(g_esp,0x2da238);sub_00084C7C();PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2da23e);sub_000844CC();}
  if(g_eax!=g_edi||MEM32(0x51e6cc)==g_edi)break;
  PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2da250);sub_002D86E8();POP32(g_esp,g_ecx);if(!g_eax){g_eax=0;break;}
 }
 resize_leave(f,0x2da25c);
}
int uc_migration_services_crt_resize(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_CRT_RESIZE");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!resize_span(g_esp-256,272))return 0;
 if(entry==0x2da217||entry==0x2da2ad){resize_unlock(entry==0x2da217?0x2da21e:0x2da2b4);return 1;}
 if(entry==0x2da220){if(!resize_span(g_seh_ebp-32,48))return 0;resize_general(g_seh_ebp);return 1;}
 if(entry==0x2da25d){
  unsigned f=resize_frame(0x497118,0x2da269);
  if(MEM32(0x520a40)==3){PUSH32(g_esp,4);PUSH32(g_esp,0x2da279);sub_002D9814();POP32(g_esp,g_ecx);MEM32(f-4)=0;MEM32(f-32)=0;MEM32(f-4)=0xffffffff;PUSH32(g_esp,0x2da28b);resize_unlock(0x2da2b4);if(MEM32(f-32))goto size_done;}
  PUSH32(g_esp,MEM32(f+8));PUSH32(g_esp,0);PUSH32(g_esp,0x2da29b);sub_00084C7C();PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2da2a1);sub_000831BB();MEM32(f-28)=g_eax;
size_done:g_eax=MEM32(f-28);resize_leave(f,0x2da2ac);return 1;
 }
 if(entry!=0x2da170)return 0;
 unsigned f=resize_frame(0x497108,0x2da17c);g_edi=0;
 if(!MEM32(f+8)){PUSH32(g_esp,MEM32(f+12));PUSH32(g_esp,0x2da18b);sub_002D83F8();POP32(g_esp,g_ecx);resize_leave(f,0x2da25c);return 1;}
 g_esi=MEM32(f+12);if(!g_esi){PUSH32(g_esp,MEM32(f+8));PUSH32(g_esp,0x2da1a0);sub_002D73A4();POP32(g_esp,g_ecx);g_eax=0;resize_leave(f,0x2da25c);return 1;}
 if(MEM32(0x520a40)!=3){resize_general(f);return 1;}
 for(;;){
  MEM32(f-28)=g_edi;
  if(g_esi<=0xffffffe0){
   PUSH32(g_esp,4);PUSH32(g_esp,0x2da1be);sub_002D9814();POP32(g_esp,g_ecx);MEM32(f-4)=g_edi;MEM32(f-32)=g_edi;MEM32(f-4)=0xffffffff;PUSH32(g_esp,0x2da1ce);resize_unlock(0x2da21e);
   if(MEM32(f-32)==g_edi){if(g_esi==g_edi)g_esi=1;g_esi=(g_esi+15)&0xfffffff0;MEM32(f+12)=g_esi;PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(f+8));PUSH32(g_esp,g_edi);PUSH32(g_esp,0x2da1ed);sub_00084C7C();PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2da1f3);sub_000844CC();MEM32(f-28)=g_eax;}
  }
  g_eax=MEM32(f-28);if(g_eax!=g_edi||MEM32(0x51e6cc)==g_edi)break;
  PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2da20b);sub_002D86E8();POP32(g_esp,g_ecx);if(!g_eax){g_eax=0;break;}
 }
 /* Retail 2DA255/257 is XOR EAX,EAX (optional), CALL 2D7CB3, RET.
    The original generated calls target unresolved stubs; this fused epilogue
    deliberately repairs their missing stack and SEH restoration. */
 resize_leave(f,0x2da25c);return 1;
}
