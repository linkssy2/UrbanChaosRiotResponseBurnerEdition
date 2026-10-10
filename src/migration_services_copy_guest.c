#include "migration_services_copy_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int copy_span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_services_copy(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_COPY");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x2d5990||MEM8(0x51c178)||!copy_span(g_esp-12,28))return 0;
 uint32_t s=g_esp,dst=MEM32(s+4),src=MEM32(s+8),n=MEM32(s+12);
 if(!copy_span(dst,n)||!copy_span(src,n))return 0;
 /* Stack-aliasing copies retain the original path: their writes can change
    arguments or saved registers before the epilogue reads them. */
 if(n&&((dst<s+16&&(uint64_t)dst+n>s-12)||(src<s+16&&(uint64_t)src+n>s-12)))return 0;
 int backward=dst>src&&(uint64_t)dst<(uint64_t)src+n;
 unsigned alignment=backward?(dst+n)&3:dst&3;
 PUSH32(g_esp,g_ebp);g_ebp=g_esp;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);
 /* Retail instructions and 9,728 independent traces establish memmove
    semantics, including backward overlap mistranslated as forward memcpy. */
 memmove((void*)XBOX_PTR(dst),(void*)XBOX_PTR(src),n);
 if(alignment&&n<4){g_ecx=backward?n:n-4;g_edx=3;}
 else{unsigned remaining=n-(alignment?(backward?alignment:4-alignment):0),words=remaining>>2;g_ecx=words>=8?0:backward?0-words:words;g_edx=remaining&3;}
 if(alignment&&n>=4)g_seh_ebp=g_ebp;
 g_eax=dst;g_esp=s+4;
 /* Existing guest frame publication and cached ZF are retained. The retail
    EFLAGS trace is recorded separately; this does not add an EFLAGS model. */
 return 1;
}
