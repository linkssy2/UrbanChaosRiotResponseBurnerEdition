#include "migration_audio_request_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_00245AD0(void);
extern void sub_00246600(void);
extern void sub_00245B80(void);
extern void sub_00245BB0(void);
extern void sub_002489F0(void);
extern void sub_0024A500(void);
extern void sub_0024B650(void);
extern void sub_0024C650(void);
extern void sub_0024C800(void);
extern void sub_002466A0(void);
extern void sub_002468E0(void);
static int request_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_REQUEST");f=v&&!strcmp(v,"1");}return f;}
static int request_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int request_admit(uint32_t a){
    if(!request_enabled()||!request_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x10100||!request_span(g_esp-256,276)||!request_span(g_ecx,0x45))return 0;
    if(!((uint64_t)g_ecx+0x45<=g_esp-256||(uint64_t)g_esp+20<=g_ecx))return 0;
    uint32_t bank=MEM32(g_esp+4);
    if(a==0x246bb0){uint32_t out=MEM32(g_esp+12);return (!bank||(request_span(bank,4)&&request_span(MEM32(bank),0x24)))&&(!out||request_span(out,4));}
    return a==0x2468e0&&request_span(bank,4)&&request_span(MEM32(bank),4);
}
/* Both resident and spatial requests retain the same bank/selection state and
 * source buffer identities. Persistent requests are published through the
 * original pending owner; selection, allocation and playback remain completing
 * services. Their original PCs and callback-visible guest frame are preserved. */
