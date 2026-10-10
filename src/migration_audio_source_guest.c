#include "migration_audio_source_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

extern void sub_00241880(void);
extern void sub_00240850(void);
extern void sub_00243F40(void);
extern void sub_0023FD10(void);
extern void sub_00248D10(void);

static int enabled(void) {
    static int flag=-1;
    if(flag<0){const char *v=getenv("UCRR_MIGRATION_AUDIO_SOURCE");flag=v&&!strcmp(v,"1");}
    return flag;
}
static int span(uint32_t p,uint32_t n){return p>=0x10000u&&(uint64_t)p+n<=g_xbox_total_ram&&p<0x80000000u;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int object(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-256,288)&&apart(p,n,0x51c178,1);}
static int ring(uint32_t head){
    if(!object(head,8))return 0;
    uint32_t p=head;unsigned count=0;
    do{if(++count>4096||!object(p,8))return 0;uint32_t next=MEM32(p),prev=MEM32(p+4);
       if(!object(next,8)||!object(prev,8)||MEM32(next+4)!=p||MEM32(prev)!=p)return 0;p=next;
    }while(p!=head);
    return 1;
}
static int admit(uint32_t address){
    if(!enabled()||!span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x10100u||!span(g_esp-256,288))return 0;
    if(address==0x24c800){uint32_t instance=MEM32(g_esp+4);
        return object(g_ecx,0x60)&&ring(g_ecx+0x58)&&(!instance||(object(instance,0x80)&&span(MEM32(instance),0x1c)&&ring(instance+0x20)));
    }
    if(!object(g_ecx,0x45))return 0;
    if(address==0x2463e0)return span(0x51cb90,4)&&object(MEM32(0x51cb90),0x84)&&span(0x44c178,4);
    if(address==0x2464d0||address==0x2464f0)return 1;
    if(address!=0x245ad0&&address!=0x246600)return 0;
    uint32_t config=MEM32(g_esp+4);
    if(!object(config,0x1c)||!ring(g_ecx+12))return 0;
    if(address==0x246600)return object(MEM32(config),0x28)&&span(MEM32(MEM32(config)),0x28);
    for(uint32_t node=MEM32(g_ecx+12);node!=g_ecx+12;node=MEM32(node)){
        if(node<32||!object(node-32,0x54)||!span(MEM32(node-28),12))return 0;
        uint32_t spatial=MEM32(node+0x20);if(spatial&&!object(spatial,0x14))return 0;
    }
    return 1;
}

/* The source remains the real 32-bit title object. These methods group its
 * ownership operations while retaining exact ABI state at completing callbacks.
 * No shadow graph, allocation, clock, RNG or host mixer is introduced. */
static void construct_source(void){
    PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e718);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
    PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+8)=g_esi;
    MEM32(g_esi)=0x483ae4;MEM32(g_esi+0x24)=2;g_eax=1;
    MEM32(g_esi+0x1c)=MEM32(g_esi+0x20)=1;
    g_xmm0=XMM_SCALAR(MEMF(0x44bf94));MEMF(g_esi+0x28)=g_xmm0.f[0];
    g_xmm0=XMM_SCALAR(MEMF(0x44c178));MEMF(g_esi+0x2c)=g_xmm0.f[0];g_xmm0=XMM_ZERO();
    for(unsigned offset=0x30;offset<=0x38;offset+=4)MEM32(g_esi+offset)=0;
    for(unsigned offset=4;offset<=0x14;offset+=8){g_eax=g_esi+offset;MEM32(g_eax)=MEM32(g_eax+4)=g_eax;}
    g_ebx=0;MEM32(g_esi+0x3c)=MEM32(g_esi+0x40)=0;MEM8(g_esi+0x44)=0;
    PUSH32(g_esp,0);MEM32(g_esp+0x18)=0;PUSH32(g_esp,0x24646f);sub_00241880();
    g_edx=MEM32(g_eax);g_esp+=4;
    {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x4fec10);g_ecx=g_eax;
     uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x24647e);RECOMP_ICALL_SAFE(target,saved);}
    if(g_eax!=g_ebx){
        g_edx=MEM32(g_eax);g_ecx=g_eax;
        {uint32_t saved=g_esp,target=MEM32(g_edx+0x18);PUSH32(g_esp,0x246489);RECOMP_ICALL_SAFE(target,saved);}
        /* Preserve the guest x87 stack slots, including the popped results. */
        g_fp_stack[g_fp_top]=MEMF(0x44bf94)/g_fp_stack[g_fp_top];
        double inverse=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=inverse;
        g_fp_stack[g_fp_top]*=MEMF(g_esi+0x28);MEMF(g_esi+0x28)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
        g_fp_stack[g_fp_top]*=MEMF(g_esi+0x2c);MEMF(g_esi+0x2c)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
    }
    PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x2464a3);sub_00241880();g_esp+=4;
    if(g_eax!=g_ebx){PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x2464b2);sub_00240850();}
    g_ecx=MEM32(g_esp+12);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;
    g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}

