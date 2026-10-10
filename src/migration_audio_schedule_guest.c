#include "migration_audio_schedule_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_002491A0(void);
extern void sub_002D57FB(void);
extern void sub_00243F90(void);
extern void sub_002473D0(void);
extern void sub_00244E50(void);
extern void sub_00247410(void);
extern void sub_002447D0(void);
extern void sub_00253190(void);
extern void sub_002408F0(void);
extern void sub_00243F40(void);
extern void sub_0023FEA0(void);
static int schedule_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_SCHEDULE");f=v&&!strcmp(v,"1");}return f;}
static int schedule_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int schedule_safe(uint32_t p,uint32_t n){return schedule_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int schedule_admit(uint32_t a){
    if((a!=0x24e010&&a!=0x24e3a0)||!schedule_enabled()||!schedule_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!schedule_span(g_esp-4096,4108)||!schedule_safe(g_ecx,104)||!schedule_safe(MEM32(g_esp+4),8))return 0;
    if(a==0x24e3a0)return schedule_safe(MEM32(g_esp+8),44);
    if(!schedule_safe(MEM32(g_esp+8),4))return 0;
    static const uint32_t targets[]={0x24e1c3,0x24e2c6,0x24e259,0x24e1f2,0x24e21e,0x24e2ce};
    static const unsigned char cases[]={0,1,5,5,5,5,5,5,5,2,3,4};
    if(!schedule_span(0x24e374,36)||memcmp((const void*)XBOX_PTR(0x24e374),targets,sizeof targets)||memcmp((const void*)XBOX_PTR(0x24e38c),cases,sizeof cases))return 0;
    uint32_t definition=MEM32(g_ecx+52);if(!definition)return 1;if(!schedule_safe(definition,4)||MEM16(definition)>255)return 0;
    uint32_t bank=MEM32(g_ecx+48);if(!schedule_safe(bank,156))return 0;
    uint64_t p=(uint64_t)MEM32(bank+152)+12+MEM16(definition+2)+MEM16(g_ecx+76);uint32_t index=MEM8(g_ecx+79),count=MEM16(definition);
    if(index>count)return 0;
    for(;index<count;index++){
        if(p>0x7fffffffu||!schedule_safe((uint32_t)p,28))return 0;
        if(MEM8((uint32_t)p+4)==13&&!(MEM8((uint32_t)p+6)&8)&&!MEM8((uint32_t)p+17))return 0;
        p+=12+MEM8((uint32_t)p+5);
    }
    return 1;
}
static void schedule_construct(uint32_t zero){
    g_ecx=g_eax+28;MEM32(g_eax)=0x483eb0;MEM32(g_eax+8)=zero;MEM32(g_eax+12)=zero;MEM32(g_eax+16)=zero;MEM32(g_eax+20)=zero;MEM32(g_eax+24)=1;MEM32(g_eax+80)=zero;MEM32(g_eax+84)=zero;MEM16(g_eax+88)=LO16(zero);MEM16(g_eax+90)=LO16(zero);MEM32(g_eax+32)=g_ecx;MEM32(g_ecx)=g_ecx;
}
static void schedule_commands(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39e8db);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=20;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=0;PUSH32(g_esp,g_ebx);g_esi=g_ecx;PUSH32(g_esp,0x24e034);sub_00241880();
    MEM32(g_esp+16)=g_eax;g_eax=MEM32(g_esp+48);g_ecx=MEM32(g_eax);MEM32(g_esi+8)=g_ecx;g_ecx=MEM32(g_esi+52);g_edx=MEM32(g_eax+4);g_esp+=4;MEM32(g_esi+12)=g_edx;
    if(g_ecx!=g_ebx){
        SET_LO8(g_eax,MEM8(g_esi+79));g_edx=MEM32(g_esi+48);MEM8(g_esi+78)=LO8(g_eax);SET_LO16(g_eax,MEM16(g_esi+76));MEM16(g_esi+74)=LO16(g_eax);g_ecx=MEM16(g_ecx+2);PUSH32(g_esp,g_edi);g_ecx+=MEM32(g_edx+152);g_eax=LO16(g_eax);g_edi=g_ecx+g_eax+12;g_ecx=MEM32(g_esi+36);MEM32(g_esp+12)=g_ebx;
        if(MEM8(g_esi+73)&2){
            if(g_ecx!=g_ebx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x4fedd4);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x24e093);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esp+12)=g_eax;}g_eax=MEM32(g_edi);MEM32(g_esi+68)=g_eax;
        }else{
            if(g_ecx!=g_ebx){PUSH32(g_esp,0x24e0a7);sub_002491A0();if(g_eax!=g_ebx){g_ecx=MEM32(g_eax+68);MEM32(g_esi+68)=g_ecx;}}MEM8(g_esi+72)&=0xfb;
        }
        SET_LO16(g_edx,MEM8(g_esi+78));g_ecx=MEM32(g_esi+52);
        if(LO16(g_edx)<MEM16(g_ecx)){
            PUSH32(g_esp,frame);
            for(;;){
                g_eax=MEM32(g_edi);
                if(g_eax<MEM32(g_esi+68)){
                    MEM8(g_esi+78)++;SET_LO16(g_eax,MEM8(g_edi+5));g_edi=MEM32(g_esi+48);g_eax+=12;MEM16(g_esi+74)+=LO16(g_eax);SET_LO16(g_eax,MEM16(g_esi+74));g_edx=MEM16(g_ecx+2);frame=MEM32(g_edi+152);g_eax=LO16(g_eax);g_edx+=frame;g_edi=g_edx+g_eax+12;
                }else{
                    g_ecx=MEM32(g_esp+56);if(g_eax>=MEM32(g_ecx))break;
                    PUSH32(g_esp,0x60);PUSH32(g_esp,0x24e10f);sub_002D57FB();g_esp+=4;if(g_eax!=g_ebx){schedule_construct(g_ebx);frame=g_eax;}else frame=0;
                    MEM32(frame+80)=g_esi;MEM32(frame+84)=g_edi;g_edx=MEM32(g_edi);g_eax=MEM32(g_esi+68);g_ecx=MEM32(g_esi+8);g_edx-=g_eax;g_eax=0;uint64_t time=(uint64_t)g_edx+g_ecx;g_edx=(uint32_t)time;g_eax=MEM32(g_esi+12)+(uint32_t)(time>>32);MEM32(g_esp+28)=g_edx;MEM32(g_esp+32)=g_eax;MEM8(g_esp+52)=1;
                    if(MEM8(g_edi+6)&8){SET_LO8(g_eax,MEM8(g_edi+4));MEM8(g_esp+52)=0;if(LO8(g_eax)>=1&&LO8(g_eax)<=3){g_ecx=0;SET_LO16(g_ecx,MEM16(g_esi+72));g_ecx=(g_ecx&0xefff)|0x24;MEM16(g_esi+72)=LO16(g_ecx);}}
                    else{
                        g_eax=(uint32_t)MEM8(g_edi+4)-2;
                        if(g_eax<=11){g_edx=MEM8(g_eax+0x24e38c);
                            switch(g_edx){
                            case 0:
                                if(MEM32(g_edi+24)!=g_ebx){PUSH32(g_esp,0x24e1d1);sub_00243F90();g_eax=LO16(g_eax);g_edx=0;g_edx=g_eax%MEM32(g_edi+24);g_eax/=MEM32(g_edi+24);g_ecx=MEM32(g_esp+28);g_eax=MEM32(g_esp+32);time=(uint64_t)g_ecx+g_edx;g_ecx=(uint32_t)time;g_eax+=g_ebx+(uint32_t)(time>>32);MEM32(g_esp+28)=g_ecx;MEM32(g_esp+32)=g_eax;}break;
                            case 1:SET_LO16(g_eax,MEM16(g_edi+8));MEM16(g_esi+80)=LO16(g_eax);break;
                            case 2:
                                g_eax=MEM32(g_edi+16)-1;g_ebx=g_edi+12;
                                if(!g_eax){SET_LO8(g_eax,MEM8(g_esi+78));SET_LO16(g_ecx,MEM16(g_esi+74));MEM8(g_esi+79)=LO8(g_eax);MEM16(g_esi+76)=LO16(g_ecx);}
                                else{g_eax--;if(!g_eax&&MEM16(g_esi+80)!=1){g_ecx=MEM32(g_edi);g_edx=MEM32(g_esp+56);MEM32(g_edx)=g_ecx;}}
                                PUSH32(g_esp,0x3c);PUSH32(g_esp,0x24e28b);sub_002D57FB();g_esp+=4;MEM32(g_esp+24)=g_eax;MEM32(g_esp+44)=0;
                                if(g_eax){PUSH32(g_esp,g_esi);g_edx=g_esp+32;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ebx);g_ecx=g_eax;PUSH32(g_esp,0x24e2ac);sub_00253190();}else g_eax=0;
                                g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_eax);MEM32(g_esp+48)=0xffffffff;PUSH32(g_esp,0x24e2c2);sub_002408F0();g_ebx=0;break;
                            case 3:
                                g_ecx=MEM32(g_esp+16);if(g_ecx!=g_ebx){PUSH32(g_esp,0x24e1ff);sub_002473D0();if(g_eax!=g_ebx){g_ecx=MEM32(g_edi+12);g_edx=MEM32(g_edi+16);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x24e214);sub_00244E50();}}MEM8(g_esp+52)=0;break;
                            case 4:
                                g_ecx=MEM32(g_esp+16);if(g_ecx!=g_ebx){PUSH32(g_esp,0x24e22b);sub_00247410();g_ecx=g_eax;if(g_ecx!=g_ebx){g_ebx=MEM8(g_edi+17);g_eax=0xc00;g_edx=0;g_edx=g_eax%g_ebx;g_eax/=g_ebx;g_edx=MEM8(g_edi+16);g_edx*=g_eax;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_eax=MEM32(g_edi+12);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x24e250);sub_002447D0();g_ebx=0;}}MEM8(g_esp+52)=0;break;
                            default:break;
                            }
                        }
                    }
                    MEM8(g_esi+78)++;SET_LO16(g_ecx,MEM8(g_edi+5));SET_LO8(g_eax,MEM8(g_esp+52));g_ecx+=12;MEM16(g_esi+74)+=LO16(g_ecx);
                    if(LO8(g_eax)){
                        g_edx=frame+28;PUSH32(g_esp,g_edx);g_eax=g_esi+28;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24e2f2);sub_00243F40();g_ecx=MEM32(g_esp+36);MEM32(frame+8)=g_ecx;g_edx=MEM32(g_esp+40);g_ecx=MEM32(g_esp+28);g_esp+=8;PUSH32(g_esp,frame);MEM32(frame+12)=g_edx;PUSH32(g_esp,0x24e30d);sub_0023FEA0();
                        if(!LO8(g_eax)&&!--MEM32(frame+24)){g_eax=MEM32(frame);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=frame;uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24e320);RECOMP_ICALL_SAFE(target,saved);}
                    }else if(!--MEM32(frame+24)){g_edx=MEM32(frame);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=frame;uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x24e331);RECOMP_ICALL_SAFE(target,saved);}
                    g_ecx=MEM32(g_esi+52);g_edx=MEM32(g_esi+48);g_edi=MEM32(g_edx+152);g_eax=MEM16(g_ecx+2);g_edx=MEM16(g_esi+74);g_eax+=g_edi;g_edi=g_eax+g_edx+12;
                }
                SET_LO16(g_eax,MEM8(g_esi+78));g_edx=MEM32(g_esi+52);if(LO16(g_eax)>=MEM16(g_edx))break;
            }
            POP32(g_esp,frame);
        }
        POP32(g_esp,g_edi);
    }
    g_ecx=MEM32(g_esp+28);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=32;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void schedule_copy(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,0x60);MEM32(g_esp+12)=g_ecx;PUSH32(g_esp,0x24e3ae);sub_002D57FB();frame=0;g_esp+=4;if(g_eax!=frame){schedule_construct(frame);g_ebx=g_eax;}else g_ebx=0;
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+24);MEM8(g_esi+6)|=1;PUSH32(g_esp,g_edi);g_eax=g_ebx+36;g_edi=g_eax;g_ecx=11;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
    g_ecx=MEM32(g_esp+24);g_edx=MEM32(g_ecx);MEM32(g_ebx+8)=g_edx;g_ecx=MEM32(g_ecx+4);MEM32(g_ebx+12)=g_ecx;g_ecx=MEM32(g_esp+16);g_edx=g_ebx+28;MEM32(g_ebx+80)=g_ecx;PUSH32(g_esp,g_edx);g_ecx+=28;PUSH32(g_esp,g_ecx);MEM32(g_ebx+84)=g_eax;PUSH32(g_esp,0x24e428);sub_00243F40();g_esp+=8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,0x24e432);sub_00241880();g_esp+=4;g_ecx=g_eax;PUSH32(g_esp,0x24e43c);sub_0023FEA0();int accepted=LO8(g_eax)!=0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);
    if(!accepted&&!--MEM32(g_ebx+24)){g_eax=MEM32(g_ebx);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_ebx;uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24e450);RECOMP_ICALL_SAFE(target,saved);}POP32(g_esp,frame);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_esp+=12;
}
int uc_migration_audio_schedule(uint32_t a){if(!schedule_admit(a))return 0;if(a==0x24e010)schedule_commands();else schedule_copy();return 1;}
