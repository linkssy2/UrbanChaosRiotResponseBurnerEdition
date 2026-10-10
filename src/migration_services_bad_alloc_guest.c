#include "migration_services_bad_alloc_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D61E8(void),sub_002D622D(void),sub_002D619E(void),sub_002D6584(void),sub_002D73A4(void),sub_002D9814(void),sub_002D9762(void),sub_002D58A4(void);
static int bad_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void bad_release(void){int empty=!MEM32(g_ecx+8);MEM32(g_ecx)=0x496f14;if(!empty){PUSH32(g_esp,MEM32(g_ecx+4));PUSH32(g_esp,0x2d6290);sub_002D73A4();POP32(g_esp,g_ecx);}g_recomp_zf=empty;g_esp+=4;}
static void type_unlock(void){PUSH32(g_esp,14);PUSH32(g_esp,0x2d62ff);sub_002D9762();POP32(g_esp,g_ecx);g_esp+=4;}
/* Retail 3ADABA: MOV ECX,51E4C4; JMP 2D58A4. The callback has no generated owner. */
void uc_migration_services_bad_alloc_exit(void){g_ecx=0x51e4c4;sub_002D58A4();}
int uc_migration_services_bad_alloc(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_BAD_ALLOC");enabled=p&&!strcmp(p,"1");}
 if(!enabled||MEM8(0x51c178)||!bad_span(g_esp-256,272))return 0;
 if(entry==0x2d62f8){type_unlock();return 1;}
 if(entry==0x2d58a4||entry==0x2d627c){if(!bad_span(g_ecx,12))return 0;if(entry==0x2d58a4)MEM32(g_ecx)=0x496edc;bad_release();return 1;}
 if(entry==0x2d62bb){
  if(!bad_span(g_ecx,8))return 0;unsigned f=g_esp-4,old=g_seh_ebp;PUSH32(g_esp,8);PUSH32(g_esp,0x496f38);PUSH32(g_esp,0x2d62c7);PUSH32(g_esp,0x2d7ccc);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(f)=old;g_esp-=8;
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(f-24)=g_esp;MEM32(g_esp-4)=0x2d62c7;MEM32(f-4)=0xffffffff;MEM32(f-8)=0x496f38;g_eax=f-16;MEM32(0)=g_eax;g_seh_ebp=f;g_recomp_zf=0;
  g_esi=g_ecx;MEM32(g_esi)=0x496f30;PUSH32(g_esp,14);PUSH32(g_esp,0x2d62d6);sub_002D9814();POP32(g_esp,g_ecx);MEM32(f-4)=0;g_esi=MEM32(g_esi+4);
  if(g_esi){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2d62e8);sub_002D73A4();POP32(g_esp,g_ecx);}MEM32(f-4)=0xffffffff;PUSH32(g_esp,0x2d62f2);type_unlock();
  g_seh_ebp=f;PUSH32(g_esp,0x2d62f7);g_ecx=MEM32(f-16);MEM32(0)=g_ecx;POP32(g_esp,g_ecx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f;POP32(g_esp,g_seh_ebp);PUSH32(g_esp,g_ecx);g_esp+=8;return 1;
 }
 if(entry!=0x2d58e3)return 0;
 PUSH32(g_esp,g_ebp);unsigned f=g_esp;g_ebp=f;g_esp-=16;int initialized=MEM8(0x51e4d0)&1;PUSH32(g_esp,g_esi);g_esi=0x496edc;
 if(!initialized){
  MEM32(0x51e4d0)|=1;g_eax=f-4;PUSH32(g_esp,g_eax);g_ecx=0x51e4c4;MEM32(f-4)=0x496ee4;g_ebp=f;PUSH32(g_esp,0x2d5914);sub_002D61E8();
  PUSH32(g_esp,0x3adaba);MEM32(0x51e4c4)=g_esi;g_ebp=f;PUSH32(g_esp,0x2d5924);sub_002D6584();POP32(g_esp,g_ecx);
 }
 PUSH32(g_esp,0x51e4c4);g_ecx=f-16;g_ebp=f;PUSH32(g_esp,0x2d5932);sub_002D622D();PUSH32(g_esp,0x4e491c);g_eax=f-16;PUSH32(g_esp,g_eax);MEM32(f-16)=g_esi;g_ebp=f;PUSH32(g_esp,0x2d5943);sub_002D619E();__debugbreak();return 1;
}
