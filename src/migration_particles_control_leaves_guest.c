#include "migration_particles_control_leaves_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023DE20(void),sub_002D8237(void);
static int leaves_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_CONTROL_LEAVES");f=p&&!strcmp(p,"1");}return f;}
static int leaves_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int leaves_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int leaves_text(uint32_t p,uint32_t s){
 /* ponytail: resource names up to4095 bytes; longer strings retain the original. */
 for(unsigned i=0;i<4096;i++){if(!leaves_span(p+i,1)||!leaves_apart(p+i,1,s-128,144))return 0;if(!MEM8(p+i))return 1;}return 0;
}
static void resource_name_contains(void){
 uint32_t frame=g_seh_ebp;int found=0,finished=0;
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_eax;
 if(MEM8(g_ebx)){
  g_edi-=g_ebx;
  do{
   g_esi=g_ebx;
   for(;;){
    if(!MEM8(g_edi+g_esi)){found=1;break;}
    SET_LO8(g_eax,MEM8(g_esi));if(!LO8(g_eax)){finished=1;break;}
    g_eax=(uint32_t)(int32_t)(int8_t)LO8(g_eax);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x238d48);sub_002D8237();
    g_ecx=(uint32_t)(int32_t)SMEM8(g_edi+g_esi);PUSH32(g_esp,g_ecx);frame=g_eax;PUSH32(g_esp,0x238d54);sub_002D8237();g_esp+=8;
    if(g_eax!=frame)break;++g_esi;
   }
   if(found||finished)break;
   SET_LO8(g_eax,MEM8(g_ebx+1));++g_ebx;--g_edi;
  }while(LO8(g_eax));
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=found?g_ebx:0;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=4;
}
int uc_migration_particles_control_leaves(unsigned entry){
 if(!leaves_enabled())return 0;uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10400||!leaves_span(s-1024,1040)||!leaves_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x2ab090){
  if(!leaves_span(p,29))return 0;
  SET_LO8(g_eax,MEM8(g_esp+4));MEM8(g_ecx+28)=LO8(g_eax);g_esp+=8;return 1;
 }
 if(entry==0x23df90||entry==0x23dfb0){
  if(!leaves_span(p,44)||!leaves_apart(p,44,s-1024,1040)||(unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
  g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_eax);g_ecx+=32;PUSH32(g_esp,entry==0x23df90?0x23df9d:0x23dfbd);sub_0023DE20();
  double value=MEMF(g_eax+(entry==0x23df90?4:8));g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;g_recomp_zf=g_ecx==0;g_esp+=8;return 1;
 }
 if(entry==0x238d20){
  if(!leaves_text(g_eax,s)||!leaves_text(MEM32(s+4),s))return 0;
  resource_name_contains();return 1;
 }
 return 0;
}
