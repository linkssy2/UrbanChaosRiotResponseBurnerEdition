#include "migration_audio_pending_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_00243F40(void);
extern void sub_00243F60(void);
extern void sub_00245A00(void);
extern void sub_00245AD0(void);
extern void sub_00246600(void);
extern void sub_00246C10(void);
extern void sub_002487F0(void);
extern void sub_002489C0(void);
extern void sub_002489F0(void);
extern void sub_00248D10(void);
extern void sub_002D57FB(void);
extern void sub_002D5CCD(void);
static int pending_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_PENDING");f=v&&!strcmp(v,"1");}return f;}
static int pending_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int pending_ring(uint32_t head,int buffers){
    /* ponytail: bounded O(n) validation; replace only after live cost evidence. */
    uint32_t p=MEM32(head),previous=head;
    for(unsigned n=0;p!=head;n++,p=MEM32(p)){
        if(n==4096||!pending_span(p,0x34)||MEM32(p+4)!=previous||(buffers&&!pending_span(p-32,0x60)))return 0;
        previous=p;
    }return MEM32(head+4)==previous;
}
static int pending_admit(uint32_t a){
    if(!pending_enabled()||!pending_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x10100||!pending_span(g_esp-256,280))return 0;
    uint32_t size=a==0x245a00?0x34:0x45;
    if(!pending_span(g_ecx,size)||!((uint64_t)g_ecx+size<=g_esp-256||(uint64_t)g_esp+24<=g_ecx))return 0;
    if(a==0x245a00||a==0x245bb0)return pending_span(MEM32(g_esp+16),25);
    if(a!=0x245c30&&a!=0x246c10&&a!=0x246d90&&a!=0x245cd0)return 0;
    if((a==0x246d90||a==0x245cd0)&&!pending_span(MEM32(g_esp+4),12))return 0;
    if(a!=0x245cd0&&!pending_ring(g_ecx+20,0))return 0;
    return a==0x246c10||pending_ring(g_ecx+12,1);
}
static void pending_construct(void){
    g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi+8)=g_eax;g_eax=MEM32(g_esp+20);
    g_ecx=MEM32(g_eax);MEM32(g_esi+12)=g_ecx;g_edx=MEM32(g_eax+4);MEM32(g_esi+16)=g_edx;g_ecx=MEM32(g_eax+8);MEM32(g_esi+20)=g_ecx;g_edx=MEM32(g_eax+12);MEM32(g_esi+24)=g_edx;g_ecx=MEM32(g_eax+16);MEM32(g_esi+28)=g_ecx;g_edx=MEM32(g_eax+20);
    SET_LO8(g_ecx,MEM8(g_esp+24));MEM32(g_esi+32)=g_edx;SET_LO8(g_eax,MEM8(g_eax+24));g_edx=MEM32(g_esp+16);MEM8(g_esi+36)=LO8(g_eax);MEM8(g_esi+40)=LO8(g_ecx);g_ecx=MEM32(g_esp+12);MEM32(g_esi+44)=g_ecx;MEM32(g_esi+48)=g_edx;MEM32(g_esi+4)=g_esi;MEM32(g_esi)=g_esi;
    if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+4);PUSH32(g_esp,0x245a5a);RECOMP_ICALL_SAFE(target,saved);}
    g_ecx=MEM32(g_esi+8);int empty=g_ecx==0;
    if(!empty){g_edx=MEM32(g_esi+48);g_eax=MEM32(g_esi+44);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x245a6e);sub_002487F0();}
    g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=empty;g_esp+=24;
}
static void pending_publish(void){
    PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e6db);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x34);g_esi=g_ecx;PUSH32(g_esp,0x245bd0);sub_002D57FB();g_esp+=4;MEM32(g_esp+4)=g_eax;MEM32(g_esp+16)=0;
    if(g_eax){g_ecx=MEM32(g_esp+40);g_edx=MEM32(g_esp+36);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+36);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+36);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+36);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x245c03);sub_00245A00();}else g_eax=0;
    PUSH32(g_esp,g_eax);g_esi+=20;PUSH32(g_esp,g_esi);MEM32(g_esp+24)=0xffffffffu;PUSH32(g_esp,0x245c19);sub_00243F40();g_ecx=MEM32(g_esp+16);g_esp+=8;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=24;
}
static void pending_cancel(void){
    uint32_t head=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+12);PUSH32(g_esp,head);head=g_ecx+20;PUSH32(g_esp,g_esi);g_esi=MEM32(head);PUSH32(g_esp,g_edi);MEM32(g_esp+16)=g_ecx;
    int populated=g_esi!=head;
    while(g_esi!=head){
        g_edi=MEM32(g_esi);int match=!g_ebx;
        if(!match&&MEM32(g_esi+44)==g_ebx){g_eax=MEM32(g_esp+28);match=MEM32(g_esi+48)==g_eax;}
        if(match){
            g_eax=MEM32(g_esi+8);
            if(g_eax){g_ecx=g_eax;g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x245c6b);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);saved=g_esp;target=MEM32(g_eax+8);PUSH32(g_esp,0x245c73);RECOMP_ICALL_SAFE(target,saved);}
            g_eax=MEM32(g_esi+44);if(g_eax){g_ecx=g_eax;g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x245c81);RECOMP_ICALL_SAFE(target,saved);}
            PUSH32(g_esp,g_esi);PUSH32(g_esp,0x245c87);sub_00243F60();PUSH32(g_esp,g_esi);PUSH32(g_esp,0x245c8d);sub_002D5CCD();g_esp+=8;
        }g_esi=g_edi;
    }
    if(populated)g_ecx=MEM32(g_esp+16);g_esi=MEM32(g_ecx+12);g_edi=g_ecx+12;
    while(g_esi!=g_edi){g_edx=MEM32(g_esp+28);g_eax=MEM32(g_esi-32);g_ecx=g_esi-32;uint32_t saved=g_esp;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0x245cb3);RECOMP_ICALL_SAFE(target,saved);g_esi=MEM32(g_esi);}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,head);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=12;
}
/* Pending distance is sampled once per owner update. Each callback may replace
 * the instance; reload the guest field at the same ownership transitions. */
