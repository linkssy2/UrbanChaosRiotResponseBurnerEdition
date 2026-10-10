#include "migration_interface_missionconstruct.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_002C3740(void),sub_00208980(void),sub_0026EF70(void),sub_0025A2E0(void),sub_000643E0(void),sub_00063C20(void),sub_0026F6A0(void),sub_002D57FB(void),sub_00270050(void);
#define MC_CALL(n,r) do{PUSH32(g_esp,r);sub_##n();}while(0)
static int mc_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void missionconstruct_selection(unsigned *frame){
 PUSH32(g_esp,*frame);*frame=g_esi+36;
 do{
  MC_CALL(000643E0,0x27013a);g_ecx=MEM32(*frame-4);int matched=0;
  if(g_ecx==MEM32(g_eax+28)){MC_CALL(000643E0,0x270147);matched=MEM32(g_eax+28)!=0xffffffff;}
  if(!matched){MC_CALL(000643E0,0x270152);g_edx=MEM32(*frame);if(g_edx==MEM32(g_eax+32)){MC_CALL(000643E0,0x27015f);matched=MEM32(g_eax+32)!=0xffffffff;}}
  if(matched){MEM32(g_esp+36)=g_edi;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=(double)SMEM32(g_esp+36);MEM32(g_esi+28)=g_edi;if((int32_t)g_edi<0)g_fp_stack[g_fp_top]+=MEMF(0x44c510);MEMF(g_esi+24)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;break;}
  g_eax=MEM32(g_esi+236);g_edi++;*frame+=12;
 }while(g_edi<g_eax);
 POP32(g_esp,*frame);
}
int migration_interface_missionconstruct(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_MISSIONCONSTRUCT");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;
 unsigned s=g_esp;if(s<0x10800||(s&3)||!mc_span(s-2048,2080))return 0;if(!op){if(!mc_span(g_ecx,280))return 0;}else if(!mc_span(MEM32(s+4),8))return 0;
 unsigned frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,op?0x39fceb:0x39fcd4);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);
 if(op){PUSH32(g_esp,280);MC_CALL(002D57FB,0x270cb0);g_esp+=4;MEM32(g_esp)=g_eax;g_ecx=0;MEM32(g_esp+12)=0;
  if(g_eax){g_ecx=MEM32(g_esp+40);g_edx=MEM32(g_esp+36);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+36);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+36);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+36);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);g_ecx=g_eax;MC_CALL(00270050,0x270ce0);g_ecx=g_eax;}
  g_eax=MEM32(g_esp+20);MEM32(g_eax)=g_ecx;g_ecx=MEM32(g_esp+4);MEM32(g_eax+4)=1;MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=4;return 1;
 }
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+12)=g_esi;g_ecx=MEM32(g_esp+36);g_eax=MEM32(g_esp+32);g_edx=MEM32(g_esp+40);g_xmm0=XMM_ZERO();MEM32(g_esi+8)=g_ecx;g_ecx=MEM32(g_esp+48);MEM32(g_esi+4)=g_eax;g_eax=MEM32(g_esp+44);g_ebx=0;MEM32(g_esi+20)=g_ecx;g_edi=g_esi+244;g_ecx=g_edi;MEM32(g_esp+24)=0;MEM32(g_esi)=0x4885fc;MEM32(g_esi+12)=g_edx;MEM32(g_esi+16)=g_eax;MEMF(g_esi+24)=g_xmm0.f[0];MEM32(g_esi+28)=0;MEMF(g_esi+240)=g_xmm0.f[0];MC_CALL(002C3740,0x2700be);
 MEM32(g_edi+4)=g_eax;MEM32(g_edi+8)=g_ebx;g_edi=g_esi+256;g_ecx=g_edi;MEM8(g_esp+24)=1;MC_CALL(002C3740,0x2700d6);MEM32(g_edi+4)=g_eax;MEM32(g_edi+8)=g_ebx;g_xmm0=XMM_SCALAR(MEMF(0x44d744));PUSH32(g_esp,g_ebx);MEMF(g_esi+272)=g_xmm0.f[0];g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,0x485f00);MEM8(g_esp+32)=2;MC_CALL(00208980,0x270102);g_edx=g_esi+32;PUSH32(g_esp,g_edx);MEM32(g_esi+276)=g_eax;MC_CALL(0026EF70,0x270111);g_esp+=4;MEM32(g_esi+236)=g_eax;MC_CALL(0025A2E0,0x27011f);MEM32(g_esi+268)=g_eax;g_eax=MEM32(g_esi+236);g_edi=0;if(g_eax>g_ebx)missionconstruct_selection(&frame);
 g_eax=MEM32(g_esi+28);g_ecx=g_eax*3+9;g_edx=MEM32(g_esi+g_ecx*4);g_eax*=3;g_ecx=MEM32(g_esi+g_eax*4+32);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);MC_CALL(000643E0,0x2701a6);g_ecx=g_eax;MC_CALL(00063C20,0x2701ad);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=(double)SMEM32(g_esi+28);g_edx=MEM32(g_esi+28);g_edi=g_eax;if((int32_t)g_edx<0)g_fp_stack[g_fp_top]+=MEMF(0x44c510);PUSH32(g_esp,0x51d0d4);MEMF(g_esi+24)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_ecx=g_esi;MC_CALL(0026F6A0,0x2701ce);
 int mismatch=0,special=0;
 if(MEM32(g_edi)!=0xffffffff){g_edx=MEM32(0x51d0d4);if(g_edx!=4){g_ecx=MEM32(0x51d0d8);SET_LO8(g_eax,0);if(LO8(g_ecx)!=MEM8(g_edi+4))SET_LO8(g_eax,1);if(HI8(g_ecx)!=MEM8(g_edi+5))SET_LO8(g_eax,1);SET_LO16(g_ecx,MEM16(0x51d0da));if(LO8(g_ecx)!=MEM8(g_edi+6))SET_LO8(g_eax,1);mismatch=HI8(g_ecx)!=MEM8(g_edi+7)||LO8(g_eax)!=LO8(g_ebx);special=!mismatch&&g_edx==0xffffffff;}}
 if(mismatch){g_xmm0=XMM_SCALAR(MEMF(g_esi+272));g_xmm0.f[0]+=MEMF(0x48817c);}else if(special)g_xmm0=XMM_SCALAR(MEMF(0x44d168));else g_xmm0=XMM_ZERO();g_eax=g_esi;g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);MEMF(g_esi+272)=g_xmm0.f[0];POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=!g_esp;g_esp+=24;return 1;
}
