#include "migration_audio_timing_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00240A60(void);
extern void sub_00243440(void);
extern void sub_00247730(void);
extern void sub_00241940(void);
extern void sub_00240430(void);
extern void sub_002413E0(void);
extern void sub_00240AD0(void);
extern void sub_00246E70(void);
extern void sub_0024A6F0(void);
extern void sub_00243930(void);
extern void sub_00253090(void);
extern void sub_00243F60(void);
extern void sub_00385A78(void);
extern void sub_002D7740(void);
extern void sub_002D7860(void);
static int timing_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_TIMING");enabled=v&&!strcmp(v,"1");}return enabled;}
static int timing_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int timing_safe(uint32_t p,uint32_t n){return timing_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int timing_ring(uint32_t head,uint32_t offset,uint32_t span){
    uint32_t p=MEM32(head),previous=head,count=0;
    /* ponytail: bounded full-list validation is O(n); cache ownership invariants when all list publishers are native. */
    while(p!=head){if(++count>65535||p<offset||!timing_safe(p-offset,span)||MEM32(p+4)!=previous)return 0;previous=p;p=MEM32(p);}return MEM32(head+4)==previous;
}
static int timing_admit(uint32_t a){
    if(!timing_enabled()||!timing_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!timing_span(g_esp-4096,4108))return 0;
    uint32_t object=g_ecx,arg=MEM32(g_esp+4);
    if(a==0x23fcb0)return timing_safe(object,16)&&(!arg||timing_safe(arg,8));
    if(a==0x240010){uint32_t delay=MEM32(g_esp+8);return timing_safe(object,16)&&(!delay||(timing_safe(delay,4)&&((int32_t)MEM32(delay)<=0||timing_safe(arg,16))));}
    if(a==0x243440)return timing_safe(object,48);
    if(a!=0x240a60&&a!=0x241980)return 0;
    if(!timing_safe(object,184)||!timing_ring(object+80,4,32))return 0;
    for(uint32_t p=MEM32(object+80);p!=object+80;p=MEM32(p))if(!MEM32(p+16)&&!timing_span(MEM32(p-4),20))return 0;
    if(a==0x240a60)return timing_safe(arg,8);
    uint32_t device=MEM32(object+100),timer=MEM32(object+164);
    if(device&&(!timing_safe(device,4)||!timing_span(MEM32(device),12)))return 0;
    if(timer&&!timing_safe(timer,48))return 0;
    return timing_ring(object+24,16,80)&&timing_ring(object+72,4,80)&&timing_ring(object+40,24,104);
}
static void timing_read(void){g_eax=MEM32(g_esp+4);int empty=g_eax==0;if(g_eax){g_edx=MEM32(g_ecx+8);MEM32(g_eax)=g_edx;g_ecx=MEM32(g_ecx+12);MEM32(g_eax+4)=g_ecx;}g_recomp_zf=empty;g_esp+=8;}
static void timing_reschedule(void){
    g_eax=MEM32(g_esp+8);if(g_eax){g_eax=MEM32(g_eax);if((int32_t)g_eax>0){g_edx=(int32_t)g_eax<0?0xffffffffu:0;PUSH32(g_esp,g_esi);g_esi=g_eax;g_eax=MEM32(g_ecx+8);g_ecx=MEM32(g_ecx+12);uint32_t carry=(uint32_t)(((uint64_t)g_esi+g_eax)>>32);g_esi+=g_eax;g_eax=MEM32(g_esp+8);g_ecx+=g_edx+carry;MEM32(g_eax+8)=g_esi;MEM32(g_eax+12)=g_ecx;SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_recomp_zf=g_ecx==0;g_esp+=12;return;}}
    SET_LO8(g_eax,0);g_recomp_zf=1;g_esp+=12;
}
static void timing_counter(void){
    g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+32);g_eax|=MEM32(g_esi+36);
    if(g_eax){PUSH32(g_esp,g_edi);g_ecx=g_esp+8;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x243462);sub_00385A78();
        g_edx=MEM32(g_esi+40);g_ecx=MEM32(g_esp+8);g_eax=MEM32(g_esi+44);g_edi=MEM32(g_esp+12);uint32_t borrow=g_ecx<g_edx;g_ecx-=g_edx;g_edi-=g_eax+borrow;g_eax=MEM32(g_esp+20);g_edx=(int32_t)g_eax<0?0xffffffffu:0;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x243482);sub_002D7740();
        g_ecx=MEM32(g_esi+36);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+32);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x243491);sub_002D7860();POP32(g_esp,g_edi);
    }else g_eax=0;
    POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void timing_retire(void){
    uint32_t frame=g_seh_ebp;g_esp-=8;PUSH32(g_esp,frame);frame=g_ecx+80;PUSH32(g_esp,g_edi);g_edi=MEM32(frame);
    if(g_edi!=frame){PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+24);PUSH32(g_esp,g_esi);
        do{g_eax=MEM32(g_edi+16);g_esi=g_edi-4;g_edi=MEM32(g_edi);
            if(!g_eax){g_eax=MEM32(g_esi);g_ecx=g_esp+16;uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_esi;uint32_t target=MEM32(g_eax+16);PUSH32(g_esp,0x240a8d);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x240a93);sub_00253090();
                g_ecx=MEM32(g_ebx+4);g_esp+=4;int due=(int32_t)g_edx<(int32_t)g_ecx||(g_edx==g_ecx&&g_eax<=MEM32(g_ebx));
                if(due){g_edx=g_esi+4;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x240aac);sub_00243F60();g_eax=MEM32(g_esi+12);g_esp+=4;g_eax--;MEM32(g_esi+12)=g_eax;
                    if(!g_eax){g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;target=MEM32(g_eax+4);PUSH32(g_esp,0x240ac1);RECOMP_ICALL_SAFE(target,saved);}}}
        }while(g_edi!=frame);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
    }POP32(g_esp,g_edi);POP32(g_esp,frame);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void timing_update(void){
    uint32_t frame=g_seh_ebp;g_esp-=8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ebx=g_ecx;PUSH32(g_esp,g_edi);frame=g_ebx+8;PUSH32(g_esp,frame);PUSH32(g_esp,0x241992);sub_00240A60();
    g_ecx=MEM32(g_ebx+100);
    if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x24199e);RECOMP_ICALL_SAFE(target,saved);g_edx=(int32_t)g_eax<0?0xffffffffu:0;}
    else{g_ecx=MEM32(g_ebx+164);if(g_ecx){PUSH32(g_esp,1000);PUSH32(g_esp,0x2419b5);sub_00243440();g_edx=(int32_t)g_eax<0?0xffffffffu:0;}else{g_edx=MEM32(g_esp+20);g_eax=MEM32(g_esp+16);}}
    g_ecx=MEM32(frame);g_ecx|=MEM32(frame+4);if(!g_ecx){MEM32(frame)=g_eax;MEM32(frame+4)=g_edx;}g_ecx=MEM32(frame+4);
    int forward=(int32_t)g_ecx<(int32_t)g_edx;
    if(g_ecx==g_edx){g_ecx=MEM32(frame);forward=g_ecx<=g_eax;}
    if(forward){g_ecx=MEM32(frame);g_edi=g_eax;g_edi-=g_ecx;}else{g_edi=MEM32(frame);g_edi-=g_eax;g_edi--;}
    g_esi=MEM32(g_ebx+24);MEM32(frame)=g_eax;g_eax=g_ebx+24;MEM32(frame+4)=g_edx;
    while(g_esi!=g_eax){g_ecx=g_esi-16;g_esi=MEM32(g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x241a0b);sub_00247730();g_eax=g_ebx+24;}
    g_eax=(uint32_t)((int32_t)g_edi>>1);if((int32_t)g_eax>30)g_eax=30;g_ecx=MEM32(frame);g_esi=MEM32(frame+4);g_edx=(int32_t)g_eax<0?0xffffffffu:0;uint32_t carry=(uint32_t)(((uint64_t)g_eax+g_ecx)>>32);g_eax+=g_ecx;g_edx+=g_esi+carry;MEM32(g_esp+20)=g_edx;g_edx=g_esp+16;PUSH32(g_esp,g_edx);g_ecx=g_ebx;MEM32(g_esp+20)=g_eax;PUSH32(g_esp,0x241a3f);sub_00241940();
    g_ecx=g_ebx;PUSH32(g_esp,0x241a46);sub_00240430();g_ecx=g_ebx;PUSH32(g_esp,0x241a4d);sub_002413E0();g_ecx=g_ebx;PUSH32(g_esp,0x241a54);sub_00240AD0();
    g_esi=MEM32(g_ebx+72);frame=g_ebx+72;while(g_esi!=frame){g_ecx=g_esi-4;g_esi=MEM32(g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x241a6b);sub_00246E70();}
    g_esi=MEM32(g_ebx+40);frame=g_ebx+40;while(g_esi!=frame){g_ecx=g_esi-24;g_esi=MEM32(g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x241a8b);sub_0024A6F0();}
    g_ecx=MEM32(g_ebx+164);g_esi=0;if(g_ecx){PUSH32(g_esp,g_edi);PUSH32(g_esp,0x241aa1);sub_00243930();}
    POP32(g_esp,g_edi);MEM32(g_ebx+180)=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_audio_timing(uint32_t address){if(!timing_admit(address))return 0;switch(address){case 0x23fcb0:timing_read();break;case 0x240010:timing_reschedule();break;case 0x243440:timing_counter();break;case 0x240a60:timing_retire();break;case 0x241980:timing_update();break;}return 1;}
