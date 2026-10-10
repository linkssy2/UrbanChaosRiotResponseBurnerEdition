#include "migration_graphics_texture_matrices.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void);
static int texmat_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_TEXTURE_MATRICES");flag=v&&!strcmp(v,"1");}return flag;}
static int texmat_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void texmat_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void texmat_store(uint32_t p){MEMF(p)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
static unsigned texmat_kind(uint32_t code){
 if(code>=0x320)return code==0x320?4:code==0x330?5:code==0x331?6:7;
 uint32_t target=MEM32(0x3edbd0+4*MEM8(0x3ed9c4+code));
 switch(target){case 0x3ed957:return 0;case 0x3ed992:return 1;case 0x3ed9d0:return 2;case 0x3eda20:return 3;default:return 8;}
}
static void texmat_row(unsigned row,unsigned source,int projected){
 uint32_t out=g_eax+12+16*row,source_row=g_esi+4*source;
 if(projected)texmat_push(MEMF(source_row+4));
 texmat_push(MEMF(source_row-12));g_edx=MEM32(source_row-44);texmat_push(MEMF(source_row-28));MEM32(out)=g_edx;texmat_store(out+4);
 if(projected)texmat_store(out+8);else MEM32(out+8)=g_ebx;
 texmat_store(out+12);
}
static void texmat_packet(unsigned kind){
 int projected=kind>=4;unsigned shape=kind&3;
 texmat_row(0,0,projected);texmat_row(1,1,projected);
 if(shape==0||shape==2)for(unsigned i=0;i<4;i++)MEM32(g_eax+44+4*i)=g_ebx;
 else texmat_row(2,2,projected);
 if(shape==2)texmat_row(3,2,projected);
 else if(shape==3)texmat_row(3,3,projected);
 else{for(unsigned i=0;i<3;i++)MEM32(g_eax+60+4*i)=g_ebx;MEM32(g_eax+72)=0x3f800000;}
}
int uc_migration_graphics_texture_matrices(uint32_t entry){
 if(!texmat_enabled()||entry!=0x3ed850)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!texmat_span(s-64,76)||!texmat_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
 uint32_t device=MEM32(s+4);if(!texmat_span(device,0x192c)||!texmat_span(MEM32(device+0x794),20))return 0;
 /* Admitted state has a known original jump-table destination. No guessed
  * replacement for invalid indirect targets or overlapping command storage. */
 for(unsigned stage=0;stage<4;stage++){
  uint32_t state=0x3f3e10+stage*128,flags=MEM32(state-28),index=MEM32(state),dimensions=index>>16?3:(MEM32(MEM32(device+0x794)+16)>>((index*8)&31))&255;if(!dimensions)dimensions=2;
  uint32_t code=((dimensions<<4)|(flags&255))<<4|((flags>>8)&1);if(flags&&texmat_kind(code)>7)return 0;
 }
 uint32_t output=MEM32(device);if(!texmat_span(output,304)||!((uint64_t)output+304<=device||output>=(uint64_t)device+0x192c))return 0;
 uint32_t frame=g_seh_ebp;g_ecx=device;g_eax=MEM32(g_ecx+0x794);SET_LO8(g_edx,MEM8(g_eax+4));g_esp-=12;
 int skip=(g_edx&0x12)!=0;if(!skip){g_edx=MEM32(g_ecx+0x1928)&~16u;skip=g_edx==1;}
 if(!skip){
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);g_eax=0x3ff9b4;PUSH32(g_esp,g_esi);g_eax-=g_ecx;frame=0x40420;MEM32(g_esp+12)=0x3f3e10;g_esi=g_ecx+0xd0c;MEM32(g_esp+16)=g_eax;g_ebx=0;PUSH32(g_esp,g_edi);
  do{
   g_eax=MEM32(g_esp+16);g_edi=MEM32(g_eax-28);g_eax=MEM32(g_ecx);if(g_eax>=MEM32(g_ecx+4)){PUSH32(g_esp,0x3ed8b3);sub_003E9810();g_ecx=MEM32(g_esp+32);}
   if(g_edi==g_ebx){MEM32(g_eax)=frame;MEM32(g_eax+4)=g_ebx;g_eax+=8;MEM32(g_ecx)=g_eax;}
   else{
    g_ecx=MEM32(MEM32(g_esp+16));g_edx=g_ecx&0xffff0000;MEM32(g_esp+24)=g_edx;
    if(g_edx)g_edx=3;else{g_edx=MEM32(g_esp+32);g_edx=MEM32(g_edx+0x794);g_edx=MEM32(g_edx+16);g_ecx=(g_ecx&65535)<<3;g_edx=(g_edx>>(g_ecx&31))&255;if(!g_edx)g_edx=2;}
    g_ecx=g_edi&255;g_edx=(g_edx<<4)|g_ecx;g_ecx=MEM32(g_esp+20);g_edi=(g_edi>>8)&1;g_edx=(g_edx<<4)|g_edi;g_ecx+=g_esi;MEM32(g_eax)=frame;MEM32(g_eax+4)=1;MEM32(g_eax+8)=g_ecx;g_ecx=g_eax+76;
    unsigned kind=texmat_kind(g_edx);
    if(kind>7){g_edx=MEM8(0x3ed9c4+g_edx);g_seh_ebp=frame;RECOMP_ITAIL(MEM32(0x3edbd0+4*g_edx));return 1;}
    texmat_packet(kind);g_eax=MEM32(g_esp+32);MEM32(g_eax)=g_ecx;g_ecx=g_eax;
   }
   g_edi=MEM32(g_esp+16);frame+=4;g_edi+=128;g_esi+=64;MEM32(g_esp+16)=g_edi;
  }while(frame<0x40430);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);
 }
 g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
