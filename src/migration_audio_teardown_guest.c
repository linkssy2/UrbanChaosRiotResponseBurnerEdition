#include "migration_audio_teardown_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00247E70(void);
extern void sub_002D69BB(void);
extern void sub_002D631D(void);
extern void sub_00243F60(void);
extern void sub_00243EE0(void);
extern void sub_00248250(void);
extern void sub_002D5CCD(void);
static int teardown_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_TEARDOWN");f=v&&!strcmp(v,"1");}return f;}
static int teardown_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int teardown_safe(uint32_t p,uint64_t n){return teardown_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+4<=p);}
static int teardown_object(uint32_t p,uint32_t slot){return !p||(teardown_safe(p,4)&&teardown_span(MEM32(p),slot+4));}
static int teardown_ring(uint32_t head,unsigned offset,unsigned slot){
    uint32_t p=MEM32(head);unsigned budget=65536;
    /* ponytail: admission scans each existing ring; reuse trusted ownership metadata if this cost matters. */
    while(p!=head){if(!budget--||p<offset||!teardown_safe(p,8)||!teardown_object(p-offset,slot))return 0;p=MEM32(p);}return 1;
}
static int teardown_admit(uint32_t a){
    if((a!=0x24ec30&&a!=0x24e640&&a!=0x24ce00)||!teardown_enabled()||!teardown_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!teardown_span(g_esp-4096,4100))return 0;
    uint32_t p=g_ecx;
    if(a==0x24ce00){unsigned budget=512;do{if(!budget--||!teardown_safe(p,8))return 0;p=MEM32(p);}while(p);return 1;}
    if(!teardown_safe(p,a==0x24ec30?192:80))return 0;
    uint32_t base=a==0x24ec30?p+4:p;
    if(!teardown_safe(MEM32(base+4),8)||!teardown_safe(MEM32(base+8),8)||!teardown_object(MEM32(base+44),12))return 0;
    if(a==0x24e640)return 1;
    if(!teardown_ring(p+104,4,4)||!teardown_ring(p+96,12,16))return 0;
    uint32_t table=MEM32(p+144);if(table){if(!teardown_safe(table,12))return 0;uint32_t count=MEM32(table+8);if(count>65536||!teardown_safe(table,12ull+20ull*count))return 0;for(uint32_t n=0;n<count;n++)if(!teardown_object(MEM32(table+28+n*20),8))return 0;}
    table=MEM32(p+188);if(table&&(!teardown_safe(table,12)||MEM32(table+8)>65536||!teardown_safe(table,12ull+8ull*MEM32(table+8))||!teardown_safe(MEM32(p+140),8)))return 0;
    for(unsigned n=168;n<=176;n+=4)if(!teardown_object(MEM32(p+n),8))return 0;
    table=MEM32(p+148);if(table&&(!teardown_safe(table-4,4)||MEM32(table-4)>65536||!teardown_safe(table,8ull*MEM32(table-4))))return 0;
    return 1;
}
static void teardown_chain(void){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);
    if(g_esi){g_ecx=g_esi;PUSH32(g_esp,0x24ce0e);teardown_chain();PUSH32(g_esp,g_esi);PUSH32(g_esp,0x24ce14);sub_002D5CCD();g_esp+=4;}
    POP32(g_esp,g_esi);g_esp+=4;
}
static void teardown_backing(void){
    PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39e8fb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x48408c;g_eax=g_esi+4;PUSH32(g_esp,g_eax);MEM32(g_esp+20)=0;PUSH32(g_esp,0x24e674);sub_00243F60();g_eax=MEM32(g_esi+72);g_esp+=4;
    if(g_eax){PUSH32(g_esp,0x24e683);sub_00243EE0();g_ecx=MEM32(g_esi+72);g_edx=MEM32(g_eax);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=g_eax;uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x24e690);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+72)=0;}
    g_ecx=MEM32(g_esi+44);if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+12);PUSH32(g_esp,0x24e6a3);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+44)=0;}
    g_ecx=g_esi+28;MEM32(g_esp+16)=0xffffffff;PUSH32(g_esp,0x24e6ba);sub_00248250();g_ecx=MEM32(g_esp+8);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void teardown_bank(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39e9df);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+20)=g_esi;MEM32(g_esi)=0x4841e0;MEM32(g_esi+4)=0x4841cc;g_edi=MEM32(g_esi+104);frame=g_esi+104;g_ebx=0;MEM32(g_esp+32)=1;
    while(g_edi!=frame){g_ecx=g_edi-4;g_edi=MEM32(g_edi);if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24ec83);RECOMP_ICALL_SAFE(target,saved);}}
    g_edi=MEM32(g_esi+96);frame=g_esi+96;while(g_edi!=frame){g_edx=MEM32(g_edi-12);g_ecx=g_edi-12;g_edi=MEM32(g_edi);uint32_t saved=g_esp,target=MEM32(g_edx+16);PUSH32(g_esp,0x24ec9c);RECOMP_ICALL_SAFE(target,saved);}
    g_eax=MEM32(g_esi+144);if(g_eax){g_ecx=MEM32(g_eax+8);frame=0;if(g_ecx){g_edi=g_eax+28;do{g_eax=MEM32(g_edi);if(g_eax){g_ecx=g_eax;g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x24ecc3);RECOMP_ICALL_SAFE(target,saved);}g_ecx=MEM32(g_esi+144);g_eax=MEM32(g_ecx+8);frame++;g_edi+=20;}while(frame<g_eax);}}
    frame=MEM32(g_esi+188);if(frame){g_eax=MEM32(frame+8);g_edi=0;if(g_eax)do{SET_LO16(g_eax,MEM16(frame+g_edi*8+12));if(LO16(g_eax)!=0xffff){g_edx=LO16(g_eax);g_eax=MEM32(g_esi+140);g_eax+=g_edx+8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24ed0f);sub_00247E70();g_esp+=8;if(g_eax){MEM32(g_eax+116)=g_ebx;MEM32(g_eax+120)=g_ebx;}}g_ecx=MEM32(g_esi+188);g_eax=MEM32(g_ecx+8);g_edi++;}while(g_edi<g_eax);}
    g_eax=MEM32(g_esi+148);if(g_eax){g_edx=MEM32(g_eax-4);g_edi=g_eax-4;PUSH32(g_esp,0x24ce00);PUSH32(g_esp,g_edx);PUSH32(g_esp,8);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24ed48);sub_002D69BB();PUSH32(g_esp,g_edi);PUSH32(g_esp,0x24ed4e);sub_002D631D();g_esp+=4;}
    static const uint32_t returns[]={0x24ed60,0x24ed75,0x24ed8a};for(unsigned n=0;n<3;n++){g_ecx=MEM32(g_esi+168+n*4);if(g_ecx){uint32_t target;if(n==1){g_edx=MEM32(g_ecx);target=MEM32(g_edx+8);}else{g_eax=MEM32(g_ecx);target=MEM32(g_eax+8);}uint32_t saved=g_esp;PUSH32(g_esp,returns[n]);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+168+n*4)=g_ebx;}}
    g_ecx=g_esi+4;MEM8(g_esp+32)=LO8(g_ebx);PUSH32(g_esp,0x24ed9c);teardown_backing();g_ecx=MEM32(g_esp+24);POP32(g_esp,g_edi);MEM32(g_esi)=0x483d50;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_audio_teardown(uint32_t a){if(!teardown_admit(a))return 0;if(a==0x24ce00)teardown_chain();else if(a==0x24e640)teardown_backing();else teardown_bank();return 1;}
