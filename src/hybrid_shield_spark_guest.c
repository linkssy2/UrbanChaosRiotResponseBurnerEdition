#include "hybrid_shield_spark_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00218A60(void),sub_001FB780(void),sub_001F8DC0(void),sub_001FB9D0(void);
extern void sub_00208C50(void),sub_00219450(void),sub_002D57FB(void),sub_0018EA80(void);
extern void sub_0018ECA0(void),sub_002D5CCD(void),sub_001FBA40(void),sub_00218CE0(void);
extern void sub_00055E00(void),sub_0018EB40(void),sub_001FB930(void),sub_001F9010(void);
extern void sub_0018ECF0(void),sub_001FB910(void),sub_00218C40(void);
static int spark_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_SHIELD_SPARK");f=p&&!strcmp(p,"1");}return f;}
static int spark_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int spark_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define SPARK_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define SPARK_VCALL(target,pc,saved) do{uint32_t t_=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(t_,saved);}while(0)
#define SPARK_PUSH(v) do{double value_=(v);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value_;}while(0)
#define SPARK_POP() (g_fp_top=(g_fp_top+1)&7)
#define SPARK_TOP() g_fp_stack[g_fp_top]
#define SPARK_ST1() g_fp_stack[(g_fp_top+1)&7]
static void spark_construct(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x396566);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+12)=g_esi;SPARK_CALL(sub_00218A60,0x18eaa4);
 g_edi=g_esi+0xd0;g_ecx=g_edi;MEM32(g_esp+24)=0;MEM32(g_esi)=0x474100;SPARK_CALL(sub_001FB780,0x18eabf);
 g_ebx=MEM32(g_esp+32);g_eax=g_esp+32;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x44c1f0);PUSH32(g_esp,2);g_ecx=g_ebx;MEM8(g_esp+36)=1;SPARK_CALL(sub_001F8DC0,0x18eadb);
 int found=LO8(g_eax)!=0;g_xmm0=XMM_SCALAR(MEMF(0x44bf94));if(found)g_xmm0=XMM_SCALAR(MEMF(g_esp+32));
 g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x44c1c4);PUSH32(g_esp,1);g_ecx=g_ebx;MEMF(g_esi+0xd4)=g_xmm0.f[0];SPARK_CALL(sub_001F8DC0,0x18eb08);
 if(LO8(g_eax))g_eax=MEM32(g_esp+32);else g_eax=0xffffffffu;
 PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ecx=g_edi;MEM32(g_esi+0xd8)=g_eax;SPARK_CALL(sub_001FB9D0,0x18eb25);
 g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void spark_recipes(void){
 static const uint32_t keys[]={0x474218,0x4741ec,0x4741c0,0x474194};
 static const uint32_t world_pc[]={0x18eb51,0x18eb83,0x18ebb5,0x18ebe7};
 static const uint32_t property_pc[]={0x18eb69,0x18eb9b,0x18ebcd,0x18ebff};
 static const uint32_t descriptor_pc[]={0x18eb6f,0x18eba1,0x18ebd3,0x18ec05};
 static const uint32_t publish_pc[]={0x18eb78,0x18ebaa,0x18ebdc,0x18ec0e};
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_edi);
 for(unsigned i=0;i<4;i++){
  if(i)g_ecx=MEM32(0x51c4dc);SPARK_CALL(sub_00208C50,world_pc[i]);
  g_edx=MEM32(g_esi);g_edi=g_eax;g_eax=g_esp+12;MEM32(g_esp+12)=keys[i];g_ebx=MEM32(g_edi);
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;SPARK_VCALL(MEM32(g_edx+44),property_pc[i],saved);}
  PUSH32(g_esp,g_eax);SPARK_CALL(sub_00219450,descriptor_pc[i]);g_esp+=4;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;SPARK_VCALL(MEM32(g_ebx+16),publish_pc[i],saved);}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_hybrid_shield_spark(unsigned entry){
 if(!spark_enabled())return 0;
 switch(entry){case 0x18ea80:case 0x18eb40:case 0x18ec20:case 0x18ec80:case 0x18eca0:case 0x18ecf0:case 0x18ed70:case 0x120c60:break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx,frame=g_seh_ebp;
 if((s&3)||s<0x12000u||!spark_span(s-8192,8224)||!spark_span(0x51c4dc,4)||!spark_span(0x51c178,1)||MEM8(0x51c178)||!spark_apart(s-8192,8224,0x51c178,1))return 0;
 if(entry!=0x18ec20&&(!spark_span(p,0xe0)||!spark_apart(p,0xe0,s-8192,8224)||!spark_apart(p,0xe0,0x51c178,1)))return 0;
 if(entry==0x120c60&&MEM32(p)!=0x474100)return 0;
 if(entry==0x18ecf0&&(unsigned)g_fp_top>7)return 0;
 /* Retained callbacks keep source-owned objects and completing lifetimes.
  * Every decline precedes publication; callbacks never cause fallback replay. */
 if(entry==0x18ea80){spark_construct();return 1;}
 if(entry==0x18eb40){spark_recipes();return 1;}
 if(entry==0x18ec20){
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39657b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xe0);SPARK_CALL(sub_002D57FB,0x18ec40);g_esp+=4;MEM32(g_esp)=g_eax;MEM32(g_esp+12)=0;
  if(g_eax){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;SPARK_CALL(sub_0018EA80,0x18ec5e);}else g_eax=0;
  g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(entry==0x18ec80){PUSH32(g_esp,g_esi);g_esi=g_ecx;SPARK_CALL(sub_0018ECA0,0x18ec88);if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);SPARK_CALL(sub_002D5CCD,0x18ec95);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 if(entry==0x18eca0){
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x396598);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;g_ecx=g_esi+0xd0;MEM32(g_esp+16)=0;SPARK_CALL(sub_001FBA40,0x18ecd0);
  g_ecx=g_esi;MEM32(g_esp+16)=0xffffffffu;SPARK_CALL(sub_00218CE0,0x18ecdf);g_ecx=MEM32(g_esp+8);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(entry==0x120c60){PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=g_esi+0xd0;SPARK_CALL(sub_001FB910,0x120c6e);g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_00218C40();return 1;}
 if(entry==0x18ed70){g_seh_ebp=frame;sub_0018ECF0();return 1;}
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_edi);SPARK_CALL(sub_00055E00,0x18ed00);
 g_edx=MEM32(g_eax);{uint32_t saved=g_esp;PUSH32(g_esp,3);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3d4ccccd);PUSH32(g_esp,0x447a0000);g_ecx=g_esi+64;PUSH32(g_esp,g_ecx);g_ecx=g_eax;SPARK_VCALL(MEM32(g_edx+20),0x18ed1c,saved);}
 g_ecx=g_esi;SPARK_CALL(sub_0018EB40,0x18ed23);g_xmm0=XMM_ZERO();MEMF(g_esp+8)=g_xmm0.f[0];g_edi=g_esi+0xd0;
 do{g_ecx=g_edi;SPARK_CALL(sub_001FB930,0x18ed39);SPARK_CALL(sub_001F9010,0x18ed3e);SPARK_TOP()+=MEMF(g_esp+8);MEMF(g_esp+8)=(float)SPARK_TOP();SPARK_PUSH(MEMF(0x46c31c));g_fp_cmp=RECOMP_FCMP(SPARK_TOP(),SPARK_ST1());SPARK_POP();SPARK_POP();}while(g_fp_cmp>0);
 g_edx=MEM32(g_esi);g_ecx=g_esi;{uint32_t saved=g_esp;SPARK_VCALL(MEM32(g_edx+36),0x18ed59,saved);}
 g_ecx=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;g_seh_ebp=frame;sub_001FB930();return 1;
}
#undef SPARK_CALL
#undef SPARK_VCALL
#undef SPARK_PUSH
#undef SPARK_POP
#undef SPARK_TOP
#undef SPARK_ST1
