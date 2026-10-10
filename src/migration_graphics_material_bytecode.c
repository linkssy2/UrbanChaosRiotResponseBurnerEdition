#include "migration_graphics_material_bytecode.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0008E840(void);
extern void sub_0008E920(void);
extern void sub_0009C7A0(void);
extern void sub_0009C7E0(void);
extern void sub_0009C830(void);
extern void sub_002D3DC0(void);
extern void sub_002D6780(void);
extern void sub_003E6950(void);
extern void sub_003E7050(void);
extern void sub_003E7080(void);
extern void sub_003E7380(void);
extern void sub_003E73C0(void);
extern void sub_003E7400(void);
extern void sub_003E7450(void);
extern void sub_003E74C0(void);
extern void sub_003E7500(void);
extern void sub_003E7540(void);
extern void sub_003E75A0(void);
extern void sub_003E7610(void);
extern void sub_003E7690(void);
extern void sub_003E7710(void);
extern void sub_003E7770(void);
extern void sub_003E77C0(void);
extern void sub_003E7820(void);
extern void sub_003E78A0(void);
extern void sub_003E78F0(void);
extern void sub_003E7A00(void);
extern void sub_003E7A70(void);
extern void sub_003E7AB0(void);
extern void sub_003E82D0(void);
extern void sub_003E8360(void);
extern void sub_003E8400(void);
extern void sub_003E8470(void);
extern void sub_003E84A0(void);
extern void sub_003E8510(void);
extern void sub_003E8580(void);
extern void sub_003E85A0(void);
extern void sub_003E85C0(void);
extern void sub_003E85E0(void);
extern void sub_003E8610(void);
extern void sub_003E8640(void);
extern void sub_003E86C0(void);
extern void sub_003E8710(void);

static int bytecode_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MATERIAL_BYTECODE");flag=v&&!strcmp(v,"1");}return flag;}
static int bytecode_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void bytecode_indirect(uint32_t table,unsigned offset,uint32_t pc,uint32_t saved){uint32_t target=MEM32(table+offset);PUSH32(esp,pc);RECOMP_ICALL_SAFE(target,saved);}
#define BC_CALL(fn,pc) do{g_ebp=frame;PUSH32(esp,pc);fn();}while(0)
#define BC_TOP g_fp_stack[g_fp_top]
#define BC_POP() (g_fp_top=(g_fp_top+1)&7)
#define BC_PUSH(v) do{double value=(v);g_fp_top=(g_fp_top+7)&7;BC_TOP=value;}while(0)
typedef struct{void(*call)(void);uint32_t reset_pc,value_pc;}BytecodeState;
static const BytecodeState bytecode_states[]={
{sub_003E7050,0x8f353,0x8f640},
{sub_003E78A0,0x8f367,0x8f653},
{sub_003E7400,0x8f37B,0x8f666},
{sub_003E7770,0x8f38F,0x8f679},
{sub_003E77C0,0x8f3A3,0x8f68C},
{sub_003E7820,0x8f3B7,0x8f69F},
{sub_003E7500,0x8f3CB,0x8f6B2},
{sub_003E82D0,0x8f3DF,0x8f6C5},
{sub_003E8360,0x8f3F3,0x8f6D8},
{sub_003E8400,0x8f407,0x8f6EB},
{sub_003E74C0,0x8f42F,0x8f711},
{sub_003E7450,0x8f41B,0x8f6FE},
{sub_003E7540,0x8f443,0x8f724},
{sub_003E7690,0x8f457,0x8f737},
{sub_003E7710,0x8f46B,0x8f74A},
{sub_003E7380,0x8f47F,0x8f75D},
{sub_003E8640,0x8f493,0x8f770},
{sub_003E86C0,0x8f4A7,0x8f783},
{sub_003E85E0,0x8f4BB,0x8f796},
{sub_003E8610,0x8f4CF,0x8f7A9},
{sub_003E73C0,0x8f4E3,0x8f7BC},
{sub_003E75A0,0x8f4F7,0x8f7CF},
{sub_003E8710,0x8f50B,0x8f7E2},
{sub_003E7610,0x8f51F,0x8f7F5},
{sub_003E8470,0x8f533,0x8f808},
{sub_003E84A0,0x8f547,0x8f81B},
{sub_003E8510,0x8f55B,0x8f82E},
{sub_003E8580,0x8f56F,0x8f841},
{sub_003E85A0,0x8f583,0x8f858},
{sub_003E85C0,0x8f59B,0x8f5E4},
};
/* Complete material VM. Guest scratch writes and callback-visible registers are
 * part of its ABI, including the inactive FP slots and caller-owned arguments. */
