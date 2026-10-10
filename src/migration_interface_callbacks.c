#include "migration_interface_callbacks.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00017E40(void),sub_00057FC0(void);
extern void sub_00266D80(void),sub_00259BC0(void),sub_002620B0(void),sub_002D5CCD(void),sub_002D73A4(void);
static int callbacks_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void callbacks_call(unsigned target,unsigned ret,unsigned saved){PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
int migration_interface_callbacks(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_CALLBACKS");enabled=s&&!strcmp(s,"1");}if(!enabled||op>11)return 0;
 unsigned s=g_esp;if(s<0x10400||(s&3)||!callbacks_span(s-1024,1040)||!callbacks_span(g_ecx,op<4?136:32))return 0;
 if(op==10&&(unsigned)g_fp_top>7)return 0;
 if(op<4){unsigned n=MEM32(g_ecx+132);if(n>32)return 0;for(unsigned i=0;i<n;i++){unsigned child=MEM32(g_ecx+4+i*4);if(!child&&op<2)continue;if(!callbacks_span(child,4)||!callbacks_span(MEM32(child),12))return 0;}}
 if(op==0||op==4||op==7){PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,op==0?0x266d68:op==4?0x259ba8:0x2629b8);if(op==0)sub_00266D80();else if(op==4)sub_00259BC0();else sub_002620B0();if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,op==0?0x266d75:op==4?0x259bb5:0x2629c5);sub_002D5CCD();g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 if(op==1){PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39f878);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+8)=g_esi;MEM32(g_esi)=0x4877fc;g_eax=MEM32(g_esi+132);g_ebx=0;MEM32(g_esp+20)=0;
  if(g_eax){PUSH32(g_esp,g_edi);g_edi=g_esi+4;do{g_ecx=MEM32(g_edi);if(g_ecx){g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,1);callbacks_call(MEM32(g_eax),0x266dcc,saved);}g_eax=MEM32(g_esi+132);g_ebx++;g_edi+=4;}while(g_ebx<g_eax);POP32(g_esp,g_edi);}g_ecx=MEM32(g_esp+12);MEM32(g_esi)=0x450708;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;
 }else if(op==2){unsigned frame=g_seh_ebp;PUSH32(g_esp,g_ecx);g_eax=MEM32(g_ecx+132);PUSH32(g_esp,g_edi);g_edi=0;if(g_eax){PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+20);PUSH32(g_esp,frame);frame=MEM32(g_esp+20);PUSH32(g_esp,g_esi);g_esi=g_ecx+g_eax*4+4;MEM32(g_esp+16)=g_eax;do{g_ecx=MEM32(g_esi-4);g_eax=MEM32(g_ecx);g_esi-=4;unsigned saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);callbacks_call(MEM32(g_eax+4),0x266cfe,saved);if(!g_edi)g_edi=g_eax;MEM32(g_esp+16)--;}while(MEM32(g_esp+16));POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);}g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_ecx);g_esp+=12;return 1;
 }else if(op==3){unsigned frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);g_ebx=g_ecx;g_eax=MEM32(g_ebx+132);PUSH32(g_esp,g_esi);frame=0;g_esi=0;if(g_eax){PUSH32(g_esp,g_edi);g_edi=g_ebx+4;do{g_ecx=MEM32(g_edi);g_eax=MEM32(g_ecx);unsigned saved=g_esp;callbacks_call(MEM32(g_eax+8),0x266d3e,saved);frame|=g_eax;g_eax=MEM32(g_ebx+132);g_esi++;g_edi+=4;}while(g_esi<g_eax);POP32(g_esp,g_edi);}POP32(g_esp,g_esi);g_eax=frame;POP32(g_esp,frame);POP32(g_esp,g_ebx);
 }else if(op==5){PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+8);PUSH32(g_esp,g_eax);MEM32(g_esi)=0x485c10;PUSH32(g_esp,0x259bd2);sub_002D73A4();g_esp+=4;MEM32(g_esi)=0x450708;POP32(g_esp,g_esi);g_recomp_zf=!g_esp;
 }else if(op==6)MEM32(g_ecx)=0x487548;
 else if(op==8)SET_LO8(g_eax,MEM8(g_ecx+16));
 else if(op==9)g_eax=MEM32(g_ecx+20);
 else if(op==10){PUSH32(g_esp,g_ecx);g_eax=MEM32(g_ecx+8);g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2634b9);sub_00017E40();MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;int extra=MEM8(g_esp+8)&1;
 if(extra){g_ecx=MEM32(0x509704);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x48762c);PUSH32(g_esp,0x2634dc);sub_00017E40();g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(0x4859f4);g_fp_stack[g_fp_top]+=g_fp_stack[g_fp_top];g_fp_stack[(g_fp_top+1)&7]+=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_fp_stack[g_fp_top]+=MEMF(g_esp);}else{g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_esp);}POP32(g_esp,g_ecx);g_recomp_zf=!extra;g_esp+=8;return 1;
 }else if(op==11){g_eax=MEM32(g_ecx+12);if(g_eax&&g_eax!=0xffffffff){PUSH32(g_esp,0x450958);PUSH32(g_esp,0x2633e6);sub_00057FC0();g_esp+=4;SET_LO8(g_eax,1);g_recomp_zf=!g_esp;}else{if(!g_eax){PUSH32(g_esp,0x485940);PUSH32(g_esp,0x2633f6);sub_00057FC0();g_esp+=4;}SET_LO8(g_eax,0);g_recomp_zf=1;}}

 g_esp+=4;return 1;
}