static void create_buffer(void){
    uint32_t owner=g_ecx,frame=g_seh_ebp;g_esp-=8;
    PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+24);
    SET_LO16(g_eax,MEM16(g_edi+12));g_ecx=MEM32(g_edi+16);MEM16(g_esp+18)=LO16(g_eax);MEM32(g_esp+12)=g_ecx;
    g_ecx=MEM32(g_edi);g_eax=g_esp+12;
    {uint32_t saved=g_esp+12;PUSH32(g_esp,g_eax);MEM16(g_esp+20)=1;g_edx=MEM32(g_ecx);PUSH32(g_esp,0x4fec1c);
     uint32_t target=MEM32(g_edx+0x24);PUSH32(g_esp,0x246634);RECOMP_ICALL_SAFE(target,saved);}
    g_ebx=g_eax;
    if(g_ebx){
        /* Capture the spatial branch once, then reload every property receiver. */
        if(MEM8(g_ebx+0x44)&2){
            g_edx=MEM32(g_ebx+4);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_eax=g_edi+24;g_esi=g_ebx+4;
            PUSH32(g_esp,g_eax);PUSH32(g_esp,7);g_ecx=g_esi;
            uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x24665b);RECOMP_ICALL_SAFE(target,saved);
            g_edx=MEM32(g_esi);g_eax=g_edi+20;saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,3);g_ecx=g_esi;
            target=MEM32(g_edx+4);PUSH32(g_esp,0x246668);RECOMP_ICALL_SAFE(target,saved);
            g_edx=MEM32(g_esi);g_edi+=4;saved=g_esp;PUSH32(g_esp,g_edi);PUSH32(g_esp,4);g_ecx=g_esi;
            target=MEM32(g_edx+4);PUSH32(g_esp,0x246675);RECOMP_ICALL_SAFE(target,saved);
            g_eax=MEM32(g_esi);g_ecx=owner+0x30;saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,5);g_ecx=g_esi;
            target=MEM32(g_eax+4);PUSH32(g_esp,0x246682);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);
        }
        g_edx=g_ebx+32;PUSH32(g_esp,g_edx);PUSH32(g_esp,owner+12);PUSH32(g_esp,0x246690);sub_00243F40();g_esp+=8;
    }
    POP32(g_esp,g_edi);POP32(g_esp,frame);
    if(g_ebx)g_eax=g_ebx;POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}

static int same_range(uint32_t value){
    double a=g_xmm0.f[0],b=MEMF(value);
    unsigned flags=(a!=a||b!=b)?0x47u:a<b?0x03u:a>b?0x02u:0x42u;
    SET_HI8(g_eax,flags);return !RECOMP_PARITY8(flags&0x44);
}
static void find_buffer(void){
    uint32_t node,frame=g_seh_ebp;g_esp-=8;PUSH32(g_esp,g_ebx);g_ebx=g_ecx+12;PUSH32(g_esp,frame);node=MEM32(g_ebx);g_eax=0;
    if(node!=g_ebx){
        PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+24);PUSH32(g_esp,g_esi);
        while(node!=g_ebx){
            g_eax=MEM32(node+20);g_esi=node-32;int matched=0;
            if(g_eax==MEM32(g_edi)){
                g_eax=MEM32(g_edi+16);
                if((g_eax&2)&&(MEM8(g_esi+0x44)&2)){
                    g_edx=MEM32(g_esi+4);g_eax=g_esp+16;g_ecx=g_esi+4;
                    {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,4);uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x245b17);RECOMP_ICALL_SAFE(target,saved);}
                    g_xmm0=XMM_SCALAR(MEMF(g_esp+16));
                    if(same_range(g_edi+4)){
                        g_xmm0=XMM_SCALAR(MEMF(g_esp+20));
                        if(same_range(g_edi+8)){
                            g_eax=MEM32(g_esi+0x40);
                            if(g_eax){g_eax=MEM32(g_eax+16);matched=MEM32(g_edi+20)==g_eax;}
                            else{g_ecx=MEM32(g_edi+20);g_eax=2;matched=g_ecx==g_eax;}
                        }
                    }
                }else matched=(g_eax&1)&&!(MEM8(g_esi+0x44)&2);
            }
            if(matched){g_eax=g_esi;break;}node=MEM32(node);g_eax=0;
        }
        POP32(g_esp,g_esi);POP32(g_esp,g_edi);
    }
    POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}

static void publish_instance(void){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_edi=g_ecx;int accepted=0;
    if(g_esi){
        PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+20);
        if(g_ebx&0x8000){PUSH32(g_esp,g_esi);PUSH32(g_esp,0);PUSH32(g_esp,0x24c81d);sub_00241880();g_esp+=4;g_ecx=g_eax;PUSH32(g_esp,0x24c827);sub_0023FD10();}
        g_eax=MEM32(g_esi);g_ecx=g_esi;
        {uint32_t saved=g_esp,target=MEM32(g_eax+4);PUSH32(g_esp,0x24c82e);RECOMP_ICALL_SAFE(target,saved);}
        g_ecx=g_esi+32;PUSH32(g_esp,g_ecx);g_edi+=0x58;PUSH32(g_esp,g_edi);PUSH32(g_esp,0x24c83b);sub_00243F40();
        g_edx=MEM32(g_esi);g_esp+=8;
        {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);g_ecx=g_esi;uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x24c846);RECOMP_ICALL_SAFE(target,saved);}
        accepted=LO8(g_eax)!=0;POP32(g_esp,g_ebx);
        if(accepted){g_ecx=g_esi;PUSH32(g_esp,0x24c852);sub_00248D10();}
    }
    POP32(g_esp,g_edi);SET_LO8(g_eax,accepted);POP32(g_esp,g_esi);g_recomp_zf=!accepted;g_esp+=12;
}

int uc_migration_audio_source(uint32_t address){
    if(!admit(address))return 0;
    switch(address){
    case 0x2463e0:construct_source();break;
    case 0x246600:create_buffer();break;
    case 0x245ad0:find_buffer();break;
    case 0x24c800:publish_instance();break;
    case 0x2464d0:MEM8(g_ecx+0x44)=1;g_esp+=4;break;
    case 0x2464f0:g_xmm0=XMM_SCALAR(MEMF(g_esp+4));MEMF(g_ecx+0x28)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+8));MEMF(g_ecx+0x2c)=g_xmm0.f[0];g_esp+=12;break;
    default:return 0;
    }
    return 1;
}
