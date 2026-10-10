#include "migration_interface_screenflow.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_0025CDC0(void),sub_00259CD0(void),sub_00272010(void),sub_002748E0(void),sub_00258660(void),sub_002733D0(void),sub_00258DF0(void),sub_00253FF0(void),sub_00273700(void),sub_00266F30(void),sub_002845B0(void),sub_002D57FB(void),sub_00274840(void),sub_00284860(void),sub_00273030(void),sub_00280D70(void);
#define SF_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
#define SF_FRAME(n,r) do{g_ebp=frame;SF_CALL(n,r);}while(0)
static void screenflow_confirm(void){
 g_esp-=8;PUSH32(g_esp,g_esi);g_eax=g_esp+4;PUSH32(g_esp,0x2715d0);PUSH32(g_esp,g_eax);SF_CALL(0025CDC0,0x271043);g_ecx=MEM32(g_eax+4);g_edx=MEM32(g_eax);g_esi=MEM32(g_esp+24);PUSH32(g_esp,0);PUSH32(g_esp,0x270e30);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x488cfc);PUSH32(g_esp,0x488ce0);PUSH32(g_esp,g_esi);SF_CALL(00259CD0,0x271065);g_esp+=36;g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=!g_esp;g_esp+=4;
}
static void screenflow_choice(void){
 unsigned frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0;g_esp-=68;SET_LO8(g_eax,MEM8(0x5010e8));int alternate=LO8(g_eax)!=0;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if(alternate){g_esi=MEM32(frame+8);PUSH32(g_esp,g_esi);SF_FRAME(002845B0,0x271bef);g_esp+=4;POP32(g_esp,g_edi);g_eax=g_esi;}
 else{
 g_xmm0=XMM_ZERO();g_eax=g_esp+64;PUSH32(g_esp,g_eax);g_ecx=g_esp+28;MEMF(g_esp+68)=g_xmm0.f[0];MEMF(g_esp+72)=g_xmm0.f[0];MEMF(g_esp+76)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44d660));PUSH32(g_esp,g_ecx);MEMF(g_esp+84)=g_xmm0.f[0];SF_FRAME(00258660,0x271b5b);PUSH32(g_esp,0);PUSH32(g_esp,0x485b28);g_edx=g_esp+48;PUSH32(g_esp,2);PUSH32(g_esp,g_edx);g_esi=g_eax;SF_FRAME(002733D0,0x271b70);
 PUSH32(g_esp,3);PUSH32(g_esp,0x272010);PUSH32(g_esp,0x2845b0);PUSH32(g_esp,0);g_edi=g_eax;g_eax=g_esp+80;PUSH32(g_esp,0x473368);PUSH32(g_esp,g_eax);SF_FRAME(00258DF0,0x271b8f);g_ecx=MEM32(g_eax+4);g_edx=MEM32(g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_eax=g_esp+112;PUSH32(g_esp,g_eax);SF_FRAME(00253FF0,0x271ba4);g_esp+=68;g_ecx=g_esp+56;PUSH32(g_esp,8);PUSH32(g_esp,g_ecx);g_ebx=g_eax;SF_FRAME(00273700,0x271bb5);
 g_edx=MEM32(g_esi+4);g_ecx=MEM32(g_esi);g_esi=MEM32(frame+8);PUSH32(g_esp,g_edx);g_edx=MEM32(g_edi+4);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edi);PUSH32(g_esp,g_edx);g_edx=MEM32(g_ebx+4);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx);PUSH32(g_esp,g_edx);g_edx=MEM32(g_eax+4);g_eax=MEM32(g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);SF_FRAME(00266F30,0x271bda);g_esp+=44;g_eax=g_esi;POP32(g_esp,g_edi);
 }POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=!g_esp;g_esp+=4;
}
static void screenflow_dialog(void){
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39fdeb);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=36;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,40);SF_CALL(002D57FB,0x2749f1);g_esp+=4;MEM32(g_esp+8)=g_eax;MEM32(g_esp+52)=0;if(g_eax){PUSH32(g_esp,0);g_ecx=g_eax;SF_CALL(00274840,0x274a0d);g_esi=g_eax;}else g_esi=0;
 g_eax=MEM32(0x50121c);g_ecx=MEM32(0x501218);g_edx=MEM32(0x501220);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_eax=g_esp+32;PUSH32(g_esp,g_eax);MEM32(g_esp+76)=0xffffffff;SF_CALL(00284860,0x274a3d);g_ecx=MEM32(g_eax+4);g_edx=MEM32(g_eax);g_esp+=24;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0);PUSH32(g_esp,0);g_esp-=8;g_eax=g_esp;MEM32(g_eax)=g_esi;MEM32(g_eax+4)=1;g_eax=g_esp+44;MEM32(g_esp+32)=g_esp;PUSH32(g_esp,g_eax);SF_CALL(00253FF0,0x274a67);
 PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x42200000);PUSH32(g_esp,0);g_ecx=g_esp+84;PUSH32(g_esp,0x4890cc);PUSH32(g_esp,g_ecx);g_esi=g_eax;SF_CALL(00273030,0x274a89);g_edx=g_esp+100;PUSH32(g_esp,9);PUSH32(g_esp,g_edx);g_edi=g_eax;SF_CALL(00273700,0x274a97);g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi);g_esi=MEM32(g_esp+132);g_esp+=64;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edi+4);PUSH32(g_esp,g_edx);g_edx=MEM32(g_edi);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_eax+4);PUSH32(g_esp,g_edx);g_edx=MEM32(g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);SF_CALL(00266F30,0x274abc);g_ecx=MEM32(g_esp+80);g_esp+=36;POP32(g_esp,g_edi);g_eax=g_esi;MEM32(0)=g_ecx;POP32(g_esp,g_esi);g_esp+=48;g_recomp_zf=!g_esp;g_esp+=4;
}
static void screenflow_entry(void){
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x3a021b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,16);SF_CALL(002D57FB,0x280dee);g_esp+=4;MEM32(g_esp+4)=g_eax;MEM32(g_esp+16)=0;if(g_eax){g_ecx=g_eax;SF_CALL(00280D70,0x280e08);}else g_eax=0;
 g_esi=MEM32(g_esp+24);PUSH32(g_esp,0);PUSH32(g_esp,0);g_esp-=8;g_ecx=g_esp;MEM32(g_esp+20)=g_esp;PUSH32(g_esp,0x488db4);PUSH32(g_esp,0);PUSH32(g_esp,g_esi);MEM32(g_esp+44)=0xffffffff;MEM32(g_ecx)=g_eax;MEM32(g_ecx+4)=1;SF_CALL(00259CD0,0x280e3b);g_ecx=MEM32(g_esp+36);g_esp+=28;g_eax=g_esi;MEM32(0)=g_ecx;POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;
}
int migration_interface_screenflow(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_SCREENFLOW");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation>5||g_esp<0x11000||(g_esp&3)||(uint64_t)g_esp+16>g_xbox_total_ram)return 0;unsigned out=MEM32(g_esp+4);if(out<0x10000||(uint64_t)out+8>g_xbox_total_ram)return 0;
 if(operation==0)screenflow_confirm();else if(operation<=2){PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_esi);if(operation==1){SF_CALL(00272010,0x270e1b);}else{MEM8(0x5154bc)=1;SF_CALL(002748E0,0x271852);}g_esp+=4;g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=!g_esp;g_esp+=4;}else if(operation==3)screenflow_choice();else if(operation==4)screenflow_dialog();else screenflow_entry();return 1;
}
