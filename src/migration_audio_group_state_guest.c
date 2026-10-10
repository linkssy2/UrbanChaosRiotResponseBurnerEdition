#include "migration_audio_group_state_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243EE0(void);
extern void sub_00244F00(void);
extern void sub_00244F40(void);
extern void sub_0024B6B0(void);
extern void sub_00247E70(void);
static int group_state_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_GROUP_STATE");f=v&&!strcmp(v,"1");}return f;}
static int group_state_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int group_state_safe(uint32_t p,uint64_t n){return group_state_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int group_state_admit(uint32_t a){
    if((a!=0x2473d0&&a!=0x247410&&a!=0x247450&&a!=0x247490&&a!=0x247680&&a!=0x248180&&a!=0x2481d0)||!group_state_enabled()||!group_state_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!group_state_span(g_esp-4096,4108)||!group_state_safe(g_ecx,120))return 0;
    if(a==0x247680){uint32_t p=g_ecx;unsigned budget=4096;while(p){if(!budget--||!group_state_safe(p,96))return 0;if(MEM32(p+92))break;p=MEM32(p+44);}return 1;}
    if(a==0x248180||a==0x2481d0){uint32_t source=MEM32(g_esp+4),track=MEM32(g_esp+8);if(!source||!track||!MEM32(g_ecx+116))return 1;if(!group_state_safe(source,100)||!group_state_safe(track,32)||!group_state_safe(MEM32(g_ecx+116),144))return 0;}
    return 1;
}
static void group_state_map(unsigned offset,uint32_t getter_return,uint32_t allocate_return){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+offset);
    if(!g_eax){PUSH32(g_esp,getter_return);sub_00243EE0();g_edx=MEM32(g_eax);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,12);g_ecx=g_eax;uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,allocate_return);RECOMP_ICALL_SAFE(target,saved);
        if(g_eax){MEM32(g_eax+8)=0;MEM32(g_eax+4)=g_eax;MEM32(g_eax)=g_eax;MEM32(g_esi+offset)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=0;g_esp+=4;return;}g_eax=0;MEM32(g_esi+offset)=g_eax;
    }
    g_eax=MEM32(g_esi+offset);POP32(g_esp,g_esi);g_esp+=4;
}
static void group_state_remove(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+100);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);if(g_eax==g_edi)MEM32(g_esi+100)=0;
    g_ecx=MEM32(g_esi+68);if(g_ecx){PUSH32(g_esp,g_edi);PUSH32(g_esp,0x247473);sub_00244F00();}g_ecx=MEM32(g_esi+72);int empty=g_ecx==0;if(g_ecx){PUSH32(g_esp,g_edi);PUSH32(g_esp,0x247480);sub_00244F40();}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=empty;g_esp+=8;
}
static void group_state_dirty(void){
    for(;;){g_eax=MEM32(g_ecx+92);SET_LO8(g_eax,g_eax!=0);if(LO8(g_eax))break;g_ecx=MEM32(g_ecx+44);if(!g_ecx)break;}g_esp+=4;
}
static void group_state_propagate(int pitch){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);
    if(g_esi){g_eax=MEM32(g_esp+12);if(g_eax){g_ecx=MEM32(g_ecx+116);if(g_ecx){g_eax=MEM32(g_eax+28);PUSH32(g_esp,g_eax);PUSH32(g_esp,pitch?0x2481f1:0x2481a1);sub_0024B6B0();PUSH32(g_esp,0);PUSH32(g_esp,g_eax);PUSH32(g_esp,pitch?0x2481f9:0x2481a9);sub_00247E70();g_esp+=8;
        if(g_eax){SET_LO16(g_ecx,MEM16(g_esi+(pitch?98:96)));SET_LO16(g_ecx,LO16(g_ecx)+MEM16(g_esi+(pitch?84:86)));if(pitch)SET_LO16(g_ecx,LO16(g_ecx)+MEM16(g_esi+82));unsigned offset=pitch?64:62;if(MEM16(g_eax+offset)!=LO16(g_ecx)){MEM16(g_eax+offset)=LO16(g_ecx);MEM32(g_eax+92)|=4;}}
    }}}
    POP32(g_esp,g_esi);g_esp+=12;
}
int uc_migration_audio_group_state(uint32_t a){
    if(!group_state_admit(a))return 0;switch(a){case 0x2473d0:group_state_map(68,0x2473df,0x2473ea);break;case 0x247410:group_state_map(72,0x24741f,0x24742a);break;case 0x247450:group_state_remove();break;case 0x247490:g_eax=MEM32(g_esp+4);g_edx=MEM32(g_esp+8);MEM32(g_ecx+100)=g_eax;MEM32(g_ecx+96)=g_edx;g_esp+=12;break;case 0x247680:group_state_dirty();break;default:group_state_propagate(a==0x2481d0);break;}return 1;
}
