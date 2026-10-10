#include "hybrid_render_target_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E2E10(void),sub_003E5530(void),sub_003E56D0(void),sub_003E7610(void),sub_003E7E40(void),sub_003E7F70(void),sub_003E9810(void),sub_003EAC30(void),sub_003EAD50(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_RENDER_TARGET");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* Complete color/depth binding. Keep lifetime callbacks before packet writes,
 * including same-target rebinding and the source's two surface packet copies.
 * The inherited local frame is not the global EBP register. */
int uc_hybrid_render_target(void){
 if(!enabled())return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10100u||!span(s-128,140)||!span(0x3f1d70,256)||!span(0x3f4238,4)||!span(0x51c178,1)||MEM8(0x51c178)||!apart(s-128,140,0x3f1cd8,0x2564)||!apart(s-128,140,0x51c178,1))return 0;
 uint32_t device=MEM32(0x3f4238);
 if(!span(device,0x1a18)||!apart(device,0x1a18,s-128,140))return 0;
 uint32_t color=MEM32(s+4),depth=MEM32(s+8),old_color=MEM32(device+0x1a04),old_depth=MEM32(device+0x1a08),packet=MEM32(device);
 if(!color)color=old_color;
 if(!color||!span(packet,256)||!apart(packet,256,s-128,140))return 0;
 uint32_t surfaces[]={color,depth,old_color,old_depth};
 for(unsigned i=0;i<4;i++)if(surfaces[i]&&(!span(surfaces[i],24)||!apart(surfaces[i],24,s-128,140)))return 0;
 uint32_t frame=g_seh_ebp;
 g_esp-=12;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+24);PUSH32(g_esp,g_esi);g_esi=0;PUSH32(g_esp,g_edi);g_edi=MEM32(0x3f4238);
 if(frame==g_esi){g_eax=MEM32(g_edi+0x1a04);MEM32(g_esp+32)=g_eax;frame=g_eax;}
 PUSH32(g_esp,frame);PUSH32(g_esp,0x3e2ff9);sub_003E5530();
 g_eax=MEM32(g_edi+0x1a04);
 if(g_eax!=g_esi){
  g_ecx=MEM32(g_eax+20);if(g_ecx!=g_esi)g_eax=g_ecx;
  g_ecx=MEM32(g_edi+44);MEM32(g_eax+8)=g_ecx;g_edx=MEM32(g_edi+0x1a04);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x3e301e);sub_003E56D0();
 }
 g_eax=MEM32(g_edi+0x1a08);MEM32(g_edi+0x1a04)=frame;
 if(g_eax!=g_esi){
  g_ecx=MEM32(g_eax+20);if(g_ecx!=g_esi)g_eax=g_ecx;
  g_ecx=MEM32(g_edi+44);MEM32(g_eax+8)=g_ecx;g_edx=MEM32(g_edi+0x1a08);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x3e3049);sub_003E56D0();
 }
 g_ebx=MEM32(g_esp+36);MEM32(g_edi+0x1a08)=g_ebx;
 if(g_ebx!=g_esi){
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x3e305d);sub_003E5530();
  g_eax=g_edi+0x948;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x3e306a);sub_003EAD50();
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x3e3070);sub_003E2E10();MEM32(g_edi+0x94c)=g_eax;
 }
 g_eax=MEM32(g_edi);if(g_eax>=MEM32(g_edi+4)){PUSH32(g_esp,0x3e3082);sub_003E9810();}
 g_ecx=MEM32(frame+4);g_edx=g_eax;MEM32(g_esp+24)=g_ecx;
 if(g_ebx!=g_esi){g_eax=MEM32(g_ebx+4);MEM32(g_esp+16)=g_eax;}else MEM32(g_esp+16)=g_esi;
 MEM32(g_esp+20)=2;frame=0x40100;g_ebx=0x40110;
 do{
  g_eax=MEM32(g_esp+32);MEM32(g_edx)=frame;MEM32(g_edx+4)=g_esi;
  if(MEM32(g_eax+16)==g_esi){
   g_ecx=(MEM32(g_eax+12)>>20)&15;g_eax=1u<<g_ecx;g_ecx=MEM32(g_esp+32);g_ecx=MEM8(g_ecx+13);
   if(g_ecx==12)g_ecx=g_eax+g_eax;
   else if(g_ecx>13&&g_ecx<=15)g_ecx=g_eax*4;
   else{g_ecx=MEM32(g_esp+32);g_ecx=MEM8(g_ecx+13);g_ecx=MEM8(g_ecx+0x3f1d70);g_ecx=((g_ecx&0x3c)*g_eax)>>3;}
  }else{g_ecx=MEM8(g_eax+19);g_ecx=(g_ecx+1)<<6;}
  g_eax=MEM32(g_edi+0x94c);MEM32(g_edx+8)=0x4020c;g_eax<<=16;g_ecx&=0xffff;g_eax|=g_ecx;MEM32(g_edx+12)=g_eax;
  g_eax=MEM32(g_esp+24);MEM32(g_edx+16)=frame;MEM32(g_edx+20)=g_esi;MEM32(g_edx+24)=g_ebx;MEM32(g_edx+28)=g_esi;
  g_ecx=MEM32(g_esp+16);MEM32(g_edx+32)=frame;MEM32(g_edx+36)=g_esi;g_edx+=32;MEM32(g_edx+8)=0x40210;MEM32(g_edx+12)=g_eax;
  g_eax=MEM32(g_esp+20);MEM32(g_edx+16)=frame;MEM32(g_edx+20)=g_esi;MEM32(g_edx+24)=g_ebx;MEM32(g_edx+28)=g_esi;g_edx+=32;
  MEM32(g_edx)=frame;MEM32(g_edx+4)=g_esi;MEM32(g_edx+8)=0x40214;MEM32(g_edx+12)=g_ecx;MEM32(g_edx+16)=frame;MEM32(g_edx+20)=g_esi;MEM32(g_edx+24)=g_ebx;MEM32(g_edx+28)=g_esi;g_edx+=32;
  --g_eax;MEM32(g_esp+20)=g_eax;
 }while(g_eax);
 g_ecx=g_edi;PUSH32(g_esp,0x3e319a);sub_003E7E40();
 g_ecx=MEM32(0x3f41dc)!=g_esi&&MEM32(g_edi+0x1a08)!=g_esi;MEM32(g_eax)=0x4030c;MEM32(g_eax+4)=g_ecx;
 g_ecx=MEM32(0x3f41e0)!=g_esi&&MEM32(g_edi+0x1a08)!=g_esi;MEM32(g_eax+8)=0x4032c;MEM32(g_eax+12)=g_ecx;
 g_ecx=MEM32(g_edi+8);g_eax+=16;
 if(g_ecx&0x100){g_edx=MEM32(g_esp+32);if(g_edx==MEM32(g_edi+0x1a14)){g_ecx&=~0x100u;MEM32(g_edi+8)=g_ecx;MEM32(g_eax)=0x40130;MEM32(g_eax+4)=g_esi;g_eax+=8;}}
 g_ecx=MEM32(g_esp+32);MEM32(g_edi)=g_eax;g_eax=MEM32(g_esp+36);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x3e321c);sub_003EAC30();
 g_edx=0x3f1cd8;g_ecx=g_edi;MEM32(g_edi+0x1a0c)=g_eax;PUSH32(g_esp,0x3e322e);sub_003E7F70();
 g_edx=MEM32(0x3f421c);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x3e323a);sub_003E7610();
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
