/* The original world queue owns deferred records and their retained objects.
 * Allocation/copy remain shared services; no second queue or object registry. */
#include "migration_physics_queue.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);void sub_002440D0(void);
static const uint32_t categories[23]={0x2fa259,0x2fa259,0x2fa259,0x2fa259,0x2fa002,0x2fa018,0x2fa079,0x2fa259,0x2fa259,0x2fa259,0x2fa259,0x2fa002,0x2fa259,0x2fa259,0x2fa0d9,0x2fa139,0x2fa199,0x2fa259,0x2fa259,0x2fa267,0x2fa259,0x2fa205,0x2fa259};
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-128,152);}
static int valid(uint32_t queue,uint32_t op){
 if(!safe(queue,16)||!safe(op,16)||!safe(0x2fa26c,92)||memcmp((const void*)XBOX_PTR(0x2fa26c),categories,92))return 0;
 uint32_t n=MEM32(queue+4),cap=MEM32(queue+8)&0x3fffffff;
 if(n>cap||cap>65536||(cap&&!safe(MEM32(queue),cap*16))||!apart(queue,16,op,16)||(cap&&(!apart(MEM32(queue),cap*16,op,16)||!apart(MEM32(queue),cap*16,queue,16))))return 0;
 unsigned code=MEM8(op);if(!code||code>23||code==20)return 1;
 uint32_t kind=categories[code-1],p=MEM32(op+4);
 if(kind==0x2fa259||kind==0x2fa002||kind==0x2fa199){
  if(!safe(p,8)||!apart(p,8,op,16)||!apart(p,8,queue,16)||(cap&&!apart(p,8,MEM32(queue),cap*16)))return 0;
  if(kind==0x2fa002){uint32_t q=MEM32(op+8);if(!safe(q,8)||!apart(q,8,op,16)||!apart(q,8,queue,16)||(cap&&!apart(q,8,MEM32(queue),cap*16)))return 0;}
  if(kind==0x2fa199){uint32_t q=MEM32(op+8);if(!safe(q,32)||!apart(q,32,op,16)||!apart(q,32,queue,16)||(cap&&!apart(q,32,MEM32(queue),cap*16)))return 0;}
 }else{
  unsigned count=MEM16(op+8);if(count&&(!safe(p,count*4)||!apart(p,count*4,op,16)||!apart(p,count*4,queue,16)||(cap&&!apart(p,count*4,MEM32(queue),cap*16))))return 0;
  for(unsigned i=0;i<count;i++){uint32_t q=MEM32(p+i*4);if(!safe(q,8)||!apart(q,8,op,16)||!apart(q,8,queue,16)||!apart(q,8,p,count*4)||(cap&&!apart(q,8,MEM32(queue),cap*16)))return 0;}
 }
 if(kind!=0x2fa259&&kind!=0x2fa002){if(!safe(0x51e6f4,4)||!safe(MEM32(0x51e6f4),4)||!safe(MEM32(MEM32(0x51e6f4)),24))return 0;}
 return 1;
}
static void enqueue(void){
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+8)&0x3fffffff;g_ecx=MEM32(g_edi+4);
 if(g_ecx==g_eax){PUSH32(g_esp,16);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x2f9fcb);sub_002E0150();g_esp+=8;}
 g_eax=MEM32(g_edi+4);g_edx=MEM32(g_edi);g_esi=g_eax*16+g_edx;++g_eax;MEM32(g_edi+4)=g_eax;g_edi=MEM32(g_esp+12);PUSH32(g_esp,16);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2f9feb);sub_002440D0();
 g_eax=MEM8(g_esi);g_esp+=12;--g_eax;int zf=g_eax==22;
 if(g_eax<=22){
  uint32_t kind=MEM32(0x2fa26c+g_eax*4);
  if(kind==0x2fa002||kind==0x2fa259){
   if(kind==0x2fa002){g_eax=MEM32(g_esi+4);if(MEM16(g_eax+4))++MEM16(g_eax+6);g_esi=MEM32(g_esi+8);}
   else g_esi=MEM32(g_esi+4);
   zf=MEM16(g_esi+4)==0;if(!zf)++MEM16(g_esi+6);
  }else if(kind==0x2fa199){
   g_eax=MEM32(g_esi+4);if(MEM16(g_eax+4))++MEM16(g_eax+6);g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);
   {uint32_t saved=g_esp;PUSH32(g_esp,0x24);PUSH32(g_esp,32);uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x2fa1b6);RECOMP_ICALL_SAFE(target,saved);}
   zf=g_eax==0;
   if(g_eax){
    g_edi=MEM32(g_edi+8);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_edi);g_ecx=g_edi+16;g_edx=g_eax;MEM32(g_edx)=g_ebx;g_ebx=MEM32(g_edi+4);MEM32(g_edx+4)=g_ebx;g_ebx=MEM32(g_edi+8);MEM32(g_edx+8)=g_ebx;g_edi=MEM32(g_edi+12);MEM32(g_edx+12)=g_edi;g_edi=MEM32(g_ecx);g_edx=g_eax+16;MEM32(g_edx)=g_edi;g_edi=MEM32(g_ecx+4);MEM32(g_edx+4)=g_edi;g_edi=MEM32(g_ecx+8);MEM32(g_edx+8)=g_edi;g_ecx=MEM32(g_ecx+12);POP32(g_esp,g_ebx);MEM32(g_edx+12)=g_ecx;
   }else g_eax=0;
   POP32(g_esp,g_edi);MEM32(g_esi+8)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=zf;g_esp+=8;return;
  }else if(kind==0x2fa018||kind==0x2fa079||kind==0x2fa0d9||kind==0x2fa139||kind==0x2fa205){
   uint32_t allocret=kind+0x15,copyret=kind+0x2a;
   g_eax=MEM16(g_esi+8);g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);PUSH32(g_esp,4);g_eax*=4;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,allocret);RECOMP_ICALL_SAFE(target,saved);}
   MEM32(g_esi+4)=g_eax;g_ecx=MEM16(g_edi+8);g_edx=MEM32(g_edi+4);g_ecx*=4;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,copyret);sub_002440D0();
   g_ecx=MEM16(g_esi+8);g_eax=MEM32(g_esi+4);g_edx=g_eax+g_ecx*4;g_esp+=12;zf=g_eax==g_edx;
   while(g_eax<g_edx){g_ecx=MEM32(g_eax);g_eax+=4;if(MEM16(g_ecx+4))++MEM16(g_ecx+6);zf=g_eax==g_edx;}
  }else if(kind!=0x2fa267){g_seh_ebp=frame;RECOMP_ITAIL(kind);return;}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=zf;g_esp+=8;
}
int uc_migration_physics_queue(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_QUEUE");enabled=v&&!strcmp(v,"1");}
 if(!enabled||(entry!=0x2ed500&&entry!=0x2f9fb0)||!span(g_esp-128,152))return 0;
 uint32_t queue=g_ecx;if(entry==0x2ed500){if(!safe(g_ecx,0x88))return 0;queue=MEM32(g_ecx+0x80);}
 if(!valid(queue,MEM32(g_esp+4)))return 0;
 if(entry==0x2f9fb0)enqueue();
 else{g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+0x80);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2ed513);enqueue();++MEM32(g_esi+0x84);int zf=MEM32(g_esi+0x84)==0;POP32(g_esp,g_esi);g_recomp_zf=zf;g_esp+=8;}
 return 1;
}
