#include "migration_audio_selector_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243F90(void);
extern void sub_00244010(void);
extern void sub_0024BE60(void);
static int selector_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_SELECTOR");f=v&&!strcmp(v,"1");}return f;}
static int selector_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int selector_admit(uint32_t a){
    if((a!=0x24be60&&a!=0x24c160)||!selector_enabled()||!selector_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!selector_span(g_esp-4096,4108)||!selector_span(g_ecx,0xa8))return 0;
    if(!((uint64_t)g_ecx+0xa8<=g_esp-4096||(uint64_t)g_esp+12<=g_ecx))return 0;
    uint32_t index=MEM32(g_esp+4),mode=MEM32(g_esp+8);
    if(a==0x24c160){if(!selector_span(mode,12))return 0;index=MEM32(mode+8);mode=0;}
    if(index==0xffffffffu)return 1;if(index>4096)return 0;
    uint64_t p=(uint64_t)MEM32(g_ecx+156)+12;
    for(uint32_t j=0;j<=index;j++){if(p>0x7fffffffu||!selector_span((uint32_t)p,8))return 0;if(j==index)break;p+=8+(uint64_t)MEM32((uint32_t)p+4);}
    p+=8;if(p>0x7fffffffu||!selector_span((uint32_t)p,4))return 0;uint32_t count=MEM32((uint32_t)p)&0x7fff;
    if(!selector_span((uint32_t)p,4+count*12)||!(p+4+count*12<=g_esp-4096||(uint64_t)g_esp+12<=p))return 0;
    if(mode==5&&count>1&&(MEM32((uint32_t)p)>>17)>=count)return 0;
    if(mode==4&&count>1){int available=0;for(uint32_t j=0;j<count;j++)available|=MEM32((uint32_t)p+8+j*12)==0xffffffffu;if(!available)return 0;}
    return 1;
}
static void selector_finish(uint32_t frame,int zf){POP32(g_esp,g_esi);POP32(g_esp,g_ebx);POP32(g_esp,g_edi);POP32(g_esp,frame);if(zf>=0)g_recomp_zf=zf;g_esp+=12;}
static void selector_choose(void){
    uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_ecx;g_ecx=MEM32(g_esp+8);PUSH32(g_esp,g_edi);
    if(g_ecx==0xffffffffu)g_edi=0;else{g_eax=MEM32(frame+156)+12;while(g_ecx){g_ecx--;g_edx=MEM32(g_eax+4);g_eax+=g_edx+8;}g_edi=g_eax+8;}
    g_eax=MEM32(frame+164);if(g_eax){g_ecx=g_eax;g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24bea0);RECOMP_ICALL_SAFE(target,saved);}
    if(!g_edi){POP32(g_esp,g_edi);g_eax=0;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=12;return;}
    PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+20);PUSH32(g_esp,g_esi);g_esi=MEM32(g_edi)&0x7fff;g_ecx=g_edi+4;MEM32(g_esp+20)=g_ecx;
    if(g_ebx==2){
        g_eax=MEM32(frame+160);
        if(g_eax){g_ecx=g_eax;g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x24beda);RECOMP_ICALL_SAFE(target,saved);g_ebx=MEM32(g_esp+20);g_edi=0;
            if(!g_esi){g_eax=g_ebx;selector_finish(frame,-1);return;}
            g_ecx=g_ebx+10;while(g_edi<g_esi){if(LO16(g_eax)>=MEM16(g_ecx-2)&&LO16(g_eax)<=MEM16(g_ecx))break;g_edi++;g_ecx+=12;}
            if(g_edi==g_esi){g_eax=g_ebx;selector_finish(frame,1);return;}
            g_eax=MEM32(frame+164);int empty=!g_eax;if(!empty){g_ecx=g_eax;g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,g_edi);target=MEM32(g_eax+4);PUSH32(g_esp,0x24c150);RECOMP_ICALL_SAFE(target,saved);}g_ecx=g_edi+g_edi*2;g_eax=g_ebx+g_ecx*4;selector_finish(frame,empty);return;
        }
    }else if(g_ebx==1){
        g_eax=MEM32(frame+160);
        if(g_eax){g_ecx=g_eax;g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+4);PUSH32(g_esp,0x24bf2e);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_eax;MEM32(g_esp+24)=g_ecx;
            if(g_ecx){
                g_ebx=0;if(g_esi)g_edi=MEM32(g_esp+20);int match=0;
                while(g_ebx<g_esi){g_eax=MEM32(g_edi);g_edx=MEM32(frame+120);g_eax=g_eax*28+g_edx+12;SET_LO16(g_eax,MEM16(g_eax+18));
                    if(LO16(g_eax)!=0xffff){g_edx=MEM32(frame+140);g_eax=LO16(g_eax);g_eax+=g_edx+8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x24bf6c);sub_00244010();g_esp+=8;if(!g_eax){match=1;break;}g_ecx=MEM32(g_esp+24);}g_ebx++;g_edi+=12;
                }
                if(!match){g_eax=MEM32(g_esp+20);selector_finish(frame,-1);return;}
                g_eax=MEM32(frame+164);int empty=!g_eax;if(!empty){g_ecx=g_eax;g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,g_ebx);target=MEM32(g_edx+4);PUSH32(g_esp,0x24bf9c);RECOMP_ICALL_SAFE(target,saved);}g_ecx=MEM32(g_esp+20);g_eax=g_ebx+g_ebx*2;g_eax=g_ecx+g_eax*4;selector_finish(frame,empty);return;
            }
        }
    }
    if(g_esi<=1){g_eax=MEM32(g_esp+20);selector_finish(frame,-1);return;}
    if(g_ebx==5){
        g_ecx=MEM32(g_edi);g_eax=(g_ecx>>17)&0x7fff;g_ebx=LO16(g_eax);g_edx=0;g_eax=g_ebx+1;g_edx=g_eax%g_esi;g_eax/=g_esi;g_ecx&=0x1ffff;g_edx=(g_edx<<17)|g_ecx;MEM32(g_edi)=g_edx;g_eax=MEM32(frame+164);int empty=!g_eax;
        if(!empty){g_ecx=g_eax;g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x24bfec);RECOMP_ICALL_SAFE(target,saved);}g_eax=g_ebx+g_ebx*2;g_eax=g_edi+g_eax*4+4;selector_finish(frame,empty);return;
    }
    /* Keep the title RNG and every rejection draw in their original order. */
    PUSH32(g_esp,0x24bfff);sub_00243F90();
    if(g_ebx==3){
        g_eax=LO16(g_eax);g_edx=0;g_edx=g_eax%g_esi;g_eax/=g_esi;g_ecx=MEM32(g_edi)>>17;g_ebx=g_edx;
        if(LO16(g_ebx)==LO16(g_ecx)){do{PUSH32(g_esp,0x24c01c);sub_00243F90();g_edx=0;g_eax=LO16(g_eax);g_edx=g_eax%g_esi;g_eax/=g_esi;g_ebx=g_edx;g_edx=MEM32(g_edi)>>17;}while(LO16(g_ebx)==LO16(g_edx));}
        g_eax=MEM32(frame+164);if(g_eax){g_edx=LO16(g_ebx);g_ecx=g_eax;g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24c044);RECOMP_ICALL_SAFE(target,saved);}
        g_edx=MEM32(g_edi);g_eax=LO16(g_ebx);g_ecx=g_eax;g_ecx<<=17;g_edx&=0x1ffff;g_ecx|=g_edx;g_eax+=g_eax*2;MEM32(g_edi)=g_ecx;g_eax=g_edi+g_eax*4+4;selector_finish(frame,g_ecx==0);return;
    }
    if(g_ebx==4){
        g_eax=LO16(g_eax);g_edx=0;g_edx=g_eax%g_esi;g_eax/=g_esi;g_eax=LO16(g_edx);g_ecx=g_eax+g_eax*2;g_eax=MEM32(g_edi+g_ecx*4+8);g_ebx=g_edi+g_ecx*4+4;
        if(g_eax!=0xffffffffu){do{PUSH32(g_esp,0x24c095);sub_00243F90();g_eax=LO16(g_eax);g_edx=0;g_edx=g_eax%g_esi;g_eax/=g_esi;g_eax=LO16(g_edx);g_eax+=g_eax*2;g_ebx=g_edi+g_eax*4+4;}while(MEM32(g_ebx+4)!=0xffffffffu);}
        g_eax=0;MEM32(g_ebx+4)=1;g_ecx=g_edi+8;
        while(g_eax<g_esi&&MEM32(g_ecx)!=0xffffffffu){g_eax++;g_ecx+=12;}
        if(g_eax==g_esi){g_eax=MEM32(g_esp+20);g_ecx=0xffffffffu;do{if(g_eax!=g_ebx)MEM32(g_eax+4)=g_ecx;g_eax+=12;g_esi--;}while(g_esi);}
        g_eax=MEM32(frame+164);if(g_eax){g_edx=LO16(g_edx);g_ecx=g_eax;g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24c0fa);RECOMP_ICALL_SAFE(target,saved);}g_eax=g_ebx;selector_finish(frame,-1);return;
    }
    g_eax&=0x7fff;g_edi=0;g_ebx=MEM32(g_esp+20);g_ecx=g_ebx+10;
    while(g_edi<g_esi){if(LO16(g_eax)>=MEM16(g_ecx-2)&&LO16(g_eax)<=MEM16(g_ecx))break;g_edi++;g_ecx+=12;}
    if(g_edi==g_esi){g_eax=MEM32(g_esp+20);selector_finish(frame,1);return;}
    g_eax=MEM32(frame+164);int empty=!g_eax;if(!empty){g_ecx=g_eax;g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24c150);RECOMP_ICALL_SAFE(target,saved);}g_ecx=g_edi+g_edi*2;g_eax=g_ebx+g_ecx*4;selector_finish(frame,empty);
}
static void selector_event(void){
    PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_edx=MEM32(g_edi+8);
    if(g_edx!=0xffffffffu){
        g_eax=MEM32(g_ecx+156)+12;PUSH32(g_esp,g_esi);
        if(g_edx){g_esi=g_edx;PUSH32(g_esp,g_ebx);do{g_esi--;g_ebx=MEM32(g_eax+4);g_eax+=g_ebx+8;}while(g_esi);POP32(g_esp,g_ebx);}g_esi=g_eax+8;
        if(!g_esi){POP32(g_esp,g_esi);MEM16(g_edi+2)=0xffff;POP32(g_esp,g_edi);g_recomp_zf=1;g_esp+=12;return;}
        if(MEM8(g_esp+12)&16){g_eax=(MEM32(g_esi)>>17)&0x7fff;POP32(g_esp,g_esi);MEM16(g_edi+2)=LO16(g_eax);POP32(g_esp,g_edi);g_recomp_zf=g_eax==0;g_esp+=12;return;}
        g_eax=MEM32(g_ecx+160);
        if(g_eax){PUSH32(g_esp,1);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24c1cb);sub_0024BE60();SET_LO16(g_eax,MEM16(g_eax));MEM16(g_edi+2)=LO16(g_eax);g_edx=MEM32(g_esi);g_ecx=LO16(g_eax);g_ecx<<=17;g_edx&=0x1ffff;g_ecx|=g_edx;MEM32(g_esi)=g_ecx;POP32(g_esp,g_esi);POP32(g_esp,g_edi);g_recomp_zf=g_ecx==0;g_esp+=12;return;}
        SET_LO16(g_eax,MEM16(g_edi));
        if(HI8(g_eax)&1){PUSH32(g_esp,3);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24c1f9);sub_0024BE60();}
        else if(HI8(g_eax)&2){PUSH32(g_esp,4);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24c208);sub_0024BE60();}
        else if(HI8(g_eax)&4){PUSH32(g_esp,5);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24c217);sub_0024BE60();}
        else{PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24c221);sub_0024BE60();if(g_eax){g_ecx=MEM32(g_eax);g_edx=MEM32(g_esi);g_ecx<<=17;g_edx&=0x1ffff;g_ecx|=g_edx;MEM32(g_esi)=g_ecx;}}
        if(g_eax){SET_LO16(g_eax,MEM16(g_eax));MEM16(g_edi+2)=LO16(g_eax);}POP32(g_esp,g_esi);
    }POP32(g_esp,g_edi);g_esp+=12;
}
int uc_migration_audio_selector(uint32_t a){if(!selector_admit(a))return 0;if(a==0x24be60)selector_choose();else selector_event();return 1;}
