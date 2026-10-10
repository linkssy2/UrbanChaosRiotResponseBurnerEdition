#include "migration_audio_voice_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00240400(void);
extern void sub_00241880(void);
extern void sub_00243F40(void);
extern void sub_00243F60(void);
extern void sub_00245200(void);
extern void sub_002456C0(void);
extern void sub_0024B0A0(void);
extern void sub_00251D70(void);
static int voice_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_VOICE");f=v&&!strcmp(v,"1");}return f;}
static int voice_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int voice_admit(uint32_t a){
    if(!voice_enabled()||!voice_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x10200||!voice_span(g_esp-512,524)||!voice_span(g_ecx,0x70))return 0;
    if(!((uint64_t)g_ecx+0x70<=g_esp-512||(uint64_t)g_esp+12<=g_ecx))return 0;
    if(a==0x248a60)return voice_span(MEM32(g_esp+4),0x64);
    return a==0x24b0a0||a==0x2456c0||a==0x245200||a==0x245900;
}
/* A stopped voice releases immediately. A running voice leaves its old list,
   records its retirement delay, and transfers to the source or engine retire list. */
static void voice_retire(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+60);int release=!g_ecx;
    if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+36);PUSH32(g_esp,0x24520f);RECOMP_ICALL_SAFE(target,saved);release=(LO8(g_eax)&1)!=0;}
    if(release){g_eax=MEM32(g_esi);g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x24521a);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);g_esp+=8;return;}
    PUSH32(g_esp,g_edi);g_edi=g_esi+8;PUSH32(g_esp,g_edi);PUSH32(g_esp,0x245228);sub_00243F60();g_eax=MEM32(g_esi+56);g_ecx=MEM32(g_esp+16);g_esp+=4;MEM32(g_esi+80)=g_ecx;
    if(!g_eax){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x245240);sub_00241880();g_esp+=4;g_ecx=g_eax;PUSH32(g_esp,0x24524a);sub_00240400();g_edx=MEM32(g_esi);g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x245251);RECOMP_ICALL_SAFE(target,saved);}
    else{g_eax+=24;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x245260);sub_00243F40();g_esp+=8;}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=8;
}
static void voice_publish(void){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_edi=g_ecx;
    if(g_esi){g_eax=MEM32(g_esi);g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_eax+4);PUSH32(g_esp,0x2456d3);RECOMP_ICALL_SAFE(target,saved);g_esi+=8;PUSH32(g_esp,g_esi);g_edi+=16;PUSH32(g_esp,g_edi);PUSH32(g_esp,0x2456e0);sub_00243F40();g_esp+=8;}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;
}
/* The existing engine factory owns shared device budgets and object identity. */
static void voice_replace(void){
    g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+44);
    if(g_ecx){PUSH32(g_esp,20);PUSH32(g_esp,0x24b0b4);sub_00245200();MEM32(g_esi+44)=0;}
    g_eax=MEM32(g_esp+20);SET_LO16(g_ecx,MEM16(g_esp+16));PUSH32(g_esp,0);MEM32(g_esp+8)=g_eax;MEM16(g_esp+12)=1;MEM16(g_esp+14)=LO16(g_ecx);PUSH32(g_esp,0x24b0db);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;g_ecx=g_esp+4;
    {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x4fedd8);g_ecx=g_eax;uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x24b0ef);RECOMP_ICALL_SAFE(target,saved);}
    MEM32(g_esi+44)=g_eax;POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void voice_acquire(void){
    PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+20);g_esi=g_ecx;g_eax=MEM32(g_esi+68);g_edi|=9;MEM32(g_esp+8)=0;
    if(g_eax){g_edx=g_esp+8;g_ecx=g_eax+4;g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);PUSH32(g_esp,9);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x248a8a);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_esi+68);if(MEM8(g_eax+68)&2)g_edi|=4;}
    if(MEM8(g_esi+100)&0x40)g_edi|=16;
    g_eax=MEM32(g_esi+80);if(g_eax){SET_LO16(g_ecx,MEM8(g_eax+8));MEM16(g_esp+8)=LO16(g_ecx);}
    g_edx=MEM32(g_esp+8);g_ecx=MEM32(g_esp+16);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x248abf);sub_0024B0A0();g_edi=g_eax;
    if(g_edi){
        g_ecx=MEM32(g_esi+68);if(g_ecx){PUSH32(g_esp,g_edi);PUSH32(g_esp,0x248ad2);sub_002456C0();}
        g_eax=MEM32(g_esi+64);
        if(g_eax){
            g_ecx=g_eax;g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+32);PUSH32(g_esp,0x248ae0);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4feda0);
            if(g_ecx==MEM32(g_eax)){
                g_edx=MEM32(g_esi+64);saved=g_esp;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_edx);g_ecx=g_edx;g_eax=MEM32(g_ecx);PUSH32(g_esp,0);target=MEM32(g_eax+32);PUSH32(g_esp,0x248af9);RECOMP_ICALL_SAFE(target,saved);
                g_ecx=MEM32(g_esi+64);saved=g_esp;PUSH32(g_esp,g_eax);target=MEM32(g_ebx);PUSH32(g_esp,0x248aff);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_edi+60);POP32(g_esp,g_ebx);
                if(g_ecx){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x248b0d);sub_00251D70();}
            }
        }
    }
    g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_esp+=12;
}
static void voice_loop(void){
    g_eax=1;if(MEM16(g_esp+4)!=LO16(g_eax))MEM32(g_ecx+72)|=g_eax;else MEM32(g_ecx+72)&=~1u;
    g_eax=MEM32(g_ecx+60);if(g_eax){g_ecx=MEM32(g_ecx+72);MEM32(g_eax+28)|=g_ecx;}g_esp+=8;
}
int uc_migration_audio_voice(uint32_t a){if(!voice_admit(a))return 0;switch(a){case 0x248a60:voice_acquire();break;case 0x24b0a0:voice_replace();break;case 0x2456c0:voice_publish();break;case 0x245200:voice_retire();break;default:voice_loop();}return 1;}