static void request_prepare(uint32_t buffer,int spatial){
    PUSH32(g_esp,buffer);g_ecx=g_esi;PUSH32(g_esp,spatial?0x246b5b:0x246a20);sub_002489F0();
    g_edx=MEM32(g_esp+0x40);g_eax=MEM32(g_esi);g_ecx=g_esp+16;
    {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+0x40);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);g_ecx=g_esi;
     uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,spatial?0x246b71:0x246a36);RECOMP_ICALL_SAFE(target,saved);}
}
static int request_start(int spatial){
    if(!LO8(g_eax))return 0;
    g_edx=MEM32(g_esp+16);
    if(spatial){g_ecx=MEM32(g_esp+24);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x246b84);}
    else{PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);g_ecx=g_ebx;PUSH32(g_esp,0x246a4b);}
    sub_0024C800();return LO8(g_eax)!=0;
}
static void request_ampc(void){
    uint32_t frame=g_seh_ebp,index,buffer=0;int return_instance=0,cleanup=0,spatial=0,failed_start=0;
    g_esp-=40;PUSH32(g_esp,frame);index=MEM32(g_esp+52);PUSH32(g_esp,g_edi);g_edi=g_ecx;
    if(index==0xffffffffu){POP32(g_esp,g_edi);g_eax=0;POP32(g_esp,frame);g_esp+=40;g_recomp_zf=g_esp==0;g_esp+=12;return;}
    g_ecx=MEM32(g_esp+52);g_eax=MEM32(g_ecx);
    {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=0;PUSH32(g_esp,0);PUSH32(g_esp,0x4fec6c);MEM32(g_esp+36)=0;
     uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x246910);RECOMP_ICALL_SAFE(target,saved);}
    PUSH32(g_esp,g_edi);g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);g_ebx=g_eax;g_edx=g_esp+28;PUSH32(g_esp,g_edx);g_eax=g_esp+28;PUSH32(g_esp,g_eax);PUSH32(g_esp,index);g_ecx=g_ebx;
    MEM32(g_esp+44)=g_ebx;MEM32(g_esp+40)=g_esi;MEM32(g_esp+36)=g_esi;PUSH32(g_esp,0x246936);sub_0024C650();
    if(LO8(g_eax)){
        int have_group=1;
        if(MEM32(g_esp+28)==g_esi){g_eax=MEM32(g_edi+0x3c);MEM32(g_esp+28)=g_eax;have_group=g_eax!=g_esi;}
        if(have_group){
            g_eax=MEM32(g_esp+20);g_ecx=MEM8(g_eax+8);MEM32(g_esp+40)=g_ecx;SET_LO8(g_ecx,MEM8(g_eax+20));
            int spatial_record=0;
            if(LO8(g_ecx)&0x80){SET_LO16(g_eax,MEM16(g_eax+22));spatial_record=LO16(g_eax)!=0xffff;}
            if(spatial_record){
                g_edx=LO16(g_eax);PUSH32(g_esp,g_edx);g_ecx=g_ebx;PUSH32(g_esp,0x24697b);sub_0024B650();MEM32(g_esp+44)=2;
                g_ecx=MEM16(g_eax+14);MEM32(g_esp+48)=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_eax+16));MEMF(g_esp+32)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_eax+20));MEMF(g_esp+36)=g_xmm0.f[0];g_edx=MEM32(g_eax+36)>>1;SET_LO8(g_edx,LO8(g_edx)&1);MEM8(g_esp+52)=LO8(g_edx);
            }else{MEM32(g_esp+44)=1;MEM8(g_esp+52)=0;}
            g_eax=g_esp+28;PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x2469c8);sub_00245AD0();buffer=g_eax;
            if(buffer==g_esi){g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x2469da);sub_00246600();buffer=g_eax;}
            if(buffer!=g_esi){
                SET_LO8(g_eax,MEM8(g_edi+0x44));if(LO8(g_eax))MEM32(g_esp+16)|=0x40;
                spatial=(MEM8(buffer+0x44)&2)!=0;
                if(!spatial){
                    g_edi=MEM32(g_esp+16);g_edx=MEM32(g_esp+28);g_edi|=1;PUSH32(g_esp,g_edx);MEM32(g_esp+20)=g_edi;PUSH32(g_esp,0x246a0b);sub_00245B80();g_esi=g_eax;g_esp+=4;
                    if(g_esi){request_prepare(buffer,0);int prepared=LO8(g_eax)!=0;
                        if(request_start(0))return_instance=1;else{cleanup=1;failed_start=prepared;}}
                    else return_instance=1;
                }else{
                    g_eax=MEM32(g_esp+20);
                    if(MEM8(g_eax+20)&0x20){g_ecx=MEM32(g_esp+28);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x246a7a);sub_00245B80();g_esp+=4;g_esi=g_eax;}
                    PUSH32(g_esp,0);PUSH32(g_esp,0x246a86);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;
                    {uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fec10);g_ecx=g_eax;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x246a96);RECOMP_ICALL_SAFE(target,saved);}
                    g_edx=MEM32(g_eax);g_ecx=g_eax;
                    {uint32_t saved=g_esp,target=MEM32(g_edx+40);PUSH32(g_esp,0x246a9d);RECOMP_ICALL_SAFE(target,saved);}
                    g_xmm2=XMM_SCALAR(MEMF(g_edi+56));g_xmm2.f[0]-=MEMF(g_eax+8);g_xmm0=XMM_SCALAR(MEMF(g_edi+48));g_xmm0.f[0]-=MEMF(g_eax);g_xmm1=XMM_SCALAR(MEMF(g_edi+52));g_xmm1.f[0]-=MEMF(g_eax+4);
                    g_xmm3=g_xmm2;g_xmm3.f[0]*=g_xmm2.f[0];g_xmm2=g_xmm0;g_xmm2.f[0]*=g_xmm0.f[0];g_xmm0=g_xmm1;g_xmm0.f[0]*=g_xmm1.f[0];g_xmm3.f[0]+=g_xmm2.f[0];g_xmm3.f[0]+=g_xmm0.f[0];
                    g_xmm0=XMM_SCALAR(MEMF(g_esp+36));g_xmm1=g_xmm0;g_xmm1.f[0]*=g_xmm0.f[0];double radius=g_xmm1.f[0],distance=g_xmm3.f[0];SET_LO8(g_ebx,radius==radius&&distance==distance&&radius>distance);
                    if(g_esi){
                        int audible=LO8(g_ebx)!=0;
                        if(!audible){g_eax=MEM32(g_esp+20);SET_LO8(g_ecx,MEM8(g_eax+9));audible=LO8(g_ecx)!=0;}
                        if(audible)g_eax=1;else SET_LO8(g_eax,0);
                        g_edx=MEM32(g_esp+64);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+64);g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x246b22);sub_00245BB0();
                    }
                    int audible=LO8(g_ebx)!=0;
                    if(!audible){g_ecx=MEM32(g_esp+20);SET_LO8(g_eax,MEM8(g_ecx+9));audible=LO8(g_eax)!=0;}
                    if(audible){
                        if(!g_esi){g_edi=MEM32(g_esp+16);g_edx=MEM32(g_esp+28);g_edi|=1;PUSH32(g_esp,g_edx);MEM32(g_esp+20)=g_edi;PUSH32(g_esp,0x246b4a);sub_00245B80();g_esi=g_eax;g_esp+=4;}
                        if(g_esi){request_prepare(buffer,1);if(request_start(1))return_instance=1;else cleanup=1;}
                        else return_instance=1;
                    }else return_instance=1;
                }
            }
        }
    }
    if(cleanup){g_ecx=g_esi;PUSH32(g_esp,failed_start?0x246a5a:0x246b8f);sub_0024A500();}
    if(return_instance)g_eax=g_esi;
    POP32(g_esp,g_esi);POP32(g_esp,g_ebx);POP32(g_esp,g_edi);if(!return_instance)g_eax=0;POP32(g_esp,frame);g_esp+=40;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void request_route(void){
    PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+16);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;if(g_ebx)MEM32(g_ebx)=0;g_esi=MEM32(g_esp+16);
    if(g_esi){g_eax=MEM32(g_esi);g_ecx=g_esi;
        {uint32_t saved=g_esp,target=MEM32(g_eax+32);PUSH32(g_esp,0x246bd2);RECOMP_ICALL_SAFE(target,saved);}
        g_ecx=MEM32(0x4fec6c);int bank=g_ecx==MEM32(g_eax);g_ecx=g_edi;
        if(bank){g_edx=MEM32(g_esp+20);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x246bf1);sub_002468E0();}
        else{PUSH32(g_esp,g_esi);PUSH32(g_esp,0x246be4);sub_002466A0();}
        g_esi=g_eax;
        if(g_ebx&&g_esi){g_eax=MEM32(g_esi);g_ecx=g_esi;
            {uint32_t saved=g_esp,target=MEM32(g_eax+4);PUSH32(g_esp,0x246c02);RECOMP_ICALL_SAFE(target,saved);}MEM32(g_ebx)=g_esi;}
    }
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=16;
}
int uc_migration_audio_request(uint32_t a){if(!request_admit(a))return 0;if(a==0x2468e0)request_ampc();else request_route();return 1;}
