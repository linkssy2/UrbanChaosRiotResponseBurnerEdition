#include "migration_audio_backend_state_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00424FC5(void);
extern void sub_00425052(void);
extern void sub_0042680B(void);
extern void sub_00426847(void);
extern void sub_00424F69(void);
extern void sub_00425048(void);
extern void sub_00426136(void);
extern void sub_00425001(void);
static int backend_state_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_BACKEND_STATE");f=v&&!strcmp(v,"1");}return f;}
static int backend_state_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int backend_state_safe(uint32_t p,uint64_t n){return backend_state_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int backend_state_admit(uint32_t a){
    if((a!=0x2515b0&&a!=0x251770&&a!=0x251810&&a!=0x251860&&a!=0x251960&&a!=0x2519a0&&a!=0x2519c0&&a!=0x251d70)||!backend_state_enabled()||!backend_state_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!backend_state_span(g_esp-4096,4108)||!backend_state_safe(g_ecx,80))return 0;
    uint32_t arg=MEM32(g_esp+4),helper=MEM32(g_ecx+40),stream=MEM32(g_ecx+36);
    if(a==0x2515b0)return 1;if(a==0x251d70)return backend_state_safe(arg,4)&&backend_state_span(MEM32(arg),60);
    if(a==0x251810&&arg&&!backend_state_safe(arg,4))return 0;
    if((a==0x251960||a==0x2519a0||a==0x2519c0)&&helper&&!backend_state_safe(helper,80))return 0;
    if(a==0x2519c0&&arg){if(!backend_state_span(arg,4)||(arg<g_esp+4&&(uint64_t)arg+4>g_esp-4096))return 0;if(stream&&(!backend_state_safe(stream,4)||!backend_state_span(MEM32(stream),16)))return 0;}return 1;
}
static void backend_state_ready(void){
    g_eax=MEM32(g_ecx+36);int ready=0;if((int32_t)g_eax<0&&(g_eax&0x40000000)){g_eax=0;g_ecx+=52;while(MEM32(g_ecx)!=0x8000000a){g_eax++;g_ecx+=12;if((int32_t)g_eax>=2){ready=1;break;}}}g_eax=ready;g_recomp_zf=1;g_esp+=4;
}
static void backend_state_stop(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+32);if(g_eax){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x251780);sub_00424FC5();}g_esi=MEM32(g_esi+36);int absent=g_esi==0;if(g_esi){PUSH32(g_esp,1);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x25178f);sub_00425052();}POP32(g_esp,g_esi);g_recomp_zf=absent;g_esp+=4;
}
static void backend_state_format(void){
    PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_esi=g_ecx;if(g_edi){g_edx=MEM32(g_esi+28)|0x1000000;MEM32(g_esi+28)=g_edx;g_eax=g_edx;if(MEM16(g_edi+2)!=1){g_eax|=0x4000000;MEM32(g_esi+28)=g_eax;}}
    g_eax=MEM32(g_esi+32);if(g_eax){PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x251847);sub_0042680B();}g_esi=MEM32(g_esi+36);int absent=g_esi==0;if(g_esi){PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x251855);sub_00426847();}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=absent;g_esp+=8;
}
static void backend_state_pitch(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+32);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);if(g_eax){PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x251876);sub_00424F69();}g_esi=MEM32(g_esi+36);int absent=g_esi==0;if(g_esi){PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x251884);sub_00425048();}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=absent;g_esp+=8;
}
static void backend_state_data(void){
    PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+12);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+32);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);if(g_eax){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x25197c);sub_00426136();}g_eax=MEM32(g_esi+40);int absent=g_eax==0;if(g_eax){MEM32(g_eax+16)=g_edi;MEM32(g_eax+20)=g_edi;MEM32(g_eax+28)=g_ebx;}POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=absent;g_esp+=12;
}
static void backend_state_status(void){
    PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_esi=g_ecx;if(g_edi){MEM32(g_edi)=0;g_eax=MEM32(g_esi+32);if(g_eax){PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2519e0);sub_00425001();}g_ecx=MEM32(g_esi+40);if(g_ecx){PUSH32(g_esp,0x2519ec);backend_state_ready();if(!LO8(g_eax)){MEM32(g_edi)=1;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=8;return;}g_eax=MEM32(g_esi+36);if(g_eax){g_ecx=g_esp+12;g_esi=g_eax;g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x251a0f);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_esp+12)&0x10000;MEM32(g_edi)=g_edx;}}}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;
}
int uc_migration_audio_backend_state(uint32_t a){
    if(!backend_state_admit(a))return 0;switch(a){case 0x2515b0:backend_state_ready();break;case 0x251770:backend_state_stop();break;case 0x251810:backend_state_format();break;case 0x251860:backend_state_pitch();break;case 0x251960:backend_state_data();break;case 0x2519c0:backend_state_status();break;
    case 0x2519a0:g_eax=MEM32(g_ecx+40);{int absent=g_eax==0;if(g_eax){g_ecx=MEM32(g_esp+4);g_edx=MEM32(g_esp+8);MEM32(g_eax+24)=g_ecx;MEM32(g_eax+32)=g_edx;}g_recomp_zf=absent;}g_esp+=12;break;
    default:g_ecx=MEM32(g_esp+4);g_eax=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0);uint32_t target=MEM32(g_eax+56);PUSH32(g_esp,0x251d7d);RECOMP_ICALL_SAFE(target,saved);}g_esp+=8;break;}return 1;
}
