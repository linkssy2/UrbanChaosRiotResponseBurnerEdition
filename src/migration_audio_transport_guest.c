#include "migration_audio_transport_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_00248DD0(void);
extern void sub_0024B2B0(void);
extern void sub_0024B310(void);
extern void sub_0023FF50(void);
extern void sub_0023FF00(void);
extern void sub_0023FCB0(void);
extern void sub_00245420(void);
extern void sub_00245430(void);
extern void sub_00245450(void);
extern void sub_0024E3A0(void);
extern void sub_0024A6F0(void);

static int transport_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_TRANSPORT");f=v&&!strcmp(v,"1");}return f;}
static int transport_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int transport_object(uint32_t p,uint32_t n){return transport_span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+20<=p);}
static int transport_admit(uint32_t a){
    if(!transport_enabled()||!transport_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x10100||!transport_span(g_esp-256,276)||!transport_object(g_ecx,0x70))return 0;
    if(a==0x24b2b0||a==0x24b310){uint32_t voice=MEM32(g_ecx+0x2c);return !voice||transport_object(voice,0x54);}
    if(a!=0x249550&&a!=0x24a900)return 0;
    uint32_t head=g_ecx+0x28,node=head;unsigned count=0;
    /* ponytail: bounded preflight walks the track ring; replace with a qualified
     * construction invariant if measured guard cost outweighs this owner. */
    do{if(++count>4096||!transport_object(node,8))return 0;uint32_t next=MEM32(node),prev=MEM32(node+4);
        if(!transport_object(next,8)||!transport_object(prev,8)||MEM32(next+4)!=node||MEM32(prev)!=node)return 0;
        if(node!=head&&(node<20||!transport_object(node-20,0x68)))return 0;node=next;
    }while(node!=head);
    return a!=0x249550||transport_span(MEM32(g_ecx),0x1c);
}
static void transport_pause_instance(void){
    PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;int change;
    if(LO8(g_ebx)){
        g_eax=MEM32(g_esi);g_edi=6;
        {uint32_t saved=g_esp,target=MEM32(g_eax+24);PUSH32(g_esp,0x249567);RECOMP_ICALL_SAFE(target,saved);}
        change=g_eax!=g_edi;
    }else{
        g_edx=MEM32(g_esi);g_edi=MEM32(g_esi+0x6c);g_ecx=g_esi;
        {uint32_t saved=g_esp,target=MEM32(g_edx+24);PUSH32(g_esp,0x24957d);RECOMP_ICALL_SAFE(target,saved);}
        change=g_eax==6;
    }
    if(change){
        g_eax=g_esi+0x6c;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x24958e);sub_00248DD0();change=LO8(g_eax)!=0;
        if(change){g_edi=g_esi+0x28;g_esi=MEM32(g_edi);
            while(g_esi!=g_edi){PUSH32(g_esp,g_ebx);g_ecx=g_esi-20;PUSH32(g_esp,0x2495a9);sub_0024B2B0();g_esi=MEM32(g_esi);}
        }
    }
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);SET_LO8(g_eax,change);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=8;
}
static void transport_pause_track(void){
    PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_esi=g_ecx;
    PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0);PUSH32(g_esp,0x24b2c1);sub_00241880();g_esp+=4;g_ecx=g_eax;PUSH32(g_esp,0x24b2cb);sub_0023FF50();
    g_ecx=MEM32(g_esi+0x2c);
    if(LO8(g_ebx))MEM8(g_esi+0x48)|=1;else MEM8(g_esi+0x48)&=0xfe;
    SET_LO16(g_eax,MEM16(g_esi+0x48));
    if(g_ecx&&(LO8(g_eax)&0x20)){
        if(LO8(g_ebx)){PUSH32(g_esp,0x24b2e7);sub_00245420();g_recomp_zf=0;}
        else{PUSH32(g_esp,0x24b301);sub_00245430();}
    }
    POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=8;
}
static void transport_stop_track(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_esi);PUSH32(g_esp,0);g_esi=g_ecx;PUSH32(g_esp,0x24b31a);sub_00241880();g_esp+=4;
    PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x24b325);sub_0023FF00();g_ecx=MEM32(g_esi+0x2c);
    g_eax=(MEM16(g_esi+0x48)&0xffdfu)|4u;MEM16(g_esi+0x48)=LO16(g_eax);POP32(g_esp,g_esi);
    if(g_ecx){g_seh_ebp=frame;sub_00245450();}
    else{g_recomp_zf=1;g_esp+=8;}
}
static void transport_stop_instance(void){
    uint32_t frame=g_seh_ebp,owner=g_ecx;g_eax=MEM32(g_esp+12);g_esp-=52;PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=(g_eax&0x10000)?4:7;
    PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ecx=owner;PUSH32(g_esp,0x24a926);sub_00248DD0();int accepted=LO8(g_eax)!=0;
    if(accepted){
        g_ecx=MEM32(owner+0x64);g_eax=MEM32(g_esp+72);g_ecx|=g_eax;PUSH32(g_esp,g_edi);MEM32(owner+0x64)=g_ecx;
        if(g_esi==4){
            PUSH32(g_esp,0);PUSH32(g_esp,0x24a947);sub_00241880();g_esp+=4;g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x24a956);sub_0023FCB0();
            g_eax=MEM32(0x483800);g_ecx=MEM32(g_esp+68);g_edi=MEM32(g_esp+72);g_esi=MEM32(g_esp+12);g_edx=(int32_t)g_eax<0?0xffffffffu:0;
            uint64_t offset=(uint64_t)g_eax+g_ecx;g_eax=(uint32_t)offset;g_ecx=MEM32(g_esp+16);g_edx+=g_edi+(uint32_t)(offset>>32);
            uint64_t low=(uint64_t)g_esi+g_eax;g_esi=(uint32_t)low;g_edi=owner+0x28;MEM32(g_esp+12)=g_esi;g_esi=MEM32(g_edi);g_ecx+=g_edx+(uint32_t)(low>>32);
            MEM32(g_esp+16)=g_ecx;MEM8(g_esp+24)=4;MEM8(g_esp+25)=4;
            while(g_esi!=g_edi){g_edx=g_esp+20;PUSH32(g_esp,g_edx);g_eax=g_esp+16;PUSH32(g_esp,g_eax);g_ecx=g_esi-20;PUSH32(g_esp,0x24a9a2);sub_0024E3A0();g_esi=MEM32(g_esi);}
        }else{
            g_esi=MEM32(owner+0x28);g_edi=owner+0x28;
            while(g_esi!=g_edi){PUSH32(g_esp,0x40);g_ecx=g_esi-20;PUSH32(g_esp,0x24a9be);sub_0024B310();g_esi=MEM32(g_esi);}
        }
        int update=MEM8(g_esp+76)&2;POP32(g_esp,g_edi);
        if(update){PUSH32(g_esp,0xffffffffu);g_ecx=owner;PUSH32(g_esp,0x24a9d5);sub_0024A6F0();}
    }
    POP32(g_esp,g_esi);SET_LO8(g_eax,accepted);POP32(g_esp,frame);g_esp+=52;g_recomp_zf=g_esp==0;g_esp+=16;
}
int uc_migration_audio_transport(uint32_t a){
    if(!transport_admit(a))return 0;
    switch(a){case 0x249550:transport_pause_instance();break;case 0x24b2b0:transport_pause_track();break;case 0x24b310:transport_stop_track();break;case 0x24a900:transport_stop_instance();break;default:return 0;}
    return 1;
}
