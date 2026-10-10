#include "migration_graphics_upload.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void),sub_003E6EE0(void),sub_003EEC90(void);
static int upload_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_UPLOAD");flag=v&&!strcmp(v,"1");}return flag;}
static int upload_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void color_words(void){
 g_xmm1=XMM_MOVE_HIGH_TO_LOW(g_xmm0,g_xmm0);
 g_mmx[0]=MMX_CVTPS2PI(g_xmm1);g_mmx[1]=MMX_CVTPS2PI(g_xmm0);
 g_mmx[0]=g_mmx[1]=MMX_PACKSSDW(g_mmx[1],g_mmx[0]);g_esp+=4;
}
static void transpose(void){
 uint32_t frame=g_esp-4;PUSH32(g_esp,g_ebp);g_ebp=frame;
 /* Snapshot all four rows before publishing any column, including in-place use. */
 RecompXmm rows[4],columns[4];for(unsigned i=0;i<4;i++)rows[i]=XMM_MEM(g_ecx+16*i);
 g_eax=MEM32(frame+8);
 for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)columns[i].u[j]=rows[j].u[i];
 g_xmm0=columns[0];g_xmm2=columns[1];g_xmm1=columns[2];g_xmm4=columns[3];
 g_xmm3.u[0]=rows[2].u[0];g_xmm3.u[1]=rows[2].u[1];g_xmm3.u[2]=rows[3].u[0];g_xmm3.u[3]=rows[3].u[1];
 for(unsigned i=0;i<4;i++)XMM_STORE(g_eax+16*i,columns[i]);
 g_esp=frame+4;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void pack_color(void){
 uint32_t frame=g_esp-4;PUSH32(g_esp,g_ebp);g_ebp=frame;g_esp=(g_esp&~15u)-16;
 g_eax=MEM32(frame+8);g_xmm1=XMM_SCALAR(MEMF(0x4505cc));g_xmm0=XMM_MEM(g_eax);
 g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm1,0);g_xmm0=XMM_MUL(g_xmm0,g_xmm1);g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0xc6);
 PUSH32(g_esp,0x999a7);color_words();g_mmx[1]=MMX_PACKUSWB(g_mmx[0],g_mmx[0]);MMX_STORE(g_esp+8,g_mmx[1]);
 g_eax=MEM32(g_esp+8);g_esp=frame+4;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void select_shader(int pixel){
 uint32_t cache=pixel?0x51a7e0:0x51a7ec;g_eax=MEM32(cache);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);
 int unchanged=g_esi==g_eax;
 if(!unchanged){
  g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);
  uint32_t target=MEM32(g_eax+0xa4),saved=g_esp;PUSH32(g_esp,pixel?0x9c7fc:0x9c7bc);RECOMP_ICALL_SAFE(target,saved);
  g_edx=MEM32(g_eax);g_ecx=g_eax;target=MEM32(g_edx+0x1c);saved=g_esp;PUSH32(g_esp,pixel?0x9c803:0x9c7c3);RECOMP_ICALL_SAFE(target,saved);
  PUSH32(g_esp,g_esi);PUSH32(g_esp,pixel?0x9c809:0x9c7c9);
  if(pixel)sub_003EEC90();else sub_003E6EE0();
  if(pixel){g_eax=0;MEM32(0x51a7e4)=0;MEM32(cache)=g_esi;MEM32(0x51a7e8)=0;}else MEM32(cache)=g_esi;
 }
 SET_LO8(g_eax,!unchanged);POP32(g_esp,g_esi);g_recomp_zf=unchanged;g_esp+=4;
}
static void constant_vector(void){
 for(;;){
  g_eax=MEM32(0x3f4240)+28;if(g_eax<MEM32(0x3f4244))break;
  PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x3e69f5);sub_003E9810();POP32(g_esp,g_ecx);POP32(g_esp,g_edx);
 }
 MEM32(0x3f4240)=g_eax;MEM32(g_eax-28)=0x41ea4;MEM32(g_eax-24)=g_ecx;MEM32(g_eax-20)=0x100b80;
 for(unsigned i=0;i<4;i++){g_ecx=MEM32(g_edx+4*i);MEM32(g_eax-16+4*i)=g_ecx;}
 g_recomp_zf=0;g_esp+=4;
}
static void constant_array(void){
 /* Entry trace belongs to the generated entry, before native admission. */
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(0x3f4240);g_esi=g_edx;g_eax=MEM32(g_esp+12);
 if(g_edi>=MEM32(0x3f4244)){PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x3e6ac7);sub_003E9810();g_edi=g_eax;POP32(g_esp,g_ecx);POP32(g_esp,g_eax);}
 MEM32(g_edi)=0x41ea4;MEM32(g_edi+4)=g_ecx;g_edi+=8;
 int tail=g_eax<16;
 while(g_eax>=16){
  for(unsigned i=0;i<8;i++)g_mmx[i]=MMX_MEM(g_esi+8*i);
  MEM32(g_edi)=0x400b80;for(unsigned i=0;i<8;i++)MMX_STORE(g_edi+4+8*i,g_mmx[i]);
  g_edi+=68;g_eax-=16;if(!g_eax)break;g_esi+=64;
  tail=g_eax<16;if(g_eax>=16&&g_edi>=MEM32(0x3f4244)){PUSH32(g_esp,g_eax);MEM32(0x3f4240)=g_edi;PUSH32(g_esp,0x3e6abb);sub_003E9810();g_edi=g_eax;POP32(g_esp,g_eax);}
 }
 if(tail){g_edi+=4;g_ecx=g_eax;g_eax=(g_eax<<18)|0xb80;MEM32(g_edi-4)=g_eax;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;}
 MEM32(0x3f4240)=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);RECOMP_TRACE_EXIT("sub_003E6A00",0x3e6a00);g_esp+=8;
}
int uc_migration_graphics_upload(uint32_t entry){
 if(!upload_enabled())return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10100||!upload_span(s-128,144)||!upload_span(0x51c178,1)||MEM8(0x51c178))return 0;
 switch(entry){
 case 0x8e920:if(!upload_span(g_ecx,64)||!upload_span(MEM32(s+4),64))return 0;transpose();break;
 case 0x99960:color_words();break;
 case 0x99980:if(!upload_span(0x4505cc,4)||!upload_span(MEM32(s+4),16))return 0;pack_color();break;
 case 0x9c7a0:case 0x9c7e0:if(!upload_span(0x519f60,4)||!upload_span(0x51a7e0,16))return 0;select_shader(entry==0x9c7e0);break;
 case 0x3e69b0:if(!upload_span(0x3f4240,8)||!upload_span(g_edx,16)||!upload_span(MEM32(0x3f4240),28))return 0;constant_vector();break;
 case 0x3e6a00:if(!upload_span(0x3f4240,8)||!upload_span(g_edx,(uint64_t)MEM32(s+4)*4)||!upload_span(MEM32(0x3f4240),76))return 0;constant_array();break;
 default:return 0;
 }return 1;
}
