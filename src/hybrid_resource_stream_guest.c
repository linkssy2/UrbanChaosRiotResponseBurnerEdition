#include "hybrid_resource_stream_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00080D27(void);
extern void sub_00080F6E(void);
extern void sub_00081221(void);
static int enabled(void){static int value=-1;if(value<0){const char*s=getenv("UCRR_NATIVE_RESOURCE_STREAM");value=s&&!strcmp(s,"1");}return value;}
static int span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int admit(unsigned op){
 uint32_t s=g_esp,p=g_ecx;
 if(op>4||(s&3)||s<0x10400u||!span(s-1024,1044)||!span(p,20)||!apart(p,20,s-1024,1044)||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(op&&MEM32(p)!=0x455be4)return 0;
 if(op==1||op==2){
  uint32_t provider=MEM32(p+16);
  if(!span(provider,4)||!span(MEM32(provider),36))return 0;
  if(op==1&&(int32_t)MEM32(s+8)<=(int32_t)MEM32(p+8)){
   uint32_t dst=MEM32(s+4),n=MEM32(s+8);
   if(!span(dst,n)||!apart(dst,n,s-1024,1044)||!apart(dst,n,p,20))return 0;
  }
 }
 return 1;
}
int uc_hybrid_resource_stream(unsigned op){
 if(!enabled()||!admit(op))return 0;
 /* The original object remains the owner. File services, provider failures and
  * deleting wrappers retain their exact call frontiers and lifetime behavior.
  * In particular a returning error callback does not undo a read/skip, and
  * remaining bytes are reloaded after callbacks rather than cached before I/O. */
 if(op==0){
  g_edx=MEM32(g_esp+12);g_eax=g_ecx;g_ecx=MEM32(g_esp+4);MEM32(g_eax+4)=g_ecx;
  g_ecx=MEM32(g_esp+8)-16;MEM32(g_eax)=0x455be4;MEM32(g_eax+8)=g_ecx;MEM32(g_eax+12)=g_ecx;MEM32(g_eax+16)=g_edx;
  g_recomp_zf=g_ecx==0;g_esp+=16;return 1;
 }
 if(op==3){g_edx=MEM32(g_ecx+8);g_eax=g_edx==0;g_recomp_zf=g_edx==0;g_esp+=4;return 1;}
 if(op==4){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);MEM32(g_esi)=0x455be4;
  PUSH32(g_esp,0x87d72);sub_00081221();MEM32(g_esi)=0x455b80;POP32(g_esp,g_esi);g_esp+=4;return 1;
 }
 PUSH32(g_esp,g_esi);
 if(op==1){
  g_esi=g_ecx;g_eax=MEM32(g_esi+8);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);
  if((int32_t)g_edi>(int32_t)g_eax){POP32(g_esp,g_edi);SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=12;return 1;}
  g_ecx=MEM32(g_esp+12);g_edx=MEM32(g_esi+4);PUSH32(g_esp,0);g_eax=g_esp+20;
  PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x87cc5);sub_00080D27();
  if(!g_eax){g_ecx=MEM32(g_esi+16);g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+32);PUSH32(g_esp,0x87cd1);RECOMP_ICALL_SAFE(target,saved);}
  MEM32(g_esi+8)-=g_edi;POP32(g_esp,g_edi);SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_esp+=12;return 1;
 }
 PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_esi=g_ecx;g_eax=MEM32(g_esi+8);PUSH32(g_esp,1);PUSH32(g_esp,0);
 if((int32_t)g_edi>(int32_t)g_eax)g_edi=g_eax;
 g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x87d0e);sub_00080F6E();
 if((int32_t)g_eax<0){g_ecx=MEM32(g_esi+16);g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+32);PUSH32(g_esp,0x87d1a);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_esi+8)-g_edi;POP32(g_esp,g_edi);MEM32(g_esi+8)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;return 1;
}
