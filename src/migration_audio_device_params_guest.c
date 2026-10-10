#include "migration_audio_device_params_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
extern void sub_002440A0(void);
extern void sub_002D6780(void);
extern void sub_002D6EB0(void);
extern void sub_00425F5B(void);
extern void sub_00425FB4(void);
extern void sub_00425F7F(void);
extern void sub_00425FD4(void);
extern void sub_00425FF8(void);
extern void sub_00426112(void);
extern void sub_0042601C(void);
extern void sub_004260A6(void);
extern void sub_004260CA(void);
extern void sub_004260EE(void);
extern void sub_00424F4D(void);
extern void sub_00425043(void);
static int device_params_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_DEVICE_PARAMS");f=v&&!strcmp(v,"1");}return f;}
static int device_params_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int device_params_safe(uint32_t p,uint64_t n){return device_params_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int device_params_admit(uint32_t a){
    if((a!=0x244140&&a!=0x2441b0&&a!=0x251a80&&a!=0x251d80)||!device_params_enabled()||!device_params_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!device_params_span(g_esp-4096,4108))return 0;
    if(a==0x244140||a==0x2441b0)return 1;if(!device_params_safe(g_ecx,40))return 0;uint32_t arg=MEM32(g_esp+4);return a!=0x251a80||!arg||device_params_safe(arg,36);
}
static unsigned device_params_compare(float a,float b){double x=a,y=b;return x!=x||y!=y?0x47u:x<y?3u:x>y?2u:0x42u;}
static void device_params_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void device_params_frequency(void){
    PUSH32(g_esp,g_ecx);g_xmm1=XMM_SCALAR(MEMF(0x44c13c));g_xmm0=XMM_SCALAR(MEMF(g_esp+8));unsigned flags=device_params_compare(g_xmm1.f[0],g_xmm0.f[0]);if(!(flags&1)){g_eax=0x8000;POP32(g_esp,g_ecx);g_recomp_zf=!!(flags&0x40);g_esp+=4;return;}flags=device_params_compare(g_xmm0.f[0],MEMF(0x483a08));if(!(flags&1)){g_eax=0;POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=4;return;}
    g_eax=MEM32(0x4839a4);device_params_push((double)SMEM32(0x4839a4));if((int32_t)g_eax<0)g_fp_stack[g_fp_top]+=MEMF(0x44c510);g_fp_stack[g_fp_top]=MEMF(g_esp+8)/g_fp_stack[g_fp_top];g_fp_stack[g_fp_top]*=MEMF(0x483a00);g_fp_stack[g_fp_top]=sin(g_fp_stack[g_fp_top]);g_fp_stack[g_fp_top]+=g_fp_stack[g_fp_top];device_params_push(0.69314718055994530942);double swap=g_fp_stack[g_fp_top];g_fp_stack[g_fp_top]=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[(g_fp_top+1)&7]=swap;g_fp_stack[(g_fp_top+1)&7]*=log2(g_fp_stack[g_fp_top]);g_fp_top=(g_fp_top+1)&7;g_fp_stack[g_fp_top]*=MEMF(0x483a04);PUSH32(g_esp,0x24419c);sub_002D6780();g_eax&=0xffff;POP32(g_esp,g_ecx);g_recomp_zf=g_eax==0;g_esp+=4;
}
static void device_params_gain(void){
    g_xmm0.f[0]=(float)(int32_t)MEM32(g_esp+4);g_xmm1=XMM_SCALAR(MEMF(0x483a20));unsigned flags=device_params_compare(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_esp+4)=g_xmm0.f[0];if(!(flags&0x41))MEMF(g_esp+4)=g_xmm1.f[0];device_params_push(MEMD(0x483a18));device_params_push(MEMF(g_esp+4));g_fp_stack[g_fp_top]*=MEMF(0x45029c);PUSH32(g_esp,0x2441e4);sub_002D6EB0();g_fp_stack[g_fp_top]*=MEMD(0x483a10);PUSH32(g_esp,0x2441ef);sub_002D6780();int equal=g_eax==0xffff;if(g_eax>0xffff)g_eax=0xffff;g_recomp_zf=equal;g_esp+=4;
}
static void device_params_filter(void){
    g_esp-=24;PUSH32(g_esp,g_esi);PUSH32(g_esp,24);g_eax=g_esp+8;PUSH32(g_esp,0);PUSH32(g_esp,g_eax);g_esi=g_ecx;PUSH32(g_esp,0x251d94);sub_002440A0();g_ecx=ZX16(MEM16(g_esp+44));g_esp+=8;g_xmm0.f[0]=(float)(int32_t)g_ecx;MEM32(g_esp+8)=1;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,0x251db2);device_params_frequency();g_edx=(uint32_t)(int32_t)SMEM16(g_esp+40);PUSH32(g_esp,g_edx);MEM32(g_esp+20)=g_eax;PUSH32(g_esp,0x251dc1);device_params_gain();g_ecx=MEM32(g_esp+20);MEM32(g_esp+24)=g_eax;MEM32(g_esp+32)=g_eax;g_eax=MEM32(g_esi+32);g_esp+=8;MEM32(g_esp+20)=g_ecx;MEM32(g_esp+8)=0;
    if(g_eax){g_edx=g_esp+4;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x251dee);sub_00424F4D();}g_esi=MEM32(g_esi+36);if(g_esi){g_eax=g_esp+4;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x251e00);sub_00425043();}POP32(g_esp,g_esi);g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void device_params_spatial(void){
    PUSH32(g_esp,g_esi);g_esi=g_ecx;if(MEM8(g_esi+28)&4){g_eax=MEM32(g_esi+32);if(g_eax){PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);if(g_edi){g_ecx=ZX16(MEM16(g_edi+10));g_edx=ZX16(MEM16(g_edi+8));PUSH32(g_esp,g_ebx);PUSH32(g_esp,1);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x251ab8);sub_00425F5B();
    g_eax=(uint32_t)(int32_t)SMEM16(g_edi+12);g_ecx=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x251ac8);sub_00425FB4();
    g_edx=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x251adc);sub_00425F7F();
    g_eax=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,0x7f7fffff);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x251aec);sub_00425FD4();
    g_ecx=MEM32(g_edi+16);g_edx=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x251afb);sub_00425FF8();
    g_eax=MEM32(g_edi+20);g_ecx=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x251b0a);sub_00425FD4();
    SET_LO16(g_eax,MEM16(g_edi+14));g_ebx=ZX16(LO16(g_eax));if(!LO16(g_eax)||LO16(g_eax)==3){g_edx=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,2);PUSH32(g_esp,0x4843f8);PUSH32(g_esp,g_edx);g_ebx=0;PUSH32(g_esp,0x251b3a);sub_00426112();}else if(LO16(g_eax)==4)g_ebx=0;
    g_eax=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x251b46);sub_0042601C();
    g_ecx=MEM32(g_edi+24);g_edx=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x251b55);sub_004260A6();
    g_eax=MEM32(g_edi+32);g_ecx=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x251b64);sub_004260CA();
    g_edx=MEM32(g_edi+28);g_eax=MEM32(g_esi+32);PUSH32(g_esp,1);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x251b73);sub_004260EE();POP32(g_esp,g_ebx);}POP32(g_esp,g_edi);}}POP32(g_esp,g_esi);g_esp+=8;
}
int uc_migration_audio_device_params(uint32_t a){if(!device_params_admit(a))return 0;if(a==0x244140)device_params_frequency();else if(a==0x2441b0)device_params_gain();else if(a==0x251d80)device_params_filter();else device_params_spatial();return 1;}
