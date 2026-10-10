#include "migration_audio_bind_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002487F0(void);
extern void sub_0024B600(void);
extern void sub_0024CB20(void);
extern void sub_00241880(void);
extern void sub_0023FE50(void);
extern void sub_002D57FB(void);
extern void sub_0024CF10(void);
extern void sub_0024BD20(void);
extern void sub_0024B050(void);
extern void sub_00243F40(void);
extern void sub_002523A0(void);
extern void sub_00247D30(void);
extern void sub_0024B650(void);
extern void sub_00251A80(void);
static int bind_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_BIND");f=v&&!strcmp(v,"1");}return f;}
static int bind_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int bind_heap(uint32_t p,uint32_t n){return bind_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+16<=p);}
static int bind_admit(uint32_t a){
    if((a!=0x2487f0&&a!=0x249280)||!bind_enabled()||!bind_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!bind_span(g_esp-4096,4112)||!bind_heap(g_ecx,108))return 0;
    uint32_t bank=MEM32(g_esp+4);if(bank&&!bind_heap(bank,188))return 0;
    uint32_t old=MEM32(g_ecx+60);if(old&&!bind_heap(old,4))return 0;
    if(a==0x249280){uint32_t flags=MEM32(g_esp+12);if(flags&&!bind_heap(flags,4))return 0;}
    return 1;
}
static void bind_bank(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+12);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+16);PUSH32(g_esp,g_edi);g_edi=g_ecx;frame=MEM32(g_edi+60);
    if(g_esi){
        MEM32(g_edi+60)=g_esi;g_eax=MEM32(g_esi);g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_eax+4);PUSH32(g_esp,0x24880f);RECOMP_ICALL_SAFE(target,saved);
        g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_edx+32);PUSH32(g_esp,0x248816);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fed94);
        if(g_ecx==MEM32(g_eax)){g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed94);g_ecx=g_esi;target=MEM32(g_edx);PUSH32(g_esp,0x24882d);RECOMP_ICALL_SAFE(target,saved);
            g_esi=g_eax;PUSH32(g_esp,g_ebx);g_ecx=g_esi;PUSH32(g_esp,0x248837);sub_0024B600();SET_LO16(g_eax,MEM16(g_eax+2));
            if(LO16(g_eax)!=0xffff){g_eax=LO16(g_eax);PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x24884c);sub_0024CB20();MEM32(g_edi+80)=g_eax;}
        }
    }
    if(frame){g_edx=MEM32(frame);g_ecx=frame;uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x24885b);RECOMP_ICALL_SAFE(target,saved);}
    g_ecx=MEM32(g_edi+24);g_eax=g_edi+24;MEM32(g_edi+84)=g_ebx;
    if(g_ecx==g_eax){PUSH32(g_esp,0);PUSH32(g_esp,0x24886f);sub_00241880();g_esp+=4;PUSH32(g_esp,g_edi);g_ecx=g_eax;PUSH32(g_esp,0x24887a);sub_0023FE50();}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=12;
}
static void bind_prepare(void){
    uint32_t frame=g_seh_ebp;g_eax=MEM32(0);uint32_t saved=g_esp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39e7e6);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_eax+24);PUSH32(g_esp,0x2492a0);RECOMP_ICALL_SAFE(target,saved);
    if(!g_eax)g_edi=MEM32(g_esp+32);
    if(g_eax||!g_edi){SET_LO8(g_eax,0);g_ecx=MEM32(g_esp+16);MEM32(0)=g_ecx;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=16;return;}
    g_ecx=MEM32(g_esp+36);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x2492d0);sub_002487F0();
    g_edx=MEM32(g_edi);g_ecx=g_edi;saved=g_esp;target=MEM32(g_edx+32);PUSH32(g_esp,0x2492d7);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fed94);
    if(g_ecx==MEM32(g_eax)){
        g_edx=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed94);g_ecx=g_edi;target=MEM32(g_edx);PUSH32(g_esp,0x2492f2);RECOMP_ICALL_SAFE(target,saved);
        g_ebx=g_eax;g_eax=MEM32(g_esi+80);SET_LO8(g_ecx,MEM8(g_eax+6));g_edi=0;
        if(LO8(g_ecx))do{
            PUSH32(g_esp,0x68);PUSH32(g_esp,0x249307);sub_002D57FB();g_esp+=4;MEM32(g_esp+32)=g_eax;frame=0;MEM32(g_esp+24)=frame;
            if(g_eax){g_ecx=g_eax;PUSH32(g_esp,0x24931f);sub_0024CF10();frame=g_eax;}
            g_ecx=MEM32(g_esi+80);g_edx=MEM32(g_ecx);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);g_ecx=g_ebx;MEM32(g_esp+32)=0xffffffff;PUSH32(g_esp,0x249337);sub_0024BD20();PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ecx=frame;PUSH32(g_esp,0x249341);sub_0024B050();
            frame+=20;g_eax=g_esi+40;PUSH32(g_esp,frame);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24934e);sub_00243F40();g_eax=MEM32(g_esi+80);g_ecx=MEM8(g_eax+6);g_esp+=8;g_edi++;
        }while(g_edi<g_ecx);
        g_eax=MEM32(g_esi+68);
        if(g_eax){g_edi=MEM32(g_eax+60);if(g_edi){
            if(!(MEM8(g_eax+68)&2)){g_edx=0;SET_LO16(g_edx,MEM16(0x4839b0));g_ecx=g_edi;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x249382);sub_002523A0();}
            else{g_ecx=MEM32(g_esi+72);if(g_ecx){PUSH32(g_esp,0x249390);sub_00247D30();}else g_eax=0xfffffda8;
                g_ecx=MEM32(g_esi+80);g_edx=0;SET_LO16(g_edx,MEM16(g_ecx+12));SET_LO16(g_edx,LO16(g_edx)+LO16(g_eax));g_ecx=g_edi;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x2493ab);sub_002523A0();
                g_eax=MEM32(g_esi+80);SET_LO16(g_eax,MEM16(g_eax+22));if(LO16(g_eax)!=0xffff){g_ecx=LO16(g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_ebx;PUSH32(g_esp,0x2493c3);sub_0024B650();PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x2493cb);sub_00251A80();}
            }
        }}
        g_edi=MEM32(g_esp+40);if(g_edi){g_edx=MEM32(g_esp+36);PUSH32(g_esp,g_edx);g_ecx=g_ebx;PUSH32(g_esp,0x2493e3);sub_0024B600();g_eax=MEM16(g_eax);g_ecx=MEM32(g_edi);g_eax=(g_eax&0x800)<<2;g_ecx|=g_eax;MEM32(g_edi)=g_ecx;}
    }else{
        PUSH32(g_esp,0x68);PUSH32(g_esp,0x2493fd);sub_002D57FB();g_esp+=4;MEM32(g_esp+36)=g_eax;MEM32(g_esp+24)=1;
        if(g_eax){g_ecx=g_eax;PUSH32(g_esp,0x249417);sub_0024CF10();g_edi=g_eax;}else g_edi=0;
        PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ecx=g_edi;MEM32(g_esp+36)=0xffffffff;PUSH32(g_esp,0x249431);sub_0024B050();g_edi+=20;g_ecx=g_esi+40;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x24943e);sub_00243F40();g_eax=MEM32(g_esi+68);g_esp+=8;
        if(g_eax&&(MEM8(g_eax+68)&2)){g_eax=MEM32(g_eax+60);if(g_eax){PUSH32(g_esp,0);g_ecx=g_eax;PUSH32(g_esp,0x24945e);sub_00251A80();}}
    }
    g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);MEM32(g_esi+104)=1;POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,1);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=16;
}
int uc_migration_audio_bind(uint32_t a){if(!bind_admit(a))return 0;if(a==0x2487f0)bind_bank();else bind_prepare();return 1;}