static void pending_update(void){
    uint32_t group=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_eax=MEM32(g_ebx+20);PUSH32(g_esp,g_edi);g_edi=g_ebx+20;
    if(g_eax!=g_edi){
        PUSH32(g_esp,g_esi);PUSH32(g_esp,0);PUSH32(g_esp,0x246c2b);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;
        {uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fec10);g_ecx=g_eax;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x246c3b);RECOMP_ICALL_SAFE(target,saved);}
        g_edx=MEM32(g_eax);g_ecx=g_eax;
        {uint32_t saved=g_esp,target=MEM32(g_edx+40);PUSH32(g_esp,0x246c42);RECOMP_ICALL_SAFE(target,saved);}
        g_xmm2=XMM_SCALAR(MEMF(g_ebx+56));g_xmm2.f[0]-=MEMF(g_eax+8);g_xmm1=XMM_SCALAR(MEMF(g_ebx+52));g_xmm1.f[0]-=MEMF(g_eax+4);g_xmm0=XMM_SCALAR(MEMF(g_ebx+48));g_xmm0.f[0]-=MEMF(g_eax);g_esi=MEM32(g_edi);
        int populated=g_esi!=g_edi;g_xmm3=g_xmm2;g_xmm3.f[0]*=g_xmm2.f[0];g_xmm2=g_xmm1;g_xmm2.f[0]*=g_xmm1.f[0];g_xmm1=g_xmm0;g_xmm3.f[0]+=g_xmm2.f[0];g_xmm1.f[0]*=g_xmm0.f[0];g_xmm3.f[0]+=g_xmm1.f[0];MEMF(g_esp+12)=g_xmm3.f[0];
        if(populated){
            PUSH32(g_esp,group);
            for(;;){
                g_xmm0=XMM_SCALAR(MEMF(g_esi+20));SET_LO8(g_eax,MEM8(g_esi+40));g_xmm1=g_xmm0;g_xmm1.f[0]*=g_xmm0.f[0];double radius=g_xmm1.f[0],distance=g_xmm3.f[0];
                if(radius==radius&&distance==distance&&radius>distance){
                    if(!LO8(g_eax)){
                        g_edi=g_esi+12;PUSH32(g_esp,g_edi);g_ecx=g_ebx;PUSH32(g_esp,0x246cbc);sub_00245AD0();
                        if(!g_eax){PUSH32(g_esp,g_edi);g_ecx=g_ebx;PUSH32(g_esp,0x246cc8);sub_00246600();}
                        if(g_eax){g_ecx=MEM32(g_esi+8);if(g_ecx){
                            PUSH32(g_esp,g_eax);PUSH32(g_esp,0x246ce1);sub_002489F0();g_edx=MEM32(g_esi+48);g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);
                            {uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+44);PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x246cf3);RECOMP_ICALL_SAFE(target,saved);}
                            if(LO8(g_eax)){g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);uint32_t target=MEM32(g_eax+16);PUSH32(g_esp,0x246d05);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+8);PUSH32(g_esp,0x246d0d);sub_00248D10();MEM8(g_esi+40)=1;}
                        }}
                    }
                }else if(LO8(g_eax)){
                    g_ecx=MEM32(g_esi+8);g_edx=MEM32(g_ecx);
                    {uint32_t saved=g_esp,target=MEM32(g_edx+24);PUSH32(g_esp,0x246d1f);RECOMP_ICALL_SAFE(target,saved);}
                    if((int32_t)g_eax<7){
                        g_ecx=MEM32(g_esi+8);
                        if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0x246d32);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+8);g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+8);PUSH32(g_esp,0x246d3a);RECOMP_ICALL_SAFE(target,saved);}
                        group=MEM32(g_esi+12);PUSH32(g_esp,0);PUSH32(g_esp,0x246d44);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;
                        {uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fec20);g_ecx=g_eax;uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x246d55);RECOMP_ICALL_SAFE(target,saved);}
                        g_edi=g_eax;if(g_edi){PUSH32(g_esp,group);g_ecx=g_edi;PUSH32(g_esp,0x246d63);sub_002489C0();}MEM32(g_esi+8)=g_edi;
                        if(g_edi){g_eax=MEM32(g_esi+48);g_ecx=MEM32(g_esi+44);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x246d79);sub_002487F0();}MEM8(g_esi+40)=0;
                    }
                }
                g_esi=MEM32(g_esi);g_eax=g_ebx+20;if(g_esi==g_eax)break;g_xmm3=XMM_SCALAR(MEMF(g_esp+16));
            }POP32(g_esp,group);
        }POP32(g_esp,g_esi);
    }POP32(g_esp,g_edi);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_esp+=4;
}
static void pending_position(void){
    uint32_t point=g_ebp;PUSH32(g_esp,point);point=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=point;g_edx=MEM32(g_ecx);g_eax=g_esi+48;MEM32(g_eax)=g_edx;g_edx=MEM32(g_ecx+4);MEM32(g_eax+4)=g_edx;g_ecx=MEM32(g_ecx+8);MEM32(g_eax+8)=g_ecx;PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x246db5);sub_00246C10();g_edi=g_esi+12;g_esi=MEM32(g_edi);
    while(g_esi!=g_edi){
        if(MEM8(g_esi+36)&2){g_eax=MEM32(g_esi+32);g_eax=g_eax?MEM32(g_eax+16):2;if(g_eax==0||g_eax==3||g_eax==4){g_edx=MEM32(g_esi-28);g_ecx=g_esi-28;uint32_t saved=g_esp;PUSH32(g_esp,point);PUSH32(g_esp,5);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x246df1);RECOMP_ICALL_SAFE(target,saved);}}
        g_esi=MEM32(g_esi);
    }POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,point);g_recomp_zf=1;g_esp+=8;
}
static void pending_velocity(void){
    uint32_t head=g_ebp;PUSH32(g_esp,head);head=g_ecx+12;PUSH32(g_esp,g_esi);g_esi=MEM32(head);
    if(g_esi!=head){PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);
        while(g_esi!=head){if(MEM8(g_esi+36)&2){g_eax=MEM32(g_esi+32);g_eax=g_eax?MEM32(g_eax+16):2;if(g_eax==0||g_eax==3||g_eax==4){g_eax=MEM32(g_esi-28);g_ecx=g_esi-28;uint32_t saved=g_esp;PUSH32(g_esp,g_edi);PUSH32(g_esp,6);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x245d12);RECOMP_ICALL_SAFE(target,saved);}}g_esi=MEM32(g_esi);}POP32(g_esp,g_edi);
    }POP32(g_esp,g_esi);POP32(g_esp,head);g_recomp_zf=1;g_esp+=8;
}
int uc_migration_audio_pending(uint32_t a){if(!pending_admit(a))return 0;switch(a){case 0x245a00:pending_construct();break;case 0x245bb0:pending_publish();break;case 0x245c30:pending_cancel();break;case 0x246c10:pending_update();break;case 0x246d90:pending_position();break;default:pending_velocity();break;}return 1;}
