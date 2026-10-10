#include "migration_audio_loops_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243F60(void);
extern void sub_00243F40(void);
extern void sub_0024A110(void);
extern void sub_00249040(void);
extern void sub_0024B4C0(void);
extern void sub_00248C50(void);
extern void sub_002495C0(void);
extern void sub_00248B80(void);
extern void sub_0024B600(void);
extern void sub_0024BDA0(void);
extern void sub_0024B670(void);
extern void sub_0024B690(void);
extern void sub_00249960(void);
extern void sub_00248A30(void);
extern void sub_0024A260(void);
extern void sub_00247490(void);
extern void sub_00241880(void);
extern void sub_00240950(void);
extern void sub_002530B0(void);
extern void sub_00252F90(void);
static int loops_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_LOOPS");enabled=v&&!strcmp(v,"1");}return enabled;}
static int loops_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int loops_safe(uint32_t p,uint32_t n){return loops_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+16<=p);}
static int loops_tracks(uint32_t owner){
    if(!loops_safe(owner,104))return 0;uint32_t head=owner+40,p=MEM32(head),previous=head,count=0;
    while(p!=head){if(++count>65535||p<20||!loops_safe(p-20,96)||MEM32(p+4)!=previous)return 0;previous=p;p=MEM32(p);}return MEM32(head+4)==previous;
}
static int loops_admit(uint32_t a){
    if(!loops_enabled()||!loops_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!loops_span(g_esp-4096,4112))return 0;
    uint32_t q=g_ecx,arg=MEM32(g_esp+4);
    if(a==0x247140)return loops_safe(q,32)&&loops_safe(MEM32(q),8)&&loops_safe(MEM32(q+4),8);
    if(a==0x247150||a==0x247170)return loops_safe(q,32)&&(!MEM32(q+28)||loops_tracks(MEM32(q+28)));
    if(a==0x249040)return loops_tracks(q);
    if(a==0x24b4c0)return loops_safe(q,96)&&(!MEM32(q+36)||loops_safe(MEM32(q+36),104));
    if(a==0x24a110){if(!loops_safe(q,104)||!loops_safe(arg,32)||!loops_safe(MEM32(q+60),4))return 0;return loops_span(MEM32(MEM32(q+60)),4);}
    if(a==0x24b350){if(!loops_safe(q,104)||!loops_safe(arg,96)||!loops_safe(MEM32(arg+84),28)||!loops_safe(MEM32(q+36),104))return 0;uint32_t parent=MEM32(q+36);return loops_span(MEM32(parent),28)&&(!MEM32(parent+72)||loops_safe(MEM32(parent+72),16));}
    if(a==0x2408f0)return loops_safe(q,96)&&loops_safe(arg,64)&&loops_safe(MEM32(q+92),8);
    if(a!=0x240950||!loops_safe(q,96))return 0;
    uint32_t when=MEM32(g_esp+8),definition=MEM32(g_esp+12);if((when&&!loops_safe(when,8))||(definition&&!loops_safe(definition,12)))return 0;
    uint32_t head=q+88,p=MEM32(head),previous=head,count=0;
    /* ponytail: bounded full notification-list admission is O(n); share the invariant once every publisher is native. */
    while(p!=head){if(++count>65535||p<4||!loops_safe(p-4,64)||MEM32(p+4)!=previous||MEM32(p+8)<2||!loops_span(MEM32(p-4),20))return 0;previous=p;p=MEM32(p);}return MEM32(head+4)==previous;
}
static void loops_destroy(void){PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x247146);sub_00243F60();POP32(g_esp,g_ecx);g_esp+=4;}
static void loops_tick(void){PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM16(g_esi+14);if(MEM32(g_esi+8)<g_eax){g_ecx=MEM32(g_esi+28);if(g_ecx){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x247169);sub_0024A110();}MEM32(g_esi+8)++;}POP32(g_esp,g_esi);g_esp+=4;}
static void loops_finished(void){PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+28);if(g_ecx){PUSH32(g_esp,0x24717f);sub_00249040();}g_eax=MEM16(g_esi+14);g_edx=MEM32(g_esi+8);g_ecx=0;int equal=g_edx==g_eax;SET_LO8(g_ecx,equal);SET_LO8(g_eax,LO8(g_ecx));POP32(g_esp,g_esi);g_recomp_zf=equal;g_esp+=4;}
static void loops_apply_pending(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+36);
    if(g_ecx){SET_LO16(g_eax,MEM16(g_esi+90));if(LO16(g_eax)!=0x7fff){PUSH32(g_esp,g_esi);MEM16(g_esi+86)=LO16(g_eax);PUSH32(g_esp,0x24b4de);sub_00248C50();MEM16(g_esi+90)=0x7fff;}
        SET_LO16(g_eax,MEM16(g_esi+88));if(LO16(g_eax)!=0x7fff){g_ecx=MEM32(g_esi+36);PUSH32(g_esp,g_esi);MEM16(g_esi+84)=LO16(g_eax);PUSH32(g_esp,0x24b4fb);sub_002495C0();MEM16(g_esi+88)=0x7fff;}
        SET_LO16(g_eax,MEM16(g_esi+92));if(LO16(g_eax)!=0x7fff){g_ecx=MEM32(g_esi+36);PUSH32(g_esp,0);PUSH32(g_esp,g_esi);MEM16(g_esi+94)=LO16(g_eax);PUSH32(g_esp,0x24b51a);sub_00248B80();MEM16(g_esi+92)=0x7fff;}}
    POP32(g_esp,g_esi);g_esp+=4;
}
static void loops_flush(void){PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+40);PUSH32(g_esp,g_edi);g_edi=g_ecx+40;while(g_esi!=g_edi){g_ecx=g_esi-20;g_esi=MEM32(g_esi);PUSH32(g_esp,0x24905a);sub_0024B4C0();}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=4;}
static void loops_trigger(void){
    uint32_t frame=g_seh_ebp,saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_ebx=g_ecx;g_ecx=MEM32(g_ebx+60);g_eax=MEM32(g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,0x4fed94);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x24a125);RECOMP_ICALL_SAFE(target,saved);
    g_ecx=MEM32(g_ebx+84);g_edi=g_eax;PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x24a132);sub_0024B600();g_edx=MEM32(g_esp+24);g_ecx=MEM32(g_edx+24);g_edx=MEM16(g_eax+2);PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24a148);sub_0024BDA0();frame=g_eax;g_eax=MEM32(frame+4);PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x24a155);sub_0024B670();g_esi=g_eax;SET_LO8(g_eax,MEM8(frame+3));MEM32(g_esp+16)=0;
    if(LO8(g_eax)){do{g_ecx=MEM32(g_esi+8);PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x24a171);sub_0024B690();g_edx=MEM32(g_esp+24);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edx);g_ecx=g_ebx;PUSH32(g_esp,0x24a17f);sub_00249960();g_eax=MEM32(g_esp+16);g_ecx=MEM8(frame+3);g_eax++;g_esi+=12;MEM32(g_esp+16)=g_eax;}while(g_eax<g_ecx);}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_esp+=8;
}
static void loops_publish(void){g_eax=MEM32(g_esp+4);MEM32(g_eax+12)++;g_eax+=4;PUSH32(g_esp,g_eax);g_ecx+=88;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x240904);sub_00243F40();g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;}
static void loops_event(void){
    uint32_t frame=g_seh_ebp,saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+12);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+36);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edi);g_edi=MEM32(frame+84);g_edi+=12;SET_LO8(g_ebx,0);uint32_t target=MEM32(g_eax+24);PUSH32(g_esp,0x24b36a);RECOMP_ICALL_SAFE(target,saved);
    int stopped=g_eax==7;if(!stopped){g_ecx=MEM32(g_esi+36);g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+24);PUSH32(g_esp,0x24b377);RECOMP_ICALL_SAFE(target,saved);stopped=g_eax==4;}
    if(stopped){g_eax=MEM32(g_esi+36);if(MEM8(g_eax+98)&0x20)MEM16(g_esi+72)&=0xfbdf;SET_LO8(g_ebx,1);}
    g_eax=MEM32(g_edi+4);g_eax--;
    if(!g_eax){if(!LO8(g_ebx)){g_ecx=MEM32(g_esi+36);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fedd4);target=MEM32(g_edx);PUSH32(g_esp,0x24b3e2);RECOMP_ICALL_SAFE(target,saved);if(g_eax){PUSH32(g_esp,0x11030);PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x24b3f3);sub_00247490();}g_eax=MEM32(g_esi+36);g_eax=MEM32(g_eax+72);if(g_eax){g_ecx=MEM32(frame+8);MEM32(g_eax+8)=g_ecx;g_edx=MEM32(frame+12);MEM32(g_eax+12)=g_edx;}}}
    else{g_eax--;if(!g_eax){if(LO8(g_ebx))MEM8(g_esi+72)&=0xdf;else{g_eax=MEM16(g_esi+80);if(LO16(g_eax)!=1){if(LO16(g_eax)>1){g_eax--;MEM16(g_esi+80)=LO16(g_eax);}g_ecx=MEM32(g_esi+36);PUSH32(g_esp,30);PUSH32(g_esp,0x24b3c0);sub_00248A30();g_ecx=MEM32(g_esi+36);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x24b3ce);sub_0024A260();}}}
        else{g_eax--;if(!g_eax)MEM8(g_esi+72)&=0xdf;}}
    PUSH32(g_esp,g_edi);frame+=8;PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,0);PUSH32(g_esp,0x24b416);sub_00241880();g_esp+=4;g_ecx=g_eax;PUSH32(g_esp,0x24b420);sub_00240950();POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=g_esp==0;g_esp+=8;
}
static void loops_remove_notifications(void){
    uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff8u;g_esp-=20;g_eax=MEM32(frame+12);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=g_ecx;int filtered=0;
    if(g_eax){g_ecx=MEM32(frame+16);filtered=g_ecx!=0;}
    if(filtered){PUSH32(g_esp,g_eax);g_eax=g_esp+20;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x24097f);sub_002530B0();g_edi=MEM32(g_esi+88);g_ebx=g_esi+88;g_esp+=8;}
    else{g_edi=MEM32(g_esi+88);g_ebx=g_esi+88;}
    while(g_edi!=g_ebx){g_ecx=MEM32(frame+8);g_eax=MEM32(g_edi+16);int match=g_eax==g_ecx;g_esi=g_edi-4;g_edi=MEM32(g_edi);
        if(match&&filtered){g_eax=MEM32(g_esi);g_edx=g_esp+16;uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);g_ecx=g_esi;uint32_t target=MEM32(g_eax+16);PUSH32(g_esp,0x2409b0);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_eax;g_ebp=frame;PUSH32(g_esp,0x2409b7);sub_00252F90();match=LO8(g_eax)!=0;
            if(match){g_eax=MEM32(frame+16);g_edx=MEM32(g_esi+48);match=g_edx==MEM32(g_eax);if(match){g_ecx=MEM32(g_esi+52);match=g_ecx==MEM32(g_eax+4);if(match){g_edx=MEM32(g_esi+56);match=g_edx==MEM32(g_eax+8);}}}}
        if(match){if(filtered){g_eax=g_esi+4;PUSH32(g_esp,g_eax);}else{g_edx=g_esi+4;PUSH32(g_esp,g_edx);}g_ebp=frame;PUSH32(g_esp,filtered?0x2409de:0x240a30);sub_00243F60();g_eax=MEM32(g_esi+12);g_esp+=4;g_eax--;MEM32(g_esi+12)=g_eax;
            if(!g_eax){if(filtered)g_edx=MEM32(g_esi);else g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32((filtered?g_edx:g_eax)+4);PUSH32(g_esp,filtered?0x2409f3:0x240a45);RECOMP_ICALL_SAFE(target,saved);}
            if(!--MEM32(g_esi+12)){if(filtered)g_eax=MEM32(g_esi);else g_edx=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32((filtered?g_eax:g_edx)+4);PUSH32(g_esp,filtered?0x240a01:0x240a53);RECOMP_ICALL_SAFE(target,saved);}}
    }POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=16;
}
int uc_migration_audio_loops(uint32_t address){if(!loops_admit(address))return 0;switch(address){case 0x247140:loops_destroy();break;case 0x247150:loops_tick();break;case 0x247170:loops_finished();break;case 0x249040:loops_flush();break;case 0x24b4c0:loops_apply_pending();break;case 0x24a110:loops_trigger();break;case 0x24b350:loops_event();break;case 0x2408f0:loops_publish();break;case 0x240950:loops_remove_notifications();break;}return 1;}
