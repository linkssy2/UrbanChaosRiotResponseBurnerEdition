#include "migration_graphics_matrices.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void),sub_003E87C0(void),sub_003E8FE0(void),sub_003E8A40(void);
static int matrices_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MATRICES");flag=v&&!strcmp(v,"1");}return flag;}
static int matrices_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int matrix_finite_product(uint32_t a,uint32_t b){
 /* The translated scalar SSE model permits compiler-dependent NaN payloads.
  * Keep exceptional/overflow-prone products on that exact original path. */
 for(unsigned i=0;i<16;i++)if((MEM32(a+4*i)&0x7fffffffu)>0x5d800000u||(MEM32(b+4*i)&0x7fffffffu)>0x5d800000u)return 0;
 return 1;
}
static void matrix_product(void){
 g_eax=MEM32(g_esp+8);g_ecx=MEM32(g_esp+12);RecompXmm* rows[]={&g_xmm2,&g_xmm3,&g_xmm4,&g_xmm5};
 /* Preserve the left-associated SSE products and defer every store until all
  * rows are ready, so output may alias either source matrix. */
 for(unsigned i=0;i<4;i++){
  *rows[i]=XMM_MEM(g_eax+16*i);*rows[i]=XMM_SHUFFLE(*rows[i],*rows[i],0);*rows[i]=XMM_MUL(*rows[i],XMM_MEM(g_ecx));
  g_xmm1=XMM_MEM(g_eax+16*i);g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm1,0x55);g_xmm1=XMM_MUL(g_xmm1,XMM_MEM(g_ecx+16));
  g_xmm0=XMM_MEM(g_eax+16*i);g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0xaa);g_xmm0=XMM_MUL(g_xmm0,XMM_MEM(g_ecx+32));*rows[i]=XMM_ADD(*rows[i],g_xmm1);
  g_xmm1=XMM_MEM(g_eax+16*i);g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm1,0xff);g_xmm1=XMM_MUL(g_xmm1,XMM_MEM(g_ecx+48));*rows[i]=XMM_ADD(*rows[i],g_xmm0);*rows[i]=XMM_ADD(*rows[i],g_xmm1);
 }
 g_ecx=MEM32(g_esp+4);for(unsigned i=0;i<4;i++)XMM_STORE(g_ecx+16*i,*rows[i]);g_esp+=16;
}
static void matrix_packet(void){
 g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_esp+8);MEM32(g_eax)=g_ecx;g_ecx=MEM32(g_esp+12);
 for(unsigned i=0;i<15;i++){g_edx=MEM32(g_ecx+(i%4)*16+(i/4)*4);MEM32(g_eax+4+i*4)=g_edx;}
 g_ecx=MEM32(g_ecx+60);MEM32(g_eax+64)=g_ecx;g_esp+=16;
}
#define MATRIX_CALL(fn,pc) do{g_ebp=frame;PUSH32(g_esp,pc);fn();}while(0)
static void matrix_state(void){
 uint32_t frame=g_esp-4;PUSH32(g_esp,g_ebp);g_ebp=frame;g_esp=(g_esp&~15u)-0x154;g_eax=MEM32(frame+12);int skip=(int32_t)g_eax<0;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);int publish_zf=0;
 if(!skip){g_esi=MEM32(frame+8);g_eax=MEM32(g_esi+0x794);skip=(MEM8(g_eax+4)&0x12)!=0;}
 if(!skip){g_ecx=MEM32(g_esi+0x1928)&~16u;skip=g_ecx==1;}
 if(!skip){
  g_ebx=MEM32(g_esi);if(g_ebx>=MEM32(g_esi+4)){MATRIX_CALL(sub_003E9810,0x3ee39b);g_ebx=g_eax;}
  g_eax=g_esi+0xc60;PUSH32(g_esp,g_eax);g_edx=g_esi+0xde0;PUSH32(g_esp,g_edx);g_eax=g_esp+40;PUSH32(g_esp,g_eax);MATRIX_CALL(sub_003E87C0,0x3ee3b5);
  g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x400480);PUSH32(g_esp,g_ebx);MATRIX_CALL(sub_003E8FE0,0x3ee3c5);
  g_eax=MEM32(g_esi+0x950);g_ebx+=68;if(!g_eax)g_eax=MEM32(0x3f4138);
  if(g_eax){
   g_eax=MEM32(0x3f41d8);g_edx=g_eax==0;g_eax=g_esp+32;g_ecx=g_esp+0xe0;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);MATRIX_CALL(sub_003E8A40,0x3ee3fa);
   g_edi=g_ebx+4;MEM32(g_ebx)=0x300580;g_ecx=12;g_esi=g_esp+0xe0;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;g_esi=MEM32(frame+8);g_ebx+=52;
  }
  g_eax=MEM32(0x3f41c4);
  if(!g_eax){
   g_edx=g_esi+0x980;PUSH32(g_esp,g_edx);g_eax=g_esp+36;PUSH32(g_esp,g_eax);g_ecx=g_esp+0xa8;PUSH32(g_esp,g_ecx);MATRIX_CALL(sub_003E87C0,0x3ee439);
   g_edx=g_esp+0xa0;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x400680);PUSH32(g_esp,g_ebx);MATRIX_CALL(sub_003E8FE0,0x3ee44c);g_ebx+=68;MEM32(g_esi)=g_ebx;publish_zf=1;
  }else{
   g_eax=g_esi+0x980;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x400680);PUSH32(g_esp,g_ebx);MATRIX_CALL(sub_003E8FE0,0x3ee46c);g_ecx=MEM32(g_esi+4);g_eax=g_ebx+68;int grow=g_eax>=g_ecx;MEM32(g_esi)=g_eax;if(grow)MATRIX_CALL(sub_003E9810,0x3ee47d);
   g_ebx=g_eax;g_eax+=4;g_edi=0x4004c0;MEM32(g_esp+24)=g_eax;MEM32(g_esp+28)=g_edi;
   do{
    g_eax=g_esi+0xc60;PUSH32(g_esp,g_eax);g_ecx=g_esi+g_edi-4191904u;PUSH32(g_esp,g_ecx);g_edx=g_esp+0x68;PUSH32(g_esp,g_edx);MATRIX_CALL(sub_003E87C0,0x3ee4a9);
    g_eax=g_esp+0x60;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);MATRIX_CALL(sub_003E8FE0,0x3ee4b5);
    g_ecx=MEM32(g_esp+24);g_eax=MEM32(g_esi+0x950);g_ecx+=68;g_ebx+=68;MEM32(g_esp+24)=g_ecx;if(!g_eax)g_eax=MEM32(0x3f4138);
    if(g_eax){
     g_edx=MEM32(0x3f41d8);g_ecx=g_edx==0;g_edx=g_esp+0x60;g_eax=g_esp+0x120;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);MATRIX_CALL(sub_003E8A40,0x3ee4f6);
     g_eax=MEM32(g_esp+24);g_edi+=0xfff00100;MEM32(g_ebx)=g_edi;g_edi=MEM32(g_esp+24);g_ecx=12;g_esi=g_esp+0x120;g_ebx+=52;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;g_edi=MEM32(g_esp+28);g_esi=MEM32(frame+8);g_eax+=52;MEM32(g_esp+24)=g_eax;
    }
    g_edi+=64;MEM32(g_esp+28)=g_edi;
   }while(g_edi<0x400580);
   MEM32(g_esi)=g_ebx;
  }
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+4;if(publish_zf)g_recomp_zf=g_ebx==0;g_esp+=12;
}
int uc_migration_graphics_matrices(uint32_t entry){
 if(!matrices_enabled())return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10400||!matrices_span(s-1024,1044)||!matrices_span(0x51c178,1)||MEM8(0x51c178))return 0;
 switch(entry){
 case 0x3e87c0:if((_mm_getcsr()&0x1f80)!=0x1f80||!matrices_span(MEM32(s+4),64)||!matrices_span(MEM32(s+8),64)||!matrices_span(MEM32(s+12),64)||!matrix_finite_product(MEM32(s+8),MEM32(s+12)))return 0;matrix_product();break;
 case 0x3e8fe0:if(!matrices_span(MEM32(s+4),68)||!matrices_span(MEM32(s+12),64))return 0;matrix_packet();break;
 case 0x3ee350:if(!matrices_span(MEM32(s+4),0x192c)||!matrices_span(0x3f4138,0xa4))return 0;matrix_state();break;
 default:return 0;
 }return 1;
}
