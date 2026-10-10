#include "migration_graphics_shaders.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E62F0(void),sub_003E6B70(void),sub_003E6BD0(void),sub_003E6DF0(void),sub_003E7540(void),sub_003E7BE0(void),sub_003E7C50(void),sub_003E9810(void);
static int shaders_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_SHADERS");flag=v&&!strcmp(v,"1");}return flag;}
static int shaders_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void vertex_shader(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+12);int object=g_ebx&1;
 PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f4238);g_eax=MEM32(g_esi+0x794);g_ecx=MEM32(g_eax+4);frame=MEM32(g_eax+16);PUSH32(g_esp,g_edi);MEM32(g_esp+16)=g_ecx;
 if(object){
  SET_LO8(g_eax,MEM8(g_ebx+3));g_edi=g_ebx-1;
  if(g_eax&16){
   frame=0;PUSH32(g_esp,frame);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x3e6f17);sub_003E6B70();
   PUSH32(g_esp,frame);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x3e6f1e);sub_003E6BD0();
   POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=frame==0;g_esp+=8;return;
  }
 }else{PUSH32(g_esp,g_ebx);g_edi=0x3f68b8;PUSH32(g_esp,g_edi);PUSH32(g_esp,0x3e6f32);sub_003E62F0();}
 g_ecx=MEM32(0x3f3d98)|0x70;MEM32(0x3f3d98)=g_ecx;
 if(frame!=MEM32(g_edi+16)){g_eax=g_ecx|0x400;MEM32(0x3f3d98)=g_eax;}
 g_edx=MEM32(g_esp+16);if(g_edx!=MEM32(g_edi+4))MEM32(0x3f3d98)|=0x1600;
 g_eax=MEM32(g_esi);g_ecx=MEM32(g_esi+4);frame=0;int grow=g_eax>=g_ecx;
 MEM32(g_esi+0x794)=g_edi;MEM32(g_esi+0x798)=g_ebx;MEM32(g_esi+0x79c)=0;
 if(grow){PUSH32(g_esp,0x3e6f87);sub_003E9810();}
 g_edx=MEM32(g_edi+4);g_ecx=0xffffffff;
 if(!(g_edx&0x400)){MEM32(g_eax)=0x4194c;MEM32(g_eax+4)=g_ecx;g_eax+=8;}
 g_edx=MEM32(g_edi+4);if(!(g_edx&0x800)){MEM32(g_eax)=0x41950;MEM32(g_eax+4)=frame;g_eax+=8;}
 g_edx=MEM32(g_edi+4);if(!(g_edx&0x1000)){MEM32(g_eax)=0x4195c;MEM32(g_eax+4)=g_ecx;g_eax+=8;}
 g_ecx=MEM32(g_edi+4);if(!(g_ecx&0x2000)){MEM32(g_eax)=0x41960;MEM32(g_eax+4)=frame;g_eax+=8;}
 g_ecx=MEM32(g_edi+4);int z;
 if(g_ecx&2){
  g_edx=g_eax;g_ecx=g_esi;PUSH32(g_esp,0x3e6feb);sub_003E7C50();
  MEM32(g_eax)=0x41ea0;MEM32(g_eax+4)=frame;MEM32(g_eax+8)=0x81e94;MEM32(g_eax+12)=6;MEM32(g_eax+16)=frame;g_eax+=20;g_ecx=g_esi;MEM32(g_esi)=g_eax;PUSH32(g_esp,0x3e7011);sub_003E6DF0();z=g_eax==0;
  POP32(g_esp,g_edi);
 }else{
  g_ecx^=MEM32(g_esp+16);z=(g_ecx&0x12)==0;
  if(!z){MEM32(g_eax)=0x41e94;g_edx=g_eax+8;g_ecx=g_esi;MEM32(g_eax+4)=4;PUSH32(g_esp,0x3e7039);sub_003E7C50();}
  POP32(g_esp,g_edi);MEM32(g_esi)=g_eax;
 }
 POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=z;g_esp+=8;
}
static void copy_shader_words(unsigned count){
 g_ecx=count;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
}
static void pixel_shader(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ebx=MEM32(0x3f4238);g_eax=MEM32(g_ebx+0x784);MEM32(g_esp+4)=g_eax;g_eax=MEM32(g_esp+12);MEM32(g_ebx+0x784)=g_eax;
 if(!g_eax){
  g_ecx=MEM32(0x3f3d98)|0x4800;MEM32(0x3f3d98)=g_ecx;g_eax=MEM32(g_ebx+0x788);if(g_eax){g_eax=g_ecx|0x2000;MEM32(0x3f3d98)=g_eax;}
  g_ecx=MEM32(0x3f41f0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x3eece4);sub_003E7540();g_eax=MEM32(g_ebx);
  if(g_eax>=MEM32(g_ebx+4)){PUSH32(g_esp,0x3eecf0);sub_003E9810();}
  g_edx=g_eax;g_ecx=g_ebx;PUSH32(g_esp,0x3eecf9);sub_003E7BE0();MEM32(g_eax)=0x41e78;MEM32(g_eax+4)=0x210000;g_eax+=8;MEM32(g_ebx)=g_eax;
 }else{
  g_edx=MEM32(g_eax+8);g_ecx=MEM32(g_edx+0x24);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ecx|=MEM32(g_edx+0x20);
  if(!g_ecx){g_edx=MEM32(g_ebx+0x788);if(g_edx)MEM32(0x3f3d98)|=0x2000;}
  MEM32(g_ebx+0x788)=g_ecx;g_edx=MEM32(g_eax+8);g_ecx=MEM32(g_edx+0xec)&0x100;MEM32(g_ebx+0x78c)=g_ecx;
  frame=MEM32(g_eax+8);g_edx=MEM32(frame+0xd8);MEM32(g_ebx+0x790)=g_edx;MEM32(0x3f3d98)|=0x4000;g_eax=MEM32(g_ebx);
  if(g_eax>=MEM32(g_ebx+4)){PUSH32(g_esp,0x3eed71);sub_003E9810();}
  int skip_copy=MEM8(g_ebx+8)&16;PUSH32(g_esp,g_edi);
  if(!skip_copy){g_esi=frame;g_edi=0x3f3fa0;copy_shader_words(57);g_ecx=MEM32(frame+0xd8);MEM32(0x3f41c0)=g_ecx;}
  g_ecx=MEM32(g_esp+16);if(!g_ecx){g_edx=g_eax;g_ecx=g_ebx;PUSH32(g_esp,0x3eeda3);sub_003E7BE0();}
  MEM32(g_eax)=0x200260;g_edi=g_eax+4;g_esi=frame;copy_shader_words(8);
  MEM32(g_eax+0x24)=0x800a60;g_esi=frame+0x28;g_edi=g_eax+0x28;copy_shader_words(32);
  g_ecx=MEM32(frame+0xa8);MEM32(g_eax+0xa8)=0x417f8;MEM32(g_eax+0xac)=g_ecx;MEM32(g_eax+0xb0)=0x81e20;g_edx=MEM32(frame+0xac);MEM32(g_eax+0xb4)=g_edx;g_ecx=MEM32(frame+0xb0);MEM32(g_eax+0xb8)=g_ecx;
  MEM32(g_eax+0xbc)=0x241e40;g_esi=frame+0xb4;g_edi=g_eax+0xc0;copy_shader_words(9);
  MEM32(g_eax+0xe4)=0x81e74;g_edx=MEM32(frame+0xdc);MEM32(g_eax+0xe8)=g_edx;g_ecx=MEM32(frame+0xe0);MEM32(g_eax+0xec)=g_ecx;g_ecx=MEM32(g_ebx+0x788);int constants=g_ecx!=0;POP32(g_esp,g_edi);
  if(constants){MEM32(g_eax+0xf0)=0x80288;g_edx=MEM32(frame+0x20);MEM32(g_eax+0xf4)=g_edx;g_ecx=MEM32(frame+0x24);MEM32(g_eax+0xf8)=g_ecx;}
  POP32(g_esp,g_esi);g_eax+=constants?252:240;POP32(g_esp,frame);MEM32(g_ebx)=g_eax;
 }
 POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=g_eax==0;g_esp+=8;
}
int uc_migration_graphics_shaders(uint32_t entry){
 if(!shaders_enabled()||(entry!=0x3e6ee0&&entry!=0x3eec90))return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10100||!shaders_span(s-128,144)||!shaders_span(0x51c178,1)||MEM8(0x51c178)||!shaders_span(0x3f3d98,0x2b38)||!shaders_span(MEM32(0x3f4238),0x7a0))return 0;
 if(entry==0x3e6ee0)vertex_shader();else pixel_shader();return 1;
}
