#include "migration_audio_params_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002440F0(void);
extern void sub_00245270(void);
extern void sub_00245290(void);
extern void sub_00245640(void);
extern void sub_00247620(void);
extern void sub_00247650(void);
extern void sub_00247CC0(void);
extern void sub_00247CF0(void);
extern void sub_00248C50(void);
extern void sub_002491A0(void);
extern void sub_002495C0(void);
extern void sub_00251D80(void);
extern void sub_002D6780(void);
static int params_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_PARAMS");f=v&&!strcmp(v,"1");}return f;}
static int params_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int params_tracks(uint32_t head){
    /* ponytail: bounded O(n) preflight; broaden only with measured need. */
    uint32_t p=MEM32(head),previous=head;
    for(unsigned n=0;p!=head;n++,p=MEM32(p)){if(n==4096||!params_span(p-20,0x70)||MEM32(p+4)!=previous)return 0;previous=p;}
    return MEM32(head+4)==previous;
}
static int params_admit(uint32_t a){
    if(!params_enabled()||!params_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!params_span(g_esp-4096,4112))return 0;
    if(a==0x248bf0)return 1;
    if(!params_span(g_ecx,0x70)||!((uint64_t)g_ecx+0x70<=g_esp-4096||(uint64_t)g_esp+16<=g_ecx))return 0;
    if(a==0x247620||a==0x247650||a==0x247cc0||a==0x247cf0){uint32_t p=g_ecx;for(unsigned n=0;p;n++,p=MEM32(p+44))if(n==128||!params_span(p,0x70))return 0;return 1;}
    if(a==0x248d10||a==0x248d40||a==0x249670||a==0x2491a0)return params_tracks(g_ecx+40);
    return a==0x248b20||a==0x248b80||a==0x248c50||a==0x2495c0;
}
static void params_route(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+80);
    if(!g_eax||MEM16(g_eax+22)!=0xffff){
        g_eax=MEM32(g_esp+16);
        if(MEM16(g_eax+2)==1){g_ecx=MEM32(g_esi+68);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=g_edi;PUSH32(g_esp,0x248b4f);sub_00245640();g_esi=MEM32(g_esi+68);if(MEM8(g_esi+68)&2){PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x248b60);sub_00245270();}POP32(g_esp,g_edi);}
        else{g_esi=MEM32(g_esi+68);if(g_esi&&(MEM8(g_esi+68)&2)){g_ecx=MEM32(g_esp+12);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x248b7c);sub_00245290();POP32(g_esp,g_esi);g_recomp_zf=0;g_esp+=16;return;}}
    }POP32(g_esp,g_esi);g_esp+=16;
}
static void params_frequency(void){
    g_eax=MEM32(g_esp+4);
    if(g_eax<MEM32(0x4839a8)||g_eax>MEM32(0x4839ac)){g_eax=0;g_recomp_zf=1;g_esp+=8;return;}
    g_ecx=MEM32(0x4839a4);if(g_eax==g_ecx){g_eax=0;g_recomp_zf=1;g_esp+=8;return;}
    MEM32(g_esp+4)=g_ecx;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=(double)SMEM32(g_esp+4);if((int32_t)g_ecx<0)g_fp_stack[g_fp_top]+=MEMF(0x44c510);
    g_fp_stack[g_fp_top]=MEMF(0x44bf94)/g_fp_stack[g_fp_top];MEM32(g_esp+4)=g_eax;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=(double)SMEM32(g_esp+4);if((int32_t)g_eax<0)g_fp_stack[g_fp_top]+=MEMF(0x44c510);
    g_fp_stack[g_fp_top]*=g_fp_stack[(g_fp_top+1)&7];PUSH32(g_esp,g_ecx);MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+2)&7;PUSH32(g_esp,0x248c45);sub_002440F0();g_esp+=4;g_recomp_zf=g_esp==0;g_esp+=8;
}
/* Preserve each ancestor's truncation/clamp and its guest return state. */
static void params_group_gain(void){
    g_eax=(uint32_t)(int32_t)SMEM16(g_ecx+56);g_edx=(uint32_t)(int32_t)SMEM16(g_ecx+50);PUSH32(g_esp,g_esi);g_esi=(uint32_t)(int32_t)SMEM16(g_ecx+62);g_ecx=MEM32(g_ecx+44);g_esi+=g_eax;g_esi+=g_edx;
    if(g_ecx){PUSH32(g_esp,0x247cdd);sub_00247CC0();g_eax=SX16(LO16(g_eax));g_esi+=g_eax;}
    g_eax=MEM32(0x4839b0);int equal=g_eax==g_esi;if((int32_t)g_eax<=(int32_t)g_esi)SET_LO16(g_eax,LO16(g_esi));POP32(g_esp,g_esi);g_recomp_zf=equal;g_esp+=4;
}
static void params_group_pitch(void){
    g_eax=(uint32_t)(int32_t)SMEM16(g_ecx+54);PUSH32(g_esp,g_esi);g_esi=(uint32_t)(int32_t)SMEM16(g_ecx+64);g_ecx=MEM32(g_ecx+44);g_esi+=g_eax;
    if(g_ecx){PUSH32(g_esp,0x247d07);sub_00247CF0();g_ecx=SX16(LO16(g_eax));g_esi+=g_ecx;}
    int equal=0;if((int32_t)g_esi<-32767)g_eax=0xffff8001u;else{equal=g_esi==8191;if((int32_t)g_esi>8191)g_eax=8191;else SET_LO16(g_eax,LO16(g_esi));}POP32(g_esp,g_esi);g_recomp_zf=equal;g_esp+=4;
}
static void params_group_float(int second){
    PUSH32(g_esp,g_ecx);g_xmm0=XMM_SCALAR(MEMF(g_ecx+(second?108:104)));g_ecx=MEM32(g_ecx+44);MEMF(g_esp)=g_xmm0.f[0];int parent=g_ecx!=0;
    if(parent){if(second){PUSH32(g_esp,0x247667);sub_00247650();}else{PUSH32(g_esp,0x247637);sub_00247620();}g_fp_stack[g_fp_top]+=MEMF(g_esp);}
    else{g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_esp);}POP32(g_esp,g_ecx);g_recomp_zf=!parent;g_esp+=4;
}
static void params_special_track(void){
    g_edx=g_ecx+40;g_ecx=MEM32(g_edx);while(g_ecx!=g_edx){g_eax=g_ecx-20;if(MEM8(g_ecx+53)&2){g_esp+=4;return;}g_ecx=MEM32(g_ecx);}g_eax=0;g_esp+=4;
}
static void params_gain(void){
    uint32_t voice=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_edi=g_ecx;
    if(g_ebx){
        g_eax=MEM32(g_ebx);uint32_t saved=g_esp;PUSH32(g_esp,voice);PUSH32(g_esp,0);PUSH32(g_esp,0x4fed44);g_ecx=g_ebx;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x248c6e);RECOMP_ICALL_SAFE(target,saved);voice=g_eax;
        if(voice){g_eax=MEM32(voice+60);if(g_eax){
            g_eax=MEM32(g_edi+80);if(g_eax)g_eax=(0u-(MEM16(g_eax+4)&0x1ffu))<<4;else g_eax=0;
            g_ecx=MEM32(g_edi+68);PUSH32(g_esp,g_esi);
            if(g_ecx&&(MEM32(g_ecx+68)&0x1000000)){g_ecx=MEM32(0x4839b0);g_esi=g_ecx;}
            else{g_edx=(uint32_t)(int32_t)SMEM16(g_edi+92);g_ecx=(uint32_t)(int32_t)SMEM16(g_edi+96);g_edx+=g_eax;g_esi=g_edx+g_ecx;g_ecx=MEM32(g_edi+72);if(g_ecx){PUSH32(g_esp,0x248ccc);sub_00247CC0();g_eax=SX16(LO16(g_eax));g_esi+=g_eax;}SET_LO16(g_ecx,MEM16(g_ebx+96));SET_LO16(g_ecx,LO16(g_ecx)+MEM16(g_ebx+86));g_edx=SX16(LO16(g_ecx));g_ecx=MEM32(0x4839b0);g_esi+=g_edx;}
            g_eax=MEM32(0x4839b4);if((int32_t)g_esi>(int32_t)g_eax)g_esi=g_eax;if((int32_t)g_esi<(int32_t)g_ecx)g_esi=g_ecx;g_ecx=MEM32(voice+60);g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,g_esi);target=MEM32(g_eax+24);PUSH32(g_esp,0x248cfc);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);
        }}POP32(g_esp,voice);
    }POP32(g_esp,g_edi);POP32(g_esp,g_ebx);g_esp+=8;
}
static void params_pitch(void){
    uint32_t voice=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_ebx=g_ecx;int low=0;
    if(g_edi){
        g_eax=MEM32(g_edi);uint32_t saved=g_esp;PUSH32(g_esp,voice);PUSH32(g_esp,0);PUSH32(g_esp,0x4fed44);g_ecx=g_edi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2495de);RECOMP_ICALL_SAFE(target,saved);voice=g_eax;
        if(voice){g_eax=MEM32(voice+60);if(g_eax){
            g_eax=MEM32(g_ebx+80);PUSH32(g_esp,g_esi);g_esi=g_eax?(uint32_t)(int32_t)SMEM16(g_eax+10):0;g_ecx=g_ebx;PUSH32(g_esp,0x249602);sub_002491A0();
            if(!g_eax){g_ecx=(uint32_t)(int32_t)SMEM16(g_ebx+94);g_esi+=g_ecx;g_ecx=MEM32(g_ebx+72);if(g_ecx){PUSH32(g_esp,0x249618);sub_00247CF0();g_edx=SX16(LO16(g_eax));g_esi+=g_edx;}}
            SET_LO16(g_eax,MEM16(g_edi+98));SET_LO16(g_eax,LO16(g_eax)+MEM16(g_edi+84));SET_LO16(g_eax,LO16(g_eax)+MEM16(g_edi+82));g_ecx=SX16(LO16(g_eax));g_esi+=g_ecx;g_ecx=MEM32(voice+60);low=(int32_t)g_esi<-32767;
            if(low){g_edx=MEM32(g_ecx);g_esi=0xffff8001u;}else{if((int32_t)g_esi>8191)g_esi=8191;g_edx=MEM32(g_ecx);}
            saved=g_esp;PUSH32(g_esp,g_esi);target=MEM32(g_edx+20);PUSH32(g_esp,low?0x249644:0x24965e);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);
        }}POP32(g_esp,voice);
    }POP32(g_esp,g_edi);POP32(g_esp,g_ebx);if(low)g_recomp_zf=0;g_esp+=8;
}
static void params_doppler(void){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_edi=g_ecx;
    if(g_esi){
        g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,0);PUSH32(g_esp,0x4fed44);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x248b9a);RECOMP_ICALL_SAFE(target,saved);g_ebx=g_eax;
        if(g_ebx){g_eax=MEM32(g_ebx+60);if(g_eax){
            g_ecx=MEM32(g_edi+72);SET_LO16(g_esi,MEM16(g_esi+94));g_eax=0;
            if(g_ecx){PUSH32(g_esp,0x248bb9);sub_00247620();g_ecx=LO16(g_esi);MEM32(g_esp+16)=g_ecx;g_fp_stack[g_fp_top]*=(double)SMEM32(g_esp+16);PUSH32(g_esp,0x248bc9);sub_002D6780();g_ecx=MEM32(g_edi+72);g_esi=g_eax;PUSH32(g_esp,0x248bd3);sub_00247650();g_fp_stack[g_fp_top]*=MEMF(0x44c174);PUSH32(g_esp,0x248bde);sub_002D6780();}
            g_ecx=MEM32(g_ebx+60);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x248be8);sub_00251D80();
        }}POP32(g_esp,g_ebx);
    }POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=12;
}
static void params_each(int pitch){
    PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=g_ecx;g_esi=MEM32(g_ebx+40);PUSH32(g_esp,g_edi);g_edi=g_ebx+40;
    while(g_esi!=g_edi){g_eax=g_esi-20;PUSH32(g_esp,g_eax);g_ecx=g_ebx;if(pitch){PUSH32(g_esp,0x24968b);sub_002495C0();}else{PUSH32(g_esp,0x248d2b);sub_00248C50();}g_esi=MEM32(g_esi);}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=4;
}
static void params_each_doppler(void){
    uint32_t node=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_eax=g_ebx+40;PUSH32(g_esp,node);node=MEM32(g_eax);
    if(node!=g_eax){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);do{
        g_esi=node-20;
        if(g_esi){
            g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed44);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x248d64);RECOMP_ICALL_SAFE(target,saved);g_edi=g_eax;
            if(g_edi){g_eax=MEM32(g_edi+60);if(g_eax){
                g_ecx=MEM32(g_ebx+72);SET_LO16(g_esi,MEM16(g_esi+94));g_eax=0;
                if(g_ecx){PUSH32(g_esp,0x248d83);sub_00247620();g_ecx=LO16(g_esi);MEM32(g_esp+16)=g_ecx;g_fp_stack[g_fp_top]*=(double)SMEM32(g_esp+16);PUSH32(g_esp,0x248d93);sub_002D6780();g_ecx=MEM32(g_ebx+72);g_esi=g_eax;PUSH32(g_esp,0x248d9d);sub_00247650();g_fp_stack[g_fp_top]*=MEMF(0x44c174);PUSH32(g_esp,0x248da8);sub_002D6780();}
                g_ecx=MEM32(g_edi+60);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x248db2);sub_00251D80();
            }}
        }node=MEM32(node);g_eax=g_ebx+40;
    }while(node!=g_eax);POP32(g_esp,g_edi);POP32(g_esp,g_esi);}
    POP32(g_esp,node);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=4;
}
int uc_migration_audio_params(uint32_t a){
    if(!params_admit(a))return 0;
    switch(a){case 0x247620:params_group_float(0);break;case 0x247650:params_group_float(1);break;case 0x247cc0:params_group_gain();break;case 0x247cf0:params_group_pitch();break;case 0x248b20:params_route();break;case 0x248b80:params_doppler();break;case 0x248bf0:params_frequency();break;case 0x248c50:params_gain();break;case 0x248d10:params_each(0);break;case 0x248d40:params_each_doppler();break;case 0x2491a0:params_special_track();break;case 0x2495c0:params_pitch();break;default:params_each(1);}
    return 1;
}
