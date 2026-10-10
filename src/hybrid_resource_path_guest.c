#include "hybrid_resource_path_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

void sub_002D76F3(void);
static int enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_RESOURCE_PATH");value=p&&!strcmp(p,"1");}return value;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int valid_name(uint32_t p,uint32_t stack){
 if(!span(p,1))return 0;
 size_t available=g_xbox_total_ram-p;if(available>4096)available=4096;
 const unsigned char*name=(const unsigned char*)XBOX_PTR(p),*end=memchr(name,0,available);
 if(!end)return 0;
 size_t size=(size_t)(end-name)+1;
 return(uint64_t)p+size<=stack-64u||p>=stack+12u;
}
static int slash(unsigned c){return c=='/'||c=='\\';}
static uint32_t lower(uint32_t c){return c>='A'&&c<='Z'?c+32:c;}
int uc_hybrid_resource_path(void){
 if(!enabled())return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10040u||!span(s-64,76)||!span(0x51c178,1)||MEM8(0x51c178)||
    !valid_name(MEM32(s+4),s)||!valid_name(MEM32(s+8),s))return 0;
 /* Whole3878F0 name ordering. Preserve guest scratch/stack publications for
  * the existing map callers; UNC disagreement keeps its original locale path.
  * The 4096-byte admission bound declines before any guest mutation. */
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);
 SET_LO8(g_ecx,MEM8(g_esi)=='\\'&&MEM8(g_esi+1)=='\\');
 PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);
 SET_LO8(g_eax,MEM8(g_edi)=='\\'&&MEM8(g_edi+1)=='\\');
 if(LO8(g_ecx)!=LO8(g_eax)){
  PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x3879e6);sub_002D76F3();
  g_esp+=8;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(LO8(g_ecx)){g_esi+=2;g_edi+=2;}
 for(;;){
  SET_LO8(g_eax,MEM8(g_esi));
  if(!LO8(g_eax)){
   SET_LO8(g_eax,MEM8(g_edi));g_eax=LO8(g_eax)?0xffffffffu:0;
   POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=4;return 1;
  }
  SET_LO8(g_ecx,MEM8(g_edi));
  if(!LO8(g_ecx)){
   POP32(g_esp,g_edi);g_eax=1;POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=4;return 1;
  }
  if(slash(LO8(g_eax))&&slash(LO8(g_ecx))){
   ++g_esi;++g_edi;
   do{SET_LO8(g_eax,MEM8(g_esi));if(!slash(LO8(g_eax)))break;++g_esi;}while(1);
   do{SET_LO8(g_eax,MEM8(g_edi));if(!slash(LO8(g_eax)))break;++g_edi;}while(1);
   continue;
  }
  g_eax=SX8(LO8(g_eax));g_edx=lower(g_eax);g_eax=lower(SX8(LO8(g_ecx)));
  if(g_edx!=g_eax){
   g_eax=(uint32_t)(int32_t)SMEM8(g_esi);g_ecx=lower(g_eax);
   g_eax=lower((uint32_t)(int32_t)SMEM8(g_edi));
   g_edx=(int32_t)g_ecx>=(int32_t)g_eax?1:0;
   POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_ecx==g_eax;
   g_edx=g_edx+g_edx-1;g_eax=g_edx;g_esp+=4;return 1;
  }
  ++g_esi;++g_edi;
 }
}
