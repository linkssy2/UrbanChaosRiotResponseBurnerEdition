#include "hybrid_particle_registry_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_000958B0(void),sub_00095090(void);
static int registry_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_REGISTRY");flag=v&&!strcmp(v,"1");}return flag;}
static int registry_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_hybrid_particle_registry(unsigned operation){
 if(!registry_enabled())return 0;
 uint32_t start=g_esp;
 if(operation>5||(start&3)||start<0x10400u||!registry_span(start-1024,1040)||
    !registry_span(g_ecx,0x4c)||!registry_span(0x519f64,8)||!registry_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(operation==5){
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);int was_null=g_esi==0;
  if(!was_null){
   g_ecx=MEM32(g_ecx+0x48);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x89ac2);sub_00095090();
   g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x89aca);RECOMP_ICALL_SAFE(target,saved);
  }
  POP32(g_esp,g_esi);g_recomp_zf=was_null;g_esp+=8;return 1;
 }
 static const uint32_t entries[]={0x89eb0,0x89f10,0x89f90,0x89ff0,0x8a100};
 static const uint32_t vtables[]={0x455f20,0x455f88,0x455ff0,0x455eb8,0x4560c0};
 uint32_t entry=entries[operation],pool=operation==1?0x519f68:0x519f64,cleanup=operation==1?16:8;
 PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=MEM32(pool);g_eax=MEM32(g_ecx);
 {uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,entry+14);RECOMP_ICALL_SAFE(target,start);}
 if(LO8(g_eax)){g_eax=0;POP32(g_esp,g_edi);g_recomp_zf=g_eax==0;g_esp+=cleanup;return 1;}
 g_ecx=MEM32(pool);g_edx=MEM32(g_ecx);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,entry+0x24);RECOMP_ICALL_SAFE(target,saved);}
 if(g_eax){
  if(operation==1){g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_ecx=MEM32(g_esp+12);MEMF(g_eax+8)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+20));}
  else g_ecx=MEM32(g_esp+12);
  g_esi=g_eax;PUSH32(g_esp,0);MEM32(g_eax)=vtables[operation];MEM32(g_eax+4)=g_ecx;
  if(operation==1)MEMF(g_eax+12)=g_xmm0.f[0];
  g_ecx=MEM32(g_edi+0x48);PUSH32(g_esp,g_esi);PUSH32(g_esp,entry+(operation==1?0x58:0x42));sub_000958B0();
  g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_edi);g_recomp_zf=0;g_esp+=cleanup;return 1;
 }
 g_ecx=MEM32(g_edi+0x48);g_esi=0;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_esi);PUSH32(g_esp,entry+(operation==1?0x6b:0x55));sub_000958B0();
 g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_edi);g_recomp_zf=g_esi==0;g_esp+=cleanup;return 1;
}
