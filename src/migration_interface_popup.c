#include "migration_interface_popup.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00017060(void),sub_00017E40(void),sub_002D57FB(void),sub_00258BC0(void),sub_00253FF0(void),sub_00258D40(void);
static int popup_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
#define PP_TOP g_fp_stack[g_fp_top]
#define PP_ST1 g_fp_stack[(g_fp_top+1)&7]
#define PP_POP() (g_fp_top=(g_fp_top+1)&7)
static void popup_push(double value){g_fp_top=(g_fp_top+7)&7;PP_TOP=value;}
static void popup_construct(void){
 unsigned frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39f238);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+16)=g_esi;g_eax=MEM32(g_esp+36);g_ecx=MEM32(g_esp+40);frame=MEM32(g_esp+44);g_edx=MEM32(g_esp+56);g_xmm0=XMM_SCALAR(MEMF(g_esp+64));MEM32(g_esi+4)=g_eax;SET_LO8(g_eax,MEM8(g_esp+60));MEM32(g_esi+8)=g_ecx;g_ecx=MEM32(g_esp+68);MEM32(g_esp+28)=0;MEM32(g_esi)=0x485b78;MEM32(g_esi+12)=frame;MEM32(g_esi+80)=g_edx;MEM8(g_esi+84)=LO8(g_eax);MEMF(g_esi+88)=g_xmm0.f[0];MEM32(g_esi+92)=g_ecx;
 if(frame){g_eax=MEM32(g_esp+52);g_edx=MEM32(g_esp+48);g_ecx=g_esi+48;g_edx-=g_eax;g_edi=frame;do{g_ebx=MEM32(g_edx+g_eax);MEM32(g_ecx-32)=g_ebx;g_ebx=MEM32(g_eax);MEM32(g_ecx)=g_ebx;g_eax+=4;g_ecx+=4;g_edi--;}while(g_edi);}
 SET_LO8(g_edx,MEM8(g_esp+60));g_xmm0=XMM_ZERO();g_eax=MEM32(g_esp+68);MEM8(g_esi+100)=LO8(g_edx);MEM8(g_esi+101)=0;MEMF(g_esi+136)=g_xmm0.f[0];g_ecx=MEM32(0x509704);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x258c6f);sub_00017060();
 if(frame){g_edi=g_esi+104;g_ebx=frame;do{g_ecx=MEM32(g_edi-88);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x509704);PUSH32(g_esp,0x258c90);sub_00017E40();MEMF(g_edi)=(float)PP_TOP;PP_TOP+=MEMF(g_esi+136);g_edi+=4;g_ebx--;MEMF(g_esi+136)=(float)PP_TOP;PP_POP();}while(g_ebx);}
 SET_LO8(g_eax,MEM8(g_esp+60));
 if(LO8(g_eax)){
  g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);g_eax=g_esi+100;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x258cc4);sub_00017E40();popup_push(MEMF(g_esp+64));PP_TOP+=PP_TOP;frame--;MEM32(g_esp+60)=frame;PP_ST1+=PP_TOP;PP_POP();MEMF(g_esi+96)=(float)PP_TOP;popup_push((double)SMEM32(g_esp+60));if((int32_t)frame<0)PP_TOP+=MEMF(0x44c510);PP_TOP*=PP_ST1;PP_TOP+=MEMF(g_esi+136);MEMF(g_esi+136)=(float)PP_TOP;PP_POP();PP_POP();
 }else{g_edx=MEM32(g_esi+12)-1;MEM32(g_esp+60)=g_edx;popup_push((double)SMEM32(g_esp+60));if((int32_t)g_edx<0)PP_TOP+=MEMF(0x44c510);PP_TOP*=MEMF(0x485a40);PP_TOP+=MEMF(g_esi+136);MEMF(g_esi+136)=(float)PP_TOP;PP_POP();}
 g_ecx=MEM32(g_esp+20);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=40;
}
int migration_interface_popup(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_POPUP");enabled=s&&!strcmp(s,"1");}if(!enabled||op>2)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!popup_span(s-1024,1080))return 0;
 if(op==0){unsigned n=MEM32(s+12);if(!popup_span(g_ecx,140)||n>8||(n&&(!popup_span(MEM32(s+16),n*4)||!popup_span(MEM32(s+20),n*4))))return 0;popup_construct();return 1;}
 if(!popup_span(MEM32(s+4),8))return 0;
 if(op==1){
  PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39f25b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,140);PUSH32(g_esp,0x258d61);sub_002D57FB();g_esp+=4;MEM32(g_esp+4)=g_eax;MEM32(g_esp+16)=0;
  if(g_eax){g_ecx=MEM32(g_esp+60);g_edx=MEM32(g_esp+56);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+56);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+56);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+56);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+56);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+56);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+56);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+56);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);g_ecx=g_eax;PUSH32(g_esp,0x258da8);sub_00258BC0();}else g_eax=0;
  g_edx=MEM32(g_esp+64);g_esi=MEM32(g_esp+24);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);g_esp-=8;g_ecx=g_esp;MEM32(g_esp+80)=g_esp;PUSH32(g_esp,g_esi);MEM32(g_esp+36)=0xffffffff;MEM32(g_ecx)=g_eax;MEM32(g_ecx+4)=1;PUSH32(g_esp,0x258dd7);sub_00253FF0();g_ecx=MEM32(g_esp+28);g_esp+=20;g_eax=g_esi;MEM32(0)=g_ecx;POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;
 }else{
  g_esp-=16;SET_LO8(g_edx,MEM8(g_esp+28));g_eax=MEM32(g_esp+32);g_ecx=MEM32(g_esp+36);PUSH32(g_esp,g_esi);PUSH32(g_esp,1);g_esi=MEM32(g_esp+28);MEM32(g_esp+8)=g_eax;if(!LO8(g_edx))g_eax=g_ecx;MEM32(g_esp+12)=g_ecx;g_ecx=MEM32(g_esp+48);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x41000000);PUSH32(g_esp,0x7c);PUSH32(g_esp,g_eax);g_eax=g_esp+24;PUSH32(g_esp,g_eax);g_eax=0;SET_LO8(g_eax,!LO8(g_edx));g_ecx=g_esp+36;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+56);PUSH32(g_esp,2);MEM32(g_esp+44)=0x48573c;MEM32(g_esp+48)=0x485750;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x258e4f);sub_00258D40();g_esp+=44;g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;
 }return 1;
}
#undef PP_TOP
#undef PP_ST1
#undef PP_POP
