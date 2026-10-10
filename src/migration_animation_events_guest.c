#include "migration_animation_events_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int enabled(void){
 static int value=-1;
 if(value<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_EVENTS");value=v&&!strcmp(v,"1");}
 return value;
}
static int span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int separate(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int admit(uint32_t bytes){
 uint32_t s=g_esp,p=g_ecx;
 return enabled()&&!(s&3)&&s>=0x10400u&&span(s-1024,1048)&&span(p,bytes)&&
 ((uint64_t)p+bytes<=s-1024||(uint64_t)s+24<=p)&&span(0x51c178,1)&&!MEM8(0x51c178);
}

/* Registration and stable removal retain stale tail slots and partial AL
 * returns. Guest frame writes remain observable to the translated callers. */
int uc_migration_animation_handler_add(void){
 if(!admit(68)||MEM32(g_ecx)>16)return 0;
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);
 if(g_esi==16){SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;return 1;}
 PUSH32(g_esp,g_ebx);g_eax=0;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);
 if(g_esi){
  g_edx=g_ecx+4;
  do{
   if(MEM32(g_edx)==g_edi){
    POP32(g_esp,g_edi);POP32(g_esp,g_ebx);SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;return 1;
   }
   g_ebx=MEM32(g_ecx);++g_eax;g_edx+=4;
  }while((int32_t)g_eax<(int32_t)g_ebx);
 }
 MEM32(g_ecx+g_esi*4+4)=g_edi;g_eax=MEM32(g_ecx);POP32(g_esp,g_edi);++g_eax;
 POP32(g_esp,g_ebx);MEM32(g_ecx)=g_eax;g_recomp_zf=g_eax==0;SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_esp+=8;return 1;
}
int uc_migration_animation_handler_remove(void){
 if(!admit(68)||MEM32(g_ecx)>16)return 0;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);g_eax=0;PUSH32(g_esp,g_edi);
 if(g_esi){
  g_edi=MEM32(g_esp+16);g_edx=g_ecx+4;
  do{
   if(MEM32(g_edx)==g_edi){
    g_edx=g_esi-1;MEM32(g_ecx)=g_edx;
    if((int32_t)g_eax<(int32_t)g_edx){
     g_edx=g_ecx+g_eax*4+4;
     do{g_esi=MEM32(g_edx+4);MEM32(g_edx)=g_esi;g_esi=MEM32(g_ecx);++g_eax;g_edx+=4;}while((int32_t)g_eax<(int32_t)g_esi);
    }
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_esp+=8;return 1;
   }
   g_ebx=MEM32(g_ecx);++g_eax;g_edx+=4;
  }while((int32_t)g_eax<(int32_t)g_ebx);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);SET_LO8(g_eax,0);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=8;return 1;
}

/* Counts and callback slots are live after each callback. Capture only the
 * original frame argument, never the handler list. Once delivery starts,
 * original callback lifetime/ABI preconditions apply; there is no replay. */
