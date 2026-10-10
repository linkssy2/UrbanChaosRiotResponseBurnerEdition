#include "migration_audio_group_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_0023FCB0(void);
extern void sub_00244770(void);
extern void sub_00244980(void);
extern void sub_00244670(void);
extern void sub_002470F0(void);
extern void sub_002470C0(void);
extern void sub_00247730(void);
extern void sub_00240620(void);
extern void sub_0024B1A0(void);
extern void sub_00240660(void);
extern void sub_002D57FB(void);
extern void sub_0024CF10(void);
extern void sub_0024BD20(void);
extern void sub_0024B080(void);
extern void sub_0023FCD0(void);
extern void sub_0024E010(void);
extern void sub_0024BD60(void);
extern void sub_0024B120(void);
static int group_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_GROUP");f=v&&!strcmp(v,"1");}return f;}
static int group_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int group_safe(uint32_t p,uint32_t n){return group_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+8<=p);}
static int group_admit(uint32_t a){
    if(a!=0x247730||!group_enabled()||!group_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!group_span(g_esp-4096,4104)||!group_safe(g_ecx,128))return 0;
    if(MEM8(g_ecx+48))return 1;
    uint32_t queue=MEM32(g_ecx+68);if(queue&&!group_safe(queue,12))return 0;
    uint32_t head=g_ecx+24,node=MEM32(head),previous=head;
    /* ponytail: O(n) child-list admission; trusted list generations can replace it if measured. */
    for(unsigned n=0;node!=head;n++){if(n>=4096||node<16||!group_safe(node-16,128)||MEM32(node+4)!=previous)return 0;previous=node;node=MEM32(node);}
    if(MEM32(head+4)!=previous)return 0;
    uint32_t definition=MEM32(g_ecx+120),track=MEM32(g_ecx+124);
    return (!definition||group_safe(definition,8))&&(!track||group_safe(track,104));
}
static void group_volume(void){
    SET_LO16(g_edx,MEM16(g_esi+58));PUSH32(g_esp,g_edi);
    if(LO16(g_edx)){
        g_edi=MEM16(g_esi+56);g_eax=MEM16(g_esi+60);
        if(LO16(g_edi)!=LO16(g_eax)){
            if((int16_t)LO16(g_edi)>(int16_t)LO16(g_eax)){g_ecx=g_edi-g_eax;if((int16_t)LO16(g_ecx)<(int16_t)LO16(g_edx))g_edx=g_ecx;g_edx=0-g_edx;}
            else{g_eax-=g_edi;if((int16_t)LO16(g_eax)<(int16_t)LO16(g_edx))g_edx=g_eax;}
            g_ecx=MEM32(0x4839b0);g_eax=g_edi+g_edx;
            if((int16_t)LO16(g_eax)<(int16_t)LO16(g_ecx))g_eax=g_ecx;
            else{g_ecx=MEM32(0x4839b4);if((int16_t)LO16(g_eax)>(int16_t)LO16(g_ecx))g_eax=g_ecx;}
            MEM16(g_esi+56)=LO16(g_eax);MEM32(g_esi+92)|=4;
        }
    }
}
static void group_update(void){
    uint32_t frame=g_seh_ebp;g_eax=MEM32(0);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39e73b);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=32;PUSH32(g_esp,g_esi);g_esi=g_ecx;SET_LO8(g_eax,MEM8(g_esi+48));
    if(!LO8(g_eax)){
        group_volume();g_eax=MEM32(g_esi+68);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);
        if(g_eax){
            g_ecx=MEM32(g_eax+8);
            if(g_ecx){
                PUSH32(g_esp,0);PUSH32(g_esp,0x2477cc);sub_00241880();g_esp+=4;g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x2477db);sub_0023FCB0();g_edx=MEM32(g_esi+8);g_eax=MEM32(g_esp+32);g_ecx=MEM32(g_esp+36);uint32_t borrow=g_eax<g_edx;g_eax-=g_edx;g_edx=MEM32(g_esi+12);g_ecx-=g_edx+borrow;
                PUSH32(g_esp,g_ecx);MEM32(g_esp+40)=g_ecx;g_ecx=MEM32(g_esi+68);PUSH32(g_esp,g_eax);MEM32(g_esp+40)=g_eax;PUSH32(g_esp,0x2477ff);sub_00244770();g_ecx=MEM32(g_esi+72);MEM32(g_esi+76)=g_eax;
                if(g_ecx){
                    g_edx=MEM32(g_esi+96);
                    if(g_edx){
                        g_edx=g_esp+28;PUSH32(g_esp,g_edx);g_edx=g_esp+20;PUSH32(g_esp,g_edx);g_edx=g_esp+28;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x247825);sub_00244980();g_ecx=MEM32(g_esp+16);
                        if(MEM32(g_esi+84)!=g_ecx){
                            g_edx=MEM32(g_esp+20);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);MEM32(g_esi+80)=g_edx;MEM32(g_esi+84)=g_ecx;uint32_t target=MEM32(g_esi+96);PUSH32(g_esp,0x24783e);RECOMP_ICALL_SAFE(target,saved);
                            frame=MEM32(g_esp+56);g_edi=MEM32(g_esi+76);g_ecx=MEM32(g_esi+68);g_esp+=12;PUSH32(g_esp,1);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x247854);sub_00244670();
                            if(g_eax){PUSH32(g_esp,1);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);g_ecx=g_eax;PUSH32(g_esp,0x247863);sub_002470F0();g_fp_stack[g_fp_top]*=MEMF(0x4febf8);PUSH32(g_esp,g_ecx);MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;PUSH32(g_esp,0x247872);sub_002470C0();MEMF(g_esi+88)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_esp+=4;}
                        }
                    }
                }
            }
        }
        g_ecx=MEM32(g_esi+92);g_edi=MEM32(g_esi+24);g_ebx=MEM32(g_esp+64);frame=g_esi+24;g_ecx>>=1;MEM32(g_esi+92)=g_ecx;
        while(g_edi!=frame){g_ecx=g_edi-16;PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x247899);sub_00247730();g_edi=MEM32(g_edi);}
        g_eax=MEM32(g_esi+120);
        if(g_eax){
            PUSH32(g_esp,0);PUSH32(g_esp,0x2478b1);sub_00241880();g_esp+=4;g_edi=g_eax;PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x2478be);sub_00240620();
            if(g_eax){
                g_ecx=MEM32(g_esi+124);
                if(g_ecx){PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x2478d3);sub_0024B1A0();}
                else{
                    g_eax=MEM32(g_esi+116);
                    if(g_eax){
                        PUSH32(g_esp,1);PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x2478fd);sub_00240660();
                        if(LO8(g_eax)){
                            PUSH32(g_esp,104);PUSH32(g_esp,0x24790c);sub_002D57FB();g_esp+=4;MEM32(g_esp+64)=g_eax;MEM32(g_esp+56)=0;
                            if(g_eax){g_ecx=g_eax;PUSH32(g_esp,0x247926);sub_0024CF10();}else g_eax=0;
                            MEM32(g_esi+124)=g_eax;g_eax=MEM32(g_esi+120);g_ecx=MEM32(g_eax+4);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+116);MEM32(g_esp+64)=0xffffffffu;PUSH32(g_esp,0x247946);sub_0024BD20();g_edx=MEM32(g_esi+116);g_ecx=MEM32(g_esi+124);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x247954);sub_0024B080();
                            g_eax=MEM32(g_esi+124);g_ecx=MEM32(g_eax+52);
                            if(g_ecx){g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x24796e);sub_0023FCD0();g_edx=MEM32(0x483c50);g_eax=g_esp+64;PUSH32(g_esp,g_eax);g_ecx=g_esp+36;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+124);MEM32(g_esp+72)=g_edx;PUSH32(g_esp,0x24798a);sub_0024E010();}
                        }
                    }
                }
            }else{
                g_eax=MEM32(g_esi+124);
                if(g_eax){
                    g_ecx=MEM32(g_esi+116);int retire=g_ecx==0;
                    if(g_ecx){
                        g_eax=MEM32(g_eax+52);g_edx=MEM32(g_esi+120);PUSH32(g_esp,g_eax);g_eax=MEM32(g_edx+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2479c5);sub_0024BD60();
                        if(!g_eax){
                            g_ecx=MEM32(g_esi+120);
                            if(MEM8(g_ecx+3)>1){
                                g_ecx=MEM32(g_esi+124);PUSH32(g_esp,0x2479da);sub_0024B120();g_edx=MEM32(g_esi+120);g_eax=MEM32(g_edx+4);g_ecx=MEM32(g_esi+116);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2479eb);sub_0024BD20();g_ecx=MEM32(g_esi+116);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+124);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2479f9);sub_0024B080();g_edx=MEM32(g_esi+124);g_eax=MEM32(g_edx+52);
                                if(g_eax){g_eax=g_esp+40;PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x247a0f);sub_0023FCD0();g_ecx=MEM32(0x483c50);g_edx=g_esp+24;PUSH32(g_esp,g_edx);g_eax=g_esp+44;MEM32(g_esp+28)=g_ecx;g_ecx=MEM32(g_esi+124);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x247a2b);sub_0024E010();}
                            }
                        }
                        g_eax=MEM32(g_esi+124);g_ebx=MEM32(g_eax+28);g_ecx=g_eax+28;SET_LO8(g_ecx,g_ebx==g_ecx);retire=LO8(g_ecx)!=0;
                    }
                    if(retire&&g_eax){
                        PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x247a4b);sub_00240660();g_ecx=MEM32(g_esi+124);
                        if(!--MEM32(g_ecx+16)){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x247a5a);RECOMP_ICALL_SAFE(target,saved);}MEM32(g_esi+124)=0;
                    }
                }
            }
        }
        POP32(g_esp,frame);POP32(g_esp,g_ebx);POP32(g_esp,g_edi);
    }
    g_ecx=MEM32(g_esp+36);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=44;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_audio_group(uint32_t a){if(!group_admit(a))return 0;group_update();return 1;}
