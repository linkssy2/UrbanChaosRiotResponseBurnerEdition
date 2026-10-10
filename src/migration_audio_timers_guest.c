#include "migration_audio_timers_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243440(void);
extern void sub_002D7740(void);
extern void sub_002D7F60(void);
static int timers_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_TIMERS");f=v&&!strcmp(v,"1");}return f;}
static int timers_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int timers_safe(uint32_t p,uint64_t n){return timers_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+16<=p);}
static int timers_argument(uint32_t p,uint64_t n){return timers_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+4<=p);}
static int timers_admit(uint32_t a){
    if((a!=0x23fcd0&&a!=0x248600&&a!=0x252f90&&a!=0x253090&&a!=0x2530b0&&a!=0x2530f0&&a!=0x253140&&a!=0x253190)||!timers_enabled()||!timers_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!timers_span(g_esp-4096,4112))return 0;
    uint32_t first=MEM32(g_esp+4),second=MEM32(g_esp+8);
    if(a==0x253090)return timers_argument(first,8);if(a==0x2530b0)return timers_argument(first,8)&&timers_argument(second,8);
    if(!timers_safe(g_ecx,a==0x23fcd0?168:60))return 0;
    if(a==0x23fcd0){uint32_t provider=MEM32(g_ecx+100),timer=MEM32(g_ecx+164);return timers_argument(first,8)&&(!provider||(timers_safe(provider,4)&&timers_span(MEM32(provider),12)))&&(!timer||timers_safe(timer,48));}
    if(a==0x253190)return timers_argument(first,12)&&timers_argument(second,8);
    return timers_argument(first,a==0x2530f0?16:a==0x253140?48:8);
}
static void timers_clock(void){
    g_eax=MEM32(g_ecx+100);int absent=0;if(g_eax){g_ecx=g_eax;g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x23fcde);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esp+4);g_edx=(int32_t)g_eax<0?0xffffffff:0;MEM32(g_ecx)=g_eax;MEM32(g_ecx+4)=g_edx;}
    else{g_ecx=MEM32(g_ecx+164);absent=g_ecx==0;if(g_ecx){PUSH32(g_esp,1000);PUSH32(g_esp,0x23fcff);sub_00243440();g_ecx=MEM32(g_esp+4);g_edx=(int32_t)g_eax<0?0xffffffff:0;MEM32(g_ecx)=g_eax;MEM32(g_ecx+4)=g_edx;}}g_recomp_zf=absent;g_esp+=8;
}
static void timers_split(void){
    g_ecx=MEM32(g_esp+8);g_edx=MEM32(g_ecx+4);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);g_eax=g_esi;SET_LO8(g_ecx,32);PUSH32(g_esp,0x2530c3);sub_002D7F60();g_ecx=g_eax;g_eax=MEM32(g_esp+8);MEM32(g_eax)=g_esi;MEM32(g_eax+4)=g_ecx;POP32(g_esp,g_esi);g_esp+=4;
}
static void timers_construct(void){
    g_eax=g_ecx;g_ecx=MEM32(g_esp+8);MEM32(g_eax+16)=g_ecx;g_ecx=0;MEM32(g_eax+20)=g_ecx;MEM32(g_eax)=0x484590;MEM32(g_eax+12)=1;MEM32(g_eax+24)=g_ecx;MEM32(g_eax+28)=g_ecx;g_ecx=g_eax+4;MEM32(g_eax+8)=g_ecx;MEM32(g_ecx)=g_ecx;g_ecx=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);g_edx=g_eax+32;MEM32(g_edx)=g_esi;g_esi=MEM32(g_ecx+4);MEM32(g_edx+4)=g_esi;g_esi=MEM32(g_ecx+8);MEM32(g_edx+8)=g_esi;g_ecx=MEM32(g_ecx+12);MEM32(g_edx+12)=g_ecx;POP32(g_esp,g_esi);g_recomp_zf=g_ecx==0;g_esp+=12;
}
static void timers_copy(void){
    g_edx=MEM32(g_esp+4);g_eax=g_ecx;MEM32(g_eax)=0x484590;MEM32(g_eax+12)=1;g_ecx=MEM32(g_edx+16);MEM32(g_eax+16)=g_ecx;g_ecx=MEM32(g_edx+20);MEM32(g_eax+20)=g_ecx;g_ecx=0;MEM32(g_eax+24)=g_ecx;MEM32(g_eax+28)=g_ecx;g_ecx=g_eax+4;MEM32(g_eax+8)=g_ecx;MEM32(g_ecx)=g_ecx;g_edx+=32;PUSH32(g_esp,g_esi);g_esi=MEM32(g_edx);g_ecx=g_eax+32;MEM32(g_ecx)=g_esi;g_esi=MEM32(g_edx+4);MEM32(g_ecx+4)=g_esi;g_esi=MEM32(g_edx+8);MEM32(g_ecx+8)=g_esi;g_edx=MEM32(g_edx+12);MEM32(g_ecx+12)=g_edx;POP32(g_esp,g_esi);g_recomp_zf=g_edx==0;g_esp+=8;
}
static void timers_schedule(void){
    g_eax=MEM32(g_esp+12);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esp+12);MEM32(g_esi+20)=g_eax;PUSH32(g_esp,g_edi);MEM32(g_esi)=0x484590;MEM32(g_esi+12)=1;MEM32(g_esi+16)=0;g_edi=MEM32(g_ecx);g_edx=MEM32(g_ecx+4);g_eax=g_edi;SET_LO8(g_ecx,32);PUSH32(g_esp,0x2531c1);sub_002D7F60();MEM32(g_esi+28)=g_eax;MEM32(g_esi+24)=g_edi;g_eax=g_esi+4;MEM32(g_esi+8)=g_eax;MEM32(g_eax)=g_eax;g_eax=MEM32(g_esp+12);MEM32(g_esi+32)=3;MEM32(g_esi+40)=0xffffffff;MEM32(g_esi+44)=0;g_ecx=MEM32(g_eax);MEM32(g_esi+48)=g_ecx;g_edx=MEM32(g_eax+4);MEM32(g_esi+52)=g_edx;g_eax=MEM32(g_eax+8);MEM32(g_esi+56)=g_eax;POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=16;
}
int uc_migration_audio_timers(uint32_t a){
    if(!timers_admit(a))return 0;switch(a){case 0x23fcd0:timers_clock();break;case 0x2530b0:timers_split();break;case 0x2530f0:timers_construct();break;case 0x253140:timers_copy();break;case 0x253190:timers_schedule();break;
    case 0x248600:g_eax=MEM32(g_esp+4);g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_eax);g_ecx=g_esp+8;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x248615);timers_split();g_edx=MEM32(g_eax);MEM32(g_esi+24)=g_edx;g_eax=MEM32(g_eax+4);g_esp+=8;MEM32(g_esi+28)=g_eax;POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;break;
    case 0x252f90:g_eax=MEM32(g_esp+4);g_edx=MEM32(g_ecx);{int equal=g_edx==MEM32(g_eax);if(equal){g_ecx=MEM32(g_ecx+4);equal=g_ecx==MEM32(g_eax+4);}g_eax=equal;}g_recomp_zf=1;g_esp+=8;break;
    default:PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi+4);PUSH32(g_esp,1);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2530a4);sub_002D7740();g_ecx=MEM32(g_esi);g_esi=0;{uint64_t sum=(uint64_t)g_eax+g_ecx;g_eax=(uint32_t)sum;g_edx+=(uint32_t)(sum>>32);}POP32(g_esp,g_esi);g_recomp_zf=g_edx==0;g_esp+=4;break;}return 1;
}
