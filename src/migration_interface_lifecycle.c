#include "migration_interface_lifecycle.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00080491(void),sub_00063FF0(void),sub_002D5CCD(void);
static int lifecycle_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_lifecycle(unsigned op){static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_LIFECYCLE");enabled=s&&!strcmp(s,"1");}if(!enabled||op>12)return 0;
 uint32_t s=g_esp;if(s<0x10100||(s&3)||!lifecycle_span(s-256,272)||!lifecycle_span(0x5150f8,0x300))return 0;
 if((op==0&&!lifecycle_span(g_ecx,42))||(op==12&&!lifecycle_span(g_ecx,32)))return 0;
 if(op==10){uint32_t p=MEM32(0x515278);if(!lifecycle_span(p,1660)||MEM32(p+8)<1||MEM32(p+8)>4||MEM32(s+4)>=MEM32(p+8))return 0;}
 switch(op){
 case 0:{g_eax=1;g_edx=50;MEM32(g_ecx+4)=MEM32(g_ecx+8)=g_edx;SET_LO8(g_edx,12);for(unsigned i=20;i<24;i++)MEM8(g_ecx+i)=LO8(g_edx);PUSH32(g_esp,g_ebx);SET_LO8(g_ebx,0);MEM8(g_ecx)=MEM8(g_ecx+15)=0;const unsigned ones[]={18,19,13,16,12,14,17};for(unsigned i=0;i<7;i++)MEM8(g_ecx+ones[i])=1;g_edx=MEM8(0x5150f8)?0x290:0x27c;MEM32(g_ecx+24)=g_edx;MEM8(g_ecx+33)=MEM8(g_ecx+35)=MEM8(g_ecx+40)=0;MEM8(g_ecx+32)=8;MEM8(g_ecx+34)=1;MEM32(g_ecx+36)=1;g_edx=MEM8(0x5150f8)?72:50;MEM32(g_ecx+28)=g_edx;MEM8(g_ecx+41)=1;POP32(g_esp,g_ebx);g_recomp_zf=0;break;}
 case 1:case 4:case 8:g_eax=0;g_recomp_zf=1;break;
 case 2:g_eax=MEM32(0x4e8358);break;
 case 3:g_esp-=24;g_eax=g_esp+8;PUSH32(g_esp,g_eax);g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);g_edx=g_esp+8;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x45236c);PUSH32(g_esp,0x63dbc);sub_00080491();g_eax=MEM32(g_esp);g_ecx=MEM32(g_esp+4);g_eax=(g_eax>>14)|(g_ecx<<18);g_ecx>>=14;g_esp+=24;g_recomp_zf=!g_esp;break;
 case 5:g_eax=MEM32(g_esp+4);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x6415c);sub_00063FF0();g_esp+=8;g_recomp_zf=!g_esp;break;
 case 6:g_ecx=MEM32(0x515278);g_eax=g_ecx!=0;g_recomp_zf=!g_ecx;break;
 case 7:g_eax=MEM32(0x515278);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x6417b);sub_002D5CCD();g_esp+=4;MEM32(0x515278)=0;g_recomp_zf=!g_esp;break;
 case 9:g_eax=MEM32(g_esp+4);g_ecx=MEM32(0x515278);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x641b1);sub_00063FF0();g_esp+=8;g_recomp_zf=!g_esp;break;
 case 10:g_eax=MEM32(0x515278);MEM8(0x515274)=1;g_ecx=MEM32(g_eax+8)-1;MEM32(g_eax+8)=g_ecx;if(g_ecx){g_ecx*=412;PUSH32(g_esp,g_esi);g_esi=g_ecx+g_eax+12;g_ecx=MEM32(g_esp+8)*412;PUSH32(g_esp,g_edi);g_edi=g_ecx+g_eax+12;g_ecx=103;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),412);g_esi+=412;g_edi+=412;g_ecx=0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);}break;
 case 11:g_eax=MEM32(g_esp+4);g_xmm0=XMM_SCALAR(MEMF(g_esp+8));MEM32(0x5153e8)=g_eax;MEMF(0x5153ec)=g_xmm0.f[0];break;
 case 12:g_xmm0=XMM_SCALAR(MEMF(0x4523cc));g_eax=g_ecx;MEMF(g_eax)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44fa80));MEMF(g_eax+4)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf80));MEMF(g_eax+20)=MEMF(g_eax+24)=g_xmm0.f[0];g_xmm0=XMM_ZERO();MEM32(g_eax+28)=0;MEMF(g_eax+16)=g_xmm0.f[0];MEM32(g_eax+8)=3;break;
 }
 g_esp+=4;return 1;
}
