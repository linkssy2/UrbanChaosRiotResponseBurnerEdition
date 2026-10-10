#include "migration_audio_transition_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0024A110(void);
extern void sub_0024B600(void);
extern void sub_0024B620(void);
extern void sub_00241880(void);
extern void sub_0023FCB0(void);
extern void sub_002440A0(void);
extern void sub_0024E3A0(void);
static int transition_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_TRANSITION");f=v&&!strcmp(v,"1");}return f;}
static int transition_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int transition_safe(uint32_t p,uint32_t n){return transition_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int transition_above(float a,float b){double x=a,y=b;return x==x&&y==y&&x>y;}
static int transition_admit(uint32_t a){
    if((a!=0x248e80&&a!=0x2476a0)||!transition_enabled()||!transition_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!transition_span(g_esp-4096,4108))return 0;
    if(a==0x2476a0)return transition_safe(g_ecx,32)&&transition_safe(MEM32(g_esp+4),12);
    if(!transition_safe(g_ecx,108))return 0;
    if((MEM32(g_esp+4)&0x20000)||MEM32(g_ecx+104)==0||MEM32(g_ecx+104)==7||MEM32(g_ecx+104)==8)return 1;
    uint32_t bank=MEM32(g_ecx+60);if(!transition_safe(bank,4)||!transition_span(MEM32(bank),4))return 0;
    uint32_t head=g_ecx+40,node=MEM32(head),previous=head;
    /* ponytail: O(n) admission; replace with trusted list generations only if measured. */
    for(unsigned n=0;node!=head;n++){if(n>=4096||node<20||!transition_safe(node-20,104)||MEM32(node+4)!=previous)return 0;previous=node;node=MEM32(node);}
    return MEM32(head+4)==previous;
}
static void transition_initialize(void){
    g_eax=MEM32(g_esp+4);g_xmm1=XMM_ZERO();g_xmm2=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi+8)=0;g_xmm0=XMM_SCALAR(MEMF(g_eax+8));
    if(transition_above(g_xmm1.f[0],g_xmm0.f[0]))g_xmm0=g_xmm1;else if(transition_above(g_xmm0.f[0],g_xmm2.f[0]))g_xmm0=g_xmm2;
    g_xmm1=XMM_SCALAR(MEMF(0x44bf80));MEMF(g_esi+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+4));
    if(transition_above(g_xmm1.f[0],g_xmm0.f[0]))g_xmm0=g_xmm1;else if(transition_above(g_xmm0.f[0],g_xmm2.f[0]))g_xmm0=g_xmm2;
    MEMF(g_esi+16)=g_xmm0.f[0];SET_LO16(g_ecx,MEM16(g_eax));MEM16(g_esi+12)=LO16(g_ecx);SET_LO16(g_edx,MEM16(g_eax+2));int trigger=LO16(g_edx)!=0;g_eax=MEM32(g_esp+12);MEM16(g_esi+14)=LO16(g_edx);MEM32(g_esi+24)=g_eax;
    if(trigger){g_ecx=MEM32(g_esi+28);if(g_ecx){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x247721);sub_0024A110();}MEM32(g_esi+8)++;}POP32(g_esp,g_esi);g_esp+=12;
}
static void transition_begin(void){
    uint32_t frame=g_seh_ebp;g_eax=MEM32(g_esp+4);g_esp-=56;PUSH32(g_esp,g_esi);g_esi=g_ecx;
    if(g_eax&0x20000)MEM32(g_esi+100)&=0xffffefffu;
    else{
        g_edx=MEM32(g_esi+100);g_eax|=0x1000;g_edx|=g_eax;g_eax=MEM32(g_esi+104);MEM32(g_esi+100)=g_edx;
        if(g_eax&&((int32_t)g_eax<=6||(int32_t)g_eax>8)){
            g_ecx=MEM32(g_esi+60);uint32_t saved=g_esp;PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);PUSH32(g_esp,0);MEM32(g_esi+104)=3;g_eax=MEM32(g_ecx);PUSH32(g_esp,0x4fed94);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x248edc);RECOMP_ICALL_SAFE(target,saved);
            g_ecx=MEM32(g_esi+84);g_edi=g_eax;PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x248ee9);sub_0024B600();g_edx=MEM16(g_eax+12);PUSH32(g_esp,g_edx);g_ecx=g_edi;PUSH32(g_esp,0x248ef5);sub_0024B620();frame=g_eax;
            if(!frame){g_eax=MEM32(g_esi+100);POP32(g_esp,g_edi);g_eax&=0xffffefffu;POP32(g_esp,frame);MEM32(g_esi+100)=g_eax;}
            else{
                PUSH32(g_esp,g_ebx);PUSH32(g_esp,0);PUSH32(g_esp,0x248f17);sub_00241880();g_esp+=4;g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x248f26);sub_0023FCB0();PUSH32(g_esp,44);g_edx=g_esp+32;PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x248f34);sub_002440A0();
                g_ebx=MEM32(g_esi+40);g_eax=g_esi+40;g_esp+=12;MEM8(g_esp+32)=0;MEM8(g_esp+33)=10;MEM32(g_esp+40)=100;
                if(g_ebx!=g_eax){
                    g_eax=MEM32(g_esp+76);g_eax&=0x4000;MEM32(g_esp+16)=g_eax;
                    for(;;){
                        g_ecx=g_ebx-20;g_ebx=MEM32(g_ebx);int publish=0,failed=0;
                        if(g_eax){
                            g_eax=MEM16(frame+8);g_edx=0;g_edx=g_eax%MEM32(g_esp+40);g_eax/=MEM32(g_esp+40);g_edi=g_eax;
                            if(!g_edi)failed=1;
                            else{g_eax=0xffff8000u;g_edx=0xffffffffu;g_eax=(uint32_t)(-32768/(int32_t)g_edi);g_edx=g_esp+20;MEM16(g_esp+44)=LO16(g_eax);g_eax=g_esp+28;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);publish=1;}
                        }else{
                            g_eax=MEM32(g_esp+76);
                            if(HI8(g_eax)&0x20){
                                g_eax=MEM16(frame+2);g_edx=0;g_edx=g_eax%MEM32(g_esp+40);g_eax/=MEM32(g_esp+40);g_edi=g_eax;
                                if(!g_edi)failed=1;
                                else{g_eax=(uint32_t)(32767/(int32_t)g_edi);g_edx=g_esp+28;PUSH32(g_esp,g_edx);MEM16(g_esp+48)=LO16(g_eax);SET_LO16(g_eax,MEM16(0x4839b0));MEM16(g_esi+96)=LO16(g_eax);g_eax=g_esp+24;PUSH32(g_esp,g_eax);publish=1;}
                            }
                        }
                        if(failed){MEM32(g_esi+100)&=0xffffefffu;break;}
                        if(publish){PUSH32(g_esp,0x248fe1);sub_0024E3A0();}
                        g_eax=g_esi+40;if(g_ebx==g_eax)break;g_eax=MEM32(g_esp+16);
                    }
                }
                POP32(g_esp,g_ebx);POP32(g_esp,g_edi);POP32(g_esp,frame);
            }
        }
    }POP32(g_esp,g_esi);g_esp+=56;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_audio_transition(uint32_t a){if(!transition_admit(a))return 0;if(a==0x2476a0)transition_initialize();else transition_begin();return 1;}
