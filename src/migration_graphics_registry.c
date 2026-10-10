#include "migration_graphics_registry.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0009CD60(void),sub_00094FD0(void);
static int registry_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_REGISTRY");flag=v&&!strcmp(v,"1");}return flag;}
static int registry_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int registry_node(uint32_t p,uint32_t s){return registry_span(p,24)&&((uint64_t)p+24<=s-64||p>=s+32);}
/* Shader hashes share the original ordered-tree services. Validate the read-only
 * search before publishing its stack frame; malformed/cyclic trees decline. */
int uc_migration_graphics_registry(uint32_t entry){
 if(!registry_enabled()||entry!=0x9cfd0)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!registry_span(s-64,96)||!registry_span(0x51c178,1)||MEM8(0x51c178)||!registry_node(g_ecx,s))return 0;
 uint32_t keyp=MEM32(s+8),result=MEM32(s+4),header=MEM32(g_ecx+4);
 if(!registry_span(keyp,4)||!((uint64_t)keyp+4<=s-64||keyp>=s)||!registry_span(result,5)||!registry_node(header,s))return 0;
 uint32_t p=MEM32(header+4),key=MEM32(keyp),steps=0;
 while(1){if(!registry_node(p,s)||++steps>4096)return 0;if(MEM8(p+21))break;p=MEM32(p+(key<MEM32(p+12)?0:8));}
 uint32_t frame=g_seh_ebp;int z;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,frame);frame=MEM32(g_esp+16);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edi=g_ecx;g_esi=MEM32(g_edi+4);g_eax=MEM32(g_esi+4);SET_LO8(g_edx,MEM8(g_eax+21));SET_LO8(g_ecx,1);MEM8(g_esp+12)=LO8(g_ecx);
 if(!LO8(g_edx)){g_edx=MEM32(frame);do{SET_LO8(g_ecx,g_edx<MEM32(g_eax+12));g_esi=g_eax;MEM8(g_esp+12)=LO8(g_ecx);g_eax=MEM32(g_eax+(LO8(g_ecx)?0:8));}while(!MEM8(g_eax+21));}
 g_edx=g_esi;MEM32(g_esp+24)=g_edx;
 if(LO8(g_ecx)){
  g_eax=MEM32(g_edi+4);z=g_esi==MEM32(g_eax);g_ecx=g_esp+24;
  if(z){
   PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,1);PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x9d02e);sub_0009CD60();
   g_edx=MEM32(g_eax);g_eax=MEM32(g_esp+20);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM8(g_eax+4)=1;MEM32(g_eax)=g_edx;
   POP32(g_esp,frame);POP32(g_esp,g_ecx);g_recomp_zf=z;g_esp+=12;return 1;
  }
  PUSH32(g_esp,0x9d046);sub_00094FD0();g_edx=MEM32(g_esp+24);
 }
 g_eax=MEM32(g_edx+12);key=MEM32(frame);z=g_eax==key;
 if(g_eax<key){
  g_ecx=MEM32(g_esp+12);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ecx);g_edx=g_esp+36;PUSH32(g_esp,g_edx);g_ecx=g_edi;PUSH32(g_esp,0x9d065);sub_0009CD60();
  g_ecx=MEM32(g_eax);g_eax=MEM32(g_esp+20);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(g_eax)=g_ecx;MEM8(g_eax+4)=1;
 }else{g_eax=MEM32(g_esp+20);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM8(g_eax+4)=0;MEM32(g_eax)=g_edx;}
 POP32(g_esp,frame);POP32(g_esp,g_ecx);g_recomp_zf=z;g_esp+=12;return 1;
}
