#include "migration_graphics_shader_support.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E6430(void),sub_003E9810(void),sub_003E9820(void),sub_003E7C50(void);
static int support_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_SHADER_SUPPORT");flag=v&&!strcmp(v,"1");}return flag;}
static int support_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void program_upload(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(0x3f4238);SET_LO8(g_eax,MEM8(frame+8));PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+20)-1;
 int update=(g_eax&16)==0;if(update){g_eax=MEM32(g_esp+24);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x3e6b91);sub_003E6430();}
 g_esi=MEM32(g_edi+12);g_ebx=g_esi+2;PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x3e6b9d);sub_003E9820();
 g_ecx=MEM32(g_esp+24);MEM32(g_eax)=0x41e9c;MEM32(g_eax+4)=g_ecx;g_ecx=g_esi;g_esi=g_edi+0x114;g_edi=g_eax+8;
 memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
 POP32(g_esp,g_edi);g_edx=g_eax+g_ebx*4;POP32(g_esp,g_esi);MEM32(frame)=g_edx;POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=update;g_esp+=12;
}
static void program_bind(void){
 g_eax=MEM32(g_esp+4);int bind=g_eax!=0;PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f4238);
 if(bind){
  g_ecx=MEM32(g_esi+0x794);g_ecx=MEM32(g_ecx+4);PUSH32(g_esp,g_edi);g_edi=g_eax-1;MEM32(g_esi+0x794)=g_edi;MEM32(g_esi+0x798)=g_eax;MEM32(0x3f3d98)|=0x70;g_edx=MEM32(g_edi+4)^g_ecx;
  if(g_edx&0x13){
   MEM32(0x3f3d98)|=0x1000;g_eax=MEM32(g_esi);if(g_eax>=MEM32(g_esi+4)){PUSH32(g_esp,0x3e6c1f);sub_003E9810();}
   g_edx=g_eax;g_ecx=g_esi;PUSH32(g_esp,0x3e6c28);sub_003E7C50();g_ecx=MEM32(g_edi+4);MEM32(g_eax)=0x81e94;g_ecx&=1;MEM32(g_eax+4)=6;MEM32(g_eax+8)=g_ecx;g_eax+=12;MEM32(g_esi)=g_eax;
  }
  POP32(g_esp,g_edi);
 }
 g_eax=MEM32(g_esi);if(g_eax>=MEM32(g_esi+4)){PUSH32(g_esp,0x3e6c50);sub_003E9810();}
 g_ecx=MEM32(g_esp+12);MEM32(g_eax)=0x41ea0;MEM32(g_eax+4)=g_ecx;g_eax+=8;MEM32(g_esi)=g_eax;MEM32(g_esi+0x79c)=g_ecx;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=12;
}
static void constant_fill(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(0x3f4238);g_eax=MEM32(g_esi+0x784);int emit=g_eax==0;
 if(emit){
  g_eax=MEM32(g_esi);if(g_eax>=MEM32(g_esi+4)){PUSH32(g_esp,0x3e755d);sub_003E9810();}
  g_ecx=MEM32(g_esp+8);MEM32(g_eax)=0x400a60;g_eax+=4;g_edx=16;
  do{MEM32(g_eax)=g_ecx;g_eax+=4;g_edx--;}while(g_edx);
  MEM32(g_esi)=g_eax;MEM32(0x3f41f0)=g_ecx;
 }else{g_eax=MEM32(g_esp+8);MEM32(0x3f41f0)=g_eax;}
 POP32(g_esp,g_esi);g_recomp_zf=emit;g_esp+=8;
}
static void texture_program(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_ecx+0x784);g_eax=g_edx;g_edx=g_ebx!=0;PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_ecx=0x181b68;g_esi=(g_edx<<7)+0x3f3df8;g_edx=g_eax+4;
 do{
  MEM32(g_eax)=g_ecx;g_edi=g_esi;g_ebx=g_edx;
  for(unsigned i=0;i<5;i++){frame=MEM32(g_edi+4*i);MEM32(g_ebx+4*i)=frame;}
  g_edi=MEM32(g_edi+20);g_ecx+=64;g_eax+=28;g_edx+=28;g_esi+=128;MEM32(g_ebx+20)=g_edi;
 }while(g_ecx<=0x181be8);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=g_ecx==0x181be8;g_esp+=4;
}
int uc_migration_graphics_shader_support(uint32_t entry){
 if(!support_enabled())return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10100||!support_span(s-128,144)||!support_span(0x51c178,1)||MEM8(0x51c178)||!support_span(0x3f3d98,0x600))return 0;
 if(entry==0x3e7be0){if(!support_span(g_ecx,0x788)||!support_span(g_edx,84))return 0;texture_program();return 1;}
 if(!support_span(MEM32(0x3f4238),0x7a0))return 0;
 switch(entry){case 0x3e6b70:program_upload();break;case 0x3e6bd0:program_bind();break;case 0x3e7540:constant_fill();break;default:return 0;}return 1;
}
