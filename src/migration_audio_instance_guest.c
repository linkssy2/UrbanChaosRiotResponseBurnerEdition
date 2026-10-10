#include "migration_audio_instance_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_00243F40(void);
extern void sub_00243F60(void);
extern void sub_00245640(void);
extern void sub_00245670(void);

static int instance_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_INSTANCE");f=v&&!strcmp(v,"1");}return f;}
static int instance_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int instance_object(uint32_t p,uint32_t n){return instance_span(p,n)&&((uint64_t)p+n<=g_esp-128||(uint64_t)g_esp+20<=p);}
static int instance_link(uint32_t p){return instance_object(p,8)&&instance_object(MEM32(p),8)&&instance_object(MEM32(p+4),8)&&MEM32(MEM32(p)+4)==p&&MEM32(MEM32(p+4))==p;}
static int instance_admit(uint32_t a){
    if(!instance_enabled()||!instance_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x10080||!instance_span(g_esp-128,148))return 0;
    uint32_t p=g_ecx,arg=MEM32(g_esp+4);
    if(a==0x245670)return instance_object(p,0x30)&&instance_link(p+0x28)&&(!arg||(instance_object(arg,0x70)&&instance_link(arg+0x30)&&instance_span(MEM32(arg),12)));
    if(a==0x2456a0)return !arg||(instance_object(arg,0x70)&&instance_link(arg+0x30)&&instance_span(MEM32(arg),12));
    if(!instance_object(p,0x70))return 0;
    if(a==0x249c30||a==0x24cf10||a==0x248730)return 1;
    if(a==0x24b050){uint32_t def=MEM32(g_esp+12);return !def||instance_span(def,5);}
    if(a==0x2487c0)return instance_span(MEM32(p+4),4);
    if(a==0x2489f0){uint32_t old=MEM32(p+0x44);return instance_link(p+0x30)&&(!arg||(instance_object(arg,0x54)&&instance_span(MEM32(arg),12)))&&(!old||(instance_object(old,0x54)&&instance_span(MEM32(old),12)));}
    if(a==0x248dd0){
        static const uint32_t table[]={0x248de3,0x248e42,0x248dfa,0x248e02,0x248e09,0x248e11,0x248e24,0x248e3c};
        if(!instance_span(0x248e58,sizeof table))return 0;
        for(unsigned i=0;i<8;i++)if(MEM32(0x248e58+4*i)!=table[i])return 0;
        uint32_t output=MEM32(g_esp+8);return !output||instance_span(output,4);
    }
    return 0;
}
static void instance_begin(void){
    g_eax=g_ecx;
    for(unsigned n=0x3c;n<=0x50;n+=4)MEM32(g_eax+n)=0;
    for(unsigned n=0x5c;n<=0x6c;n+=4)MEM32(g_eax+n)=0;
    for(unsigned n=8;n<=0x30;n+=8)MEM32(g_eax+n)=MEM32(g_eax+n+4)=g_eax+n;
    MEM32(g_eax)=0x483c10;MEM32(g_eax+4)=0x483c04;MEM32(g_eax+0x38)=1;
    MEM32(g_eax+0x54)=MEM32(g_eax+0x58)=g_edx=0xffffffffu;g_ecx=g_eax+0x30;g_recomp_zf=0;g_esp+=4;
}
static void instance_track_begin(void){
    g_eax=g_ecx;MEM32(g_eax+8)=MEM32(g_eax+12)=0;
    for(unsigned n=0x24;n<=0x38;n+=4)MEM32(g_eax+n)=0;
    for(unsigned n=0x40;n<=0x54;n+=4)MEM32(g_eax+n)=0;
    MEM32(g_eax+0x60)=0;
    MEM32(g_eax+0x14)=MEM32(g_eax+0x18)=g_eax+0x14;
    MEM32(g_eax+0x1c)=MEM32(g_eax+0x20)=g_eax+0x1c;
    MEM32(g_eax)=0x483eb8;MEM32(g_eax+0x10)=1;MEM32(g_eax+0x3c)=0xffffffffu;
    g_edx=0x7fff;for(unsigned n=0x58;n<=0x5c;n+=2)MEM16(g_eax+n)=0x7fff;
    MEM16(g_eax+0x5e)=8000;g_ecx=g_eax+0x1c;g_recomp_zf=g_ecx==0;g_esp+=4;
}
static void instance_state(void){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);SET_LO8(g_eax,1);int allow=1;
    if(g_esi<7&&g_esi!=1){g_edx=MEM32(g_ecx+0x68);int32_t old=(int32_t)g_edx;
        switch(g_esi){
        case 0:allow=(old>=0&&old<=1)||old==6;break;
        case 2:allow=old!=2&&(old<=6||old>8);break;
        case 3:allow=old!=0&&(old<=6||old>8);break;
        case 4:allow=old<7||old>8;break;
        case 5:allow=old<6||old>8;break;
        case 6:allow=old!=5&&(old<=6||old>8);break;
        }
    }else if(g_esi==7)allow=MEM32(g_ecx+0x68)!=8;
    if(allow){
        g_edx=MEM32(g_esp+12);
        if(g_edx){PUSH32(g_esp,g_edi);g_edi=MEM32(g_ecx+0x68);MEM32(g_edx)=g_edi;POP32(g_esp,g_edi);}
        MEM32(g_ecx+0x68)=g_esi;g_recomp_zf=g_edx==0;
    }else{SET_LO8(g_eax,0);g_recomp_zf=1;}
    POP32(g_esp,g_esi);g_esp+=12;
}
static void instance_release(void){
    g_eax=MEM32(g_ecx+0x38)-1;MEM32(g_ecx+0x38)=g_eax;
    if(g_eax==1&&(MEM8(g_ecx+0x64)&1))MEM32(g_ecx+0x38)=0;
    g_eax=MEM32(g_ecx+0x38);
    if(!g_eax){g_eax=MEM32(g_ecx+4);g_ecx+=4;
        uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2487ea);RECOMP_ICALL_SAFE(target,saved);g_eax=0;}
    g_esp+=4;
}
static void instance_attach(void){
    PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=MEM32(g_esp+8);
    if(g_ecx){
        g_eax=MEM32(g_ecx+0x30);PUSH32(g_esp,g_esi);g_esi=g_ecx+0x30;
        if(g_eax==g_esi){g_eax=MEM32(g_ecx);
            {uint32_t saved=g_esp,target=MEM32(g_eax+4);PUSH32(g_esp,0x24568b);RECOMP_ICALL_SAFE(target,saved);}
            PUSH32(g_esp,g_esi);g_edi+=0x28;PUSH32(g_esp,g_edi);PUSH32(g_esp,0x245695);sub_00243F40();g_esp+=8;
        }POP32(g_esp,g_esi);
    }POP32(g_esp,g_edi);g_esp+=8;
}
static void instance_detach(void){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);
    if(g_esi){g_eax=g_esi+0x30;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2456b2);sub_00243F60();g_edx=MEM32(g_esi);g_esp+=4;g_ecx=g_esi;
        uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x2456bc);RECOMP_ICALL_SAFE(target,saved);}
    POP32(g_esp,g_esi);g_esp+=8;
}
static void instance_bind_buffer(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esp+8);
    if(g_ecx){uint32_t saved=g_esp;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+0x44);MEM32(g_esi+0x44)=g_ecx;g_eax=MEM32(g_ecx);
        uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x248a07);RECOMP_ICALL_SAFE(target,saved);
        g_ecx=MEM32(g_esi+0x44);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x248a13);sub_00245640();
        g_ecx=MEM32(g_esi+0x44);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x248a1c);sub_00245670();
        if(g_edi){g_edx=MEM32(g_edi);g_ecx=g_edi;saved=g_esp;target=MEM32(g_edx+8);PUSH32(g_esp,0x248a27);RECOMP_ICALL_SAFE(target,saved);}
        POP32(g_esp,g_edi);
    }POP32(g_esp,g_esi);g_esp+=8;
}
int uc_migration_audio_instance(uint32_t a){
    if(!instance_admit(a))return 0;
    switch(a){
    case 0x249c30:instance_begin();break;
    case 0x24cf10:instance_track_begin();break;
    case 0x248dd0:instance_state();break;
    case 0x248730:g_eax=++MEM32(g_ecx+0x38);g_recomp_zf=g_eax==0;g_esp+=4;break;
    case 0x2487c0:instance_release();break;
    case 0x245670:instance_attach();break;
    case 0x2456a0:instance_detach();break;
    case 0x2489f0:instance_bind_buffer();break;
    case 0x24b050:
        g_eax=MEM32(g_esp+4);g_edx=MEM32(g_esp+8);MEM32(g_ecx+0x24)=g_eax;g_eax=MEM32(g_esp+12);MEM32(g_ecx+0x34)=g_eax;MEM32(g_ecx+0x30)=g_edx;
        if(g_eax){SET_LO8(g_edx,MEM8(g_eax+4));SET_LO8(g_eax,1);if(LO8(g_edx)&1)MEM8(g_ecx+0x49)|=2;}
        else SET_LO8(g_eax,1);g_esp+=16;break;
    default:return 0;
    }return 1;
}
