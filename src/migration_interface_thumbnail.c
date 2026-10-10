#include "migration_interface_thumbnail.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_000871A0(void),sub_00065760(void),sub_002D50C0(void),ucrr_level_thumb_bounds(void);
#define TH_CALL(n,r) do{g_ebp=frame;PUSH32(g_esp,r);sub_##n();}while(0)
static unsigned th_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static int th_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void thumbnail_back(unsigned frame){
 g_esp-=64;g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_xmm2=XMM_SCALAR(MEMF(0x44c1f8));MEMF(g_esp+48)=MEMF(g_esp+52)=MEMF(g_esp+56)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(frame+12));MEMF(g_esp+60)=g_xmm0.f[0];g_xmm0.f[0]*=MEMF(0x48856c);g_xmm0.f[0]+=MEMF(0x48817c);g_xmm1=g_xmm0;g_xmm0.f[0]*=MEMF(0x488178);g_xmm1.f[0]*=MEMF(0x488174);g_xmm0.f[0]*=g_xmm2.f[0];MEMF(g_esp+20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(frame+16));g_xmm0.f[0]+=MEMF(0x488170);g_eax=g_esp+48;g_xmm1.f[0]*=g_xmm2.f[0];PUSH32(g_esp,g_eax);g_ecx=g_esp+16;MEMF(g_esp+20)=g_xmm1.f[0];MEMF(g_esp+48)=g_xmm0.f[0];TH_CALL(000871A0,0x26f13e);
 g_xmm2=XMM_SCALAR(MEMF(g_esp+16));g_xmm1=XMM_SCALAR(MEMF(0x48816c));g_xmm3=XMM_SCALAR(MEMF(g_esp+20));PUSH32(g_esp,0);g_ecx=g_esp+16;PUSH32(g_esp,g_ecx);g_ecx=MEM32(frame+8);g_edx=g_esp+32;g_xmm0=g_xmm2;g_xmm0.f[0]+=g_xmm1.f[0];PUSH32(g_esp,g_edx);MEMF(g_esp+36)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+56));g_eax=g_esp+44;g_xmm4=g_xmm0;PUSH32(g_esp,g_eax);g_xmm4.f[0]+=g_xmm3.f[0];g_xmm1.f[0]-=g_xmm2.f[0];g_xmm0.f[0]-=g_xmm3.f[0];PUSH32(g_esp,g_ecx);MEMF(g_esp+48)=g_xmm4.f[0];MEMF(g_esp+52)=g_xmm1.f[0];MEMF(g_esp+56)=g_xmm0.f[0];TH_CALL(00065760,0x26f1a0);g_esp+=20;
}
static void thumbnail_alpha(unsigned frame){
 g_eax=MEM32(frame+12);PUSH32(g_esp,g_eax);TH_CALL(002D50C0,0x26f1e1);MEMF(frame+12)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(0x44bf94);g_xmm0=XMM_MEM(0x500a80);g_fp_stack[g_fp_top]-=MEMF(frame+12);g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm7=XMM_SCALAR(MEMF(frame+16));XMM_STORE(g_esp+52,g_xmm0);XMM_STORE(g_esp+84,g_xmm0);XMM_STORE(g_esp+68,g_xmm0);g_xmm0=XMM_SCALAR(MEMF(0x500a4c));g_fp_stack[g_fp_top]*=MEMF(0x44c1f8);g_esp+=4;g_xmm0.f[0]+=g_xmm7.f[0];unsigned cmp=th_cmp(g_xmm1.f[0],g_xmm0.f[0]);MEMF(g_esp+60)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_xmm4=g_xmm1;g_xmm4.f[0]-=MEMF(frame+20);g_xmm5=XMM_ZERO();
 if(cmp&0x41)g_xmm3=g_xmm1;else{g_xmm3=g_xmm0;if(th_cmp(g_xmm0.f[0],g_xmm5.f[0])&0x41)g_xmm3=g_xmm5;}
 cmp=th_cmp(g_xmm1.f[0],g_xmm0.f[0]);g_xmm3.f[0]*=MEMF(frame+12);MEMF(g_esp+92)=g_xmm3.f[0];if(cmp&0x41)g_xmm2=g_xmm1;else{g_xmm2=g_xmm0;if(th_cmp(g_xmm0.f[0],g_xmm5.f[0])&0x41)g_xmm2=g_xmm5;}
 cmp=th_cmp(g_xmm1.f[0],g_xmm0.f[0]);g_xmm2.f[0]*=MEMF(g_esp+60);MEMF(g_esp+60)=g_xmm2.f[0];if(cmp&0x41)g_xmm0=g_xmm1;else if(th_cmp(g_xmm0.f[0],g_xmm5.f[0])&0x41)g_xmm0=g_xmm5;
 SET_LO8(g_eax,MEM8(frame+24));g_xmm0.f[0]*=MEMF(g_esp+76);MEMF(g_esp+76)=g_xmm0.f[0];if(LO8(g_eax)){g_ecx=MEM32(0x51c4dc);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_ecx+8);g_fp_stack[g_fp_top]*=MEMF(0x44d95c);g_fp_stack[g_fp_top]=sin(g_fp_stack[g_fp_top]);g_fp_stack[g_fp_top]*=MEMF(0x44d660);g_fp_stack[g_fp_top]+=MEMF(0x44c47c);g_fp_stack[g_fp_top]*=MEMF(g_esp+60);MEMF(g_esp+60)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_xmm2=XMM_SCALAR(MEMF(g_esp+60));}
 g_xmm6=XMM_SCALAR(MEMF(0x500a50));if(!(th_cmp(g_xmm7.f[0],g_xmm6.f[0])&0x41)){g_xmm7.f[0]-=g_xmm6.f[0];g_xmm1.f[0]-=g_xmm7.f[0];g_xmm7=XMM_SCALAR(MEMF(frame+16));g_xmm6=g_xmm1;g_xmm6.f[0]*=g_xmm3.f[0];g_xmm3=g_xmm6;g_xmm6=g_xmm1;g_xmm6.f[0]*=g_xmm2.f[0];g_xmm1.f[0]*=g_xmm0.f[0];g_xmm2=g_xmm6;g_xmm0=g_xmm1;}
}
static void thumbnail_geometry(unsigned frame){
 g_xmm1=XMM_SCALAR(MEMF(0x500a44));g_xmm0.f[0]*=g_xmm4.f[0];MEMF(g_esp+76)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x500a38));g_xmm2.f[0]*=g_xmm4.f[0];g_xmm3.f[0]*=g_xmm4.f[0];g_xmm4=XMM_SCALAR(MEMF(0x500a30));g_xmm0.f[0]+=g_xmm4.f[0];g_xmm0.f[0]*=g_xmm7.f[0];MEMF(g_esp+60)=g_xmm2.f[0];g_xmm2=g_xmm5;g_xmm2.f[0]-=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x500a48));MEMF(g_esp+92)=g_xmm3.f[0];g_xmm3=XMM_SCALAR(MEMF(frame+12));unsigned cmp=th_cmp(g_xmm3.f[0],g_xmm5.f[0]);g_xmm0.f[0]-=g_xmm1.f[0];g_xmm0.f[0]*=g_xmm3.f[0];g_xmm0.f[0]+=g_xmm1.f[0];MEMF(g_esp+12)=g_xmm0.f[0];int multiply=1,load=1;
 if(!(cmp&0x41)){g_xmm1=XMM_SCALAR(MEMF(0x500a68));g_xmm3=XMM_SCALAR(MEMF(frame+28));if(!(th_cmp(g_xmm1.f[0],g_xmm3.f[0])&0x41)){g_xmm1=XMM_SCALAR(MEMF(0x500a64));g_xmm1.f[0]*=g_xmm3.f[0];g_xmm1.f[0]+=MEMF(0x500a60);}else if(!(th_cmp(g_xmm3.f[0],g_xmm1.f[0])&1)){g_xmm3=XMM_SCALAR(MEMF(0x500a64));g_xmm3.f[0]*=g_xmm1.f[0];g_xmm3.f[0]+=MEMF(0x500a60);g_xmm3.f[0]*=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm3.f[0];multiply=0;}else multiply=load=0;}else g_xmm1=XMM_SCALAR(MEMF(0x500a60));
 if(multiply){g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+12)=g_xmm1.f[0];}if(load)g_xmm0=XMM_SCALAR(MEMF(g_esp+12));
 g_xmm1=XMM_SCALAR(MEMF(0x51d0e8));g_xmm5=XMM_SCALAR(MEMF(0x500a34));g_xmm6=XMM_SCALAR(MEMF(0x44c1f8));g_esi=MEM32(frame+8);g_xmm1.f[0]-=g_xmm2.f[0];g_xmm2=g_xmm4;g_xmm2.f[0]*=g_xmm0.f[0];g_xmm2.f[0]*=g_xmm6.f[0];g_xmm3=g_xmm5;g_xmm3.f[0]*=g_xmm0.f[0];g_xmm3.f[0]*=g_xmm6.f[0];g_xmm6=g_xmm1;g_xmm6.f[0]-=g_xmm2.f[0];g_xmm2.f[0]+=g_xmm1.f[0];MEMF(g_esp+32)=g_xmm2.f[0];g_xmm2=XMM_SCALAR(MEMF(g_esp+92));unsigned flags=th_cmp(g_xmm2.f[0],MEMF(0x44c174));MEMF(g_esp+40)=g_xmm6.f[0];g_xmm6=XMM_SCALAR(MEMF(0x488168));SET_HI8(g_eax,flags);g_xmm7=g_xmm6;g_xmm7.f[0]-=g_xmm3.f[0];g_xmm3.f[0]+=g_xmm6.f[0];MEMF(g_esp+24)=g_xmm1.f[0];MEMF(g_esp+44)=g_xmm7.f[0];MEMF(g_esp+36)=g_xmm3.f[0];ucrr_level_thumb_bounds();
 if(RECOMP_PARITY8(flags&0x44)){
  g_edx=g_esp+80;PUSH32(g_esp,g_edx);g_ecx=g_esp+20;TH_CALL(000871A0,0x26f45d);PUSH32(g_esp,0);g_eax=g_esp+20;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+16);g_ecx=g_esp+40;PUSH32(g_esp,g_ecx);g_edx=g_esp+52;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);TH_CALL(00065760,0x26f477);g_xmm4=XMM_SCALAR(MEMF(0x500a30));g_xmm6=XMM_SCALAR(MEMF(0x488168));g_xmm5=XMM_SCALAR(MEMF(0x500a34));g_xmm0=XMM_SCALAR(MEMF(g_esp+32));g_xmm1=XMM_SCALAR(MEMF(g_esp+44));g_esp+=20;
 }
 g_xmm2=XMM_SCALAR(MEMF(g_esp+60));SET_HI8(g_eax,th_cmp(g_xmm2.f[0],MEMF(0x44c174)));
 if(RECOMP_PARITY8(HI8(g_eax)&0x44)){
  g_ecx=g_esp+48;PUSH32(g_esp,g_ecx);g_ecx=g_esp+20;TH_CALL(000871A0,0x26f4bf);PUSH32(g_esp,0);g_edx=g_esp+20;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+20);g_eax=g_esp+40;PUSH32(g_esp,g_eax);g_ecx=g_esp+52;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);TH_CALL(00065760,0x26f4d9);g_xmm4=XMM_SCALAR(MEMF(0x500a30));g_xmm6=XMM_SCALAR(MEMF(0x488168));g_xmm5=XMM_SCALAR(MEMF(0x500a34));g_xmm0=XMM_SCALAR(MEMF(g_esp+32));g_xmm1=XMM_SCALAR(MEMF(g_esp+44));g_esp+=20;
 }
}
static int thumbnail_badge(unsigned frame){
 g_eax=MEM32(frame+32);if((int32_t)g_eax<0)return 0;unsigned index=g_eax;
 g_xmm2=XMM_SCALAR(MEMF(0x500a5c));g_xmm3=XMM_SCALAR(MEMF(0x44bf94));g_xmm3.f[0]-=g_xmm2.f[0];g_xmm4.f[0]*=g_xmm0.f[0];g_xmm5.f[0]*=g_xmm0.f[0];g_xmm7=g_xmm2;g_xmm2=XMM_SCALAR(MEMF(0x500a54));g_xmm7.f[0]*=MEMF(frame+12);g_xmm4.f[0]*=g_xmm2.f[0];g_xmm0=g_xmm5;g_xmm5=XMM_SCALAR(MEMF(0x500a58));g_xmm0.f[0]*=g_xmm2.f[0];MEMF(g_esp+24)=g_xmm4.f[0];g_ecx=MEM32(g_esp+24);MEMF(g_esp+28)=g_xmm0.f[0];g_edx=MEM32(g_esp+28);g_xmm7.f[0]+=g_xmm3.f[0];g_xmm7.f[0]*=MEMF(g_esp+76);MEM32(g_esp+16)=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_esp+16));MEM32(g_esp+20)=g_edx;g_xmm3=XMM_SCALAR(MEMF(g_esp+20));g_xmm2=g_xmm0;g_xmm2.f[0]*=g_xmm5.f[0];g_xmm4=g_xmm3;g_xmm4.f[0]*=g_xmm5.f[0];g_xmm5=g_xmm1;g_xmm5.f[0]-=g_xmm2.f[0];g_xmm2=g_xmm6;MEMF(g_esp+24)=g_xmm5.f[0];g_ecx=MEM32(g_esp+24);g_xmm2.f[0]-=g_xmm4.f[0];MEMF(g_esp+28)=g_xmm2.f[0];g_edx=MEM32(g_esp+28);g_xmm0.f[0]+=g_xmm1.f[0];g_xmm3.f[0]+=g_xmm6.f[0];MEM32(g_esp+40)=g_ecx;MEM32(g_esp+44)=g_edx;MEMF(g_esp+24)=g_xmm0.f[0];g_ecx=MEM32(g_esp+24);MEMF(g_esp+28)=g_xmm3.f[0];g_edx=MEM32(g_esp+28);MEMF(g_esp+76)=g_xmm7.f[0];MEM32(g_esp+32)=g_ecx;MEM32(g_esp+36)=g_edx;if(index>3)return 0;
 unsigned target=MEM32(g_eax*4+0x26f688);if(target!=0x26f5ee&&target!=0x26f621&&target!=0x26f653){g_seh_ebp=frame;RECOMP_ITAIL(target);return 2;}
 if(target==0x26f5ee){g_eax=g_esp+64;PUSH32(g_esp,g_eax);g_ecx=g_esp+20;TH_CALL(000871A0,0x26f5fc);PUSH32(g_esp,0);g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x51d0e4);g_edx=g_esp+40;PUSH32(g_esp,g_edx);g_eax=g_esp+52;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);TH_CALL(00065760,0x26f619);}
 else if(target==0x26f621){g_edx=g_esp+64;PUSH32(g_esp,g_edx);g_ecx=g_esp+16;TH_CALL(000871A0,0x26f62f);PUSH32(g_esp,0);g_eax=g_esp+16;PUSH32(g_esp,g_eax);g_eax=MEM32(0x51d0dc);g_ecx=g_esp+40;PUSH32(g_esp,g_ecx);g_edx=g_esp+52;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);TH_CALL(00065760,0x26f64b);}
 else{g_ecx=g_esp+64;PUSH32(g_esp,g_ecx);g_ecx=g_esp+28;TH_CALL(000871A0,0x26f661);PUSH32(g_esp,0);g_edx=g_esp+28;PUSH32(g_esp,g_edx);g_edx=MEM32(0x51d0e0);g_eax=g_esp+40;PUSH32(g_esp,g_eax);g_ecx=g_esp+52;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);TH_CALL(00065760,0x26f67e);}
 g_esp+=20;return target!=0x26f653;
}
int migration_interface_thumbnail(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_THUMBNAIL");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;
 unsigned s=g_esp;if(s<0x10800||(s&3)||!th_span(s-2048,2096))return 0;if(op){if(!th_span(MEM32(s+4),24)||(MEM8(s+20)&&!th_span(MEM32(0x51c4dc),12)))return 0;for(unsigned i=0;i<4;i++){unsigned t=MEM32(0x26f688+i*4);if(t!=0x26f5ee&&t!=0x26f621&&t!=0x26f653)return 0;}}
 PUSH32(g_esp,g_ebp);unsigned frame=g_esp;g_ebp=frame;g_esp&=~15u;
 if(!op){thumbnail_back(frame);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=!g_esp;g_esp+=4;return 1;}
 g_esp-=92;g_xmm0=XMM_SCALAR(MEMF(0x500a50));g_xmm0.f[0]+=MEMF(0x44bf94);g_xmm1=XMM_SCALAR(MEMF(frame+16));int active=(th_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41)!=0;PUSH32(g_esp,g_esi);int result=0;
 if(active){thumbnail_alpha(frame);thumbnail_geometry(frame);result=thumbnail_badge(frame);if(result==2)return 1;}
 POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);if(result)g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
