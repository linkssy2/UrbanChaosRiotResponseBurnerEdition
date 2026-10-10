#include "migration_interface_videoresult.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00011030(void),sub_0005AAB0(void),sub_00064DC0(void),sub_002767A0(void),sub_00258800(void),sub_00259CD0(void),sub_00266EE0(void),sub_00266EA0(void),sub_002D57FB(void),sub_002D6322(void),sub_002C2440(void);
#define VR_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
static int vr_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void videoresult_caption(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,40);VR_CALL(002D57FB,0x258808);g_esi=g_eax;g_esp+=4;if(g_esi){g_edx=MEM32(g_esp+28);g_eax=MEM32(g_esp+12);g_xmm0=XMM_SCALAR(MEMF(g_esp+16));g_ecx=MEM32(g_esp+24);PUSH32(g_esp,g_edx);MEMF(g_esi+8)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+24));MEM32(g_esi+4)=g_eax;g_eax=g_esi+24;MEMF(g_esi+12)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+36));PUSH32(g_esp,0x450a30);PUSH32(g_esp,g_eax);MEM32(g_esi)=0x485b3c;MEM32(g_esi+16)=g_ecx;MEMF(g_esi+20)=g_xmm0.f[0];VR_CALL(002D6322,0x258859);g_eax=MEM32(g_esp+20);g_esp+=12;MEM32(g_eax)=g_esi;MEM32(g_eax+4)=1;POP32(g_esp,g_esi);g_recomp_zf=!g_esp;}else{g_eax=MEM32(g_esp+8);g_esi=0;MEM32(g_eax)=g_esi;MEM32(g_eax+4)=1;POP32(g_esp,g_esi);g_recomp_zf=!g_esi;}g_esp+=4;
}
static void videoresult_screen(void){
 g_esp-=32;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x489078);VR_CALL(00011030,0x273e7f);g_esp+=4;VR_CALL(0005AAB0,0x273e87);g_eax=MEM32(g_eax+324);g_edx=MEM32(g_eax);g_ecx=g_eax;unsigned saved=g_esp,target=MEM32(g_edx+460);PUSH32(g_esp,0x273e97);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,0);PUSH32(g_esp,2);g_ecx=0x4e8364;g_ebx=g_eax;VR_CALL(00064DC0,0x273ea7);VR_CALL(0005AAB0,0x273eac);g_eax=MEM32(g_eax+324);g_edx=MEM32(g_eax);g_ecx=g_eax;saved=g_esp;target=MEM32(g_edx+476);PUSH32(g_esp,0x273ebc);RECOMP_ICALL_SAFE(target,saved);
 if(LO8(g_eax)){
  PUSH32(g_esp,g_edi);VR_CALL(0005AAB0,0x273eca);g_eax=MEM32(g_eax+324);g_edx=MEM32(g_eax);g_ecx=g_eax;saved=g_esp;target=MEM32(g_edx+160);PUSH32(g_esp,0x273eda);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(0x485a18);PUSH32(g_esp,0xbf800000);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x4509a8);PUSH32(g_esp,0);PUSH32(g_esp,0x254cb0);PUSH32(g_esp,g_eax);g_ecx=g_esp+40;PUSH32(g_esp,g_ecx);VR_CALL(002767A0,0x273eff);
  PUSH32(g_esp,0xbf800000);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x450958);PUSH32(g_esp,0);PUSH32(g_esp,0x273250);g_edx=g_esp+76;PUSH32(g_esp,22);PUSH32(g_esp,g_edx);g_esi=g_eax;VR_CALL(002767A0,0x273f22);g_esp+=64;PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,3);PUSH32(g_esp,0x43700000);PUSH32(g_esp,0x43a00000);g_edi=g_eax;g_eax=g_esp+48;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);VR_CALL(00258800,0x273f42);
  g_ecx=MEM32(g_eax+4);g_edx=MEM32(g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0);g_eax=g_esp+84;PUSH32(g_esp,0);PUSH32(g_esp,g_eax);VR_CALL(00259CD0,0x273f5b);g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi);g_esi=MEM32(g_esp+104);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edi+4);PUSH32(g_esp,g_edx);g_edx=MEM32(g_edi);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_eax+4);PUSH32(g_esp,g_edx);g_edx=MEM32(g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);VR_CALL(00266EE0,0x273f7a);g_esp+=84;POP32(g_esp,g_edi);
 }else{
  PUSH32(g_esp,0xbf800000);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x450958);PUSH32(g_esp,0x485c58);PUSH32(g_esp,0x254cb0);g_eax=g_esp+56;PUSH32(g_esp,22);PUSH32(g_esp,g_eax);VR_CALL(002767A0,0x273faa);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,3);PUSH32(g_esp,0x43700000);PUSH32(g_esp,0x43a00000);g_ecx=g_esp+76;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);g_esi=g_eax;VR_CALL(00258800,0x273fc7);g_edx=MEM32(g_eax+4);g_eax=MEM32(g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_ecx=g_esp+96;PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);VR_CALL(00259CD0,0x273fe0);
  g_edx=MEM32(g_esi+4);g_ecx=MEM32(g_esi);g_esi=MEM32(g_esp+132);g_esp+=88;PUSH32(g_esp,g_edx);g_edx=MEM32(g_eax+4);g_eax=MEM32(g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);VR_CALL(00266EA0,0x273ffe);g_esp+=20;
 }g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=32;g_recomp_zf=!g_esp;g_esp+=4;
}
int migration_interface_videoresult(unsigned operation){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_INTERFACE_VIDEORESULT");enabled=v&&v[0]=='1'&&!v[1];}if(!enabled||operation>2||(g_esp&3)||!vr_span((uint64_t)g_esp-1024,1064)||!vr_span(MEM32(g_esp+4),8))return 0;
 if(operation==1)videoresult_caption();else if(operation==2){PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,6);VR_CALL(002C2440,0x27325b);g_eax=MEM32(g_esp+16);g_esp+=12;MEM32(g_eax)=0;MEM32(g_eax+4)=1;g_recomp_zf=!g_esp;g_esp+=4;}else videoresult_screen();return 1;
}
