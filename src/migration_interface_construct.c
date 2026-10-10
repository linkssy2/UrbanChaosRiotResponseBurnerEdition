#include "migration_interface_construct.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002D57FB(void),sub_00266E00(void),sub_000650D0(void),sub_00259200(void);
static int construct_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_construct(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_CONSTRUCT");enabled=s&&!strcmp(s,"1");}if(!enabled||op>8)return 0;unsigned s=g_esp;
 if(s<0x10400||(s&3)||!construct_span(s-1024,1072)||!construct_span(0x51d0d4,4))return 0;
 if(op<6&&!construct_span(MEM32(s+4),8))return 0;if(op==1&&!construct_span(MEM32(s+8),16))return 0;
 if((op==7&&!construct_span(g_ecx,32))||(op==8&&!construct_span(MEM32(s+4),8)))return 0;
 if(op==6){MEM32(0x51d0d4)=0xffffffff;g_esp+=4;return 1;}
 if(op==7){PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39f238);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;g_eax=MEM32(g_esp+32);g_ecx=MEM32(g_esp+24);g_edx=MEM32(g_esp+28);MEM32(g_esi+4)=g_eax;SET_LO8(g_eax,MEM8(g_esp+36));PUSH32(g_esp,0x485b00);MEM32(g_esp+20)=0;MEM32(g_esi)=0x485c04;MEM32(g_esi+8)=g_ecx;MEM32(g_esi+12)=g_edx;MEM8(g_esi+16)=LO8(g_eax);PUSH32(g_esp,0x259251);sub_000650D0();PUSH32(g_esp,0x485b00);MEM32(g_esi+20)=g_eax;PUSH32(g_esp,0x25925e);sub_000650D0();PUSH32(g_esp,0x485bdc);MEM32(g_esi+24)=g_eax;PUSH32(g_esp,0x25926b);sub_000650D0();g_ecx=MEM32(g_esp+20);MEM32(g_esi+28)=g_eax;g_esp+=12;g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=20;return 1;}
 if(op==8){PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39f25b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,32);PUSH32(g_esp,0x25931d);sub_002D57FB();g_esp+=4;MEM32(g_esp)=g_eax;g_ecx=0;MEM32(g_esp+12)=g_ecx;if(g_eax){g_ecx=MEM32(g_esp+40);g_edx=MEM32(g_esp+32);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+32);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+32);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;PUSH32(g_esp,0x259348);sub_00259200();g_ecx=g_eax;}g_eax=MEM32(g_esp+20);MEM32(g_eax)=g_ecx;g_ecx=MEM32(g_esp+36);MEM32(g_eax+4)=g_ecx;g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;return 1;}
 if(op<4){const unsigned sizes[]={16,32,12,32},ret[]={0x253ff7,0x258667,0x2591b7,0x2767a7};PUSH32(g_esp,sizes[op]);PUSH32(g_esp,ret[op]);sub_002D57FB();g_esp+=4;int failed=!g_eax;
  if(g_eax){
   if(op==0){g_ecx=MEM32(g_esp+8);g_edx=MEM32(g_esp+20);MEM32(g_eax+4)=g_ecx;SET_LO8(g_ecx,MEM8(g_esp+16));MEM32(g_eax+12)=g_edx;MEM32(g_eax)=0x4846a4;MEM8(g_eax+8)=LO8(g_ecx);g_edx=MEM32(0x51cebc)+1;MEM32(0x51cebc)=g_edx;MEM8(g_eax+9)=1;if(LO8(g_ecx))MEM32(0x51cec0)++;}
   else if(op==1){g_ecx=MEM32(g_esp+8);MEM32(g_eax)=0x485b30;g_xmm0=XMM_MEM(g_ecx);XMM_STORE(g_eax+16,g_xmm0);}
   else if(op==2){g_ecx=MEM32(g_esp+8);g_edx=MEM32(g_esp+12);MEM32(g_eax+4)=g_ecx;MEM32(g_eax)=0x485bd0;MEM32(g_eax+8)=g_edx;}
   else{g_ecx=MEM32(g_esp+8);g_edx=MEM32(g_esp+12);g_xmm0=XMM_SCALAR(MEMF(g_esp+32));MEM32(g_eax+4)=g_ecx;g_ecx=MEM32(g_esp+16);MEM32(g_eax+8)=g_edx;g_edx=MEM32(g_esp+20);MEM32(g_eax+12)=g_ecx;SET_LO8(g_ecx,MEM8(g_esp+24));MEM32(g_eax+16)=g_edx;g_edx=MEM32(g_esp+28);MEM8(g_eax+20)=LO8(g_ecx);MEM32(g_eax)=0x489974;MEM32(g_eax+24)=g_edx;MEMF(g_eax+28)=g_xmm0.f[0];}
   g_ecx=g_eax;g_eax=MEM32(g_esp+4);
  }else{g_eax=MEM32(g_esp+4);g_ecx=0;}
  MEM32(g_eax)=g_ecx;if(!op){g_ecx=MEM32(g_esp+12);MEM32(g_eax+4)=g_ecx;if(failed)g_recomp_zf=!g_ecx;}else{MEM32(g_eax+4)=1;g_recomp_zf=failed;}g_esp+=4;return 1;
 }
 if(op==4){g_esp-=32;g_edx=MEM32(g_esp+48);g_eax=MEM32(g_esp+40);g_ecx=MEM32(g_esp+44);MEM32(g_esp+8)=g_edx;g_edx=MEM32(g_esp+60);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+40);MEM32(g_esp+4)=g_eax;g_eax=MEM32(g_esp+56);MEM32(g_esp+8)=g_ecx;g_ecx=MEM32(g_esp+60);MEM32(g_esp+24)=g_edx;PUSH32(g_esp,4);g_edx=g_esp+8;MEM32(g_esp+20)=g_eax;g_eax=MEM32(g_esp+72);MEM32(g_esp+24)=g_ecx;g_ecx=MEM32(g_esp+76);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);MEM32(g_esp+40)=g_eax;MEM32(g_esp+44)=g_ecx;PUSH32(g_esp,0x266f85);sub_00266E00();}
 else{g_esp-=40;g_ecx=MEM32(g_esp+52);g_eax=MEM32(g_esp+48);g_edx=MEM32(g_esp+56);MEM32(g_esp+4)=g_ecx;g_ecx=MEM32(g_esp+64);MEM32(g_esp)=g_eax;g_eax=MEM32(g_esp+60);MEM32(g_esp+16)=g_ecx;g_ecx=MEM32(g_esp+76);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+48);MEM32(g_esp+12)=g_edx;g_edx=MEM32(g_esp+72);MEM32(g_esp+16)=g_eax;g_eax=MEM32(g_esp+76);MEM32(g_esp+32)=g_ecx;PUSH32(g_esp,5);g_ecx=g_esp+8;MEM32(g_esp+28)=g_edx;g_edx=MEM32(g_esp+88);MEM32(g_esp+32)=g_eax;g_eax=MEM32(g_esp+92);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);MEM32(g_esp+48)=g_edx;MEM32(g_esp+52)=g_eax;PUSH32(g_esp,0x266ff4);sub_00266E00();}
 g_esp+=12;g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=op==4?32:40;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
