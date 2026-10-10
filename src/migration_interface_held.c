#include "migration_interface_held.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
static int held_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void held_call(unsigned target,unsigned ret,unsigned saved){PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
int migration_interface_held(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_HELD");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;
 unsigned frame=g_seh_ebp;unsigned s=g_esp;if(s<0x10400||(s&3)||(unsigned)g_fp_top>7||!held_span(s-1024,1032)||!held_span(0x51d2bd,12))return 0;
 unsigned p=MEM32(0x51c4dc);if(!held_span(p,4)||!held_span(MEM32(p),12))return 0;
 unsigned action=MEM32(s+4),target=0;if(action-28<6){target=MEM32(0x274820+(action-28)*4);if(target!=0x274811&&target!=0x274808&&target!=0x2747af&&target!=0x2747f9&&target!=0x2747e4&&target!=0x2747f2)return 0;}
 g_esp-=8;g_ecx=MEM32(0x51c4dc);g_eax=MEM32(g_ecx);unsigned saved=g_esp;PUSH32(g_esp,3);PUSH32(g_esp,18);PUSH32(g_esp,0);held_call(MEM32(g_eax+8),0x274784,saved);MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
 g_ecx=MEM32(0x51c4dc);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,3);PUSH32(g_esp,19);PUSH32(g_esp,0);held_call(MEM32(g_edx+8),0x274798,saved);MEMF(g_esp+4)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_edx=MEM32(g_esp+12);g_eax=g_edx-28;target=g_eax<6?MEM32(0x274820+g_eax*4):0;if(g_eax<6&&target!=0x274811&&target!=0x274808&&target!=0x2747af&&target!=0x2747f9&&target!=0x2747e4&&target!=0x2747f2){g_seh_ebp=frame;RECOMP_ITAIL(target);return 1;}
 if(target==0x274811||target==0x274808){SET_LO8(g_eax,MEM8(target==0x274811?0x51d2bd:0x51d2c8));g_esp+=8;g_recomp_zf=!g_esp;g_esp+=4;return 1;}
 int pressed=0;if(target){double other;if(target==0x2747af||target==0x2747f2){g_xmm0=XMM_SCALAR(MEMF(g_esp+(target==0x2747af?4:0)));other=MEMF(0x44ec20);}else{g_xmm0=XMM_SCALAR(MEMF(0x48908c));other=MEMF(g_esp+(target==0x2747f9?4:0));}double value=g_xmm0.f[0];unsigned flags=value!=value||other!=other?0x47:value<other?3:value>other?2:0x42;pressed=(flags&0x41)==0;}
 if(!pressed){g_ecx=MEM32(0x51c4dc);g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,0);held_call(MEM32(g_eax+8),0x2747ce,saved);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(0x44c174);g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;SET_HI8(g_eax,g_fp_cmp==2?0x47:g_fp_cmp<0?3:g_fp_cmp>0?2:0x42);pressed=RECOMP_PARITY8(HI8(g_eax)&0x44);}
 SET_LO8(g_eax,pressed?1:0);g_esp+=8;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
