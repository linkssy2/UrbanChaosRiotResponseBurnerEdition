#include "hybrid_particle_line_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00099980(void),sub_000999C0(void),sub_00099C40(void),sub_0009C7A0(void),sub_0009C7E0(void);
extern void sub_003E2FD0(void),sub_003E35D0(void),sub_003E35F0(void),sub_003E3840(void),sub_003E55B0(void),sub_003E7080(void);
extern void sub_003EF600(void),sub_003EF6B0(void),sub_003EF700(void),sub_003EF740(void),sub_003F1900(void);
static int line_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_LINE");flag=v&&!strcmp(v,"1");}return flag;}
static int line_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int line_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define LINE_CALL(fn,ret) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
#define LINE_VCALL(target,ret,saved) do{uint32_t t_=(target);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(t_,saved);}while(0)
int uc_hybrid_particle_line_draw(void){
 if(!line_enabled())return 0;
 uint32_t start=g_esp,owner=g_ecx,frame=g_ebp;
 if((start&3)||start<0x12000u||!line_span(start-8192,8200)||!line_span(owner,8)||
    !line_apart(owner,8,start-8192,8200)||!line_span(0x51c178,1)||MEM8(0x51c178)||
    !line_apart(start-8192,8200,0x51c178,1)||!line_apart(owner,8,0x51c178,1))return 0;
 /* The source owns all subsequent callback validity and alias feedback.
  * No list snapshot, geometry prepass, reordered submission or late decline. */
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-0xe34;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edi);
 MEM32(g_esp+0x20)=g_esi;LINE_VCALL(MEM32(g_eax+0xa4),0x9a513,start);
 g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;LINE_VCALL(MEM32(g_edx+0x1c),0x9a51a,saved);}
 g_edi=MEM32(g_eax+0x10);g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);MEM32(g_esp+0x3c)=g_edi;
 {uint32_t saved=g_esp;LINE_VCALL(MEM32(g_eax+0x50),0x9a52c,saved);}
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);MEM32(g_esp+0x30)=g_eax;
 {uint32_t saved=g_esp;LINE_VCALL(MEM32(g_edx+0x10),0x9a538,saved);}
 g_ecx=MEM32(g_esi+4);g_ebx=g_eax;g_eax=MEM32(g_ecx);--g_ebx;g_edx=g_esp+0x38;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t carry=g_ebx!=0;g_ebx=0u-g_ebx;g_edx=g_esp+0x28;PUSH32(g_esp,g_edx);
  SET_LO8(g_ebx,carry?0xff:0);g_edx=g_esp+0x20;SET_LO8(g_ebx,(uint8_t)(LO8(g_ebx)+1));PUSH32(g_esp,g_edx);MEM8(g_esp+0x23)=LO8(g_ebx);
  LINE_VCALL(MEM32(g_eax+8),0x9a55c,saved);}
 g_eax=MEM32(g_esp+0x18);if(!g_eax)goto finish;
 g_ecx=MEM32(g_esp+0x24);g_eax=MEM32(g_esp+0x38);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);g_ecx=g_esi;MEM32(g_esp+0x18)=g_eax;MEM8(g_esp+0x17)=1;
 LINE_CALL(sub_000999C0,0x9a582);g_eax=MEM32(g_esp+0x10);
 if(g_eax){
  g_edi=1;do{PUSH32(g_esp,0);PUSH32(g_esp,g_edi);LINE_CALL(sub_003E3840,0x9a59b);++g_edi;}while((int32_t)g_edi<4);
  g_ecx=MEM32(g_esp+0x10);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,0);LINE_VCALL(MEM32(g_edx+0x1c),0x9a5ac,saved);}
  g_eax=MEM32(0x51a150);PUSH32(g_esp,g_eax);LINE_CALL(sub_0009C7A0,0x9a5b7);g_esp+=4;
  if(LO8(g_ebx)){
   g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;LINE_VCALL(MEM32(g_edx+0x9c),0x9a5d0,saved);}
   g_edx=MEM32(g_esp+0x18);g_edi=MEM32(g_eax+0x28);g_ecx=MEM32(g_eax+0x34);PUSH32(g_esp,0);g_edi+=g_edx;PUSH32(g_esp,g_ecx);MEM32(g_eax+0x28)=g_edi;LINE_CALL(sub_003F1900,0x9a5e7);
   g_edi=g_eax;MEM32(g_esp+0x34)=g_edi;LINE_CALL(sub_003E35D0,0x9a5f2);MEM32(g_esp+0x2c)=g_eax;LINE_CALL(sub_003E35F0,0x9a5fb);
   PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);MEM32(g_esp+0x30)=g_eax;LINE_CALL(sub_003E2FD0,0x9a606);
   g_eax=MEM32(0x51a274);PUSH32(g_esp,g_eax);LINE_CALL(sub_0009C7E0,0x9a611);g_esp+=4;g_edx=0x1000100;g_ecx=0x40358;LINE_CALL(sub_003E7080,0x9a623);
   g_edx=0;g_ecx=0x40300;MEM32(0x3f40ac)=0x1000100;LINE_CALL(sub_003E7080,0x9a639);MEM32(0x3f4090)=0;
  }else{g_ecx=MEM32(0x51a1f4);PUSH32(g_esp,g_ecx);LINE_CALL(sub_0009C7E0,0x9a651);g_esp+=4;}
 }else{
  MEM8(g_esp+0x0f)=0;g_edi=0;do{PUSH32(g_esp,0);PUSH32(g_esp,g_edi);LINE_CALL(sub_003E3840,0x9a668);++g_edi;}while((int32_t)g_edi<4);
  g_edx=MEM32(0x51a18c);PUSH32(g_esp,g_edx);LINE_CALL(sub_0009C7A0,0x9a67a);g_eax=MEM32(0x51a1c0);PUSH32(g_esp,g_eax);LINE_CALL(sub_0009C7E0,0x9a685);g_esp+=8;
 }
 g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;LINE_VCALL(MEM32(g_edx+0x50),0x9a693,saved);}
 g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;LINE_VCALL(MEM32(g_edx+0x30),0x9a69a,saved);}
 g_eax=MEM32(g_esp+0x18);MEM32(g_esp+0x10)=0;
 if((int32_t)g_eax>0){
  do{
   g_eax-=MEM32(g_esp+0x10);g_ebx=64;if((int32_t)g_eax<=64)g_ebx=g_eax;
   g_eax=MEM32(g_esi+4);g_edi=MEM32(g_eax);g_ecx=g_esp+0x640;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+0x34);g_eax=MEM32(g_ecx);g_edx=g_esp+0x44;PUSH32(g_esp,g_edx);LINE_VCALL(MEM32(g_eax+8),0x9a6db,saved);}
   g_ecx=MEM32(g_esp+0x18);{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+4);LINE_VCALL(MEM32(g_edi+12),0x9a6e8,saved);}
   PUSH32(g_esp,2);LINE_CALL(sub_003EF700,0x9a6ef);g_eax=g_ebx;g_edx=(int32_t)g_eax<0?0xffffffffu:0;g_eax-=g_edx;g_eax=(uint32_t)((int32_t)g_eax>>1);
   g_esi=g_esp+0x40;g_edi=g_esp+0x640;
   if((int32_t)g_eax>0){
    MEM32(g_esp+0x1c)=g_eax;
    do{
     PUSH32(g_esp,g_edi);LINE_CALL(sub_00099980,0x9a716);g_esp+=4;PUSH32(g_esp,g_eax);PUSH32(g_esp,2);LINE_CALL(sub_003EF6B0,0x9a721);
     SET_LO8(g_ebx,MEM8(g_esp+0xf));
     if(LO8(g_ebx)){PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x3f000000);PUSH32(g_esp,3);LINE_CALL(sub_003EF600,0x9a73e);}
     g_edx=MEM32(g_esi+8);g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xffffffffu);LINE_CALL(sub_003EF600,0x9a755);
     g_edi+=16;PUSH32(g_esp,g_edi);g_esi+=12;LINE_CALL(sub_00099980,0x9a761);g_esp+=4;PUSH32(g_esp,g_eax);PUSH32(g_esp,2);LINE_CALL(sub_003EF6B0,0x9a76c);
     if(LO8(g_ebx)){PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3f000000);PUSH32(g_esp,3);LINE_CALL(sub_003EF600,0x9a788);}
     g_edx=MEM32(g_esi+8);g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xffffffffu);LINE_CALL(sub_003EF600,0x9a79f);
     g_eax=MEM32(g_esp+0x1c);g_esi+=12;g_edi+=16;--g_eax;MEM32(g_esp+0x1c)=g_eax;
    }while(g_eax);
   }
   LINE_CALL(sub_003EF740,0x9a7b9);g_ecx=MEM32(g_esp+0x10);g_eax=MEM32(g_esp+0x18);g_esi=MEM32(g_esp+0x20);g_ecx+=64;MEM32(g_esp+0x10)=g_ecx;
  }while((int32_t)g_ecx<(int32_t)g_eax);
  SET_LO8(g_ebx,MEM8(g_esp+0x17));
 }
 g_edx=MEM32(g_esp+0x3c);PUSH32(g_esp,g_edx);g_ecx=g_esi;LINE_CALL(sub_00099C40,0x9a7e4);
 if(LO8(g_ebx)){
  g_ebx=MEM32(g_esp+0x28);g_edi=MEM32(g_esp+0x2c);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);LINE_CALL(sub_003E2FD0,0x9a7f7);
  g_edx=0x10101;g_ecx=0x40358;LINE_CALL(sub_003E7080,0x9a806);g_eax=MEM32(g_esp+0x34);PUSH32(g_esp,g_eax);MEM32(0x3f40ac)=0x10101;LINE_CALL(sub_003E55B0,0x9a81a);
  PUSH32(g_esp,g_edi);LINE_CALL(sub_003E55B0,0x9a820);PUSH32(g_esp,g_ebx);LINE_CALL(sub_003E55B0,0x9a826);
 }
 finish:
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;LINE_VCALL(MEM32(g_edx+0x18),0x9a82e,saved);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=8;return 1;
}
#undef LINE_CALL
#undef LINE_VCALL
