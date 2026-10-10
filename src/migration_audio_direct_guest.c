#include "migration_audio_direct_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_00245AA0(void);
extern void sub_00245AD0(void);
extern void sub_00246600(void);
extern void sub_00245B80(void);
extern void sub_00245BB0(void);
extern void sub_002471A0(void);
extern void sub_0023F950(void);
extern void sub_002489C0(void);
extern void sub_002489F0(void);
extern void sub_00248D10(void);
extern void sub_002D6780(void);
static int direct_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_DIRECT");f=v&&!strcmp(v,"1");}return f;}
static int direct_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int direct_ring(uint32_t head){
    /* ponytail: bounded preflight costs O(n); fuse with broader ownership only if measured hot. */
    uint32_t p=MEM32(head);for(unsigned n=0;p!=head;n++,p=MEM32(p))if(n==4096||!direct_span(p,0x30))return 0;return 1;
}
static int direct_admit(uint32_t a){
    if(!direct_enabled()||!direct_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x10100||!direct_span(g_esp-256,268))return 0;
    if(a==0x245b80)return 1;
    uint32_t size=a==0x2489c0?0x4c:0x45;
    if(!direct_span(g_ecx,size)||!((uint64_t)g_ecx+size<=g_esp-256||(uint64_t)g_esp+12<=g_ecx))return 0;
    if(a==0x245aa0)return direct_ring(g_ecx+20);
    if(a==0x2489c0)return 1;
    if(a==0x2466a0){uint32_t resource=MEM32(g_esp+4);return !MEM32(g_ecx+60)||(direct_span(resource,4)&&direct_span(MEM32(resource),0x24)&&direct_ring(g_ecx+20));}
    return 0;
}
static void direct_duplicate(void){
    g_eax=MEM32(g_ecx+20);g_ecx+=20;
    if(g_eax!=g_ecx){g_edx=MEM32(g_esp+4);while(g_eax!=g_ecx){if(MEM32(g_eax+44)==g_edx){SET_LO8(g_eax,1);g_recomp_zf=1;g_esp+=8;return;}g_eax=MEM32(g_eax);}}
    SET_LO8(g_eax,0);g_recomp_zf=1;g_esp+=8;
}
static void direct_group(void){
    PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_esi=g_ecx;
    if(g_edi){g_ecx=MEM32(g_esi+72);if(g_ecx){PUSH32(g_esp,0x2489d8);sub_002471A0();MEM32(g_esi+72)=0;}
        g_ecx=g_edi;MEM32(g_esi+72)=g_edi;PUSH32(g_esp,0x2489e9);sub_0023F950();}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;
}
static void direct_factory(void){
    PUSH32(g_esp,g_esi);PUSH32(g_esp,0);PUSH32(g_esp,0x245b88);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;
    {uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fec20);g_ecx=g_eax;uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x245b99);RECOMP_ICALL_SAFE(target,saved);}
    g_esi=g_eax;int empty=g_esi==0;
    if(!empty){g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x245bab);sub_002489C0();}
    g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=empty;g_esp+=4;
}
/* The provider still owns resident/stream data and the factory still owns guest
 * allocations. This owner selects the original request path, preserves pending
 * requests and publishes through the existing instance/track graph. */
