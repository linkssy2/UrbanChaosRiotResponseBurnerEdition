#include "hybrid_texture_bind_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E5450(void),sub_003E9810(void);
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_TEXTURE_BIND");flag=v&&!strcmp(v,"1");}return flag;}
static int span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* Complete 3E3840 ownership and packet order. Retirement and buffer growth
 * remain completing services. In particular, rebinding the same texture is
 * not a no-op: the source releases and then acquires the live binding. */
int uc_hybrid_texture_bind(void){
 if(!enabled())return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10100u||!span(s-128,140)||!span(0x3f3d98,4)||!span(0x3f4238,4)||!span(0x51c178,1)||MEM8(0x51c178)||!apart(s-128,140,0x51c178,1)||!apart(s-128,140,0x3f4238,4))return 0;
 uint32_t device=MEM32(0x3f4238),stage=MEM32(s+4),next=MEM32(s+8);
 if(stage>=4||!span(device,0xf98)||!apart(device,0xf98,s-128,140))return 0;
 uint32_t previous=MEM32(device+0xf88+stage*4),packet=MEM32(device);
 if(!span(packet,36)||!apart(packet,36,s-128,140)||(previous&&(!span(previous,12)||!apart(previous,12,s-128,140)))||(next&&(!span(next,20)||!apart(next,20,s-128,140))))return 0;
 uint32_t frame=g_seh_ebp;
 g_esp-=8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(0x3f4238);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+24);g_eax=MEM32(frame+g_esi*4+0xf88);
 MEM32(g_esp+12)=frame;MEM32(g_esp+16)=g_eax;
 if(g_eax){
  g_ecx=MEM32(frame+44);g_edx=MEM32(g_eax);g_edx-=0x80000;MEM32(g_eax+8)=g_ecx;g_ecx=g_edx;MEM32(g_eax)=g_edx;
  if(!(g_ecx&0x78ffff)){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x3e3883);sub_003E5450();}
 }
 g_ebx=MEM32(g_esp+28);MEM32(frame+g_esi*4+0xf88)=g_ebx;
 if(!g_ebx){
  g_eax=MEM32(frame);if(g_eax>=MEM32(frame+4)){PUSH32(g_esp,0x3e389f);sub_003E9810();}
  g_edx=(g_esi<<6)+0x41b0c;MEM32(g_eax)=g_edx;MEM32(g_eax+4)=0;g_eax+=8;MEM32(frame)=g_eax;MEM32(frame+g_esi*4+12)=0x80000000;
  g_eax=MEM32(0x3f3d98);POP32(g_esp,g_esi);g_eax|=0x4800;POP32(g_esp,frame);MEM32(0x3f3d98)=g_eax;POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
 }
 MEM32(g_ebx)+=0x80000;g_eax=MEM32(frame);if(g_eax>=MEM32(frame+4)){PUSH32(g_esp,0x3e38ec);sub_003E9810();}
 g_ecx=MEM32(g_ebx+12);g_edx=MEM32(g_ebx+4);PUSH32(g_esp,g_edi);g_edi=(g_esi+0x206c)<<6;MEM32(g_eax)=g_edi;MEM32(g_eax+4)=g_edx;MEM32(g_eax+8)=g_ecx;g_ecx=MEM32(g_ebx+16);g_edi=g_eax+12;
 if(g_ecx){
  g_edx=MEM8(g_ebx+19);g_eax=g_ecx;g_ecx>>=12;g_eax&=0xfff;g_ecx&=0xfff;++g_eax;++g_ecx;MEM32(g_esp+28)=g_ecx;++g_edx;g_ecx=g_esi<<6;frame=g_ecx+0x41b10;g_edx<<=6;MEM32(g_edi)=frame;frame=MEM32(g_esp+16);
  g_edx<<=16;g_ecx+=0x41b1c;MEM32(g_edi+4)=g_edx;MEM32(g_edi+8)=g_ecx;g_ecx=MEM32(g_esp+28);g_eax<<=16;g_eax|=g_ecx;MEM32(g_edi+12)=g_eax;g_edi+=16;
 }
 g_ebx=MEM32(g_ebx+12);g_ecx=MEM32(frame+g_esi*4+12);g_eax=g_ebx&0x20f4;
 if(g_ecx!=g_eax){
  if(g_eax&0x2000){g_ebx&=0xff00;g_eax&=~0x2000u;if(g_ebx>=0x2a00&&g_ebx<=0x3100)g_eax|=0x40000000;}
  MEM32(frame+g_esi*4+12)=g_eax;g_ecx=MEM32(0x3f3d98);g_eax=MEM32(g_esp+20);g_ecx|=0x4000;MEM32(0x3f3d98)=g_ecx;
  if(!g_eax){g_eax=MEM32(frame+g_esi*4+0x774);g_esi=(g_esi<<6)+0x41b0c;MEM32(g_edi)=g_esi;MEM32(g_edi+4)=g_eax;g_eax=MEM32(0x3f3d98);g_edi+=8;g_eax|=0x800;MEM32(0x3f3d98)=g_eax;}
 }
 MEM32(frame)=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
