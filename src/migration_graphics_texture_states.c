#include "migration_graphics_texture_states.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void);
static int texture_state_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_TEXTURE_STATES");flag=v&&!strcmp(v,"1");}return flag;}
static int texture_state_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void texture_state_grow(uint32_t pc){PUSH32(esp,pc);sub_003E9810();}
int uc_migration_graphics_texture_states(uint32_t entry){
 if(!texture_state_enabled()||(entry!=0x3e78f0&&entry!=0x3e7a00&&entry!=0x3e7a70&&entry!=0x3e7ab0))return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!texture_state_span(s-128,144)||!texture_state_span(0x51c178,1)||MEM8(0x51c178)||MEM32(s+4)>3)return 0;
 uint32_t device=MEM32(0x3f4238);if(!texture_state_span(device,0x954)||!texture_state_span(MEM32(device),32)||(entry==0x3e7a00&&MEM32(s+8)>31))return 0;
 uint32_t frame=g_seh_ebp;
 if(entry==0x3e78f0){
  PUSH32(esp,ebx);ebx=MEM32(esp+12);PUSH32(esp,frame);PUSH32(esp,esi);esi=MEM32(0x3f4238);PUSH32(esp,edi);edi=MEM32(esp+20);eax=edi<<7;MEM32(eax+0x3f3e10)=ebx;eax=MEM32(esi);ecx=MEM32(esi+4);edx=0;frame=0;MEM32(esp+20)=edx;MEM32(esp+24)=ebx;
  if(eax>=ecx){texture_state_grow(0x3e7927);edx=MEM32(esp+20);}
  ebx&=0xffff0000;ecx=1;
  if(ebx){frame=edi*4+0x41964;MEM32(eax)=frame;MEM32(eax+4)=0xff000000;eax+=8;MEM32(esp+24)=edi;
   if(ebx>0x30000)frame=0x2401;else if(ebx==0x30000){MEM32(esp+20)=ecx;frame=0x8512;edx=ecx;}else if(ebx==0x10000){MEM32(esp+20)=ecx;frame=0x8511;edx=ecx;}else frame=0x2400;
  }
  SET_LO8(ebx,MEM8(esp+24));SET_LO8(ebx,LO8(ebx)+9);MEM8(edi+0x4eaa61)=LO8(ebx);ebx=(edi+0xc03c)<<4;MEM32(eax)=ebx;MEM32(eax+4)=frame;MEM32(eax+8)=frame;MEM32(eax+12)=frame;eax+=16;MEM32(esi)=eax;eax=MEM32(esi+0x950);if(!eax&&edx)MEM32(0x3f3d98)|=0x200;
  ebx=MEM32(esi+0x950);eax=ecx;ecx=edi;eax<<=LO8(ecx);edx<<=LO8(ecx);POP32(esp,edi);eax=(~eax&ebx)|edx;MEM32(esi+0x950)=eax;eax=MEM32(0x3f3d98);POP32(esp,esi);eax|=0x47f;POP32(esp,frame);MEM32(0x3f3d98)=eax;POP32(esp,ebx);g_recomp_zf=eax==0;esp+=12;return 1;
 }
 if(entry==0x3e7a00){PUSH32(esp,ebx);PUSH32(esp,frame);PUSH32(esp,esi);esi=MEM32(esp+16);PUSH32(esp,edi);edi=MEM32(0x3f4238);eax=MEM32(edi+0x784);ebx=esi;if(!eax)ebx=esi+1;frame=MEM32(esp+28);
  if(LO8(ebx)&3){eax=MEM32(edi);if(eax>=MEM32(edi+4))texture_state_grow(0x3e7a32);ecx=MEM32(esp+24);ebx=(ebx<<4)+ecx;ecx=ebx*4+0x41ad0;MEM32(eax)=ecx;MEM32(eax+4)=frame;eax+=8;MEM32(edi)=eax;}
  eax=MEM32(esp+24);esi=(esi<<5)+eax;POP32(esp,edi);MEM32(esi*4+0x3f3da0)=frame;POP32(esp,esi);POP32(esp,frame);POP32(esp,ebx);g_recomp_zf=esi==0;esp+=16;return 1;
 }
 PUSH32(esp,esi);esi=MEM32(0x3f4238);eax=MEM32(esi);if(eax>=MEM32(esi+4))texture_state_grow(entry+19);ecx=MEM32(esp+8);edx=entry==0x3e7a70?(ecx<<6)+0x41b24:ecx*4+0x40ae0;MEM32(eax)=edx;edx=MEM32(esp+12);MEM32(eax+4)=edx;eax+=8;MEM32(esi)=eax;ecx<<=7;MEM32(ecx+(entry==0x3e7a70?0x3f3e14:0x3f3e18))=edx;POP32(esp,esi);g_recomp_zf=ecx==0;esp+=12;return 1;
}
