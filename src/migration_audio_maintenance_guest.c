#include "migration_audio_maintenance_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00245640(void);
extern void sub_00252D20(void);
extern void sub_002519C0(void);
extern void sub_00243F60(void);
extern void sub_002459C0(void);
extern void sub_00247150(void);
extern void sub_00247170(void);
extern void sub_00247140(void);
extern void sub_002D5CCD(void);
extern void sub_00248960(void);
extern void sub_00244010(void);
extern void sub_0024B600(void);
extern void sub_0024A260(void);
extern void sub_0024A9F0(void);
static int maintenance_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_MAINTENANCE");enabled=v&&!strcmp(v,"1");}return enabled;}
static int maintenance_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int maintenance_safe(uint32_t p,uint32_t n){return maintenance_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int maintenance_ring(uint32_t head,uint32_t offset,uint32_t span,int virtual_object){
    uint32_t p=MEM32(head),previous=head,count=0;
    /* ponytail: full admission scans are O(n); share validated list ownership once every publisher is native. */
    while(p!=head){if(++count>65535||p<offset||!maintenance_safe(p-offset,span)||MEM32(p+4)!=previous)return 0;if(virtual_object&&!maintenance_span(MEM32(p-offset),36))return 0;previous=p;p=MEM32(p);}return MEM32(head+4)==previous;
}
static int maintenance_admit(uint32_t a){
    if(!maintenance_enabled()||!maintenance_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!maintenance_span(g_esp-4096,4108)||!maintenance_safe(g_ecx,104))return 0;
    if(a==0x240430)return maintenance_ring(g_ecx+48,8,80,1)&&maintenance_ring(g_ecx+56,8,80,1);
    if(a==0x2413e0)return maintenance_ring(g_ecx+64,0,32,0);
    if(a!=0x240ad0||!maintenance_ring(g_ecx+32,8,104,1)||!maintenance_ring(g_ecx+40,24,104,1))return 0;
    for(uint32_t p=MEM32(g_ecx+40);p!=g_ecx+40;p=MEM32(p))if(MEM32(p+56)&&!maintenance_safe(MEM32(p+56),16))return 0;return 1;
}
static void maintenance_retire(void){
    uint32_t frame=g_seh_ebp;g_esp-=12;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=g_ecx+48;PUSH32(g_esp,g_edi);g_edi=MEM32(g_ebx);MEM32(g_esp+20)=g_ecx;MEM8(g_esp+15)=1;
    if(g_edi!=g_ebx){PUSH32(g_esp,frame);do{g_esi=g_edi-8;g_edi=MEM32(g_edi);PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_esi;PUSH32(g_esp,0x240460);sub_00245640();g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe800);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x24046d);RECOMP_ICALL_SAFE(target,saved);frame=g_eax;int keep=0;
            if(frame){g_edx=MEM32(frame);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=frame;MEM32(g_esp+28)=0;target=MEM32(g_edx+16);PUSH32(g_esp,0x240487);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,0);g_ecx=frame;PUSH32(g_esp,0x240490);sub_00252D20();g_eax=g_esp+20;PUSH32(g_esp,g_eax);g_ecx=frame;PUSH32(g_esp,0x24049c);sub_002519C0();g_eax=MEM32(g_esp+20);keep=g_eax!=0;}
            if(keep)MEM8(g_esp+19)=0;
            else{g_ecx=g_esi+8;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x2404b4);sub_00243F60();g_edx=MEM32(g_esi);g_esp+=4;g_ecx=g_esi;saved=g_esp;target=MEM32(g_edx+8);PUSH32(g_esp,0x2404be);RECOMP_ICALL_SAFE(target,saved);}
        }while(g_edi!=g_ebx);g_ecx=MEM32(g_esp+24);POP32(g_esp,frame);}
    g_edi=MEM32(g_ecx+56);g_ebx=g_ecx+56;
    while(g_edi!=g_ebx){g_esi=g_edi-8;g_edi=MEM32(g_edi);g_ecx=g_esi;PUSH32(g_esp,0x2404dd);sub_002459C0();
        if(!g_eax){g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0);g_ecx=g_esi;uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0x2404ec);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_edx+8);PUSH32(g_esp,0x2404f3);RECOMP_ICALL_SAFE(target,saved);}
        else MEM8(g_esp+15)=0;
    }SET_LO8(g_eax,MEM8(g_esp+15));POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void maintenance_loops(void){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+64);PUSH32(g_esp,g_edi);g_edi=g_ecx+64;
    while(g_esi!=g_edi){g_ecx=g_esi;g_esi=MEM32(g_esi);PUSH32(g_esp,0x2413f9);sub_00247150();}
    g_esi=MEM32(g_edi);if(g_esi!=g_edi){PUSH32(g_esp,g_ebx);do{g_ebx=g_esi;g_esi=MEM32(g_esi);g_ecx=g_ebx;PUSH32(g_esp,0x24140f);sub_00247170();if(LO8(g_eax)){g_ecx=g_ebx;PUSH32(g_esp,0x24141a);sub_00247140();PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x241420);sub_002D5CCD();g_esp+=4;}}while(g_esi!=g_edi);POP32(g_esp,g_ebx);}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=4;
}
static void maintenance_promote(void){
    g_eax=MEM32(g_ebx);g_ecx=g_ebx;uint32_t saved=g_esp,target=MEM32(g_eax+28);PUSH32(g_esp,0x240b43);RECOMP_ICALL_SAFE(target,saved);if(!(g_eax&0x10000))return;
    g_edx=MEM32(g_ebx);g_ecx=g_ebx;saved=g_esp;target=MEM32(g_edx+24);PUSH32(g_esp,0x240b55);RECOMP_ICALL_SAFE(target,saved);if(g_eax==4||!g_edi)return;
    g_eax=MEM32(g_edi);g_ecx=g_edi;saved=g_esp;target=MEM32(g_eax+28);PUSH32(g_esp,0x240b6d);RECOMP_ICALL_SAFE(target,saved);if(!(g_eax&0x10000))return;
    g_edx=MEM32(g_edi);g_ecx=g_edi;saved=g_esp;target=MEM32(g_edx+24);PUSH32(g_esp,0x240b7f);RECOMP_ICALL_SAFE(target,saved);if(g_eax!=2)return;
    g_eax=MEM32(g_esp+20);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x240b92);sub_00243F60();g_edx=MEM32(g_ebx);g_esp+=4;g_ecx=g_ebx;saved=g_esp;target=MEM32(g_edx+8);PUSH32(g_esp,0x240b9c);RECOMP_ICALL_SAFE(target,saved);if(!g_eax)return;
    g_eax=MEM32(g_ebx);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe848);g_ecx=g_ebx;target=MEM32(g_eax);PUSH32(g_esp,0x240bad);RECOMP_ICALL_SAFE(target,saved);g_esi=g_eax;if(!g_esi)return;
    g_edx=MEM32(g_ebx);g_ecx=g_ebx;saved=g_esp;target=MEM32(g_edx+32);PUSH32(g_esp,0x240bba);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x240bc2);sub_0024B600();g_eax=g_eax?MEM16(g_eax):0;
    PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x240bfa);sub_0024A9F0();g_eax=MEM32(g_ebx);g_ecx=g_ebx;saved=g_esp;target=MEM32(g_eax+28);PUSH32(g_esp,0x240c01);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_ebx);g_eax=(g_eax&1)|0x10000;saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_ebx;target=MEM32(g_edx+16);PUSH32(g_esp,0x240c11);RECOMP_ICALL_SAFE(target,saved);
}
static void maintenance_deferred(void){
    uint32_t frame=g_seh_ebp;g_esp-=8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=g_ecx;g_ebx=MEM32(frame+32);g_eax=frame+32;MEM32(g_esp+12)=g_ebx;
    if(g_ebx!=g_eax){PUSH32(g_esp,g_esi);g_ebx-=8;PUSH32(g_esp,g_edi);g_ecx=g_ebx;g_edi=0;PUSH32(g_esp,0x240af7);sub_00248960();g_esi=MEM32(frame+40);frame+=40;MEM32(g_esp+16)=g_eax;int matched=0;
        while(g_esi!=frame){g_eax=MEM32(g_esi+56);g_edi=g_esi-24;
            if(g_eax&&MEM16(g_eax+14)!=0xffff){g_eax=MEM32(g_esp+16);PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x240b23);sub_00248960();PUSH32(g_esp,g_eax);PUSH32(g_esp,0x240b29);sub_00244010();g_esp+=8;if(!g_eax){matched=1;break;}}
            g_esi=MEM32(g_esi);g_edi=0;
        }
        int started=0;if(matched){g_edx=MEM32(g_edi);g_ecx=g_edi;uint32_t saved=g_esp,target=MEM32(g_edx+24);PUSH32(g_esp,0x240bd2);RECOMP_ICALL_SAFE(target,saved);if(g_eax==5){PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_edi;PUSH32(g_esp,0x240be8);sub_0024A260();started=1;}}
        if(!started)maintenance_promote();POP32(g_esp,g_edi);POP32(g_esp,g_esi);
    }POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_audio_maintenance(uint32_t address){if(!maintenance_admit(address))return 0;switch(address){case 0x240430:maintenance_retire();break;case 0x2413e0:maintenance_loops();break;case 0x240ad0:maintenance_deferred();break;}return 1;}
