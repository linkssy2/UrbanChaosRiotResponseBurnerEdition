#include "hybrid_particle_property_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D6D9A(void),sub_002D76F3(void),sub_0022B210(void),sub_002E0E00(void),sub_002A7B50(void),sub_00011030(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_PROPERTY");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int enum_string(uint32_t p,uint32_t s){
 for(unsigned i=0;i<1024;i++){if(!span(p+i,1)||!apart(p+i,1,s-128,140))return 0;if(!MEM8(p+i))return 1;}return 0;
}
/* Ten original property readers. Conversion, texture ownership and locale
 * services stay live; success bytes, upper return bits and retired stack are
 * observable by the original dispatcher. No decline after the prologue. */
int uc_hybrid_particle_property(unsigned op){
 if(!enabled())return 0;
 uint32_t s=g_esp,o=g_ecx;
 if(op>9||(s&3)||s<0x10100u||!span(s-128,140)||!span(0x10000,0x5f0000)||MEM8(0x51c178)||!apart(s-128,140,0x51c178,1))return 0;
 if(op==9){
  if(!span(o,12)||!apart(o,12,s-128,140))return 0;
  int32_t count=(int32_t)MEM32(o+8);
  if(count>0){
   uint32_t rows=MEM32(o+4);
   if(count>4096||!span(rows,(unsigned)count*4)||!apart(rows,(unsigned)count*4,s-128,140)||!enum_string(MEM32(s+4),s)||!span(MEM32(s+8),4))return 0;
   for(int32_t i=0;i<count;i++)if(MEM32(rows+4*i)&&!enum_string(MEM32(rows+4*i),s))return 0;
  }
 }else{
  unsigned n=op==3?8:op==4?12:op==5?16:op==8?1:4;
  if(!span(MEM32(s+4),1)||!span(MEM32(s+8),n))return 0;
  if(op==7){uint32_t owner=MEM32(0x5150b0);if(!span(owner,4)||!span(MEM32(owner),0x9c)||!apart(owner,4,s-128,140)||!apart(MEM32(owner),0x9c,s-128,140))return 0;}
 }
 if(op==0||op==2){
  g_eax=MEM32(g_esp+8);g_ecx=MEM32(g_esp+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,op==0?0x48316c:0x450a30);PUSH32(g_esp,g_ecx);
  PUSH32(g_esp,op==0?0x23ebd4:0x23ec34);sub_002D6D9A();g_esp+=12;g_eax=g_eax==1;g_recomp_zf=!g_eax;g_esp+=12;return 1;
 }
 if(op==1){
  g_ecx=MEM32(g_esp+4);g_eax=g_esp+4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x48316c);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23ebf4);sub_002D6D9A();g_esp+=12;
  if(g_eax==1){g_xmm0=XMM_SCALAR(MEMF(g_esp+4));g_edx=MEM32(g_esp+8);g_xmm0.f[0]*=MEMF(0x44c1f8);MEMF(g_edx)=g_xmm0.f[0];}else SET_LO8(g_eax,0);
  g_recomp_zf=1;g_esp+=12;return 1;
 }
 if(op==3){
  g_eax=MEM32(g_esp+8);g_edx=MEM32(g_esp+4);g_ecx=g_eax+4;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x483188);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x23ec58);sub_002D6D9A();
  g_eax=g_eax==2;g_esp+=16;g_recomp_zf=!g_eax;g_esp+=12;return 1;
 }
 if(op==4){
  g_eax=MEM32(g_esp+8);g_ecx=g_eax+8;PUSH32(g_esp,g_ecx);g_edx=g_eax+4;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+16);PUSH32(g_esp,0x483198);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23ec8c);sub_002D6D9A();
  g_eax=g_eax==3;g_esp+=20;g_recomp_zf=!g_eax;g_esp+=12;return 1;
 }
 if(op==5){
  uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-12;g_edx=MEM32(frame+8);PUSH32(g_esp,g_esi);g_esi=MEM32(frame+12);
  g_eax=g_esi+8;PUSH32(g_esp,g_eax);g_ecx=g_esi+4;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x483198);PUSH32(g_esp,g_edx);g_ebp=frame;PUSH32(g_esp,0x23ecc4);sub_002D6D9A();
  g_xmm1=XMM_MEM(g_esi);g_xmm0=XMM_SCALAR(MEMF(0x44c23c));g_esp+=20;g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);unsigned same=g_eax==3;g_xmm1=XMM_MUL(g_xmm1,g_xmm0);XMM_STORE(g_esi,g_xmm1);SET_LO8(g_eax,same);
  POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=same;g_esp+=12;return 1;
 }
 if(op==6){
  g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x48316c);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23ed05);sub_002D6D9A();
  g_xmm0=XMM_SCALAR(MEMF(g_esi));g_xmm0.f[0]*=MEMF(0x44c23c);g_esp+=12;unsigned same=g_eax==1;g_eax=0u-(g_eax-1u);SET_LO8(g_eax,same);MEMF(g_esi)=g_xmm0.f[0];POP32(g_esp,g_esi);g_recomp_zf=!same;g_esp+=12;return 1;
 }
 if(op==7){
  g_eax=MEM32(0x5150b0);g_esp-=8;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_eax);PUSH32(g_esp,0x23ed41);sub_0022B210();g_ecx=g_eax;PUSH32(g_esp,0x23ed48);sub_002E0E00();
  g_esi=MEM32(g_esp+20);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ecx=g_esp+16;PUSH32(g_esp,0x23ed57);sub_002A7B50();g_ecx=MEM32(0x5150b0);
  {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edi+0x98);PUSH32(g_esp,0x23ed64);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_esp+24);g_edx=g_eax!=0;PUSH32(g_esp,g_esi);PUSH32(g_esp,0x4831bc);MEM32(g_ecx)=g_eax;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x23ed7d);sub_00011030();
  g_esp+=12;POP32(g_esp,g_edi);SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
 }
 if(op==8){
  static const uint32_t keys[]={0x450110,0x45011c,0x450108,0x450118},pcs[]={0x23eda0,0x23edb2,0x23edc4,0x23edd6};
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);
  for(unsigned i=0;i<4;i++){
   PUSH32(g_esp,keys[i]);PUSH32(g_esp,g_esi);PUSH32(g_esp,pcs[i]);sub_002D76F3();g_esp+=8;
   if(!g_eax){
    if(i<2){g_ecx=MEM32(g_esp+12);MEM8(g_ecx)=1;}else{g_eax=MEM32(g_esp+12);MEM8(g_eax)=0;}
    SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=12;return 1;
   }
  }
  SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=12;return 1;
 }
 uint32_t frame=g_seh_ebp;
 g_edx=MEM32(g_ecx+8);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=0;
 if((int32_t)g_edx>0){
  frame=MEM32(g_ecx+4);
  do{
   g_esi=MEM32(frame);
   if(g_esi){
    g_eax=MEM32(g_esp+20);
    for(;;){
     SET_LO8(g_ebx,MEM8(g_eax));SET_LO8(g_ecx,LO8(g_ebx));unsigned rhs=MEM8(g_esi);
     if(LO8(g_ebx)!=rhs){g_eax=LO8(g_ebx)<rhs?0xffffffffu:1;break;}
     if(!LO8(g_ecx)){g_eax=0;break;}
     SET_LO8(g_ebx,MEM8(g_eax+1));SET_LO8(g_ecx,LO8(g_ebx));rhs=MEM8(g_esi+1);
     if(LO8(g_ebx)!=rhs){g_eax=LO8(g_ebx)<rhs?0xffffffffu:1;break;}
     g_eax+=2;g_esi+=2;if(!LO8(g_ecx)){g_eax=0;break;}
    }
    if(!g_eax){g_eax=MEM32(g_esp+24);MEM32(g_eax)=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=12;return 1;}
   }
   ++g_edi;frame+=4;
  }while((int32_t)g_edi<(int32_t)g_edx);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,0);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=12;return 1;
}
