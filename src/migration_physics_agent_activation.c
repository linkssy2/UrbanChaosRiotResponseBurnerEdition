/* Original agent activation traversal, time reset and current dispatch. */
#include "migration_physics_agent_activation.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-512,544);}
static int record_valid(uint32_t p){return safe(p,48);}
static void activate_agent(void){
 g_ecx=MEM32(g_esp+4);g_eax=MEM8(g_ecx)-2u;PUSH32(g_esp,g_esi);int timed=0;
 if(g_eax){g_eax-=2;if(!g_eax)timed=1;else{g_eax-=2;if(g_eax){POP32(g_esp,g_esi);g_esp+=4;return;}}}
 if(timed){
  double value=MEMF(g_esp+12);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;g_esi=g_ecx+48;value=MEMF(g_ecx+28);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;
  g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;
  g_eax=(g_eax&0xffff0000u)|(uint16_t)(((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000));
  if(RECOMP_PARITY8(((g_eax>>8)&255)&0x44))MEM32(g_ecx+28)=0xbf800000u;else{g_eax=MEM32(g_esp+16);MEM32(g_ecx+28)=g_eax;}
  g_eax=MEM8(g_ecx+1);g_edx=MEM32(g_esp+20);g_eax*=52;PUSH32(g_esp,g_edi);g_edi=MEM32(g_edx);g_eax=MEM32(g_eax+g_edi+0x16b4);POP32(g_esp,g_edi);
 }else{g_eax=MEM8(g_ecx+1);g_edx=MEM32(g_esp+20);g_eax*=52;g_esi=MEM32(g_edx);g_eax=MEM32(g_eax+g_esi+0x16b4);}
 if(g_eax){
  uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+20);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+20);PUSH32(g_esp,g_edx);
  if(timed)PUSH32(g_esp,g_esi);else{g_edx=g_ecx+32;PUSH32(g_esp,g_edx);}PUSH32(g_esp,g_ecx);uint32_t target=g_eax;PUSH32(g_esp,timed?0x329652:0x329601);RECOMP_ICALL_SAFE(target,saved);g_esp+=20;
  if(timed){POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=4;return;}
 }
 POP32(g_esp,g_esi);g_esp+=4;
}
static void activate_streams(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);g_eax=MEM32(g_ebx+0x48);PUSH32(g_esp,frame);frame=0;
 if((int32_t)g_eax>0){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);do{
  g_ecx=MEM32(g_ebx+0x44);g_esi=MEM32(g_ecx+frame*4);++frame;g_edi=frame==g_eax?MEM32(g_ebx+0x54):MEM16(g_ebx+0x5a);g_edi+=g_esi;
  while(g_esi<g_edi){g_edx=MEM32(g_esp+32);g_eax=MEM32(g_esp+28);g_ecx=MEM32(g_esp+24);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2fe0a5);activate_agent();g_edx=MEM8(g_esi+3);g_esi+=g_edx;g_esp+=16;}
  g_eax=MEM32(g_ebx+0x48);
 }while((int32_t)frame<(int32_t)g_eax);POP32(g_esp,g_edi);POP32(g_esp,g_esi);}
 POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=4;
}
int uc_migration_physics_agent_activation(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_AGENT_ACTIVATION");enabled=v&&!strcmp(v,"1");}
 if(!enabled||!span(g_esp-512,544)||g_fp_top<0||g_fp_top>7)return 0;uint32_t p=MEM32(g_esp+4),d=MEM32(g_esp+16);if(!safe(d,4)||!safe(MEM32(d),0x16b4+256*52))return 0;
 if(entry==0x3295c0){if(!record_valid(p))return 0;activate_agent();return 1;}
 if(entry!=0x2fe060||!safe(p,0x5c))return 0;uint32_t n=MEM32(p+0x48),base=MEM32(p+0x44);if(n>65535||(n&&!safe(base,n*4)))return 0;
 for(unsigned i=0;i<n;i++){uint32_t block=MEM32(base+i*4),bytes=i+1==n?MEM32(p+0x54):MEM16(p+0x5a);if(bytes>65535||!safe(block,bytes))return 0;for(unsigned j=0;j<bytes;){uint32_t r=block+j;if(!record_valid(r))return 0;unsigned stride=MEM8(r+3);if(stride<48||stride>bytes-j)return 0;j+=stride;}}
 activate_streams();return 1;
}
