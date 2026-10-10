#include "migration_particles_boot_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00238520(void),sub_002D7FCC(void),sub_0023E940(void),sub_002D6584(void),sub_002D631D(void),sub_0023E830(void);
static int boot_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_BOOT");f=p&&!strcmp(p,"1");}return f;}
static int boot_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int boot_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define BOOT_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
static void boot_angles(void){
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_esi=0;
 uint32_t count;
 do{
  BOOT_CALL(sub_002D7FCC,0x238cec);MEM32(g_esp+8)=g_eax;
  g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=(double)SMEM32(g_esp+8);
  g_esi++;count=g_esi;
  g_fp_stack[g_fp_top]*=MEMF(0x47fff0);g_fp_stack[g_fp_top]*=MEMF(0x44d95c);
  double angle=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=angle;
  g_fp_stack[g_fp_top]=sin(g_fp_stack[g_fp_top]);MEMF(g_edi+g_esi*4-4)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
  g_fp_stack[g_fp_top]=cos(g_fp_stack[g_fp_top]);MEMF(g_edi+g_esi*4+60)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
 }while((int32_t)count<16);
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_recomp_zf=count==16;g_esp+=4;
}
static void boot_random(void){
 PUSH32(g_esp,g_esi);g_esi=0;uint32_t end;
 do{BOOT_CALL(sub_002D7FCC,0x3a8658);g_xmm0.f[0]=(float)(int32_t)g_eax;g_xmm0.f[0]*=MEMF(0x47fff0);MEMF(g_esi+0x51ca98)=g_xmm0.f[0];g_esi+=4;end=g_esi;}while((int32_t)end<64);
 POP32(g_esp,g_esi);g_recomp_zf=end==64;g_esp+=4;
}
static void boot_shutdown(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e41b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;
 g_eax=MEM32(g_esi+8);g_edi=0;MEM32(g_esp+20)=0;
 if(g_eax)do{
  g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_edi*4);
  if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x23e900);RECOMP_ICALL_SAFE(target,saved);}
  g_eax=MEM32(g_esi+8);g_edi++;
 }while(g_edi<g_eax);
 g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);BOOT_CALL(sub_002D631D,0x23e911);g_esp+=4;g_ecx=g_esi+32;MEM32(g_esp+20)=0xffffffffu;BOOT_CALL(sub_0023E830,0x23e924);
 g_ecx=MEM32(g_esp+12);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_particles_boot(unsigned entry){
 if(!boot_enabled())return 0;
 unsigned pool=entry==0x3a85d0?1:entry==0x3a8630?2:entry==0x3a86b0?3:entry==0x3a86d0?4:0;
 if(!pool&&entry!=0x3a8650&&entry!=0x3a8680&&entry!=0x3a8690&&entry!=0x238ce0&&entry!=0x23e8c0&&entry!=0x3ad8d0)return 0;
 uint32_t s=g_esp,o=entry==0x3ad8d0?0x51cb58:g_ecx;
 if((s&3)||s<0x10100u||!boot_span(s-128,148)||!boot_span(0x51c178,0xa20)||MEM8(0x51c178)||!boot_apart(s-128,148,0x51c178,0xa20))return 0;
 if(entry==0x238ce0||entry==0x3a8680||entry==0x3a8650){
  if((g_fp_control_word&63)!=63||(_mm_getcsr()&0x1f80)!=0x1f80||!boot_span(0x44d95c,4)||!boot_span(0x47fff0,4))return 0;
  uint32_t table=entry==0x238ce0?g_ecx:entry==0x3a8680?0x51cad8:0x51ca98;unsigned bytes=entry==0x3a8650?64:128;
  if(!boot_span(table,bytes)||!boot_apart(table,bytes,s-128,148)||!boot_apart(table,bytes,0x51c178,1))return 0;
 }
 if(entry==0x23e8c0||entry==0x3ad8d0){
  if(!boot_span(o,48)||!boot_apart(o,48,s-128,148)||!boot_apart(o,48,0x51c178,1))return 0;
  uint32_t n=MEM32(o+8),list=MEM32(o+4);
  /* ponytail: bound the initial manager walk at4096 entries. Keep larger or
   * invalid graphs on the original path; shared callbacks must remain valid. */
  if(n>4096||(n&&(!boot_span(list,n*4)||!boot_apart(list,n*4,s-128,148))))return 0;
  for(uint32_t i=0;i<n;i++){uint32_t p=MEM32(list+i*4);if(p&&(!boot_span(p,4)||!boot_apart(p,4,s-128,148)||!boot_span(MEM32(p),4)||!boot_apart(MEM32(p),4,s-128,148)))return 0;}
 }
 if(pool){
  static const uint32_t sizes[]={0x34,0x140,0x150,0x14},counts[]={266,100,256,266},names[]={0x480ce4,0x4830d4,0x482f60,0x4830f8},returns[]={0x3a85e1,0x3a8641,0x3a86c4,0x3a86e1},globals[]={0x51c604,0x51cb88,0x51ca90,0x51cb8c};
  unsigned i=pool-1;PUSH32(g_esp,counts[i]);PUSH32(g_esp,sizes[i]);PUSH32(g_esp,names[i]);BOOT_CALL(sub_00238520,returns[i]);g_esp+=12;MEM32(globals[i])=g_eax;g_recomp_zf=g_esp==0;g_esp+=4;
 }else if(entry==0x3a8650)boot_random();
 else if(entry==0x238ce0||entry==0x3a8680){if(entry==0x3a8680)g_ecx=0x51cad8;boot_angles();}
 else if(entry==0x3a8690){g_ecx=0x51cb58;BOOT_CALL(sub_0023E940,0x3a869a);PUSH32(g_esp,0x3ad8d0);BOOT_CALL(sub_002D6584,0x3a86a4);POP32(g_esp,g_ecx);g_esp+=4;}
 else{if(entry==0x3ad8d0)g_ecx=0x51cb58;boot_shutdown();}
 return 1;
}
#undef BOOT_CALL