static void invoke_handlers(void){
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ebx=g_ecx;g_eax=MEM32(g_ebx);PUSH32(g_esp,g_edi);g_edi=0;
 unsigned handled=0;
 if((int32_t)g_eax>0){
  frame=MEM32(g_esp+24);g_esi=g_ebx+4;
  do{
   g_eax=MEM32(g_esp+20);
   uint32_t saved=g_esp;PUSH32(g_esp,frame);PUSH32(g_esp,g_eax);
   uint32_t target=MEM32(g_esi);PUSH32(g_esp,0x1f0a9d);RECOMP_ICALL_SAFE(target,saved);
   g_esp+=8;
   if(LO8(g_eax)){handled=1;break;}
   g_eax=MEM32(g_ebx);++g_edi;g_esi+=4;
  }while((int32_t)g_edi<(int32_t)g_eax);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,handled);POP32(g_esp,g_ebx);g_recomp_zf=!handled;g_esp+=12;
}
int uc_migration_animation_handler_invoke(void){
 if(!admit(68)||MEM32(g_ecx)>16)return 0;
 invoke_handlers();return 1;
}
int uc_migration_animation_events_deliver(void){
 if(!admit(0x74)||MEM32(g_ecx+44)>16)return 0;
 uint32_t local=MEM32(g_ecx+112);
 if(!span(local,68)||!span(0x51c0d0,68)||MEM32(local)>16||MEM32(0x51c0d0)>16)return 0;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+44);g_ebx=0;PUSH32(g_esp,g_edi);
 if((int32_t)g_eax>0){
  g_edi=g_esi+48;
  do{
   g_eax=MEM32(g_edi);g_ecx=MEM32(g_esi+112);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f1cfd);invoke_handlers();
   if(!LO8(g_eax)){
    g_eax=MEM32(g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_ecx=0x51c0d0;PUSH32(g_esp,0x1f1d0f);invoke_handlers();
   }
   if(LO8(g_eax))MEM32(g_edi)=0;
   g_eax=MEM32(g_esi+44);++g_ebx;g_edi+=4;
  }while((int32_t)g_ebx<(int32_t)g_eax);
 }
 g_eax=MEM32(g_esi+44);g_ebx=0;g_edi=0;
 if((int32_t)g_eax>0){
  g_ecx=g_esi+48;g_edx=g_ecx;
  do{
   g_eax=MEM32(g_edx);
   if(g_eax){MEM32(g_ecx)=g_eax;++g_ebx;g_ecx+=4;}
   g_eax=MEM32(g_esi+44);++g_edi;g_edx+=4;
  }while((int32_t)g_edi<(int32_t)g_eax);
 }
 POP32(g_esp,g_edi);MEM32(g_esi+44)=g_ebx;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=4;return 1;
}

/* The original interval is [begin,end). Keep authored order, duplicates, and
 * the 16-slot append cap. Collection does not advance the controller clock. */
int uc_migration_animation_events_collect(void){
 if(!admit(0x60))return 0;
 uint32_t clip=g_ecx,count=MEM32(clip+88),owner=MEM32(g_esp+4),events=MEM32(clip+92);
 /* ponytail: bounded borrowed guest projection; extend only with larger-asset proof. */
 if(count>65536||!span(owner,0x74)||MEM32(owner+44)>16||
 !separate(owner,0x74,g_esp-1024,1048)||!separate(owner,0x74,clip,0x60)||
 (count&&(!span(events,count*24)||!separate(events,count*24,g_esp-1024,1048)||!separate(events,count*24,owner,0x74))))return 0;
 g_eax=MEM32(g_ecx+88);PUSH32(g_esp,g_ebx);g_ebx=0;
 if((int32_t)g_eax>0){
  g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_xmm1=XMM_SCALAR(MEMF(g_esp+12));
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);PUSH32(g_esp,g_edi);g_edi=0;
  do{
   g_eax=MEM32(g_ecx+92);g_xmm2=XMM_SCALAR(MEMF(g_edi+g_eax+8));g_edx=g_edi+g_eax;
   double phase=g_xmm2.f[0],begin=g_xmm1.f[0];
   unsigned flags=(phase!=phase||begin!=begin)?0x47u:phase<begin?3u:phase>begin?2u:0x42u;
   if(!(flags&1)){
    double end=g_xmm0.f[0];phase=g_xmm2.f[0];
    flags=(end!=end||phase!=phase)?0x47u:end<phase?3u:end>phase?2u:0x42u;
    if(!(flags&0x41)){
     g_eax=MEM32(g_esi+44);
     if((int32_t)g_eax<16){MEM32(g_esi+g_eax*4+48)=g_edx;++MEM32(g_esi+44);}
    }
   }
   g_eax=MEM32(g_ecx+88);++g_ebx;g_edi+=24;
  }while((int32_t)g_ebx<(int32_t)g_eax);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);
 }
 POP32(g_esp,g_ebx);g_esp+=16;return 1;
}
