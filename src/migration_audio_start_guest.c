#include "migration_audio_start_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023F960(void);
extern void sub_0023FCB0(void);
extern void sub_0023FCD0(void);
extern void sub_002408D0(void);
extern void sub_00241880(void);
extern void sub_00243F60(void);
extern void sub_002440A0(void);
extern void sub_002440D0(void);
extern void sub_002474B0(void);
extern void sub_00248600(void);
extern void sub_00248DD0(void);
extern void sub_00248E80(void);
extern void sub_00249480(void);
extern void sub_00249CC0(void);
extern void sub_0024A1A0(void);
extern void sub_0024A260(void);
extern void sub_0024B600(void);
extern void sub_0024B950(void);
extern void sub_0024D190(void);
extern void sub_0024E010(void);
extern void sub_0024E3A0(void);
extern void sub_00253140(void);
extern void sub_002D57FB(void);
static int start_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_START");f=v&&!strcmp(v,"1");}return f;}
static int start_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int start_admit(uint32_t a){
    if(!start_enabled()||!start_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x10200||!start_span(g_esp-512,532)||!start_span(g_ecx,0x6c))return 0;
    if(!((uint64_t)g_ecx+0x6c<=g_esp-512||(uint64_t)g_esp+20<=g_ecx))return 0;
    if(a!=0x24a1a0&&a!=0x24a260&&a!=0x24a420&&a!=0x249480)return 0;
    if(a==0x249480&&!start_span(MEM32(g_esp+4),8))return 0;
    /* ponytail: bounded O(n) admission; broader ownership can remove repeated validation if measured. */
    uint32_t head=g_ecx+40,previous=head,p=MEM32(head);
    for(unsigned n=0;p!=head;n++,p=MEM32(p)){if(n==4096||!start_span(p-20,0x4c)||MEM32(p+4)!=previous)return 0;previous=p;}
    return MEM32(head+4)==previous;
}
static void start_preload(void){
    uint32_t node=g_seh_ebp;g_esp-=44;PUSH32(g_esp,g_edi);g_edi=g_ecx;int success=1;
    if(!(MEM8(g_edi+100)&4)){
        PUSH32(g_esp,g_ebx);g_ebx=g_edi+40;PUSH32(g_esp,node);node=MEM32(g_ebx);PUSH32(g_esp,g_esi);
        while(node!=g_ebx){
            g_eax=MEM32(node+32);MEM8(node+52)|=1;g_esi=node-20;
            if(!g_eax){
                g_eax=MEM32(g_edi+80);int bank=g_eax!=0;PUSH32(g_esp,44);PUSH32(g_esp,0);g_eax=g_esp+24;PUSH32(g_esp,g_eax);PUSH32(g_esp,bank?0x24a1e3:0x24a206);sub_002440A0();
                if(bank){g_ecx=MEM32(g_edi+80);PUSH32(g_esp,4);PUSH32(g_esp,g_ecx);g_edx=g_esp+48;PUSH32(g_esp,g_edx);MEM8(g_esp+45)=4;PUSH32(g_esp,0x24a1f8);sub_002440D0();g_esp+=24;}
                else{g_esp+=12;MEM8(g_esp+20)=3;MEM8(g_esp+21)=4;}g_eax=g_esp+16;
            }else{PUSH32(g_esp,3);PUSH32(g_esp,1);g_ecx=g_esi;PUSH32(g_esp,0x24a224);sub_0024D190();if(!g_eax){success=0;break;}}
            if(!MEM32(g_eax)){PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x24a236);sub_00249CC0();}node=MEM32(node);
        }
        if(success){g_eax=MEM32(g_edi+100);POP32(g_esp,g_esi);POP32(g_esp,node);g_eax|=4;MEM32(g_edi+100)=g_eax;POP32(g_esp,g_ebx);}
        else{POP32(g_esp,g_esi);POP32(g_esp,node);POP32(g_esp,g_ebx);}
    }
    SET_LO8(g_eax,success?1:0);POP32(g_esp,g_edi);g_esp+=44;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void start_tracks(void){
    uint32_t head=g_seh_ebp;g_esp-=52;PUSH32(g_esp,g_ebx);PUSH32(g_esp,head);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_esi=MEM32(g_edi+40);head=g_edi+40;PUSH32(g_esp,0);PUSH32(g_esp,0x249496);sub_00241880();g_esp+=4;g_ecx=g_esp+16;PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x2494a5);sub_0023FCD0();
    g_eax=MEM32(g_esp+72);g_edx=MEM32(g_eax);g_ebx=MEM32(g_esp+16);g_ecx=MEM32(g_esp+20);g_eax=MEM32(g_eax+4);uint64_t sum=(uint64_t)g_ebx+g_edx;g_ebx=(uint32_t)sum;g_ecx+=g_eax+(uint32_t)(sum>>32);MEM32(g_esp+16)=g_ebx;MEM32(g_esp+20)=g_ecx;
    while(g_esi!=head){
        g_eax=MEM32(g_esi+32);g_ebx=g_esi-20;g_esi=MEM32(g_esi);
        if(g_eax){g_ecx=g_esp+76;PUSH32(g_esp,g_ecx);g_edx=g_esp+20;PUSH32(g_esp,g_edx);g_ecx=g_ebx;PUSH32(g_esp,0x2494e3);sub_0024E010();}
        else{
            g_eax=MEM32(g_edi+80);int bank=g_eax!=0;PUSH32(g_esp,44);PUSH32(g_esp,0);g_eax=g_esp+32;PUSH32(g_esp,g_eax);PUSH32(g_esp,bank?0x2494fa:0x249519);sub_002440A0();
            if(bank){g_ecx=MEM32(g_edi+80);PUSH32(g_esp,4);PUSH32(g_esp,g_ecx);g_edx=g_esp+56;PUSH32(g_esp,g_edx);MEM8(g_esp+53)=4;PUSH32(g_esp,0x24950f);sub_002440D0();g_esp+=24;}
            else{g_esp+=12;MEM8(g_esp+28)=3;MEM8(g_esp+29)=4;}
            g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_edx=g_esp+20;PUSH32(g_esp,g_edx);g_ecx=g_ebx;PUSH32(g_esp,0x249537);sub_0024E3A0();
        }
    }POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,head);SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_esp+=52;g_recomp_zf=g_esp==0;g_esp+=12;
}
/* Publication retains the title's flags, initial event ownership and 64-bit
 * clock. Allocators, event construction and queue submission keep their guest
 * identities; no replacement clock or backend graph is introduced here. */
