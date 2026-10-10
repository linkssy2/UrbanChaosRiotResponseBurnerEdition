#include "migration_graphics_viewport_helpers.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E7110(void);
#define VH_PUSH(v) do{double value=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value;}while(0)
#define VH_POP() (g_fp_top=(g_fp_top+1u)&7u)
#define VH_ST(i) g_fp_stack[(g_fp_top+(i))&7u]
static int vh_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_VIEWPORT_HELPERS");flag=v&&!strcmp(v,"1");}return flag;}
static int vh_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void vh_unsigned(uint32_t bits){VH_PUSH((double)(int32_t)bits);if((int32_t)bits<0)VH_ST(0)+=MEMF(0x44c510);}
static void vh_exchange(void){double t=VH_ST(0);VH_ST(0)=VH_ST(1);VH_ST(1)=t;}
/* Build scale and offset vectors through the modeled x87 stack. Intermediate
 * guest spills and final store order are part of the existing alias contract. */
static void viewport_vectors(void){
 g_eax=MEM32(0x3f4238);g_ecx=MEM32(g_eax+0xee0);VH_PUSH((double)(int32_t)g_ecx);g_esp-=16;if((int32_t)g_ecx<0)VH_ST(0)+=MEMF(0x44c510);
 VH_ST(0)*=MEMF(g_eax+0x95c);g_edx=MEM32(g_eax+0xee4);VH_ST(0)+=MEMF(g_eax+0xef8);vh_unsigned(g_edx);
 VH_ST(0)*=MEMF(g_eax+0x960);g_edx=MEM32(g_eax+8);g_ecx=0;VH_ST(0)+=MEMF(g_eax+0xefc);
 if((int8_t)HI8(g_edx)<0&&MEM32(0x3f4200)!=g_ecx){vh_exchange();VH_ST(0)-=MEMF(0x44c1f8);vh_exchange();VH_ST(0)-=MEMF(0x44c1f8);}
 g_edx=MEM32(g_eax+0xee8);vh_unsigned(g_edx);VH_ST(0)*=MEMF(g_eax+0x95c);g_edx=MEM32(g_eax+0xeec);VH_ST(0)*=MEMF(0x44c1f8);MEMF(g_esp)=(float)VH_ST(0);VH_POP();
 vh_unsigned(g_edx);VH_ST(0)*=MEMF(g_eax+0x960);VH_ST(0)*=MEMF(0x44e868);MEMF(g_esp+4)=(float)VH_ST(0);VH_POP();
 VH_PUSH(MEMF(g_eax+0xef4));VH_ST(0)-=MEMF(g_eax+0xef0);VH_ST(0)*=MEMF(g_eax+0x948);MEMF(g_esp+12)=(float)VH_ST(0);VH_POP();
 VH_PUSH(MEMF(g_eax+0xef0));VH_ST(0)*=MEMF(g_eax+0x948);g_eax=MEM32(g_esp+20);MEM32(g_eax+12)=g_ecx;MEMF(g_esp+8)=(float)VH_ST(0);VH_POP();
 VH_PUSH(MEMF(g_esp));g_edx=MEM32(g_esp+8);VH_ST(0)+=VH_ST(2);MEM32(g_eax+8)=g_edx;g_edx=MEM32(g_esp+4);MEMF(g_eax)=(float)VH_ST(0);VH_POP();
 VH_PUSH(MEMF(g_esp+4));VH_ST(0)=-VH_ST(0);VH_ST(0)+=VH_ST(1);MEMF(g_eax+4)=(float)VH_ST(0);VH_POP();g_eax=MEM32(g_esp+24);MEM32(g_eax+4)=g_edx;g_edx=MEM32(g_esp+12);VH_POP();VH_POP();MEM32(g_eax+8)=g_edx;
 VH_PUSH(MEMF(g_esp));MEM32(g_eax+12)=g_ecx;MEMF(g_eax)=(float)VH_ST(0);VH_POP();g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void viewport_packet(void){
 g_esp-=36;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x794);int transformed=MEM8(g_eax+4)&0x12;PUSH32(g_esp,g_edi);g_edi=g_edx;
 if(transformed){
  g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);g_edx=g_esp+16;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x3e7c78);sub_003E7110();g_eax=MEM32(g_esi+8);
  if(!(g_eax&0x200)){
   g_eax=MEM32(g_esp+12);g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_esp+20);MEM32(g_edi+4)=g_eax;g_eax=MEM32(g_esp+24);MEM32(g_edi+8)=g_ecx;g_ecx=MEM32(g_esp+28);MEM32(g_edi+12)=g_edx;g_edx=MEM32(g_esp+32);MEM32(g_edi+16)=g_eax;g_eax=MEM32(g_esp+36);MEM32(g_edi)=0x100a20;MEM32(g_edi+24)=g_ecx;g_ecx=MEM32(g_esp+40);MEM32(g_edi+20)=0x100af0;MEM32(g_edi+28)=g_edx;MEM32(g_edi+32)=g_eax;MEM32(g_edi+36)=g_ecx;g_edi+=40;
  }
  SET_LO8(g_ecx,MEM8(g_esi+0x1c18));g_eax=2;
  if(!(g_ecx&2)){
   if(MEM32(0x3f41dc)==g_eax){VH_PUSH(MEMF(g_esi+0x944));g_edx=MEM32(g_esi+0x948);VH_ST(0)*=MEMF(g_esi+0x93c);MEM32(g_esi+0x1c14)=g_edx;VH_ST(0)*=MEMF(g_esi+0x948);MEMF(g_esi+0x1c10)=(float)VH_ST(0);VH_POP();}
   else{g_eax=MEM32(g_esi+0x948);MEM32(g_esi+0x1c10)=0;MEM32(g_esi+0x1c14)=g_eax;}
  }
  VH_PUSH(MEMF(g_esi+0x1c14));VH_PUSH(MEMF(g_esi+0x1c10));
 }else{
  g_ecx=MEM32(g_esi+0xee0);vh_unsigned(g_ecx);VH_ST(0)*=MEMF(g_esi+0x95c);g_edx=MEM32(g_esi+0xee4);VH_ST(0)+=MEMF(g_esi+0xef8);vh_unsigned(g_edx);VH_ST(0)*=MEMF(g_esi+0x960);g_ecx=MEM32(g_esi+8);g_eax=0;VH_ST(0)+=MEMF(g_esi+0xefc);
  if((int8_t)HI8(g_ecx)<0&&MEM32(0x3f4200)!=g_eax){vh_exchange();VH_ST(0)-=MEMF(0x44c1f8);vh_exchange();VH_ST(0)-=MEMF(0x44c1f8);}
  vh_exchange();MEM32(g_edi)=0x100a20;MEMF(g_edi+4)=(float)VH_ST(0);VH_POP();MEM32(g_edi+12)=g_eax;MEM32(g_edi+16)=g_eax;MEMF(g_edi+8)=(float)VH_ST(0);VH_POP();SET_LO8(g_eax,MEM8(g_esi+0x1c18));g_edi+=20;
  if(!(g_eax&1)){
   if(MEM32(0x3f41dc)==2){VH_PUSH(MEMF(g_esi+0x944));g_eax=MEM32(g_esi+0x948);VH_ST(0)*=MEMF(g_esi+0x93c);MEM32(g_esi+0x1c0c)=g_eax;VH_ST(0)*=MEMF(g_esi+0x948);MEMF(g_esi+0x1c08)=(float)VH_ST(0);VH_POP();}
   else{VH_PUSH(MEMF(g_esi+0xef0));VH_ST(0)*=MEMF(g_esi+0x948);MEMF(g_esi+0x1c08)=(float)VH_ST(0);VH_POP();VH_PUSH(MEMF(g_esi+0xef4));VH_ST(0)*=MEMF(g_esi+0x948);MEMF(g_esi+0x1c0c)=(float)VH_ST(0);VH_POP();}
  }
  VH_PUSH(MEMF(g_esi+0x1c0c));VH_PUSH(MEMF(g_esi+0x1c08));
 }
 MEMF(g_edi+4)=(float)VH_ST(0);VH_POP();MEM32(g_edi)=0x80394;g_eax=g_edi+12;MEMF(g_edi+8)=(float)VH_ST(0);VH_POP();POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=36;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void program_packets(void){
 uint32_t frame=g_seh_ebp;g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_esp+12);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 do{
  g_ebx=g_ecx<32?g_ecx:32;g_esi=MEM32(g_esp+24);g_ecx=(g_ebx<<18)+0xb00;MEM32(g_eax)=g_ecx;g_edx=g_ebx*4;frame=g_edx;g_ecx=g_edx>>2;g_edi=g_eax+4;
  memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=frame&3;
  memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx);g_esi+=g_ecx;g_edi+=g_ecx;g_ecx=0;
  g_esi=MEM32(g_esp+24);g_ecx=MEM32(g_esp+28);g_esi+=g_edx;g_ecx-=g_ebx;MEM32(g_esp+24)=g_esi;g_eax+=g_edx+4;MEM32(g_esp+28)=g_ecx;
 }while(g_ecx);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=16;
}
int uc_migration_graphics_viewport_helpers(uint32_t entry){
 if(!vh_enabled())return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10400||!vh_span(s-1024,1044)||!vh_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x3e64e0){uint32_t count=MEM32(s+12);if(!vh_span(MEM32(s+8),(uint64_t)count*4)||!vh_span(MEM32(s+4),(uint64_t)count*4+((uint64_t)count+31)/32*4+4))return 0;program_packets();return 1;}
 if((unsigned)g_fp_top>7||!vh_span(0x3f4238,4)||!vh_span(0x44bf94,0x2900)||!vh_span(0x3f41dc,40))return 0;
 if(entry==0x3e7110){if(!vh_span(MEM32(0x3f4238),0x1c1c)||!vh_span(MEM32(s+4),16)||!vh_span(MEM32(s+8),16))return 0;viewport_vectors();}
 else if(entry==0x3e7c50){if(!vh_span(g_ecx,0x1c1c)||!vh_span(g_edx,52))return 0;viewport_packet();}else return 0;
 return 1;
}