static void direct_request(void){
    uint32_t resource=g_seh_ebp;int saved_resource=0,saved_buffer=0,saved_instance=0;
    g_esp-=28;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+60);
    if(!g_eax)g_eax=0;
    else{
        {uint32_t saved=g_esp;PUSH32(g_esp,resource);saved_resource=1;resource=MEM32(g_esp+40);g_eax=MEM32(resource);g_ecx=resource;uint32_t target=MEM32(g_eax+32);PUSH32(g_esp,0x2466c3);RECOMP_ICALL_SAFE(target,saved);}
        g_ecx=MEM32(0x4fec60);int supported=g_ecx==MEM32(g_eax);
        if(!supported){g_edx=MEM32(resource);g_ecx=resource;uint32_t saved=g_esp,target=MEM32(g_edx+32);PUSH32(g_esp,0x2466d5);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fec64);supported=g_ecx==MEM32(g_eax);}
        if(supported){PUSH32(g_esp,resource);g_ecx=g_esi;PUSH32(g_esp,0x2466f1);sub_00245AA0();supported=!LO8(g_eax);}
        if(!supported)g_eax=0;
        else{
            g_edx=MEM32(g_esi+60);g_ecx=MEM32(g_esi+44);g_eax=MEM32(g_esi+40);MEM32(g_esp+8)=g_edx;g_edx=MEM32(g_esi+36);MEM32(g_esp+16)=g_ecx;g_ecx=MEM32(g_esi+60);
            {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);saved_buffer=1;MEM32(g_esp+16)=g_eax;MEM32(g_esp+32)=g_edx;g_eax=MEM32(g_ecx);PUSH32(g_esp,0x400);uint32_t target=MEM32(g_eax+16);PUSH32(g_esp,0x24671f);RECOMP_ICALL_SAFE(target,saved);}
            PUSH32(g_esp,0x246724);sub_002D6780();g_ecx=MEM32(g_esi+32);g_edx=g_esp+12;MEM32(g_esp+28)=g_ecx;PUSH32(g_esp,g_edx);g_ecx=g_esi;MEM32(g_esp+28)=g_eax;MEM8(g_esp+40)=0;PUSH32(g_esp,0x246740);sub_00245AD0();g_ebx=g_eax;
            if(!g_ebx){g_eax=g_esp+12;PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x246752);sub_00246600();g_ebx=g_eax;}
            if(g_ebx){
                SET_LO8(g_eax,MEM8(g_ebx+68));PUSH32(g_esp,g_edi);saved_instance=1;g_edi=0;
                if(!(LO8(g_eax)&2)){
                    g_edx=MEM32(g_ebx);uint32_t saved=g_esp;PUSH32(g_esp,resource);g_ecx=g_ebx;uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x246773);RECOMP_ICALL_SAFE(target,saved);g_edi=g_eax;
                }else{
                    g_eax=MEM32(resource);g_ecx=resource;
                    {uint32_t saved=g_esp,target=MEM32(g_eax+32);PUSH32(g_esp,0x246787);RECOMP_ICALL_SAFE(target,saved);}
                    g_ecx=MEM32(0x4fec60);int resident=g_ecx==MEM32(g_eax),queried=resident;g_edx=MEM32(resource);g_ecx=resource;
                    if(!resident){uint32_t saved=g_esp,target=MEM32(g_edx+32);PUSH32(g_esp,0x2467ad);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fec64);queried=g_ecx==MEM32(g_eax);if(queried)g_edx=MEM32(resource);}
                    if(queried){
                        uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,resident?0x4fec60:0x4fec64);if(!resident)g_ecx=resource;uint32_t target=MEM32(g_edx);PUSH32(g_esp,resident?0x24679f:0x2467c5);RECOMP_ICALL_SAFE(target,saved);
                        g_edx=MEM32(g_eax);saved=g_esp;PUSH32(g_esp,0);g_ecx=g_eax;target=MEM32(g_edx+(resident?72:80));PUSH32(g_esp,resident?0x2467a8:0x2467ce);RECOMP_ICALL_SAFE(target,saved);
                        if(g_eax){g_eax=MEM32(g_esp+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2467dc);sub_00245B80();g_esp+=4;g_edi=g_eax;}
                    }
                    PUSH32(g_esp,0);PUSH32(g_esp,0x2467e8);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;
                    {uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fec10);g_ecx=g_eax;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x2467f8);RECOMP_ICALL_SAFE(target,saved);}
                    g_edx=MEM32(g_eax);g_ecx=g_eax;
                    {uint32_t saved=g_esp,target=MEM32(g_edx+40);PUSH32(g_esp,0x2467ff);RECOMP_ICALL_SAFE(target,saved);}
                    g_xmm2=XMM_SCALAR(MEMF(g_esi+56));g_xmm2.f[0]-=MEMF(g_eax+8);g_xmm0=XMM_SCALAR(MEMF(g_esi+48));g_xmm0.f[0]-=MEMF(g_eax);g_xmm1=XMM_SCALAR(MEMF(g_esi+52));g_xmm1.f[0]-=MEMF(g_eax+4);g_xmm3=XMM_SCALAR(MEMF(g_esi+44));
                    g_xmm4=g_xmm2;g_xmm4.f[0]*=g_xmm2.f[0];g_xmm2=g_xmm0;g_xmm2.f[0]*=g_xmm0.f[0];g_xmm0=g_xmm1;g_xmm0.f[0]*=g_xmm1.f[0];g_xmm4.f[0]+=g_xmm2.f[0];g_xmm4.f[0]+=g_xmm0.f[0];g_xmm0=g_xmm3;g_xmm0.f[0]*=g_xmm3.f[0];
                    double radius=g_xmm0.f[0],distance=g_xmm4.f[0];MEM8(g_esp+48)=(radius==radius&&distance==distance&&radius>distance)?1:0;
                    if(g_edi){g_eax=MEM32(g_esp+48);PUSH32(g_esp,g_eax);g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,resource);PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x24686d);sub_00245BB0();}
                    SET_LO8(g_eax,MEM8(g_esp+48));int audible=LO8(g_eax)!=0,failed=0;
                    if(audible){
                        g_esi=0;
                        if(!g_edi){g_edx=MEM32(g_esp+16);PUSH32(g_esp,g_edx);g_esi=1;PUSH32(g_esp,0x24688a);sub_00245B80();g_edi=g_eax;g_esp+=4;}
                        if(g_edi){
                            PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x24689b);sub_002489F0();g_eax=MEM32(g_edi);
                            {uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,resource);g_ecx=g_edi;uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x2468a7);RECOMP_ICALL_SAFE(target,saved);}
                            int prepared=LO8(g_eax)!=0;g_ecx=g_edi;
                            if(!prepared){g_edx=MEM32(g_edi);uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x2468b2);RECOMP_ICALL_SAFE(target,saved);failed=1;}
                            else{g_eax=MEM32(g_edi);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+16);PUSH32(g_esp,0x2468c4);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_edi;PUSH32(g_esp,0x2468cb);sub_00248D10();}
                        }
                    }
                    g_eax=failed?0:g_edi;
                }
            }
        }
    }
    if(saved_instance)POP32(g_esp,g_edi);if(saved_buffer)POP32(g_esp,g_ebx);if(saved_resource)POP32(g_esp,resource);POP32(g_esp,g_esi);g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_audio_direct(uint32_t a){if(!direct_admit(a))return 0;switch(a){case 0x245aa0:direct_duplicate();break;case 0x245b80:direct_factory();break;case 0x2489c0:direct_group();break;default:direct_request();break;}return 1;}
