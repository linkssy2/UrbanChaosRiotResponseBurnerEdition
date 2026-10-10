#include "migration_graphics_pixel_constants.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void);
static int constants_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_PIXEL_CONSTANTS");flag=v&&!strcmp(v,"1");}return flag;}
static int constants_span(uint64_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&p+n<=g_xbox_total_ram;}
/* Publish each packed constant to exactly the combiner slots whose live shader
 * selector matches it. Growth remains an original synchronous service. */
int uc_migration_graphics_pixel_constants(uint32_t entry){
 if(!constants_enabled()||entry!=0x3eee90)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!constants_span(s-80,112)||!constants_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t count=MEM32(s+12),index=MEM32(s+4),device=MEM32(0x3f4238);
 if(index>16||count>16-index||!constants_span(device,0x928))return 0;
 uint32_t shader=MEM32(device+0x784);if(!constants_span(shader,12))return 0;shader=MEM32(shader+8);if(!constants_span(shader,0xf0))return 0;
 if(count&&(!constants_span(MEM32(s+8),(uint64_t)count*16)||!constants_span(MEM32(device),160)))return 0;
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp-=8;g_edx=MEM32(0x3f4238);g_eax=MEM32(g_edx+0x784);g_ecx=MEM32(g_eax+8);g_eax=MEM32(frame+12);MEM32(frame+12)=g_eax;g_eax=MEM32(frame+16);MEM32(frame-4)=g_edx;MEM32(frame-8)=g_ecx;
 if(g_eax){
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(frame+8);MEM32(frame+16)=g_eax;
  do{
   g_eax=MEM32(frame+12);g_xmm0=XMM_MEM(g_eax);g_xmm0=XMM_MIN(g_xmm0,XMM_MEM(0x4eaa80));g_xmm0=XMM_MAX(g_xmm0,XMM_MEM(0x519e10));g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(0x4eaa90));
   g_eax=(uint32_t)(int32_t)g_xmm0.f[0];g_eax<<=16;g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0x39);g_ebx=(uint32_t)(int32_t)g_xmm0.f[0];g_ebx<<=8;g_eax|=g_ebx;g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0x39);g_ebx=(uint32_t)(int32_t)g_xmm0.f[0];g_eax|=g_ebx;g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0x39);g_ebx=(uint32_t)(int32_t)g_xmm0.f[0];g_ebx<<=24;g_eax|=g_ebx;MEM32(frame+8)=g_eax;g_esi=MEM32(frame+8);MEM32(g_edx+g_edi*4+0x8e4)=g_esi;g_eax=MEM32(g_edx);
   if(g_eax>=MEM32(g_edx+4)){g_ebp=frame;PUSH32(g_esp,0x3eef2f);sub_003E9810();g_edx=MEM32(frame-4);}
   for(unsigned bank=0;bank<3;bank++){
    g_ecx=MEM32(g_edi*4+0x4eaaa0);if(!bank)g_ebx=MEM32(frame-8);g_ecx^=MEM32(g_ebx+0xe4+bank*4);
    unsigned slots=bank==2?2:8;for(unsigned slot=0;slot<slots;slot++)if(!(g_ecx&(15u<<(slot*4)))){
     MEM32((bank==2?0x3f404c:0x3f3fc8+bank*32)+slot*4)=g_esi;MEM32(g_eax)=(bank==2?0x41e20:0x40a60+bank*32)+slot*4;MEM32(g_eax+4)=g_esi;g_eax+=8;
    }
   }
   if(!g_edi){g_esi=MEM32(frame+12);MEM32(g_eax)=0xc181c;g_ebx=MEM32(g_esi);g_ecx=g_eax+4;MEM32(g_ecx)=g_ebx;g_ebx=MEM32(g_esi+4);MEM32(g_ecx+4)=g_ebx;g_esi=MEM32(g_esi+8);MEM32(g_ecx+8)=g_esi;g_eax+=16;}
   g_ecx=MEM32(frame+12);MEM32(g_edx)=g_eax;g_eax=MEM32(frame+16);g_ecx+=16;g_edi++;g_eax--;MEM32(frame+12)=g_ecx;MEM32(frame+16)=g_eax;
  }while(g_eax);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
 }
 g_esp=frame;POP32(g_esp,frame);g_esp+=16;return 1;
}
