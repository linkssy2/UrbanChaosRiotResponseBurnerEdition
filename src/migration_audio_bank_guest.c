#include "migration_audio_bank_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_0024B760(void);
extern void sub_0024BE60(void);
static int bank_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_BANK");f=v&&!strcmp(v,"1");}return f;}
static int bank_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int bank_admit(uint32_t a){
    if(!bank_enabled()||!bank_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!bank_span(g_esp-4096,4108)||!bank_span(g_ecx,0xa8))return 0;
    if(!((uint64_t)g_ecx+0xa8<=g_esp-4096||(uint64_t)g_esp+12<=g_ecx))return 0;
    if(a==0x24d000)return bank_span(MEM32(g_esp+4),16)&&bank_span(MEM32(g_esp+8),2);
    if(a!=0x24b760)return 0;
    uint32_t data=MEM32(g_ecx+144);
    if(data){if(!bank_span(data,12))return 0;uint32_t index=MEM32(g_esp+4);return index>=MEM32(data+8)||(uint64_t)data+12+(uint64_t)(index+1)*20<=g_xbox_total_ram;}
    /* ponytail: bounded O(n) admission for the existing bank resource list. */
    uint32_t head=g_ecx+96,node=MEM32(head),previous=head;
    for(unsigned n=0;node!=head;n++,node=MEM32(node)){if(n==4096||!bank_span(node-12,20)||MEM32(node+4)!=previous)return 0;previous=node;}return MEM32(head+4)==previous;
}
static void bank_resource(void){
    g_esp-=304;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+144);
    if(g_eax){
        g_ecx=MEM32(g_esp+316);
        if(g_ecx>=MEM32(g_eax+8))g_eax=0;
        else{
            g_ecx+=g_ecx*4;g_esi=g_eax+g_ecx*4+12;g_eax=MEM32(g_esi+16);
            if(!g_eax){
                PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24b7a0);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fedd0);g_ecx=g_eax;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x24b7b0);RECOMP_ICALL_SAFE(target,saved);
                g_ecx=MEM32(g_edi+84);MEM32(g_esp+300)=g_ecx;g_edx=g_esi;g_ecx=MEM32(g_edx);MEM32(g_esp+272)=g_ecx;g_ecx=MEM32(g_edx+4);MEM32(g_esp+276)=g_ecx;g_ecx=MEM32(g_edx+8);g_edx=MEM32(g_edx+12);MEM32(g_esp+280)=g_ecx;g_ecx=g_esp+8;MEM32(g_esp+8)=0x104;MEM32(g_esp+284)=g_edx;g_edx=MEM32(g_eax);saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_eax;target=MEM32(g_edx+4);PUSH32(g_esp,0x24b7f7);RECOMP_ICALL_SAFE(target,saved);
                POP32(g_esp,g_edi);MEM32(g_esi+16)=g_eax;POP32(g_esp,g_esi);g_esp+=304;g_recomp_zf=g_esp==0;g_esp+=8;return;
            }
        }
    }else{
        g_eax=MEM32(g_edi+96);g_ecx=g_edi+96;g_edx=0;
        if(g_eax!=g_ecx){g_esi=MEM32(g_esp+316);while(g_edx!=g_esi){g_eax=MEM32(g_eax);g_edx++;if(g_eax==g_ecx)break;}
            if(g_eax!=g_ecx){g_edx=MEM32(g_eax-12);g_ecx=g_eax-12;uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fedd0);uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x24b840);RECOMP_ICALL_SAFE(target,saved);}
            else g_eax=0;
        }else g_eax=0;
    }
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=304;g_recomp_zf=g_esp==0;g_esp+=8;
}
/* Track selection owns the selected resource/subsound and lazily connects
   missing resident/stream services. Selector policy remains a separate owner. */
