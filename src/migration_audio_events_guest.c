#include "migration_audio_events_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241130(void);
extern void sub_002411F0(void);
extern void sub_00240050(void);
extern void sub_002495C0(void);
extern void sub_00248C50(void);
extern void sub_00249CC0(void);
extern void sub_0024B1E0(void);
extern void sub_00245430(void);
extern void sub_0024B310(void);
extern void sub_002481D0(void);
extern void sub_00248180(void);
extern void sub_00248B80(void);
extern void sub_0024B350(void);
extern void sub_0023FEA0(void);
static int events_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_EVENTS");enabled=v&&!strcmp(v,"1");}return enabled;}
static int events_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int events_safe(uint32_t p,uint32_t n){return events_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int events_admit(uint32_t address){
    static const uint32_t targets[]={0x24165a,0x241692,0x2416dc,0x24173c,0x241751,0x2417a3,0x2417f5,0x241821,0x241676};
    static const uint8_t indices[]={0,1,1,2,3,4,5,6,8,8,8,7,8,8,7,4,5};
    if(address!=0x241610||!events_enabled()||!events_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!events_span(g_esp-4096,4108))return 0;
    if(memcmp((void*)(g_xbox_mem_offset+0x241848),targets,sizeof targets)||memcmp((void*)(g_xbox_mem_offset+0x24186c),indices,sizeof indices))return 0;
    uint32_t command=MEM32(g_esp+4);if(!events_safe(g_ecx,184)||!events_safe(command,96))return 0;
    uint32_t track=MEM32(command+80),definition=MEM32(command+84);
    return events_safe(track,104)&&events_safe(definition,28)&&events_span(MEM32(track),4)&&events_span(MEM32(command),8);
}
int uc_migration_audio_events(uint32_t address){
    if(!events_admit(address))return 0;
    uint32_t frame=g_seh_ebp;g_esp-=8;g_eax=MEM32(g_esp+12);uint32_t saved=g_esp;
    PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_eax+84);PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax+80);g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0);g_ebx=g_ecx;PUSH32(g_esp,0x4fe7fc);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x241630);RECOMP_ICALL_SAFE(target,saved);
    g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe7f8);g_ecx=g_esi;g_edi=g_eax;target=MEM32(g_edx);PUSH32(g_esp,0x24163f);RECOMP_ICALL_SAFE(target,saved);
    MEM32(g_esp+20)=g_eax;g_eax=MEM8(frame+4);uint32_t kind=8;if(g_eax<=16){g_eax=MEM8(g_eax+0x24186c);kind=g_eax;}
    int repeat=0,release=1;
    switch(kind){
    case 0:
        g_ecx=MEM32(g_esp+28);PUSH32(g_esp,g_ecx);g_ecx=g_ebx;PUSH32(g_esp,0x241666);sub_00241130();MEM8(g_esp+19)=LO8(g_eax);SET_LO8(g_eax,MEM8(g_esp+19));repeat=LO8(g_eax)!=0;break;
    case 1:
        g_edx=MEM16(frame+18);g_eax=MEM16(frame+16);g_ecx=g_ebx;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2416a7);sub_00240050();MEM16(g_esi+98)=LO16(g_eax);
        if(g_edi){PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x2416b7);sub_002495C0();}
        g_ecx=MEM16(frame+22);g_edx=MEM16(frame+20);PUSH32(g_esp,g_ecx);g_ecx=g_ebx;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x2416cc);sub_00240050();MEM16(g_esi+96)=LO16(g_eax);
        if(g_edi){PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x2416dc);sub_00248C50();}
        /* Both randomized start variants continue through ordinary playback. */
        /* fall through */
    case 2:
        if(!(MEM8(g_esi+73)&2)){
            g_eax=MEM32(g_esp+20);
            if(!g_eax&&g_edi){PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x2416f7);sub_00249CC0();
                g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe7f8);g_ecx=g_esi;target=MEM32(g_eax);PUSH32(g_esp,0x241704);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esp+20)=g_eax;}
            if(MEM8(g_esi+73)&0x20){g_ecx=g_esi;PUSH32(g_esp,0x241715);sub_0024B1E0();}
            else{g_ecx=MEM32(g_esp+20);if(g_ecx){PUSH32(g_esp,0x241724);sub_00245430();}}
        }
        g_ecx=MEM16(g_esi+72);g_ecx=(g_ecx&0xefff)|0x24;MEM16(g_esi+72)=LO16(g_ecx);break;
    case 3:
        PUSH32(g_esp,0x40);g_ecx=g_esi;PUSH32(g_esp,0x241745);sub_0024B310();release=0;break;
    case 4:
        g_eax=MEM32(g_esp+28);g_edx=g_esp+20;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_ecx=g_ebx;PUSH32(g_esp,0x241762);sub_002411F0();SET_LO16(g_ecx,MEM16(g_esp+20));MEM8(g_esp+19)=LO8(g_eax);MEM16(g_esi+98)=LO16(g_ecx);
        if(g_edi){PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x24177b);sub_002495C0();}
        else{g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe7f4);g_ecx=g_esi;target=MEM32(g_edx);PUSH32(g_esp,0x24178d);RECOMP_ICALL_SAFE(target,saved);
            if(g_eax){PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x24179e);sub_002481D0();}}
        SET_LO8(g_eax,MEM8(g_esp+19));repeat=LO8(g_eax)!=0;break;
    case 5:
        g_ecx=MEM32(g_esp+28);g_eax=g_esp+20;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_ebx;PUSH32(g_esp,0x2417b4);sub_002411F0();SET_LO16(g_edx,MEM16(g_esp+20));MEM8(g_esp+19)=LO8(g_eax);MEM16(g_esi+96)=LO16(g_edx);
        if(g_edi){PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x2417cd);sub_00248C50();}
        else{g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe7f4);g_ecx=g_esi;target=MEM32(g_eax);PUSH32(g_esp,0x2417df);RECOMP_ICALL_SAFE(target,saved);
            if(g_eax){PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x2417f0);sub_00248180();}}
        SET_LO8(g_eax,MEM8(g_esp+19));repeat=LO8(g_eax)!=0;break;
    case 6:
        g_edx=MEM32(g_esp+28);g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_ebx;PUSH32(g_esp,0x241806);sub_002411F0();MEM8(g_esp+19)=LO8(g_eax);
        if(g_edi){PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x24181c);sub_00248B80();}
        SET_LO8(g_eax,MEM8(g_esp+19));repeat=LO8(g_eax)!=0;break;
    case 7:
        g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x24182d);sub_0024B350();break;
    default:break;
    }
    if(repeat){g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_eax);g_ecx=g_ebx;PUSH32(g_esp,0x24183e);sub_0023FEA0();}
    else if(release){g_ecx=MEM32(g_esp+28);if(!--MEM32(g_ecx+24)){g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,1);target=MEM32(g_edx+4);PUSH32(g_esp,0x241686);RECOMP_ICALL_SAFE(target,saved);}}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);if(!repeat)SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
