#include "migration_animation_resource_table_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
extern void sub_0005F1E0(void),sub_002D589F(void),sub_002D631D(void),sub_002D6780(void),sub_002D68FB(void),sub_002D69BB(void);
static int resource_table_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int resource_table_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static void resource_table_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
int uc_migration_animation_resource_table(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_RESOURCE_TABLE");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;if(!enabled||(s&3)||s<0x10400||!resource_table_span(s-1024,1032)||!resource_table_span(p,12)||!resource_table_apart(p,12,s-1024,1032)||!resource_table_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x1f4920){
  PUSH32(g_esp,g_esi);g_esi=p;PUSH32(g_esp,0x1f4928);sub_0005F1E0();MEM32(g_esi+4)=g_eax;MEM8(g_eax+17)=1;g_eax=MEM32(g_esi+4);MEM32(g_eax+4)=g_eax;g_eax=MEM32(g_esi+4);MEM32(g_eax)=g_eax;g_eax=MEM32(g_esi+4);MEM32(g_eax+8)=g_eax;MEM32(g_esi+8)=0;g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=4;return 1;
 }
 if(entry==0x1f4a10){
  uint32_t buffer=MEM32(p+4);if(buffer&&(!resource_table_span(buffer-4,4)||MEM32(buffer-4)>2048||!resource_table_span(buffer,MEM32(buffer-4)*12)||!resource_table_apart(buffer-4,4+MEM32(buffer-4)*12,s-1024,1032)))return 0;
  g_eax=buffer;if(g_eax){g_ecx=MEM32(g_eax-4);PUSH32(g_esp,g_esi);g_esi=g_eax-4;PUSH32(g_esp,0x1f48f0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,12);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f4a2c);sub_002D69BB();PUSH32(g_esp,g_esi);PUSH32(g_esp,0x1f4a32);sub_002D631D();g_esp+=4;POP32(g_esp,g_esi);}g_esp+=4;return 1;
 }
 if(entry!=0x1f4950||MEM32(s+4)>4096||g_fp_top<0||g_fp_top>7||(_mm_getcsr()&0x1f80)!=0x1f80||!resource_table_span(0x44c790,8)||MEM32(0x44c790)||MEM32(0x44c794)!=0x40000000||!resource_table_span(0x44bf94,4)||MEM32(0x44bf94)!=0x3f800000)return 0;
 g_eax=MEM32(0);PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39b95b);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;
 if((int32_t)MEM32(g_esp+28)>15){
  resource_table_push(MEMD(0x44c790));resource_table_push(0.69314718055994530942);unsigned top=(unsigned)g_fp_top,next=(top+1)&7;double swap=g_fp_stack[top];g_fp_stack[top]=g_fp_stack[next];g_fp_stack[next]=swap;g_fp_stack[next]*=log2(g_fp_stack[top]);g_fp_top=(int)next;
  resource_table_push((double)SMEM32(g_esp+28));resource_table_push(0.69314718055994530942);top=(unsigned)g_fp_top;next=(top+1)&7;swap=g_fp_stack[top];g_fp_stack[top]=g_fp_stack[next];g_fp_stack[next]=swap;g_fp_stack[next]*=log2(g_fp_stack[top]);g_fp_top=(int)next;
  g_fp_stack[g_fp_top]/=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[g_fp_top]-=MEMF(0x44bf94);PUSH32(g_esp,0x1f4994);sub_002D6780();g_fp_top=(g_fp_top+1)&7;g_esi=1;g_ecx=g_eax;g_esi<<=LO8(g_ecx);
 }else g_esi=4;
 g_eax=g_esi+g_esi*2;g_ecx=g_eax*4+4;PUSH32(g_esp,g_ecx);MEM32(g_edi)=g_esi;PUSH32(g_esp,0x1f49b8);sub_002D589F();g_esp+=4;MEM32(g_esp+28)=g_eax;MEM32(g_esp+20)=0;
 if(g_eax){PUSH32(g_esp,0x1f48f0);PUSH32(g_esp,0x1f4920);PUSH32(g_esp,g_esi);g_ebx=g_eax+4;PUSH32(g_esp,12);PUSH32(g_esp,g_ebx);MEM32(g_eax)=g_esi;PUSH32(g_esp,0x1f49e3);sub_002D68FB();MEM32(g_edi+4)=g_ebx;}else{g_eax=0;MEM32(g_edi+4)=0;}
 g_ecx=MEM32(g_esp+12);g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
