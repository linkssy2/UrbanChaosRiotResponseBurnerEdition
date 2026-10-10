#include "migration_services_segment_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00082723(void),sub_00082C18(void);
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static void initialize_segment(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;g_esp-=12;g_eax=MEM32(f+32)-MEM32(f+24);PUSH32(g_esp,g_ebx);g_edx=(uint32_t)((int32_t)g_eax%4096);g_eax=(uint32_t)((int32_t)g_eax/4096);g_ebx=0x1000;g_ecx=g_ebx;
 PUSH32(g_esp,g_esi);g_esi=MEM32(f+12);PUSH32(g_esp,g_edi);g_edi=(g_esi+0x57)&~15u;MEM32(f-8)=g_eax;g_eax=MEM32(f+8);
 if(g_eax==MEM32(f+24)){SET_LO16(g_eax,MEM16(g_eax));MEM16(f-4)=LO16(g_eax);}else MEM32(f-4)=0;
 g_ecx=MEM32(f+28);g_eax=(uint32_t)((int32_t)(g_edi-g_esi)>>4);MEM32(f-12)=g_eax;g_eax=g_edi+16;
 if(g_eax>=g_ecx){
  if(g_eax>=MEM32(f+32)){SET_LO8(g_eax,0);goto done;}
  g_eax=g_edi-g_ecx+16;uint32_t saved=g_esp;PUSH32(g_esp,4);MEM32(f+12)=g_eax;PUSH32(g_esp,g_ebx);g_eax=f+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_eax=f+28;PUSH32(g_esp,g_eax);uint32_t target=MEM32(0x44b3d8);PUSH32(g_esp,0x8325a);RECOMP_ICALL_SAFE(target,saved);
  if((int32_t)g_eax<0){SET_LO8(g_eax,0);goto done;}
  g_ecx=MEM32(f+28)+MEM32(f+12);MEM32(f+28)=g_ecx;
 }
 g_eax=MEM32(f+32)-g_ecx;g_edx=(uint32_t)((int32_t)g_eax%4096);g_eax=(uint32_t)((int32_t)g_eax/4096);g_ebx=0x1000;
 SET_LO16(g_edx,MEM16(f-4));SET_LO8(g_ebx,MEM8(f+16));MEM16(g_esi+2)=LO16(g_edx);SET_LO16(g_edx,MEM16(f-12));MEM16(g_esi)=LO16(g_edx);
 g_edx=MEM32(f+20);MEM32(g_esi+20)=g_edx;g_edx=MEM32(f+8);MEM32(g_esi+24)=g_edx;g_edx=MEM32(f+24);MEM32(g_esi+32)=g_edx;
 g_edx=(MEM32(f-8)<<12)+MEM32(f+24);MEM8(g_esi+5)=1;MEM32(g_esi+0x2c)=g_edx;g_edx=MEM32(f-8);MEM8(g_esi+4)=LO8(g_ebx);MEM32(g_esi+16)=0xffeeffee;MEM32(g_esi+0x28)=g_edi;MEM32(g_esi+0x24)=g_edx;MEM32(g_esi+0x30)=g_eax;
 if(g_eax){g_eax<<=12;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_ebp=f;PUSH32(g_esp,0x832d4);sub_00082723();g_ecx=MEM32(f+28);}
 g_edx=MEM32(f+8);g_eax=LO8(g_ebx);g_ecx-=g_edi;MEM32(g_edx+g_eax*4+0x60)=g_esi;SET_LO16(g_eax,MEM16(g_esi));g_ecx=(uint32_t)((int32_t)g_ecx>>4);
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);MEM8(g_edi+5)=16;MEM32(g_esi+0x40)=g_edi;PUSH32(g_esp,g_edx);MEM16(g_edi+2)=LO16(g_eax);MEM8(g_edi+4)=LO8(g_ebx);g_ebp=f;PUSH32(g_esp,0x832ff);sub_00082C18();SET_LO8(g_eax,1);
done:POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f+36;
}
int uc_migration_services_segment(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_SEGMENT");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x831ea)return 0;
 uint32_t s=g_esp;if((s&3)||!span(s-256,292)||MEM8(0x51c178))return 0;
 uint32_t heap=MEM32(s+4),seg=MEM32(s+8),base=MEM32(s+20),commit=MEM32(s+24),end=MEM32(s+28);
 if((heap&15)||!span(heap,0x588)||!span(seg,0x60)||end<=base||!span(base,end-base+16)||commit<base||commit>end||seg<base||seg+0x60>end||MEM8(s+12)>=64)return 0;
 if(!apart(heap,0x588,s-256,292)||!apart(base,end-base+16,s-256,292))return 0;
 /* A new segment starts with no range records. Existing ownership is retained
  * by the original path; this admission does not clear live lists. */
 if(MEM32(seg+0x38)||MEM32(seg+0x34)||MEM32(seg+0x1c))return 0;
 initialize_segment();return 1;
}