static void bank_track(void){
    g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi+56)=0;MEM32(g_esi+60)=0xffffffffu;SET_LO16(g_ecx,MEM16(g_eax+6));
    if(LO8(g_ecx)&4){
        if(LO8(g_ecx)&16){g_eax=MEM32(g_eax+12);PUSH32(g_esp,2);PUSH32(g_esp,g_eax);}
        else if(LO8(g_ecx)&64){PUSH32(g_esp,3);g_ecx=MEM32(g_eax+12);PUSH32(g_esp,g_ecx);}
        else if(LO8(g_ecx)&128){g_edx=MEM32(g_eax+12);PUSH32(g_esp,4);PUSH32(g_esp,g_edx);}
        else if(HI8(g_ecx)&1){g_eax=MEM32(g_eax+12);PUSH32(g_esp,5);PUSH32(g_esp,g_eax);}
        else{PUSH32(g_esp,0);g_ecx=MEM32(g_eax+12);PUSH32(g_esp,g_ecx);}
        g_ecx=MEM32(g_esi+48);PUSH32(g_esp,0x24d05b);sub_0024BE60();
        if(g_eax){SET_LO16(g_ecx,MEM16(g_eax));if(LO16(g_ecx)!=0xffff){g_edx=LO16(g_ecx);g_ecx=MEM32(g_esi+48);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24d075);sub_0024B760();MEM32(g_esi+56)=g_eax;}
            else if(MEM16(g_eax+2)!=0xffff){g_ecx=MEM32(g_esi+48);g_edx=MEM32(g_esp+12);MEM32(g_esi+56)=g_ecx;SET_LO16(g_eax,MEM16(g_eax+2));MEM16(g_edx)=LO16(g_eax);g_eax=LO16(g_eax);MEM32(g_esi+60)=g_eax;}}
    }else{
        SET_LO16(g_ecx,MEM16(g_eax+12));if(LO16(g_ecx)!=0xffff){g_ecx=LO16(g_ecx);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+48);PUSH32(g_esp,0x24d0b2);sub_0024B760();MEM32(g_esi+56)=g_eax;}
        else if(MEM16(g_eax+14)!=0xffff){g_edx=MEM32(g_esi+48);g_ecx=MEM32(g_esp+12);MEM32(g_esi+56)=g_edx;SET_LO16(g_eax,MEM16(g_eax+14));g_edx=LO16(g_eax);MEM16(g_ecx)=LO16(g_eax);MEM32(g_esi+60)=g_edx;}
    }
    g_ecx=MEM32(g_esi+56);
    if(g_ecx){
        g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_eax+32);PUSH32(g_esp,0x24d0e7);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fee1c);int resident=g_ecx==MEM32(g_eax);g_ecx=MEM32(g_esi+56);int connect=0;
        if(resident){
            g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fee1c);target=MEM32(g_edx);PUSH32(g_esp,0x24d0ff);RECOMP_ICALL_SAFE(target,saved);g_edi=g_eax;g_eax=MEM32(g_edi);g_ecx=g_edi;saved=g_esp;target=MEM32(g_eax+52);PUSH32(g_esp,0x24d108);RECOMP_ICALL_SAFE(target,saved);
            if(!g_eax){g_edx=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4fee44);g_ecx=g_edi;target=MEM32(g_edx);PUSH32(g_esp,0x24d118);RECOMP_ICALL_SAFE(target,saved);if(!g_eax){MEM32(g_esp+16)=g_eax;connect=1;}}
        }else{
            g_eax=MEM32(g_ecx);saved=g_esp;target=MEM32(g_eax+32);PUSH32(g_esp,0x24d127);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fee20);
            if(g_ecx==MEM32(g_eax)){
                g_ecx=MEM32(g_esi+56);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fee20);target=MEM32(g_edx);PUSH32(g_esp,0x24d13f);RECOMP_ICALL_SAFE(target,saved);g_edi=g_eax;g_eax=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fedec);g_ecx=g_edi;target=MEM32(g_eax);PUSH32(g_esp,0x24d14e);RECOMP_ICALL_SAFE(target,saved);
                if(!g_eax){g_edx=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4fee44);g_ecx=g_edi;target=MEM32(g_edx);PUSH32(g_esp,0x24d15e);RECOMP_ICALL_SAFE(target,saved);if(!g_eax){MEM32(g_esp+16)=1;connect=1;}}
            }
        }
        if(connect){g_ecx=MEM32(g_esi+48);g_eax=MEM32(g_ecx);g_edx=g_esp+16;saved=g_esp;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x4fee44);target=MEM32(g_eax);PUSH32(g_esp,0x24d17b);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;target=MEM32(g_edx+24);PUSH32(g_esp,0x24d183);RECOMP_ICALL_SAFE(target,saved);}
        POP32(g_esp,g_edi);
    }
    g_eax=MEM32(g_esi+56);POP32(g_esp,g_esi);g_esp+=12;
}
int uc_migration_audio_bank(uint32_t a){if(!bank_admit(a))return 0;if(a==0x24b760)bank_resource();else bank_track();return 1;}
