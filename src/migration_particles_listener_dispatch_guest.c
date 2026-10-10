#include "migration_particles_listener_dispatch_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_LISTENER_DISPATCH");f=p&&!strcmp(p,"1");}return f;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_migration_particles_listener_dispatch(unsigned entry){
 if((entry!=0x173790&&entry!=0x173810&&entry!=0x110510)||!enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10040||!span(s-64,88)||!span(p,24)||!span(0x51c178,1)||MEM8(0x51c178)||!apart(p,24,s-64,88))return 0;
 uint32_t key=MEM32(s+4),begin=MEM32(p+8),end=MEM32(p+12);
 unsigned stride=entry==0x110510?12:24;
 if(stride==12&&MEM32(p)!=0x472fbc&&MEM32(p)!=0x4742b4&&MEM32(p)!=0x47bff0)return 0;
 if(!span(key,4)||!apart(key,4,s-64,88))return 0;
 /* ponytail: bounded original listener vector; qualify larger vectors if observed. */
 if(begin&&(end<begin||(end-begin)%stride||(end-begin)/stride>4096||!span(begin,end-begin)||!apart(begin,end-begin,s-64,88)))return 0;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_seh_ebp);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edi=begin;g_esi=0;g_ebx=0;
 for(;;){
  if(!g_edi)break;
  g_edx=MEM32(g_ecx+12)-g_edi;
  /* The original reciprocal multiply leaves the quotient in EDX and EAX. */
  g_edx=(uint32_t)(((int64_t)(int32_t)g_edx*0x2aaaaaab)>>(stride==12?33:34));g_eax=g_edx+(g_edx>>31);
  if(g_esi>=g_eax)break;
  g_eax=MEM32(g_ecx+8);g_edx=MEM32(key);g_eax=MEM32(g_eax+g_ebx);
  if(g_eax!=g_edx){g_esi++;g_ebx+=stride;continue;}
  g_eax=MEM32(g_ecx+8);g_ecx=MEM32(g_ecx+20);g_edx=g_esi*3;
  uint32_t saved=g_esp;
  if(stride==12){
   g_eax+=g_edx*4;g_edx=MEM32(g_esp+32);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+28);PUSH32(g_esp,g_edx);
   uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x11056a);RECOMP_ICALL_SAFE(target,saved);
   POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=20;return 1;
  }
  g_esi=MEM32(g_esp+32);
  if(entry==0x173790){
   PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+32);g_eax+=g_edx*8;g_edx=MEM32(g_eax+16);g_eax=MEM32(g_eax+20);
   PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+32);PUSH32(g_esp,g_esi);
  }else{
   g_eax+=g_edx*8;g_edx=MEM32(g_eax+8);g_eax=MEM32(g_eax+12);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+28);PUSH32(g_esp,g_esi);
  }
  g_ecx+=g_eax;uint32_t target=g_edx;PUSH32(g_esp,entry==0x173790?0x1737f7:0x173872);RECOMP_ICALL_SAFE(target,saved);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_recomp_zf=g_ecx==0;g_esp+=20;return 1;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;SET_LO8(g_eax,0);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=20;return 1;
}
