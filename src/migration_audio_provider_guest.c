#include "migration_audio_provider_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_00243FD0(void);
extern void sub_0020D5B0(void);
static int provider_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_PROVIDER");f=v&&!strcmp(v,"1");}return f;}
static int provider_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int provider_safe(uint32_t p,uint32_t n){return provider_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int provider_admit(uint32_t a){
    if((a!=0x24b850&&a!=0x24ba60)||!provider_enabled()||!provider_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!provider_span(g_esp-4096,4108)||!provider_safe(MEM32(g_esp+4),4))return 0;
    if(a==0x24ba60){
        uint32_t id=MEM32(MEM32(g_esp+4));if(id!=MEM32(0x4fee44))return !g_ecx||provider_safe(g_ecx,180);
        if(!provider_safe(g_ecx,180)||!provider_safe(MEM32(g_esp+8),4))return 0;uint32_t kind=MEM32(MEM32(g_esp+8));if(kind>2||MEM32(g_ecx+168+kind*4))return 1;
        uint32_t header=MEM32(g_ecx+112);if(!provider_safe(header,16))return 0;uint64_t p=(uint64_t)MEM32(g_ecx+140)+MEM16(header+(kind?14:12))+8;if(p>0x7fffffffu)return 0;
        for(unsigned n=0;n<256;n++){if(!provider_safe((uint32_t)p+n,1))return 0;if(!MEM8((uint32_t)p+n))return 1;}return 0;
    }
    if(!provider_safe(g_ecx,180)||!provider_span(MEM32(MEM32(g_esp+4)),52))return 0;
    for(unsigned n=0;n<3;n++){uint32_t p=MEM32(g_ecx+168+n*4);if(p&&(!provider_safe(p,4)||!provider_span(MEM32(p),12)))return 0;}
    uint32_t head=g_ecx+96,node=MEM32(head),previous=head;
    /* ponytail: O(n) admission; use trusted list generations only when measured. */
    for(unsigned n=0;node!=head;n++){if(n>=4096||node<12||!provider_safe(node-12,20)||MEM32(node+4)!=previous||!provider_span(MEM32(node-12),12))return 0;previous=node;node=MEM32(node);}return MEM32(head+4)==previous;
}
static void provider_replace(void){
    uint32_t frame=g_seh_ebp,saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+20);g_eax=MEM32(g_edi);g_esi=g_ecx;g_ecx=g_edi;uint32_t target=MEM32(g_eax+48);PUSH32(g_esp,0x24b861);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_edi;int replace=0,kind=0;
    if(!g_eax){g_ebx=MEM32(g_esi+168);MEM32(g_esi+168)=g_edi;replace=1;}
    else{
        g_edx=MEM32(g_edi);saved=g_esp;target=MEM32(g_edx+48);PUSH32(g_esp,0x24b87a);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_edi;
        if(g_eax==1){g_ebx=MEM32(g_esi+172);MEM32(g_esi+172)=g_edi;kind=1;replace=1;}
        else{g_eax=MEM32(g_edi);saved=g_esp;target=MEM32(g_eax+48);PUSH32(g_esp,0x24b8a4);RECOMP_ICALL_SAFE(target,saved);if(g_eax==2){g_ebx=MEM32(g_esi+176);MEM32(g_esi+176)=g_edi;g_ecx=g_edi;replace=1;}}
    }
    if(replace){
        saved=g_esp;if(kind){g_eax=MEM32(g_edi);target=MEM32(g_eax+4);}else{g_edx=MEM32(g_edi);target=MEM32(g_edx+4);}PUSH32(g_esp,kind?0x24b892:0x24b8bc);RECOMP_ICALL_SAFE(target,saved);
        if(g_ebx){saved=g_esp;if(kind){g_edx=MEM32(g_ebx);target=MEM32(g_edx+8);}else{g_eax=MEM32(g_ebx);target=MEM32(g_eax+8);}g_ecx=g_ebx;PUSH32(g_esp,kind?0x24b89d:0x24b8c7);RECOMP_ICALL_SAFE(target,saved);}
    }
    g_edx=MEM32(g_edi);g_ecx=g_edi;saved=g_esp;target=MEM32(g_edx+48);PUSH32(g_esp,0x24b8ce);RECOMP_ICALL_SAFE(target,saved);g_ebx=MEM32(g_esi+96);g_esi+=96;frame=g_eax;MEM32(g_esp+20)=g_esi;
    while(g_ebx!=MEM32(g_esp+20)){
        g_eax=MEM32(g_ebx-12);g_ecx=g_ebx-12;saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fedd0);target=MEM32(g_eax+8);PUSH32(g_esp,0x24b8f0);RECOMP_ICALL_SAFE(target,saved);g_esi=g_eax;
        if(g_esi){
            g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_edx+32);PUSH32(g_esp,0x24b8fd);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fee28);int match=g_ecx==MEM32(g_eax);
            if(!match&&(frame==0||frame==1||frame==2)){
                g_eax=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_eax+32);PUSH32(g_esp,frame?0x24b92b:0x24b912);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(frame?0x4fee20:0x4fee1c);match=g_ecx==MEM32(g_eax);
            }
            if(match){g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;PUSH32(g_esp,g_edi);target=MEM32(g_edx+24);PUSH32(g_esp,0x24b93d);RECOMP_ICALL_SAFE(target,saved);}
        }g_ebx=MEM32(g_ebx);
    }
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=8;
}
static void provider_query(void){
    g_eax=MEM32(g_esp+4);g_eax=MEM32(g_eax);g_esp-=304;PUSH32(g_esp,g_esi);g_esi=g_ecx;
    if(MEM32(0x4fedd0)==g_eax)g_eax=g_esi;
    else if(MEM32(0x4fee18)==g_eax)g_eax=g_esi?g_esi+4:0;
    else{
        int resource=MEM32(0x4fee44)==g_eax;PUSH32(g_esp,g_edi);
        if(resource){
            g_ecx=MEM32(g_esp+320);g_eax=MEM32(g_ecx);uint32_t kind=g_eax;
            if(kind<=2){
                uint32_t offset=168+kind*4;g_eax=MEM32(g_esi+offset);
                if(!g_eax){
                    static const uint32_t pc[][4]={{0x24bad7,0x24bae7,0x24bb0d,0x24bb19},{0x24bb60,0x24bb70,0x24bb96,0x24bba2},{0x24bbe9,0x24bbf9,0x24bc1f,0x24bc2b}};
                    PUSH32(g_esp,0);PUSH32(g_esp,pc[kind][0]);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fedd0);g_ecx=g_eax;uint32_t target=MEM32(g_edx);PUSH32(g_esp,pc[kind][1]);RECOMP_ICALL_SAFE(target,saved);
                    g_edx=MEM32(g_esi+140);g_edi=g_eax;g_eax=MEM32(g_esi+112);MEM32(g_esp+8)=0x22;g_ecx=MEM16(g_eax+(kind?14:12));g_eax=g_ecx+g_edx+8;PUSH32(g_esp,g_eax);g_ecx=g_esp+16;PUSH32(g_esp,g_ecx);PUSH32(g_esp,pc[kind][2]);sub_00243FD0();g_edx=g_esp+20;PUSH32(g_esp,1);PUSH32(g_esp,g_edx);PUSH32(g_esp,pc[kind][3]);sub_0020D5B0();g_esp+=16;g_ecx=g_esp+8;MEM32(g_esp+268)=g_eax;g_eax=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_edi;target=MEM32(g_eax+4);PUSH32(g_esp,kind==0?0x24bb2f:kind==1?0x24bbb8:0x24bc41);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+offset)=g_eax;
                }g_eax=MEM32(g_esi+offset);
            }else g_eax=g_esi;
        }else g_eax=g_esi;
        POP32(g_esp,g_edi);
    }POP32(g_esp,g_esi);g_esp+=304;g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_audio_provider(uint32_t a){if(!provider_admit(a))return 0;if(a==0x24b850)provider_replace();else provider_query();return 1;}