int uc_migration_graphics_material_bytecode(uint32_t entry){
 if(!bytecode_enabled()||entry!=0x8ec20)return 0;
 uint32_t stack=esp;
 if((stack&3)||stack<0x10400||!bytecode_span(stack-1024,1044)||!bytecode_span(ecx,80)||!bytecode_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t mode=MEM32(stack+12),count=mode<2?MEM32(ecx+8+mode*4):0;
 if(count>65535||(count&&!bytecode_span(MEM32(ecx+20+mode*4),count)))return 0;
 uint32_t frame=g_ebp,saved,target;
 PUSH32(esp,frame);frame=esp;g_ebp=frame;esp=(esp&~15u)-0x394;PUSH32(esp,ebx);ebx=ecx;ecx=MEM32(frame+16);edx=0;PUSH32(esp,esi);eax=0;esi=0;PUSH32(esp,edi);MEM32(esp+0x38)=ebx;MEM32(esp+0x2c)=0;MEM32(esp+0x28)=0;MEM32(esp+0x14)=0;
 if(!ecx){eax=MEM32(ebx+8);esi=MEM32(ebx+20);MEM32(esp+20)=eax;}
 else if(!--ecx){eax=MEM32(ebx+12);esi=MEM32(ebx+24);MEM32(esp+20)=eax;}
 SET_LO8(ecx,MEM8(frame+12));
 if(LO8(ecx)&&!eax){ecx=MEM32(frame+8);eax=MEM32(ecx);bytecode_indirect(eax,4,0x8ec76,esp);POP32(esp,edi);POP32(esp,esi);POP32(esp,ebx);esp=frame;POP32(esp,frame);g_recomp_zf=1;esp+=20;return 1;}
 edi=esp+0x1a0;
 if(eax)for(;;){
  MEM32(esp+20)--;SET_LO8(eax,MEM8(esi));MEM8(esp+0x27)=LO8(eax);eax=LO8(eax);esi++;eax-=2;edx=16;MEM32(esp+24)=esi;
  if(eax<=195){eax=MEM8(eax+0x8faf8);target=MEM32(eax*4+0x8fa98);
   switch(target){
   case 0x8ecce:
    ecx=esi;eax=MEM32(ecx);MEM32(esp+0x70)=eax;eax=MEM32(ecx+4);MEM32(esp+0x74)=eax;eax=MEM32(ecx+8);ecx=MEM32(ecx+12);MEM32(esp+0x7c)=ecx;ecx=MEM32(esp+20);MEM32(esp+0x78)=eax;xmm0=XMM_MEM(esp+0x70);eax=edi;ecx-=edx;edi+=edx;esi+=edx;XMM_STORE(eax,xmm0);MEM32(esp+20)=ecx;MEM32(esp+24)=esi;break;
   case 0x8ed0c:
    ecx=MEM32(esp+20);eax=MEM32(esi+edx-4);ecx-=edx;esi+=edx;MEM32(esp+20)=ecx;ecx=MEM32(ebx+56);eax=ecx+eax*4;ecx=edi;edi+=edx;edx=MEM32(eax);xmm0=XMM_MEM(edx);MEM32(esp+24)=esi;XMM_STORE(ecx,xmm0);break;
   case 0x8ed37:
    ecx=MEM32(esp+20);eax=MEM32(esi+edx-4);ecx-=edx;esi+=edx;MEM32(esp+20)=ecx;ecx=MEM32(ebx+56);eax=ecx+eax*4;eax=MEM32(eax);xmm0=XMM_SCALAR(MEMF(eax+12));for(unsigned i=0;i<4;i++)MEMF(esp+0xb0+i*4)=xmm0.f[0];xmm0=XMM_MEM(esp+0xb0);eax=edi;MEM32(esp+24)=esi;edi+=edx;XMM_STORE(eax,xmm0);break;
   case 0x8ed90:case 0x8edd7:case 0x8edf0:case 0x8ee09:
    edi-=edx;xmm1=XMM_MEM(edi);edi-=edx;xmm0=XMM_MEM(edi);eax=edi;
    if(target==0x8ed90)xmm0=XMM_MUL(xmm0,xmm1);else if(target==0x8edd7)xmm0=XMM_ADD(xmm0,xmm1);else if(target==0x8edf0)xmm0=XMM_SUB(xmm0,xmm1);else xmm0=XMM_DIV(xmm0,xmm1);
    edi+=edx;XMM_STORE(eax,xmm0);break;
   case 0x8eda9:
    edi-=edx;xmm0=XMM_MEM(edi);XMM_STORE(esp+0xe0,xmm0);xmm0=XMM_SCALAR(MEMF(esp+0xec));edi-=edx;xmm1=XMM_MEM(edi);eax=edi;xmm0=XMM_SHUFFLE(xmm0,xmm0,0);xmm1=XMM_MUL(xmm1,xmm0);edi+=edx;XMM_STORE(eax,xmm1);break;
   case 0x8ee22:
    xmm0=XMM_MEM(0x51e4a0);edi-=edx;xmm1=XMM_MEM(edi);eax=edi;xmm0=XMM_XOR(xmm0,xmm1);edi+=edx;XMM_STORE(eax,xmm0);break;
   case 0x8ee3d:
    xmm0=XMM_SCALAR(MEMF(0x44bf94));edi-=edx;xmm1=XMM_MEM(edi);for(unsigned i=0;i<4;i++)MEMF(esp+0x40+i*4)=xmm0.f[0];xmm0=XMM_MEM(esp+0x40);eax=edi;xmm0=XMM_SUB(xmm0,xmm1);edi+=edx;XMM_STORE(eax,xmm0);break;
   case 0x8ee76:
    eax=MEM32(0x51e390);eax-=edx;ecx=eax;ecx-=0x51e1d0;ecx=(uint32_t)((int32_t)ecx>>31);ecx&=0x1c0;eax+=ecx;ecx=MEM32(0x51e394);ecx-=edx;esi=ecx;esi-=0x51e1d0;esi=(uint32_t)((int32_t)esi>>31);esi&=0x1c0;ecx+=esi;MEM32(0x51e390)=eax;MEM32(0x51e394)=ecx;
    xmm1=XMM_MEM(ecx);xmm0=XMM_MEM(eax);xmm0=XMM_SUB(xmm0,xmm1);xmm1=xmm0;xmm1=XMM_XOR(xmm1,xmm0);xmm2=xmm0;xmm2=XMM_CMP_LT(xmm2,xmm1);xmm1=XMM_MEM(0x51e1b0);xmm1=XMM_AND(xmm1,xmm2);xmm0=XMM_ADD(xmm0,xmm1);XMM_STORE(eax,xmm0);xmm1=XMM_MEM(0x51e190);xmm0=XMM_MUL(xmm0,xmm1);XMM_STORE(esp+0x110,xmm0);xmm0=XMM_SCALAR(MEMF(esp+0x11c));eax=edi;for(unsigned i=0;i<4;i++)MEMF(esp+0xd0+i*4)=xmm0.f[0];xmm0=XMM_MEM(esp+0xd0);edi+=edx;XMM_STORE(eax,xmm0);break;
   case 0x8ef2f:
    edi-=edx;xmm0=XMM_SCALAR(MEMF(edi+12));xmm0.f[0]*=MEMF(0x4505cc);eax=(int32_t)xmm0.f[0];MEM32(esp+32)=eax;if((int32_t)eax<0)MEM32(esp+32)=0;else if((int32_t)eax>255)MEM32(esp+32)=255;
    xmm0=XMM_SCALAR(MEMF(esp+32));eax=edi;for(unsigned i=0;i<4;i++)MEMF(esp+0xc0+i*4)=xmm0.f[0];xmm0=XMM_MEM(esp+0xc0);edi+=edx;XMM_STORE(eax,xmm0);break;
   case 0x8efa1:{
    edi-=edx;xmm0=XMM_MEM(edi);XMM_STORE(esp+0xa0,xmm0);xmm1=XMM_SCALAR(MEMF(esp+0xa0));xmm0=XMM_SCALAR(MEMF(0x4505cc));xmm1.f[0]*=xmm0.f[0];esi=(int32_t)xmm1.f[0];if((int32_t)esi<0)esi=0;else if((int32_t)esi>255)esi=255;
    uint32_t*regs[]={&g_edx,&g_ecx,&g_eax};for(unsigned i=0;i<3;i++){xmm1=XMM_SCALAR(MEMF(esp+0xa4+i*4));xmm1.f[0]*=xmm0.f[0];*regs[i]=(int32_t)xmm1.f[0];if((int32_t)*regs[i]<0)*regs[i]=0;else if((int32_t)*regs[i]>255)*regs[i]=255;}
    eax=((((eax<<8)|esi)<<8)|edx)<<8|ecx;MEM32(esp+0x30)=eax;xmm0=XMM_SCALAR(MEMF(esp+0x30));eax=edi;for(unsigned i=0;i<4;i++)MEMF(esp+0x80+i*4)=xmm0.f[0];xmm0=XMM_MEM(esp+0x80);edi+=16;XMM_STORE(eax,xmm0);break;}
   case 0x8f09f:
    xmm0=XMM_SCALAR(MEMF(0x44bf94));edi-=edx;MEMF(esp+0x9c)=xmm0.f[0];xmm0=XMM_SCALAR(MEMF(edi+12));edi-=edx;MEMF(esp+0x98)=xmm0.f[0];xmm0=XMM_SCALAR(MEMF(edi+12));edi-=edx;MEMF(esp+0x94)=xmm0.f[0];xmm0=XMM_SCALAR(MEMF(edi+12));MEMF(esp+0x90)=xmm0.f[0];xmm0=XMM_MEM(esp+0x90);eax=edi;edi+=edx;XMM_STORE(eax,xmm0);break;
   case 0x8f0f4:
    edx=MEM32(edi-4);edi-=16;ecx=MEM32(edi-4);eax=edx;edi-=16;saved=esp;PUSH32(esp,eax);PUSH32(esp,ecx);ecx=MEM32(0x519f60);MEM32(esp+0x3c)=edx;edx=MEM32(ecx);bytecode_indirect(edx,0xe4,0x8f116,saved);
    MEMF(esp+0x24)=(float)BC_TOP;BC_POP();SET_LO8(eax,MEM8(esp+0x2f));xmm0=XMM_SCALAR(MEMF(esp+0x24));BC_CALL(sub_0008E840,0x8f129);
    MEMF(esp+0x58)=(float)BC_TOP;MEMF(esp+0x5c)=(float)BC_TOP;eax=edi;MEMF(esp+0x60)=(float)BC_TOP;esp+=8;MEMF(esp+0x5c)=(float)BC_TOP;BC_POP();xmm0=XMM_MEM(esp+0x50);edi+=16;XMM_STORE(eax,xmm0);break;

   case 0x8f14e:
    eax=MEM32(esp+20);edx=MEM32(ebx+32);xmm0=XMM_SCALAR(MEMF(edi-4));eax-=16;esi+=16;MEM32(esp+20)=eax;eax=MEM32(esp+0x28);MEM32(esp+24)=esi;esi=MEM32(esi-4);ecx=esi+eax*8;ebx=MEM32(edx+ecx*4+16);eax=MEM32(ebx);edi-=16;ecx=ebx;MEMF(esp+0x3c)=xmm0.f[0];bytecode_indirect(eax,0x24,0x8f186,esp);
    if(!eax)break;
    ecx=MEM32(frame+20);
    if(ecx){
     eax=MEM32(esp+0x2c);edx=MEM32(ecx);esi+=eax;saved=esp;PUSH32(esp,esi);bytecode_indirect(edx,12,0x8f1a1,saved);
     esi=eax;edx=MEM32(esi);ecx=esi;bytecode_indirect(edx,20,0x8f1aa,esp);
     BC_PUSH(MEMF(0x44c174));g_fp_cmp=RECOMP_FCMP(BC_TOP,g_fp_stack[(g_fp_top+1)&7]);BC_POP();BC_POP();SET_HI8(eax,(g_fp_cmp==2?0x47u:g_fp_cmp<0?3u:g_fp_cmp>0?2u:0x42u));
     if(!RECOMP_PARITY8(HI8(eax)&0x44)){eax=MEM32(esi);ecx=esi;bytecode_indirect(eax,12,0x8f1c1,esp);edx=MEM32(ebx);saved=esp;PUSH32(esp,eax);ecx=ebx;bytecode_indirect(edx,32,0x8f1c9,saved);break;}
    }
    eax=MEM32(ebx);ecx=ebx;bytecode_indirect(eax,36,0x8f1d5,esp);MEM32(esp+28)=eax;BC_PUSH((double)SMEM32(esp+28));BC_TOP*=MEMF(esp+0x3c);BC_CALL(sub_002D6780,0x8f1e6);
    edx=MEM32(ebx);ecx=ebx;esi=eax;bytecode_indirect(edx,36,0x8f1ef,esp);ecx=eax;edx=0;eax=esi;{uint64_t dividend=eax;eax=(uint32_t)(dividend/ecx);edx=(uint32_t)(dividend%ecx);}eax=MEM32(ebx);ecx=ebx;saved=esp;PUSH32(esp,edx);bytecode_indirect(eax,32,0x8f1ff,saved);break;
   case 0x8f204:
    ebx=MEM32(esp+20);SET_LO8(eax,MEM8(frame+12));edx=MEM32(edi-4);esi+=16;ebx-=16;edi-=16;MEM32(esp+24)=esi;esi=MEM32(esi-4);MEM32(esp+20)=ebx;
    if(LO8(eax))break;
    eax=esi;eax&=0x3fffffff;esi>>=30;ecx=esi;
    if((int32_t)eax<22){if((int32_t)eax<12){esi=1;esi<<=LO8(ecx);}else esi=MEM32(eax*4+0x44b4d0);ebx=MEM32(0x3f3d98);ebx|=esi;ecx<<=5;ecx+=eax;MEM32(0x3f3d98)=ebx;MEM32(ecx*4+0x3f3da0)=edx;}
    else if(eax==28){PUSH32(esp,edx);PUSH32(esp,ecx);BC_CALL(sub_003E78F0,0x8f27b);}
    else if(eax==29){PUSH32(esp,edx);PUSH32(esp,ecx);BC_CALL(sub_003E7A70,0x8f28c);}
    else if(eax==30){PUSH32(esp,edx);PUSH32(esp,ecx);BC_CALL(sub_003E7AB0,0x8f29d);}
    else if((int32_t)eax<=27){PUSH32(esp,edx);PUSH32(esp,eax);PUSH32(esp,ecx);BC_CALL(sub_003E7A00,0x8f2b3);}break;
   case 0x8f2b8:{
    esi=MEM32(esp+20);eax=MEM32(esp+24);ebx=MEM32(edi-4);esi-=16;eax+=16;MEM32(esp+20)=esi;esi=MEM32(eax-4);MEM32(esp+24)=eax;SET_LO8(eax,MEM8(frame+12));edi-=16;
    int reset=0,fallback=0;
    if(LO8(eax)){eax=esi-59;fallback=1;if(eax<=90){ecx=MEM8(eax+0x8fbc8);uint32_t jump=MEM32(ecx*4+0x8fbbc);if(jump==0x8fa83)break;if(jump==0x8f2fc)reset=1;else if(jump!=0x8f5a0){g_seh_ebp=frame;RECOMP_ITAIL(jump);return 1;}}}
    else if(esi==6)break;
    if((int32_t)esi<92){ecx=MEM32(esi*4+0x44b5d8);edx=reset?0:ebx;g_ebp=frame;PUSH32(esp,reset?0x8f30f:0x8f605);sub_003E7080();MEM32(esi*4+0x3f3fa0)=reset?0:ebx;}
    else if((int32_t)esi<136){
     if(reset){eax=MEM32(0x3f3d98);edx=MEM32(esi*4+0x44b3b8);eax|=edx;MEM32(0x3f3d98)=eax;MEM32(esi*4+0x3f3fa0)=0;}
     else if(fallback){ecx=MEM32(0x3f3d98);eax=MEM32(esi*4+0x44b3b8);ecx|=eax;MEM32(0x3f3d98)=ecx;MEM32(esi*4+0x3f3fa0)=ebx;}
     else{eax=MEM32(0x3f3d98);ecx=MEM32(esi*4+0x44b3b8);eax|=ecx;MEM32(0x3f3d98)=eax;MEM32(esi*4+0x3f3fa0)=ebx;}
    }else if(esi<=0xa5){const BytecodeState*state=&bytecode_states[esi-0x88];PUSH32(esp,reset?0:ebx);g_ebp=frame;PUSH32(esp,reset?state->reset_pc:state->value_pc);state->call();}
    break;}
   case 0x8f85d:
    ebx=MEM32(esp+20);edi-=edx;xmm0=XMM_MEM(edi);ebx-=edx;esi+=edx;MEM32(esp+20)=ebx;MEM32(esp+24)=esi;XMM_STORE(esp+0x100,xmm0);
    if(!LO8(ecx)){ecx=MEM32(esi-4);ecx+=0x60;edx=esp+0x100;BC_CALL(sub_003E6950,0x8f894);}break;
   case 0x8f899:
    xmm0=XMM_ZERO();MEMF(esp+0x60)=xmm0.f[0];MEMF(esp+0x64)=xmm0.f[0];MEMF(esp+0x68)=xmm0.f[0];xmm0=XMM_SCALAR(MEMF(0x44bf94));edx=esp+0x60;PUSH32(esp,edx);PUSH32(esp,ecx);MEMF(esp+0x74)=xmm0.f[0];xmm0=XMM_SCALAR(MEMF(edi-4));xmm0.f[0]*=MEMF(0x44d95c);edi-=16;MEMF(esp)=xmm0.f[0];PUSH32(esp,0);PUSH32(esp,0);ecx=esp+0x170;BC_CALL(sub_002D3DC0,0x8f8e7);
    eax=esp+0x120;PUSH32(esp,eax);ecx=esp+0x164;BC_CALL(sub_0008E920,0x8f8fb);
    xmm0=XMM_MEM(esp+0x120);eax=edi;XMM_STORE(eax,xmm0);xmm0=XMM_MEM(esp+0x130);edi+=16;eax=edi;edi+=16;XMM_STORE(eax,xmm0);xmm0=XMM_MEM(esp+0x140);eax=edi;edi+=16;XMM_STORE(eax,xmm0);break;
   case 0x8f930:
    if(LO8(ecx)){ebx=MEM32(esp+20);ebx-=edx;esi+=edx;MEM32(esp+20)=ebx;MEM32(esp+24)=esi;edi-=edx;break;}
    ecx=MEM32(esp+0x28);edx=MEM32(ebx+32);ecx<<=5;eax=MEM32(ecx+edx+12);PUSH32(esp,eax);BC_CALL(sub_0009C7E0,0x8f948);
    ecx=MEM32(esp+24);edx=MEM32(esi+12);xmm0=XMM_MEM(edi-16);ecx-=16;MEM32(esp+24)=ecx;esi+=16;ecx=esp+0xf4;PUSH32(esp,ecx);edi-=16;PUSH32(esp,edx);MEM32(esp+0x24)=esi;XMM_STORE(esp+0xfc,xmm0);BC_CALL(sub_0009C830,0x8f97a);esp+=12;break;
   case 0x8f999:
    if(!LO8(ecx)){
     eax=MEM32(esp+0x28);ecx=MEM32(ebx+32);eax<<=5;SET_LO8(edx,MEM8(eax+ecx));esi=0;MEM32(esp+28)=eax;
     if(LO8(edx)){eax+=16;MEM32(esp+32)=eax;do{
      edx=MEM32(ebx+32);eax=MEM32(esp+32);ecx=MEM32(eax+edx);edx=MEM32(ecx);saved=esp;PUSH32(esp,esi);bytecode_indirect(edx,28,0x8f9d0,saved);
      ecx=MEM32(esp+32);eax=MEM32(ebx+32);ecx+=4;MEM32(esp+32)=ecx;ecx=MEM32(esp+28);edx=MEM8(eax+ecx);esi++;
     }while((int32_t)esi<(int32_t)edx);eax=ecx;}
     ecx=MEM32(ebx+32);edx=MEM32(ecx+eax+8);PUSH32(esp,edx);BC_CALL(sub_0009C7A0,0x8f9fa);
     eax=MEM32(ebx+32);ecx=MEM32(esp+32);edx=MEM32(eax+ecx+12);PUSH32(esp,edx);BC_CALL(sub_0009C7E0,0x8fa0b);esp+=8;
    }
    ecx=MEM32(frame+8);eax=MEM32(ecx);bytecode_indirect(eax,4,0x8fa16,esp);eax=MEM32(esp+0x28);edx=MEM32(ebx+32);ecx=eax;ecx<<=5;ecx=MEM8(ecx+edx);edx=MEM32(esp+0x2c);edx+=ecx;eax++;MEM32(esp+0x2c)=edx;MEM32(esp+0x28)=eax;break;
   case 0x8fa37:
    if(!LO8(ecx)){edx=1;ecx=0x40304;BC_CALL(sub_003E7080,0x8fa4a);edx=0x306;ecx=0x40344;MEM32(0x3f408c)=1;BC_CALL(sub_003E7080,0x8fa63);edx=0;ecx=0x40348;MEM32(0x3f4098)=0x306;BC_CALL(sub_003E7080,0x8fa79);MEM32(0x3f409c)=0;}break;
   case 0x8fa83:break;
   default:g_seh_ebp=frame;RECOMP_ITAIL(target);return 1;
   }
  }
  eax=MEM32(esp+20);if(!eax)break;ebx=MEM32(esp+0x38);esi=MEM32(esp+24);SET_LO8(ecx,MEM8(frame+12));
 }
 POP32(esp,edi);POP32(esp,esi);POP32(esp,ebx);esp=frame;POP32(esp,frame);esp+=20;return 1;
}
#undef BC_CALL
#undef BC_TOP
#undef BC_POP
#undef BC_PUSH
