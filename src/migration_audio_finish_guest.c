#include "migration_audio_finish_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023FCB0(void);
extern void sub_0023FD40(void);
extern void sub_0023FD60(void);
extern void sub_0023FE80(void);
extern void sub_0023FF00(void);
extern void sub_00240810(void);
extern void sub_002408D0(void);
extern void sub_00240950(void);
extern void sub_00241880(void);
extern void sub_00243F60(void);
extern void sub_002456A0(void);
extern void sub_00245810(void);
extern void sub_00246360(void);
extern void sub_002471A0(void);
extern void sub_00247450(void);
extern void sub_00247680(void);
extern void sub_00248600(void);
extern void sub_00248960(void);
extern void sub_00248D10(void);
extern void sub_00248D40(void);
extern void sub_00249670(void);
extern void sub_0024A260(void);
extern void sub_0024A500(void);
extern void sub_0024B120(void);
extern void sub_0024B1A0(void);
extern void sub_0024B950(void);
extern void sub_0024C250(void);
extern void sub_00252CF0(void);
extern void sub_00253140(void);
extern void sub_002D57FB(void);
static int finish_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_FINISH");f=v&&!strcmp(v,"1");}return f;}
static int finish_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int finish_ring(uint32_t head,int tracks){
    /* ponytail: bounded O(n) preflight; measure before replacing with a broader admission. */
    uint32_t p=MEM32(head),previous=head;
    for(unsigned n=0;p!=head;n++,p=MEM32(p)){if(n==4096||!finish_span(p,8)||MEM32(p+4)!=previous||(tracks&&!finish_span(p-20,0x64)))return 0;previous=p;}
    return MEM32(head+4)==previous;
}
static int finish_admit(uint32_t a){
    if(!finish_enabled()||!finish_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x10200||!finish_span(g_esp-512,524)||!finish_span(g_ecx,0x70))return 0;
    if(!((uint64_t)g_ecx+0x70<=g_esp-512||(uint64_t)g_esp+12<=g_ecx))return 0;
    if(a==0x24b120)return 1;
    if(a==0x24a6f0)return finish_ring(g_ecx+40,1);
    if(a==0x24a500){for(unsigned offset=8;offset<=48;offset+=8)if(!finish_ring(g_ecx+offset,offset==40))return 0;return 1;}
    return 0;
}
static void finish_track(void){
    PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0);g_esi=g_ecx;PUSH32(g_esp,0x24b12b);sub_00241880();g_esp+=4;g_edi=g_eax;PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x24b138);sub_0023FF00();PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x24b144);sub_00240950();
    g_ecx=MEM32(g_esi+64);
    if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0x24b152);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+64);g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+8);PUSH32(g_esp,0x24b15a);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+64)=0;}
    g_ecx=MEM32(g_esi+36);
    if(g_ecx&&(MEM8(g_esi+73)&2)){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fedd4);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x24b179);RECOMP_ICALL_SAFE(target,saved);if(g_eax){PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x24b185);sub_00247450();}}
    g_ecx=MEM32(g_esi+44);int empty=g_ecx==0;
    if(!empty){g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x24b191);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+44)=0;}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=empty;g_esp+=4;
}
static void finish_instance(void){
    uint32_t engine=g_seh_ebp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e83b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=16;PUSH32(g_esp,g_ebx);PUSH32(g_esp,engine);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_ebx=0;PUSH32(g_esp,g_ebx);g_esi=g_ecx;PUSH32(g_esp,0x24a526);sub_00241880();engine=g_eax;g_eax=MEM32(g_esi);g_esp+=4;g_ecx=g_esi;
    {uint32_t saved=g_esp,target=MEM32(g_eax+4);PUSH32(g_esp,0x24a532);RECOMP_ICALL_SAFE(target,saved);}
    g_ecx=MEM32(g_esi+60);int bank=g_ecx!=g_ebx;MEM32(g_esp+16)=g_ebx;
    if(bank){g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+32);PUSH32(g_esp,0x24a546);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fed94);bank=g_ecx==MEM32(g_eax);}
    if(bank){g_ecx=MEM32(g_esi+60);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x4fed94);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x24a561);RECOMP_ICALL_SAFE(target,saved);bank=g_eax!=g_ebx;MEM32(g_esp+16)=g_eax;}
    if(bank){
        g_eax=MEM32(g_esi+84);g_ecx=MEM32(g_esp+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,2);PUSH32(g_esp,0x24a57c);sub_0024B950();g_edi=g_eax;
        if(g_edi!=g_ebx){
            if(MEM8(g_edi+44)&1){PUSH32(g_esp,60);PUSH32(g_esp,0x24a58f);sub_002D57FB();g_esp+=4;MEM32(g_esp+20)=g_eax;MEM32(g_esp+40)=g_ebx;
                if(g_eax!=g_ebx){PUSH32(g_esp,g_edi);g_ecx=g_eax;PUSH32(g_esp,0x24a5a6);sub_00253140();MEM32(g_esp+40)=0xffffffffu;g_edi=g_eax;}
                else{g_eax=0;MEM32(g_esp+40)=0xffffffffu;g_edi=g_eax;}
            }else{g_ecx=g_edi+4;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x24a5c9);sub_00243F60();g_esp+=4;}
            g_edx=g_esp+24;PUSH32(g_esp,g_edx);g_ecx=engine;PUSH32(g_esp,0x24a5d8);sub_0023FCB0();g_eax=g_esp+24;PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x24a5e4);sub_00248600();PUSH32(g_esp,g_edi);g_ecx=engine;PUSH32(g_esp,0x24a5ec);sub_002408D0();MEM32(g_edi+12)--;
            if(!MEM32(g_edi+12)){g_edx=MEM32(g_edi);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_edi;uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x24a5fa);RECOMP_ICALL_SAFE(target,saved);}
        }
    }
    g_ecx=MEM32(g_esi+76);
    if(g_ecx!=g_ebx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x24a606);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+76)=g_ebx;}
    g_edi=MEM32(g_esi+40);g_ebx=g_esi+40;
    while(g_edi!=g_ebx){g_ecx=g_edi-20;g_edi=MEM32(g_edi);PUSH32(g_esp,0x24a61d);sub_0024B120();}
    PUSH32(g_esp,g_esi);g_ecx=engine;PUSH32(g_esp,0x24a629);sub_00240810();g_ecx=MEM32(g_esi+48);g_eax=g_esi+48;
    if(g_ecx!=g_eax){g_ecx=MEM32(g_esi+68);if(g_ecx){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x24a640);sub_002456A0();}}
    g_ecx=MEM32(g_esi+24);g_eax=g_esi+24;
    if(g_ecx!=g_eax){PUSH32(g_esp,g_esi);g_ecx=engine;PUSH32(g_esp,0x24a652);sub_0023FE80();}
    g_ecx=MEM32(g_esi+8);g_eax=g_esi+8;
    if(g_ecx!=g_eax){PUSH32(g_esp,g_esi);g_ecx=engine;PUSH32(g_esp,0x24a664);sub_0023FD40();}
    else{g_eax=MEM32(g_esi+80);if(g_eax&&MEM16(g_eax+14)!=0xffff){g_ecx=g_esi;PUSH32(g_esp,0x24a67c);sub_00248960();PUSH32(g_esp,g_eax);g_ecx=engine;PUSH32(g_esp,0x24a684);sub_0023FD60();if(g_eax){PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_eax;PUSH32(g_esp,0x24a695);sub_0024A260();}}}
    g_ecx=MEM32(g_esi+32);g_eax=g_esi+32;
    if(g_ecx!=g_eax){g_ecx=MEM32(g_esp+16);if(g_ecx){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x24a6ad);sub_0024C250();}}
    g_ecx=MEM32(g_esi+72);g_edi=0;MEM32(g_esi+104)=8;
    if(g_ecx!=g_edi){PUSH32(g_esp,0x24a6c2);sub_002471A0();MEM32(g_esi+72)=g_edi;}
    g_ecx=MEM32(g_esi+68);
    if(g_ecx!=g_edi){g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x24a6d1);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+68)=g_edi;}
    g_eax=MEM32(g_esi);g_ecx=g_esi;
    {uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x24a6db);RECOMP_ICALL_SAFE(target,saved);}
    g_ecx=MEM32(g_esp+32);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,engine);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void finish_state(uint32_t pc,int use_edx){
    if(use_edx)g_edx=MEM32(g_esi);else g_eax=MEM32(g_esi);g_ecx=g_esi;
    uint32_t saved=g_esp,target=MEM32((use_edx?g_edx:g_eax)+24);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);
}
static void finish_update(void){
    uint32_t voice=g_seh_ebp;g_esp-=8;
    {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);uint32_t target=MEM32(g_eax+24);PUSH32(g_esp,0x24a6fb);RECOMP_ICALL_SAFE(target,saved);}int active=g_eax!=0;
    if(active){finish_state(0x24a70a,1);active=g_eax!=1;}if(active){finish_state(0x24a71a,0);active=g_eax!=8;}if(active){finish_state(0x24a72a,1);active=g_eax!=6;}
    if(active){
        g_ecx=MEM32(g_esi+68);PUSH32(g_esp,g_edi);g_edi=0;if(g_ecx!=g_edi){g_eax=MEM32(g_esp+20);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24a747);sub_00246360();}
        g_eax=g_esi+40;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_eax);MEM32(g_esp+12)=g_edi;MEM32(g_esp+16)=g_edi;int complete=g_ebx==g_eax;
        if(!complete){
            PUSH32(g_esp,voice);
            do{
                g_edx=MEM32(g_esp+20);SET_LO16(g_eax,MEM16(g_ebx+52));g_edi=g_ebx-20;g_ebx=MEM32(g_ebx);g_edx++;MEM32(g_esp+20)=g_edx;
                if(HI8(g_eax)&0x22){
                    if(HI8(g_eax)&0x20){g_ecx=MEM32(g_esp+28);PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x24a788);sub_0024B1A0();}
                    if(!(MEM8(g_edi+72)&0x20))MEM32(g_esp+16)++;
                    else{finish_state(0x24a79e,1);if((int32_t)g_eax>=7){g_ecx=MEM32(g_esp+16)+1;MEM8(g_edi+72)&=0xdf;MEM32(g_esp+16)=g_ecx;}}
                }else{
                    g_eax=MEM32(g_edi);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed44);g_ecx=g_edi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x24a7c6);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_esi);g_ecx=g_esi;voice=g_eax;saved=g_esp;target=MEM32(g_edx+24);PUSH32(g_esp,0x24a7cf);RECOMP_ICALL_SAFE(target,saved);
                    int inspect=g_eax==7;
                    if(!inspect&&(MEM8(g_edi+72)&4)){
                        finish_state(0x24a7e9,0);int running=g_eax!=6;
                        if(running){finish_state(0x24a7f9,1);running=g_eax!=5;}
                        if(running){
                            finish_state(0x24a809,0);int waiting=g_eax==3;
                            if(!waiting){finish_state(0x24a815,1);waiting=g_eax==4;}
                            if(waiting){
                                g_eax=MEM32(g_esi+100);
                                if(!(HI8(g_eax)&0x10)){
                                    if(HI8(g_eax)&0x20){g_eax&=~0x2000u;MEM32(g_esi+100)=g_eax;g_eax=MEM32(g_esi+104);if(g_eax!=2&&((int32_t)g_eax<=6||(int32_t)g_eax>8))MEM32(g_esi+104)=2;}
                                    else if(HI8(g_eax)&0x40){g_eax&=~0x4000u;MEM32(g_esi+100)=g_eax;g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);g_ecx=g_esi;target=MEM32(g_eax+20);PUSH32(g_esp,0x24a860);RECOMP_ICALL_SAFE(target,saved);}
                                }
                            }inspect=1;
                        }
                    }
                    if(inspect){
                        int done=!voice;
                        if(voice){if(MEM32(voice+68)&0x40000000){g_ecx=MEM32(voice+60);if(g_ecx){PUSH32(g_esp,0x24a879);sub_00252CF0();}}g_ecx=voice;PUSH32(g_esp,0x24a880);sub_00245810();done=!LO8(g_eax);}
                        if(done){g_eax=MEM32(g_esp+16)+1;g_ecx=g_edi;MEM32(g_esp+16)=g_eax;PUSH32(g_esp,0x24a894);sub_0024B120();}
                    }
                }
                g_eax=g_esi+40;
            }while(g_ebx!=g_eax);
            g_ecx=MEM32(g_esp+20);complete=MEM32(g_esp+16)==g_ecx;POP32(g_esp,voice);
        }
        if(complete){g_ecx=g_esi;PUSH32(g_esp,0x24a8f4);sub_0024A500();}
        else{
            PUSH32(g_esp,0);PUSH32(g_esp,0x24a8b1);sub_00241880();g_ecx=MEM32(g_esi+72);g_esp+=4;g_edi=g_eax;int dirty=0;
            if(g_ecx){PUSH32(g_esp,0x24a8c2);sub_00247680();dirty=LO8(g_eax)!=0;}
            if(!dirty)dirty=(MEM8(g_edi+180)&2)!=0;
            if(dirty){g_ecx=g_esi;PUSH32(g_esp,0x24a8d6);sub_00248D10();g_ecx=g_esi;PUSH32(g_esp,0x24a8dd);sub_00248D40();g_ecx=g_esi;PUSH32(g_esp,0x24a8e4);sub_00249670();}
        }POP32(g_esp,g_ebx);POP32(g_esp,g_edi);
    }POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_audio_finish(uint32_t a){if(!finish_admit(a))return 0;if(a==0x24a500)finish_instance();else if(a==0x24b120)finish_track();else finish_update();return 1;}
