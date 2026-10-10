#include "migration_audio_ramps_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243F90(void);
extern void sub_00240050(void);
extern void sub_00244260(void);
extern void sub_00244200(void);
extern void sub_00240010(void);
extern void sub_00248D10(void);
extern void sub_00248E80(void);
extern void sub_0023FCB0(void);
extern void sub_002D7740(void);
extern void sub_002D7860(void);
static int ramps_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_RAMPS");enabled=v&&!strcmp(v,"1");}return enabled;}
static int ramps_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int ramps_safe(uint32_t p,uint32_t n){return ramps_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int ramps_admit(uint32_t a){
    if(!ramps_enabled()||!ramps_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!ramps_span(g_esp-4096,4108))return 0;
    if(a==0x240050)return 1;
    if(a!=0x241130&&a!=0x2411f0)return 0;
    uint32_t command=MEM32(g_esp+4);if(!ramps_safe(g_ecx,16)||!ramps_safe(command,96)||!ramps_safe(MEM32(command+84),28))return 0;
    uint32_t track=MEM32(command+80);
    if(track&&(!ramps_safe(track,104)||!ramps_span(MEM32(track),4)))return 0;
    if(a==0x2411f0){uint32_t output=MEM32(g_esp+8);if(output&&!ramps_safe(output,2))return 0;if(!track&&(MEM32(MEM32(command+84)+16)&0x80000000u))return 0;}
    return 1;
}
static void ramps_random(void){
    SET_LO16(g_eax,MEM16(g_esp+4));PUSH32(g_esp,g_edi);SET_LO16(g_edi,MEM16(g_esp+12));
    if(LO16(g_eax)!=LO16(g_edi)){PUSH32(g_esp,g_esi);g_esi=SX16(LO16(g_eax));PUSH32(g_esp,0x240069);sub_00243F90();g_eax=LO16(g_eax);g_ecx=SX16(LO16(g_edi));g_edx=(int32_t)g_eax<0?0xffffffffu:0;g_ecx-=g_esi;int64_t dividend=(int64_t)(int32_t)g_edx*4294967296ll+g_eax;g_edx=(uint32_t)(dividend%(int32_t)g_ecx);g_eax=g_edx+g_esi;POP32(g_esp,g_esi);}POP32(g_esp,g_edi);g_esp+=12;
}
static void ramps_pitch(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+12);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx+84);frame=g_ecx;g_ecx=MEM32(g_ebx+80);g_esi+=12;PUSH32(g_esp,g_edi);int again=0;
    if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe7fc);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x241157);RECOMP_ICALL_SAFE(target,saved);
        g_ecx=(uint32_t)(int32_t)SMEM16(g_esi+4);g_edi=g_eax;g_edx=MEM16(g_edi+96);g_xmm0.f[0]=(float)(int32_t)g_ecx;g_xmm0.f[0]*=MEMF(0x481784);MEMF(g_esp+16)=g_xmm0.f[0];PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24117b);sub_00244260();
        MEMF(g_esp+28)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_xmm0=XMM_SCALAR(MEMF(g_esp+28));g_xmm0.f[0]+=MEMF(g_esp+20);MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,0x241195);sub_00244200();MEM16(g_edi+96)=LO16(g_eax);g_ecx=MEM32(0x4839b0);g_eax=SX16(LO16(g_eax));g_esp+=4;
        if((int32_t)g_eax>(int32_t)g_ecx&&(int32_t)g_eax<(int32_t)MEM32(0x4839b4)){
            g_eax=MEM32(g_esi);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ecx=frame;MEM32(g_esp+32)=g_eax;PUSH32(g_esp,0x2411c4);sub_00240010();g_ecx=g_edi;PUSH32(g_esp,0x2411cb);sub_00248D10();again=1;
        }else{PUSH32(g_esp,0x20000);g_ecx=g_edi;PUSH32(g_esp,0x2411e1);sub_00248E80();}
    }POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,again);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=!again;g_esp+=8;
}
static void ramps_parameter(void){
    uint32_t frame=g_seh_ebp;g_esp-=12;PUSH32(g_esp,frame);frame=MEM32(g_esp+24);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+28);g_esi=MEM32(g_edi+84);MEM32(g_esp+12)=g_ecx;MEM8(g_esp+28)=0;
    if(frame){MEM16(frame)=0;SET_LO8(g_eax,MEM8(g_esi+4));PUSH32(g_esp,g_ebx);uint32_t type=LO8(g_eax);
        if(type==5||type==15||type==6||type==16){int pitch=type==5||type==15;g_edx=MEM32(g_esi+16);g_eax=g_edx&0x7fffffff;
            if((int32_t)g_edx<0){g_edx=MEM32(g_edi+80);SET_LO16(g_edx,MEM16(g_edx+(pitch?98:96)));MEM16(frame)=LO16(g_edx);}
            if(!g_eax){SET_LO16(g_eax,MEM16(g_esi+12));MEM16(frame)+=LO16(g_eax);}
            else if(g_eax==2){g_edx=MEM16(g_esi+14);g_eax=MEM16(g_esi+12);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x241283);sub_00240050();MEM16(frame)+=LO16(g_eax);}
            else if(g_eax==1){
                if(pitch){g_edx=g_esp+20;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x2412a8);sub_0023FCB0();g_ecx=MEM32(g_edi+20);g_eax=MEM32(g_edi+16);g_ebx=MEM32(g_esp+20);g_eax|=g_ecx;g_ecx=MEM32(g_esp+24);
                    if(!g_eax){MEM32(g_edi+16)=g_ebx;MEM32(g_edi+20)=g_ecx;g_edx=(uint32_t)(int32_t)SMEM16(frame);MEM32(g_esi+24)=g_edx;}
                    g_eax=MEM32(g_edi+16);g_edx=MEM32(g_edi+20);uint32_t borrow=g_ebx<g_eax;g_ebx-=g_eax;g_ecx-=g_edx+borrow;g_edx=MEM32(g_esi+20);g_eax=0;
                }else{g_ebx=MEM32(g_ecx+8);g_edx=MEM32(g_edi+16);g_edx|=MEM32(g_edi+20);g_ecx=MEM32(g_esp+16);g_ecx=MEM32(g_ecx+12);
                    if(!g_edx){MEM32(g_edi+16)=g_ebx;MEM32(g_edi+20)=g_ecx;g_eax=(uint32_t)(int32_t)SMEM16(frame);MEM32(g_esi+24)=g_eax;}
                    g_edx=MEM32(g_edi+16);g_eax=MEM32(g_edi+20);uint32_t borrow=g_ebx<g_edx;g_ebx-=g_edx;g_edx=MEM32(g_esi+20);g_ecx-=g_eax+borrow;g_eax=0;
                }
                int complete=(int32_t)g_ecx>0||(g_ecx==0&&g_ebx>=g_edx);
                if(complete){
                    if(pitch){SET_LO16(g_eax,MEM16(g_esi+24));SET_LO16(g_eax,LO16(g_eax)+MEM16(g_esi+14));POP32(g_esp,g_ebx);POP32(g_esp,g_edi);MEM16(frame)=LO16(g_eax);SET_LO8(g_eax,MEM8(g_esp+24));POP32(g_esp,g_esi);POP32(g_esp,frame);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;return;}
                    SET_LO16(g_ecx,MEM16(g_esi+24));SET_LO16(g_ecx,LO16(g_ecx)+MEM16(g_esi+14));SET_LO8(g_eax,MEM8(g_esp+32));POP32(g_esp,g_ebx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM16(frame)=LO16(g_ecx);POP32(g_esp,frame);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;return;
                }
                if(g_edx){g_edx=(uint32_t)(int32_t)SMEM16(g_esi+12);g_eax=(uint32_t)(int32_t)SMEM16(g_esi+14);PUSH32(g_esp,g_ecx);g_eax-=g_edx;g_edx=(int32_t)g_eax<0?0xffffffffu:0;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24139f);sub_002D7740();g_ecx=0;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+20);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2413ad);sub_002D7860();
                    SET_LO16(g_eax,LO16(g_eax)+MEM16(g_esi+24));g_ecx=MEM32(g_esp+16);MEM16(frame)=LO16(g_eax);g_eax=g_esp+32;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);MEM32(g_esp+40)=31;PUSH32(g_esp,0x2413cc);sub_00240010();MEM8(g_esp+32)=LO8(g_eax);}
            }
        }POP32(g_esp,g_ebx);
    }
    SET_LO8(g_eax,MEM8(g_esp+28));POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_audio_ramps(uint32_t address){if(!ramps_admit(address))return 0;switch(address){case 0x240050:ramps_random();break;case 0x241130:ramps_pitch();break;case 0x2411f0:ramps_parameter();break;}return 1;}
