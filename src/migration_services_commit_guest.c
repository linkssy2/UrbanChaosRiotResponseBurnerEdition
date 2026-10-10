#include "migration_services_commit_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static void commit_range(void){
 PUSH32(g_esp,g_ebp);uint32_t f=g_esp;g_ebp=f;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ecx);MEM32(f-4)=0;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(f+12);g_eax=g_edi+0x38;g_esi=MEM32(g_eax);MEM32(f-8)=g_eax;
 if(!g_esi)goto failed;
 g_ebx=MEM32(f+16);g_eax=MEM32(g_ebx);g_ecx=MEM32(f+20);
 while(MEM32(g_esi+8)<g_eax||(g_ecx&&MEM32(g_esi+4)!=g_ecx)){
  MEM32(f-4)=g_esi;MEM32(f-8)=g_esi;g_esi=MEM32(g_esi);if(!g_esi)goto failed;
 }
 g_eax=MEM32(g_esi+4);g_ecx=MEM32(f+8);MEM32(f+12)=g_eax;g_eax=MEM32(g_ecx+0x584);
 {uint32_t saved=g_esp,target,pc;
  if(g_eax){PUSH32(g_esp,g_ebx);g_edx=f+12;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);target=g_eax;pc=0x82817;}
  else{PUSH32(g_esp,4);PUSH32(g_esp,0x1000);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0);g_eax=f+12;PUSH32(g_esp,g_eax);target=MEM32(0x44b3d8);pc=0x8282d;}
  PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);
 }
 if((int32_t)g_eax<0)goto failed;
 g_eax=MEM32(g_ebx)>>12;MEM32(g_edi+0x30)-=g_eax;g_eax=MEM32(g_edi+0x1c);if(g_eax==MEM32(g_esi+8))MEM32(g_edi+0x1c)=0;
 g_ebx=MEM32(g_edi+0x40);g_eax=MEM32(f+12);MEM32(f+8)=g_eax;
 if(MEM8(g_ebx+5)&16){g_ecx=(MEM16(g_ebx)<<4)+g_ebx;if(g_ecx==MEM32(g_esi+4))goto found;}
 g_ecx=MEM32(f-4);
 if(!g_ecx)g_ebx=MEM32(g_edi+0x28);else{g_ebx=MEM32(g_ecx+8);g_ecx=MEM32(f-4);g_ebx+=MEM32(g_ecx+4);}
 if(!(MEM8(g_ebx+5)&16)){
  g_ecx=MEM32(g_edi+0x2c);
  for(;;){g_edx=MEM16(g_ebx)<<4;MEM32(f+20)=g_ebx;g_ebx+=g_edx;
   if(g_ebx>=g_ecx||!MEM16(g_ebx)){if(g_ebx!=MEM32(f+12))goto failed;g_ebx=MEM32(f+20);break;}
   if(MEM8(g_ebx+5)&16)break;
  }
 }
found:
 MEM8(g_ebx+5)&=0xef;g_ecx=MEM32(f+16);g_edx=MEM32(g_ecx);MEM32(g_esi+4)+=g_edx;g_edx=MEM32(g_ecx);MEM32(g_esi+8)-=g_edx;
 if(!MEM32(g_esi+8)){
  g_ecx=MEM32(g_esi+4);if(g_ecx==MEM32(g_edi+0x2c))MEM8(g_eax+5)=16;else{MEM8(g_eax+5)=0;g_eax=MEM32(g_edi+0x28);}
  g_ecx=MEM32(f-8);MEM32(g_edi+0x40)=g_eax;g_eax=MEM32(g_esi);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(g_ecx)=g_eax;g_ebp=f;PUSH32(g_esp,0x828e1);
  /* Return the consumed range record to its guest-owned node pool. */
  g_ecx=MEM32(g_esp+4);g_eax=MEM32(g_ecx+24);g_edx=MEM32(g_eax+0x4c);g_eax=MEM32(g_esp+8);MEM32(g_eax)=g_edx;
  g_ecx=MEM32(g_ecx+24);MEM32(g_ecx+0x4c)=g_eax;MEM32(g_eax+4)=0;MEM32(g_eax+8)=0;g_esp+=12;
  MEM32(g_edi+0x34)--;g_eax=MEM32(f+8);g_ecx=MEM32(f+16);
 }else{MEM8(g_eax+5)=16;MEM32(g_edi+0x40)=g_eax;}
 SET_LO8(g_edx,MEM8(g_ebx+4));MEM8(g_eax+4)=LO8(g_edx);g_ecx=MEM32(g_ecx)>>4;MEM16(g_eax)=LO16(g_ecx);SET_LO16(g_ecx,MEM16(g_ebx));MEM16(g_eax+2)=LO16(g_ecx);
 if(!(MEM8(g_eax+5)&16)){SET_LO16(g_ecx,MEM16(g_eax));g_edx=LO16(g_ecx)<<4;MEM16(g_edx+g_eax+2)=LO16(g_ecx);}
 if(!MEM32(g_edi+0x1c)){g_ecx=MEM32(g_edi+0x38);while(g_ecx){g_edx=MEM32(g_ecx+8);if(g_edx>=MEM32(g_edi+0x1c))MEM32(g_edi+0x1c)=g_edx;g_ecx=MEM32(g_ecx);}}
 goto done;
failed:g_eax=0;
done:POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=f+24;
}
int uc_migration_services_commit(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_COMMIT");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x827b6)return 0;
 uint32_t s=g_esp;if((s&3)||!span(s-128,152)||MEM8(0x51c178))return 0;
 uint32_t heap=MEM32(s+4),seg=MEM32(s+8),size=MEM32(s+12);
 if(!span(heap,0x588)||!span(seg,0x44)||!span(size,4)||!apart(heap,0x588,s-128,152)||!apart(seg,0x44,s-128,152)||!apart(size,4,s-128,128)||MEM32(seg+24)!=heap)return 0;
 unsigned bytes=MEM32(size);if(!bytes||bytes>0xff000||(bytes&0xfff))return 0;
 uint32_t node=MEM32(seg+0x38);unsigned seen=0;
 while(node){if(++seen>4096||!span(node,12)||!apart(node,12,s-128,152)||!span(MEM32(node+4),MEM32(node+8)+16u)||MEM32(node+8)>0x10000000u)return 0;node=MEM32(node);}
 if(MEM32(seg+0x38)){
  uint32_t first=MEM32(seg+0x28),end=MEM32(seg+0x2c),last=MEM32(seg+0x40);
  if(end<=first||!span(first,end-first+16)||!span(last,8)||!apart(first,end-first+16,s-128,152))return 0;
  /* ponytail: bound the existing block walk; replace with a segment index only if profiling warrants it. */
  unsigned count=0;while(first<end){if(++count>65536)return 0;if(!MEM16(first))break;if(MEM8(first+5)&16)break;first+=MEM16(first)*16;}
 }
 commit_range();return 1;
}
