#include "migration_audio_scalars_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
extern void sub_002D6780(void);
extern void sub_002D6EB0(void);
static int scalars_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_SCALARS");f=v&&!strcmp(v,"1");}return f;}
static int scalars_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int scalars_admit(uint32_t a){return (a==0x244200||a==0x244260||a==0x2470c0)&&scalars_enabled()&&scalars_span(0x51c178,1)&&!MEM8(0x51c178)&&!(g_esp&3)&&g_esp>=0x11000&&scalars_span(g_esp-4096,4104);}
static unsigned scalars_compare(float a,float b){double x=a,y=b;return x!=x||y!=y?0x47u:x<y?3u:x>y?2u:0x42u;}
static void scalars_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void scalars_attenuation(void){
    g_xmm1=XMM_ZERO();g_xmm0=XMM_SCALAR(MEMF(g_esp+4));unsigned flags=scalars_compare(g_xmm1.f[0],g_xmm0.f[0]);
    if(!(flags&1)){SET_LO16(g_eax,MEM16(0x4839b0));g_recomp_zf=!!(flags&0x40);g_esp+=4;return;}
    flags=scalars_compare(g_xmm0.f[0],MEMF(0x44bf94));if(!(flags&1)){SET_LO16(g_eax,MEM16(0x4839b4));g_recomp_zf=!!(flags&0x40);g_esp+=4;return;}
    scalars_push(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=MEMF(g_esp+4);scalars_push(0.30102999566398119521);double swap=g_fp_stack[g_fp_top];g_fp_stack[g_fp_top]=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[(g_fp_top+1)&7]=swap;g_fp_stack[(g_fp_top+1)&7]*=log2(g_fp_stack[g_fp_top]);g_fp_top=(g_fp_top+1)&7;g_fp_stack[g_fp_top]*=MEMF(0x483a24);PUSH32(g_esp,0x244240);sub_002D6780();
    g_ecx=MEM32(0x4839b0);int equal=(int16_t)LO16(g_eax)==(int16_t)LO16(g_ecx);if((int16_t)LO16(g_eax)<(int16_t)LO16(g_ecx))g_eax=g_ecx;else{g_ecx=MEM32(0x4839b4);equal=(int16_t)LO16(g_eax)==(int16_t)LO16(g_ecx);if((int16_t)LO16(g_eax)>(int16_t)LO16(g_ecx))g_eax=g_ecx;}g_recomp_zf=equal;g_esp+=4;
}
static void scalars_volume(void){
    g_eax=(uint32_t)(int32_t)SMEM16(g_esp+4);MEM32(g_esp+4)=g_eax;if((int32_t)g_eax>=(int32_t)MEM32(0x4839b4)){scalars_push(MEMF(0x44bf94));g_recomp_zf=g_eax==MEM32(0x4839b4);g_esp+=4;return;}if((int32_t)g_eax<=(int32_t)MEM32(0x4839b0)){scalars_push(MEMF(0x44c174));g_recomp_zf=g_eax==MEM32(0x4839b0);g_esp+=4;return;}
    scalars_push(MEMD(0x483a18));scalars_push((double)SMEM32(g_esp+4));g_fp_stack[g_fp_top]*=MEMF(0x452834);PUSH32(g_esp,0x24429c);sub_002D6EB0();MEMF(g_esp+4)=(float)g_fp_stack[g_fp_top];scalars_push(MEMF(0x44c174));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;
    if(g_fp_cmp>0){g_xmm0=XMM_ZERO();MEMF(g_esp+4)=g_xmm0.f[0];scalars_push(MEMF(g_esp+4));g_recomp_zf=g_fp_cmp==0;}else{g_xmm0=XMM_SCALAR(MEMF(g_esp+4));g_xmm1=XMM_SCALAR(MEMF(0x44bf94));unsigned flags=scalars_compare(g_xmm0.f[0],g_xmm1.f[0]);MEMF(g_esp+4)=g_xmm1.f[0];if(flags&0x41)MEMF(g_esp+4)=g_xmm0.f[0];scalars_push(MEMF(g_esp+4));g_recomp_zf=!!(flags&0x40);}g_esp+=4;
}
int uc_migration_audio_scalars(uint32_t a){
    if(!scalars_admit(a))return 0;if(a==0x244200)scalars_attenuation();else if(a==0x244260)scalars_volume();else{g_xmm0=XMM_SCALAR(MEMF(g_esp+4));unsigned flags=scalars_compare(g_xmm0.f[0],MEMF(0x44c174));SET_HI8(g_eax,flags);unsigned bits=HI8(g_eax)&0x44;if(RECOMP_PARITY8(bits)){scalars_push(MEMF(0x483a34));g_fp_stack[g_fp_top]/=MEMF(g_esp+4);}else scalars_push(MEMF(0x44e088));g_recomp_zf=bits==0;g_esp+=4;}return 1;
}