static void start_publish(void){
    g_eax=MEM32(0);
    {uint32_t saved=g_esp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e81b);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);uint32_t target=MEM32(g_eax+24);PUSH32(g_esp,0x24a280);RECOMP_ICALL_SAFE(target,saved);}
    int ready=(int32_t)g_eax<7;
    if(ready){g_edx=MEM32(g_esi+100);g_ecx=MEM32(g_esp+28);g_edx|=g_ecx;g_ecx=g_esi;MEM32(g_esi+100)=g_edx;PUSH32(g_esp,0x24a29c);sub_0024A1A0();ready=LO8(g_eax)!=0;}
    if(ready){
        g_eax=MEM32(g_esi+40);g_ecx=g_esi+40;if(g_eax!=g_ecx){g_edx=0xfffe;do{MEM16(g_eax+52)&=LO16(g_edx);g_eax=MEM32(g_eax);}while(g_eax!=g_ecx);}
        g_eax=MEM32(g_esi+100);
        if(HI8(g_eax)&0x20){g_edx=MEM32(g_esi);g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_edx+24);PUSH32(g_esp,0x24a2cc);RECOMP_ICALL_SAFE(target,saved);if(g_eax!=3){PUSH32(g_esp,0x2000);g_ecx=g_esi;PUSH32(g_esp,0x24a2dd);sub_00248E80();}}
        else if(g_eax&0x18000){g_eax=MEM32(g_esi+104);if(g_eax!=2&&((int32_t)g_eax<=6||(int32_t)g_eax>8))MEM32(g_esi+104)=2;}
        int published=(MEM32(g_esi+100)&0x40000)!=0;PUSH32(g_esp,g_ebx);
        if(!published){
            g_ecx=MEM32(g_esi+60);int bank=g_ecx!=0;
            if(bank){g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+32);PUSH32(g_esp,0x24a31d);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fed94);bank=g_ecx==MEM32(g_eax);}
            if(bank){g_ecx=MEM32(g_esi+60);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed94);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x24a339);RECOMP_ICALL_SAFE(target,saved);bank=g_eax!=0;}
            if(bank){
                g_ecx=MEM32(g_esi+84);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,1);g_ecx=g_eax;PUSH32(g_esp,0x24a34f);sub_0024B950();g_edi=g_eax;
                if(g_edi){
                    if(MEM8(g_edi+44)&1){
                        PUSH32(g_esp,60);PUSH32(g_esp,0x24a366);sub_002D57FB();g_esp+=4;MEM32(g_esp+36)=g_eax;MEM32(g_esp+28)=0;
                        if(g_eax){PUSH32(g_esp,g_edi);g_ecx=g_eax;PUSH32(g_esp,0x24a381);sub_00253140();MEM32(g_esp+28)=0xffffffffu;g_edi=g_eax;}
                        else{g_eax=0;MEM32(g_esp+28)=0xffffffffu;g_edi=g_eax;}
                    }else{g_edx=g_edi+4;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24a3a4);sub_00243F60();g_esp+=4;}
                    PUSH32(g_esp,0);PUSH32(g_esp,0x24a3ae);sub_00241880();g_ebx=g_eax;g_esp+=4;g_eax=g_esp+12;PUSH32(g_esp,g_eax);g_ecx=g_ebx;PUSH32(g_esp,0x24a3bf);sub_0023FCB0();g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x24a3cb);sub_00248600();PUSH32(g_esp,g_edi);g_ecx=g_ebx;PUSH32(g_esp,0x24a3d3);sub_002408D0();g_ecx=g_edi;PUSH32(g_esp,0x24a3da);sub_0023F960();
                }POP32(g_esp,g_edi);
            }MEM32(g_esi+100)|=0x40000;
        }
        MEM32(g_esi+100)&=~4u;g_edx=MEM32(0x483c50);PUSH32(g_esp,g_edx);g_eax=g_esp+40;PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x24a3f9);sub_00249480();POP32(g_esp,g_ebx);POP32(g_esp,g_esi);g_ecx=MEM32(g_esp+8);
    }else{g_ecx=MEM32(g_esp+12);SET_LO8(g_eax,0);POP32(g_esp,g_esi);}
    MEM32(0)=g_ecx;g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=16;
}
static void start_instance(void){
    g_esp-=8;
    {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);g_edi=2;uint32_t target=MEM32(g_eax+24);PUSH32(g_esp,0x24a432);RECOMP_ICALL_SAFE(target,saved);}
    g_eax=MEM32(g_esi+72);g_ebx=MEM32(g_esp+24);if(g_eax){if((int8_t)HI8(g_ebx)<0)g_edi=5;else if(g_ebx&0x10000)g_edi=4;}
    g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x24a462);sub_00248DD0();int accepted=LO8(g_eax)!=0;
    if(accepted){
        MEM32(g_esi+100)=g_ebx;
        if(g_edi==2){PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x24a47c);sub_0024A260();}
        else if(g_edi==4){
            if(MEM32(g_esi+84)==0xffffffffu)accepted=0;
            else{
                g_ecx=MEM32(g_esi+60);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed94);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x24a49e);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+84);PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x24a4a9);sub_0024B600();g_eax=g_eax?MEM16(g_eax):0;g_ecx=MEM32(g_esi+72);g_edx=g_esp+12;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24a4c2);sub_002474B0();g_eax=MEM32(g_esp+16);g_ecx=MEM32(g_esp+12);g_edx=MEM32(g_esi+100);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,0x24a4d7);sub_0024A260();
            }
        }else if(g_edi==5)SET_LO8(g_eax,1);else accepted=0;
    }
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);if(!accepted)SET_LO8(g_eax,0);POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_audio_start(uint32_t a){if(!start_admit(a))return 0;switch(a){case 0x24a1a0:start_preload();break;case 0x249480:start_tracks();break;case 0x24a260:start_publish();break;default:start_instance();break;}return 1;}
