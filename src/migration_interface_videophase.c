#include "migration_interface_videophase.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00273DC0(void),sub_00274010(void),sub_0005AAB0(void),sub_000643E0(void),sub_00087960(void),sub_000877B0(void);
#define VP_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
static unsigned vp_cmp(double a,double b){return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
static int vp_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void videophase_playback(void){
 unsigned frame=g_seh_ebp;g_xmm0=XMM_SCALAR(MEMF(g_esp+20));unsigned c=vp_cmp(g_xmm0.f[0],MEMF(0x44c174));PUSH32(g_esp,g_ebx);SET_LO8(g_ebx,MEM8(g_esp+16));SET_HI8(g_eax,c);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+24);
 if(!RECOMP_PARITY8(HI8(g_eax)&0x44)&&(LO8(g_ebx)&2)&&!(MEM8(g_esi)&2)&&!(LO8(g_ebx)&4)){VP_CALL(000643E0,0x273df1);g_eax=(uint32_t)(int32_t)SMEM8(g_eax+59);g_xmm0.f[0]=(float)(int32_t)g_eax;g_xmm0.f[0]*=MEMF(0x44ff80);MEMF(g_esp+28)=g_xmm0.f[0];if((int8_t)LO8(g_ebx)<0){g_xmm0=XMM_ZERO();MEMF(g_esp+28)=g_xmm0.f[0];}g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_esp+28);g_eax=MEM32(g_esp+12);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);VP_CALL(00087960,0x273e30);g_esp+=28;}
 g_xmm0=XMM_SCALAR(MEMF(g_esp+32));SET_HI8(g_eax,vp_cmp(g_xmm0.f[0],MEMF(0x44bf94)));if(!RECOMP_PARITY8(HI8(g_eax)&0x44)){if(((LO8(g_ebx)&2)&&!(MEM8(g_esi+8)&2))||(MEM8(g_esi+8)&4)){VP_CALL(000877B0,0x273e5c);}if(LO8(g_ebx)&32){POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_seh_ebp=frame;sub_000877B0();return;}}
 POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=4;
}
static void videophase_update(void){
 unsigned frame=g_seh_ebp;PUSH32(g_esp,g_ecx);g_xmm3=XMM_SCALAR(MEMF(0x46c320));g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_xmm2=XMM_ZERO();PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+28);g_xmm0=XMM_SCALAR(MEMF(g_edi+4));g_xmm0.f[0]*=g_xmm3.f[0];unsigned c=vp_cmp(g_xmm1.f[0],g_xmm0.f[0]);g_esi=g_ecx;if(c&0x41){g_xmm0=g_xmm1;MEMF(g_esp+16)=g_xmm0.f[0];}else if(vp_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41)MEMF(g_esp+16)=g_xmm2.f[0];else MEMF(g_esp+16)=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(g_edi+12));if(!(vp_cmp(g_xmm0.f[0],g_xmm2.f[0])&0x41)){g_xmm0.f[0]*=g_xmm3.f[0];if(!(vp_cmp(g_xmm1.f[0],g_xmm0.f[0])&0x41)){c=vp_cmp(g_xmm0.f[0],g_xmm2.f[0]);g_xmm1=g_xmm0;if(!(c&0x41))g_xmm2=g_xmm1;}else g_xmm2=g_xmm1;}
 frame=MEM32(g_esp+16);g_eax=MEM32(g_esi+4);g_edx=MEM32(g_esi+8);MEMF(g_esp+28)=g_xmm2.f[0];g_ebx=MEM32(g_esp+28);PUSH32(g_esp,g_ebx);g_ecx=0;SET_LO8(g_ecx,MEM8(g_esi+12));PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);VP_CALL(00273DC0,0x273d1c);g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);VP_CALL(00274010,0x273d28);g_esp+=40;VP_CALL(0005AAB0,0x273d30);
 if(g_eax){VP_CALL(0005AAB0,0x273d39);g_ecx=MEM32(g_eax+324);if(g_ecx){VP_CALL(0005AAB0,0x273d48);g_eax=MEM32(g_eax+324);g_edx=MEM32(g_eax);g_ecx=g_eax;unsigned saved=g_esp,target=MEM32(g_edx+460);PUSH32(g_esp,0x273d58);RECOMP_ICALL_SAFE(target,saved);if(g_eax){VP_CALL(0005AAB0,0x273d61);g_eax=MEM32(g_eax+324);g_edx=MEM32(g_eax);g_ecx=g_eax;saved=g_esp;target=MEM32(g_edx+472);PUSH32(g_esp,0x273d71);RECOMP_ICALL_SAFE(target,saved);if(!LO8(g_eax)){g_xmm0=XMM_SCALAR(MEMF(g_edi+12));SET_HI8(g_eax,vp_cmp(g_xmm0.f[0],MEMF(0x44c174)));unsigned tested=HI8(g_eax)&0x44;if(!RECOMP_PARITY8(tested)){VP_CALL(0005AAB0,0x273d8c);g_eax=MEM32(g_eax+324);g_edx=MEM32(g_eax);saved=g_esp;PUSH32(g_esp,1);g_ecx=g_eax;target=MEM32(g_edx+468);PUSH32(g_esp,0x273d9e);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=0x273e70;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=!tested;g_esp+=12;return;}}}}}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=0;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=12;
}
int migration_interface_videophase(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_VIDEOPHASE");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation>1||(g_esp&3)||!vp_span((uint64_t)g_esp-1024,1056))return 0;
 if(operation){if(!vp_span(MEM32(g_esp+16),16))return 0;videophase_playback();}else{if(!vp_span(g_ecx,16)||!vp_span(MEM32(g_esp+8),16))return 0;videophase_update();}return 1;
}
