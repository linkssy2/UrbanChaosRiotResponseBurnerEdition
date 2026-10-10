#include "hybrid_particle_quad_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D7290(void),sub_00099980(void),sub_000999C0(void),sub_00099C40(void),sub_0009C7A0(void),sub_0009C7E0(void);
extern void sub_003E2FD0(void),sub_003E35D0(void),sub_003E35F0(void),sub_003E3840(void),sub_003E55B0(void),sub_003E7080(void);
extern void sub_003EF600(void),sub_003EF6B0(void),sub_003EF700(void),sub_003EF740(void),sub_003F1900(void);
static int quad_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_QUAD");flag=v&&!strcmp(v,"1");}return flag;}
static int quad_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int quad_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define QUAD_CALL(fn,ret) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
#define QUAD_VCALL(target,ret,saved) do{uint32_t t_=(target);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(t_,saved);}while(0)
int uc_hybrid_particle_quad_draw(void){
 if(!quad_enabled())return 0;
 uint32_t start=g_esp,owner=g_ecx,frame=g_ebp;
 if((start&3)||start<0x14000u||!quad_span(start-16384,16392)||!quad_span(owner,8)||
    !quad_apart(owner,8,start-16384,16392)||!quad_span(0x51c178,1)||MEM8(0x51c178)||
    !quad_apart(start-16384,16392,0x51c178,1)||!quad_apart(owner,8,0x51c178,1))return 0;
 /* Completing callbacks retain their original live-buffer/lifetime contract.
  * Re-read each vertex and UV after preceding device calls, as the source does. */
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_eax=0x2034;QUAD_CALL(sub_002D7290,0x9ad40);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edi);
  MEM32(g_esp+0x14)=g_esi;QUAD_VCALL(MEM32(g_eax+0xa4),0x9ad57,saved);}
 g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;QUAD_VCALL(MEM32(g_edx+0x1c),0x9ad5e,saved);}
 g_ecx=MEM32(0x519f60);g_eax=MEM32(g_eax+0x10);g_edx=MEM32(g_ecx);MEM32(g_esp+0x24)=g_eax;
 {uint32_t saved=g_esp;QUAD_VCALL(MEM32(g_edx+0x50),0x9ad70,saved);}
 g_ecx=MEM32(g_esi+4);MEM32(g_esp+0x28)=g_eax;g_eax=MEM32(g_ecx);{uint32_t saved=g_esp;QUAD_VCALL(MEM32(g_eax+0x10),0x9ad7c,saved);}
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);--g_eax;
 {uint32_t carry=g_eax!=0;g_eax=0u-g_eax;SET_LO8(g_eax,carry?0xff:0);SET_LO8(g_eax,(uint8_t)(LO8(g_eax)+1));}
 MEM8(g_esp+0x13)=LO8(g_eax);g_eax=g_esp+0x38;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=g_esp+0x40;PUSH32(g_esp,g_eax);g_eax=g_esp+0x28;PUSH32(g_esp,g_eax);QUAD_VCALL(MEM32(g_edx+8),0x9ad9e,saved);}
 g_eax=MEM32(g_esp+0x20);if(!g_eax)goto finish;
 g_ecx=MEM32(g_esp+0x3c);g_edx=MEM32(g_esp+0x24);g_edi=MEM32(g_esp+0x38);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_esi;SET_LO8(g_ebx,1);QUAD_CALL(sub_000999C0,0x9adc1);
 if(g_edi){
  g_esi=1;do{PUSH32(g_esp,0);PUSH32(g_esp,g_esi);QUAD_CALL(sub_003E3840,0x9add8);++g_esi;}while((int32_t)g_esi<4);
  g_eax=MEM32(g_edi);{uint32_t saved=g_esp;PUSH32(g_esp,0);g_ecx=g_edi;QUAD_VCALL(MEM32(g_eax+0x1c),0x9ade7,saved);}
  g_ecx=MEM32(0x51a150);PUSH32(g_esp,g_ecx);QUAD_CALL(sub_0009C7A0,0x9adf3);SET_LO8(g_eax,MEM8(g_esp+0x17));g_esp+=4;
  if(LO8(g_eax)){
   g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;QUAD_VCALL(MEM32(g_edx+0x9c),0x9ae10,saved);}
   g_edx=MEM32(g_esp+0x20);g_esi=MEM32(g_eax+0x28);g_ecx=MEM32(g_eax+0x34);PUSH32(g_esp,0);g_esi+=g_edx;PUSH32(g_esp,g_ecx);MEM32(g_eax+0x28)=g_esi;QUAD_CALL(sub_003F1900,0x9ae27);
   g_esi=g_eax;MEM32(g_esp+0x34)=g_esi;QUAD_CALL(sub_003E35D0,0x9ae32);MEM32(g_esp+0x30)=g_eax;QUAD_CALL(sub_003E35F0,0x9ae3b);
   PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);MEM32(g_esp+0x34)=g_eax;QUAD_CALL(sub_003E2FD0,0x9ae46);
   g_eax=MEM32(0x51a274);PUSH32(g_esp,g_eax);QUAD_CALL(sub_0009C7E0,0x9ae51);g_esp+=4;g_edx=0x1000100;g_ecx=0x40358;QUAD_CALL(sub_003E7080,0x9ae63);
   g_edx=0;g_ecx=0x40300;MEM32(0x3f40ac)=0x1000100;QUAD_CALL(sub_003E7080,0x9ae79);MEM32(0x3f4090)=0;
  }else{g_ecx=MEM32(0x51a1f4);PUSH32(g_esp,g_ecx);QUAD_CALL(sub_0009C7E0,0x9ae91);g_esp+=4;}
 }else{
  SET_LO8(g_ebx,0);g_esi=0;do{PUSH32(g_esp,0);PUSH32(g_esp,g_esi);QUAD_CALL(sub_003E3840,0x9aea8);++g_esi;}while((int32_t)g_esi<4);
  g_edx=MEM32(0x51a18c);PUSH32(g_esp,g_edx);QUAD_CALL(sub_0009C7A0,0x9aeba);g_eax=MEM32(0x51a1c0);PUSH32(g_esp,g_eax);QUAD_CALL(sub_0009C7E0,0x9aec5);g_esp+=8;
 }
 g_esi=MEM32(g_esp+0x28);g_edx=MEM32(g_esi);g_ecx=g_esi;{uint32_t saved=g_esp;QUAD_VCALL(MEM32(g_edx+0x30),0x9aed3,saved);}
 g_eax=MEM32(g_esp+0x20);MEM32(g_esp+0x1c)=0;
 if((int32_t)g_eax>0){
  for(;;){
   g_eax-=MEM32(g_esp+0x1c);g_edi=128;if((int32_t)g_eax<=128)g_edi=g_eax;
   g_eax=MEM32(g_esp+0x14);g_ecx=MEM32(g_eax+4);g_eax=MEM32(g_ecx);g_edx=g_esp+0x40;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi);g_ecx=g_esp+0x844;PUSH32(g_esp,g_ecx);g_ecx=g_esi;MEM32(g_esp+0x20)=g_eax;QUAD_VCALL(MEM32(g_edx+8),0x9af23,saved);}
   g_ecx=MEM32(g_esp+0x1c);g_ecx=MEM32(g_ecx+4);
   {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x28);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x2c);QUAD_VCALL(MEM32(g_eax+12),0x9af38,saved);}
   PUSH32(g_esp,8);QUAD_CALL(sub_003EF700,0x9af3f);
   if((int32_t)g_edi>0){
    g_edx=g_esp+0x40;MEM32(g_esp+0x18)=g_edx;g_esi=g_esp+0x844;
    do{
     g_eax=MEM32(g_esp+0x18);PUSH32(g_esp,g_eax);QUAD_CALL(sub_00099980,0x9af60);g_esp+=4;PUSH32(g_esp,g_eax);PUSH32(g_esp,2);QUAD_CALL(sub_003EF6B0,0x9af6b);
     if(LO8(g_ebx)){g_ecx=MEM32(0x4f87a0);g_edx=MEM32(0x4f879c);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,3);QUAD_CALL(sub_003EF600,0x9af8b);}
     g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi);g_edx=MEM32(g_esi-4);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0xffffffffu);QUAD_CALL(sub_003EF600,0x9afa2);
     if(LO8(g_ebx)){g_eax=MEM32(0x4f87a8);g_ecx=MEM32(0x4f87a4);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,3);QUAD_CALL(sub_003EF600,0x9afc1);}
     g_edx=MEM32(g_esi+0x10);g_eax=MEM32(g_esi+12);g_ecx=MEM32(g_esi+8);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xffffffffu);QUAD_CALL(sub_003EF600,0x9afd9);
     if(LO8(g_ebx)){g_edx=MEM32(0x4f87b8);g_eax=MEM32(0x4f87b4);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,3);QUAD_CALL(sub_003EF600,0x9aff8);}
     g_ecx=MEM32(g_esi+0x28);g_edx=MEM32(g_esi+0x24);g_eax=MEM32(g_esi+0x20);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0xffffffffu);QUAD_CALL(sub_003EF600,0x9b010);
     if(LO8(g_ebx)){g_ecx=MEM32(0x4f87b0);g_edx=MEM32(0x4f87ac);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,3);QUAD_CALL(sub_003EF600,0x9b030);}
     g_eax=MEM32(g_esi+0x1c);g_ecx=MEM32(g_esi+0x18);g_edx=MEM32(g_esi+0x14);PUSH32(g_esp,0x3f800000);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0xffffffffu);QUAD_CALL(sub_003EF600,0x9b048);
     g_ecx=MEM32(g_esp+0x18);g_ecx+=16;g_esi+=48;--g_edi;MEM32(g_esp+0x18)=g_ecx;
    }while(g_edi);
   }
   QUAD_CALL(sub_003EF740,0x9b062);g_ecx=MEM32(g_esp+0x1c);g_eax=MEM32(g_esp+0x20);g_ecx+=128;MEM32(g_esp+0x1c)=g_ecx;
   if((int32_t)g_ecx>=(int32_t)g_eax)break;
   g_esi=MEM32(g_esp+0x28);
  }
 }
 g_eax=MEM32(g_esp+0x24);g_ecx=MEM32(g_esp+0x14);PUSH32(g_esp,g_eax);QUAD_CALL(sub_00099C40,0x9b08a);SET_LO8(g_eax,MEM8(g_esp+0x13));
 if(LO8(g_eax)){
  g_edi=MEM32(g_esp+0x2c);g_esi=MEM32(g_esp+0x30);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);QUAD_CALL(sub_003E2FD0,0x9b0a1);
  g_edx=0x10101;g_ecx=0x40358;QUAD_CALL(sub_003E7080,0x9b0b0);g_ecx=MEM32(g_esp+0x34);PUSH32(g_esp,g_ecx);MEM32(0x3f40ac)=0x10101;QUAD_CALL(sub_003E55B0,0x9b0c4);
  PUSH32(g_esp,g_esi);QUAD_CALL(sub_003E55B0,0x9b0ca);PUSH32(g_esp,g_edi);QUAD_CALL(sub_003E55B0,0x9b0d0);
 }
 g_esi=MEM32(g_esp+0x14);
 finish:
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;QUAD_VCALL(MEM32(g_edx+0x18),0x9b0dc,saved);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=8;return 1;
}
#undef QUAD_CALL
#undef QUAD_VCALL
