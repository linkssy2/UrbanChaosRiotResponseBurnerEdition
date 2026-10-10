#include "migration_audio_curve_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0024B430(void);
static int curve_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_CURVE");f=v&&!strcmp(v,"1");}return f;}
static int curve_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int curve_safe(uint32_t p,uint32_t n){return curve_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+16<=p);}
static int curve_above(float a,float b){double x=a,y=b;return x==x&&y==y&&x>y;}
static int curve_admit(uint32_t a){
    if((a!=0x249960&&a!=0x24b430)||!curve_enabled()||!curve_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!curve_span(g_esp-4096,4112))return 0;
    if(a==0x24b430)return curve_safe(g_ecx,94);
    if(!curve_safe(g_ecx,48)||!curve_safe(MEM32(g_esp+4),24)||!curve_safe(MEM32(g_esp+8),6))return 0;
    uint32_t definition=MEM32(g_esp+8),count=MEM16(definition+4);
    if(!count||!curve_safe(MEM32(g_esp+12),count*4))return 0;
    uint32_t head=g_ecx+40,node=MEM32(head),previous=head;
    /* ponytail: O(n) admission; use trusted list generations if this becomes measurable. */
    for(unsigned n=0;node!=head;n++){
        if(n>=4096||node<20||!curve_safe(node-20,94)||MEM32(node+4)!=previous||!curve_span(MEM32(node-20),4))return 0;
        previous=node;node=MEM32(node);
    }
    return MEM32(head+4)==previous;
}
static void curve_accumulate(void){
    g_eax=MEM32(g_esp+8);uint32_t mode=g_eax;
    if(mode<=2){uint32_t offset=mode==0?90:mode==1?88:92;SET_LO16(g_eax,MEM16(g_ecx+offset));
        if(LO16(g_eax)==0x7fff){SET_LO16(g_eax,MEM16(g_esp+4));MEM16(g_ecx+offset)=LO16(g_eax);g_recomp_zf=1;}
        else{g_edx=MEM32(g_esp+4);g_eax+=g_edx;if(mode==1){g_eax=SX16(LO16(g_eax));g_edx=(int32_t)g_eax<0?0xffffffffu:0;g_eax-=g_edx;g_eax=(uint32_t)((int32_t)g_eax>>1);}MEM16(g_ecx+offset)=LO16(g_eax);if(mode!=2)g_recomp_zf=g_eax==0;}
    }g_esp+=12;
}
static void curve_interpolate(void){
    uint32_t frame=g_seh_ebp;
    g_eax=MEM32(g_esp+4);g_edx=MEM32(g_eax+8);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=(double)SMEM32(g_eax+8);
    if((int32_t)g_edx<0)g_fp_stack[g_fp_top]+=MEMF(0x44c510);
    g_fp_stack[g_fp_top]*=MEMF(g_eax+16);g_fp_stack[g_fp_top]+=MEMF(g_eax+20);MEMF(g_esp+4)=(float)g_fp_stack[g_fp_top];
    g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(0x44c174);g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;
    if(g_fp_cmp>0){g_xmm0=XMM_ZERO();MEMF(g_esp+4)=g_xmm0.f[0];}
    else{g_xmm0=XMM_SCALAR(MEMF(g_esp+4));g_xmm1=XMM_SCALAR(MEMF(0x44bf94));MEMF(g_esp+4)=g_xmm1.f[0];if(!curve_above(g_xmm0.f[0],g_xmm1.f[0]))MEMF(g_esp+4)=g_xmm0.f[0];}
    g_eax=MEM32(g_ecx+40);g_ecx+=40;g_edx=0;
    if(g_eax!=g_ecx){
        PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);g_esi=MEM16(g_edi);PUSH32(g_esp,g_ebx);int found=0;
        for(;;){g_ebx=g_eax-20;g_eax=MEM32(g_eax);if(g_esi==g_edx){found=1;break;}g_edx++;if(g_eax==g_ecx)break;}
        if(!found)g_recomp_zf=1;
        else{
            g_eax=MEM32(g_ebx);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed44);g_ecx=g_ebx;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2499e9);RECOMP_ICALL_SAFE(target,saved);
            if(g_eax){
                g_edx=MEM16(g_edi+4);g_ecx=MEM32(g_esp+24);g_xmm0=XMM_SCALAR(MEMF(0x483bbc));g_eax=0;PUSH32(g_esp,frame);int above=0;
                if(g_edx>=4){
                    g_esi=g_ecx+8;
                    do{
                        for(unsigned j=0;j<4;j++){
                            frame=MEM16(g_esi-8+j*4);if(!j)g_xmm1=XMM_SCALAR(MEMF(g_esp+20));g_xmm2.f[0]=(float)(int32_t)frame;g_xmm2.f[0]*=g_xmm0.f[0];
                            if(curve_above(g_xmm2.f[0],g_xmm1.f[0])){g_eax+=j;above=1;break;}
                        }
                        if(above)break;g_eax+=4;frame=g_edx-3;g_esi+=16;
                    }while(g_eax<frame);
                }else g_xmm1=XMM_SCALAR(MEMF(g_esp+20));
                if(!above)while(g_eax<g_edx){g_esi=MEM16(g_ecx+g_eax*4);g_xmm2.f[0]=(float)(int32_t)g_esi;g_xmm2.f[0]*=g_xmm0.f[0];if(curve_above(g_xmm2.f[0],g_xmm1.f[0]))break;g_eax++;}
                int end=g_eax==g_edx;POP32(g_esp,frame);if(end)g_eax=g_edx-1;g_esi=g_eax?g_eax-1:0;SET_LO16(g_edx,MEM16(g_edi+2));uint32_t mode=LO16(g_edx);
                if(mode<=2){
                    SET_LO16(g_edi,MEM16(g_ecx+g_esi*4));int equal=MEM16(g_ecx+g_eax*4)==LO16(g_edi);
                    if(equal){SET_LO16(g_eax,MEM16(g_ecx+g_eax*4+2));PUSH32(g_esp,mode);g_ecx=g_ebx;PUSH32(g_esp,g_eax);PUSH32(g_esp,mode==0?0x249ace:mode==1?0x249b51:0x249bd0);sub_0024B430();g_recomp_zf=1;}
                    else{
                        g_edx=MEM16(g_ecx+g_eax*4);
                        if(mode==1)g_eax=(uint32_t)(int32_t)SMEM16(g_ecx+g_eax*4+2);
                        SET_LO16(g_esi,MEM16(g_ecx+g_esi*4+2));
                        if(mode==0)g_ecx=(uint32_t)(int32_t)SMEM16(g_ecx+g_eax*4+2);
                        if(mode==1)g_edi=LO16(g_edi);
                        g_xmm2.f[0]=(float)(int32_t)g_edx;g_xmm2.f[0]*=g_xmm0.f[0];
                        if(mode!=1)g_edi=LO16(g_edi);
                        g_xmm2.f[0]-=g_xmm1.f[0];
                        if(mode==1)g_ecx=SX16(LO16(g_esi));
                        g_edx-=g_edi;
                        if(mode==1)g_eax-=g_ecx;
                        g_xmm1.f[0]=(float)(int32_t)g_edx;
                        if(mode==2)g_edx=(uint32_t)(int32_t)SMEM16(g_ecx+g_eax*4+2);
                        g_xmm1.f[0]*=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf94));
                        if(mode==0){g_edx=SX16(LO16(g_esi));g_ecx-=g_edx;}
                        if(mode==2){g_eax=SX16(LO16(g_esi));g_edx-=g_eax;}
                        g_xmm2.f[0]/=g_xmm1.f[0];
                        if(mode!=2)g_xmm1.f[0]=(float)(int32_t)(mode==0?g_ecx:g_eax);
                        g_xmm0.f[0]-=g_xmm2.f[0];
                        if(mode==2)g_xmm1.f[0]=(float)(int32_t)g_edx;
                        g_xmm0.f[0]*=g_xmm1.f[0];g_eax=(int32_t)g_xmm0.f[0];g_eax+=g_esi;
                        PUSH32(g_esp,mode);g_ecx=g_ebx;PUSH32(g_esp,g_eax);PUSH32(g_esp,mode==0?0x249b28:mode==1?0x249bab:0x249c2a);sub_0024B430();if(mode!=2)g_recomp_zf=g_eax==0;
                    }
                }
            }
        }
        POP32(g_esp,g_ebx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);
    }g_esp+=16;
}
int uc_migration_audio_curve(uint32_t a){if(!curve_admit(a))return 0;if(a==0x249960)curve_interpolate();else curve_accumulate();return 1;}
