#include "migration_graphics_samplers.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void),sub_003EAA50(void);
static int samplers_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_SAMPLERS");flag=v&&!strcmp(v,"1");}return flag;}
static int samplers_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_graphics_samplers(uint32_t entry){
 if(!samplers_enabled()||entry!=0x3ed480)return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10400||!samplers_span(s-1024,1044)||(unsigned)g_fp_top>7||!samplers_span(0x51c178,1)||MEM8(0x51c178)||!samplers_span(0x3f3da0,0x600)||!samplers_span(MEM32(s+4),0xfa0))return 0;
 uint32_t frame=g_seh_ebp;g_esp-=24;g_ecx=MEM32(g_esp+28);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ecx+=0x774;PUSH32(g_esp,g_edi);MEM32(g_esp+20)=0x41b14;MEM32(g_esp+24)=g_ecx;g_esi=0x3f3da4;MEM32(g_esp+28)=4;
 do{
  if(MEM8(g_esp+48)&1){
   g_eax=MEM32(g_esi+4);frame=MEM32(g_esi);g_edx=MEM32(g_esi+0x6c);g_ebx=MEM32(g_esi+32);g_eax=(g_eax<<8)|frame;frame=MEM32(g_esi+40);g_edx&=0xffff;g_edi=MEM32(0x3f4128+g_edx*4);g_edx=MEM32(g_esi-4);g_eax=(g_eax<<8)|g_edi;g_edi=MEM32(g_esi+24)<<26;g_eax|=g_edx;g_edi|=frame;g_edi|=g_ebx;MEM32(g_esp+36)=g_eax;g_eax=MEM32(g_ecx+0x814);g_edi|=0x3ffc0;if(g_eax)g_edi|=0x40000000;
   g_ebx=MEM32(g_esi+12);frame=MEM32(g_esi+8);MEM32(g_esp+16)=0x2000;
   if(g_ebx>=3||frame>=3){
    if(g_ebx<=3&&frame<=3){g_eax=MEM32(g_esi+28);if(!g_eax){g_ebx=1;g_eax<<=4;frame=g_ebx;g_edi|=g_eax;}else{g_eax--;g_ebx=2;g_eax<<=4;frame=g_ebx;g_edi|=g_eax;}}
    else{if(g_ebx==5||frame==5)MEM32(g_esp+16)=0x4000;MEM32(g_esp+16)|=0x70000;g_ebx=2;frame=4;}
   }
   g_edx=MEM32(g_esp+44);g_eax=g_edi|0x40000000;MEM32(g_ecx)=g_eax;g_ecx=MEM32(g_esi+20);MEM32(g_esp+32)=g_ecx;
   double value=MEMF(g_esp+32);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value;PUSH32(g_esp,g_ecx);g_fp_stack[g_fp_top]+=MEMF(g_edx+0x968);g_fp_stack[g_fp_top]*=MEMF(0x453a28);g_fp_stack[g_fp_top]+=MEMF(0x44c1f8);MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1u)&7u;PUSH32(g_esp,0x3ed5a6);sub_003EAA50();
   if((int32_t)g_eax<-4096)g_eax=0xfffff000;else if((int32_t)g_eax>4095)g_eax=4095;
   g_ecx=0xc0000000;if(MEM32(g_esi+44)<25)g_ecx=MEM32(g_esi+36);
   g_edx=g_ebx*3;g_ebx=MEM32(g_esi+16)+g_edx;g_edx=MEM32(0x3f1fc8+g_ebx*4);g_ebx=MEM32(g_esp+44);g_eax=(g_eax&0x1fff)|g_edx;g_edx=MEM32(g_esp+16);frame<<=24;g_eax|=frame;g_eax|=g_ecx;g_ecx=MEM32(g_ebx+4);g_eax|=g_edx;frame=g_eax;g_eax=MEM32(g_ebx);
   if(g_eax>=g_ecx){PUSH32(g_esp,0x3ed605);sub_003E9810();}
   g_ecx=MEM32(g_esp+20);g_edx=g_ecx+0x3fff4;MEM32(g_eax)=g_edx;g_edx=MEM32(g_esp+36);MEM32(g_eax+4)=g_edx;MEM32(g_eax+8)=g_edi;MEM32(g_eax+12)=g_ecx;g_ecx=MEM32(g_esp+24);MEM32(g_eax+16)=frame;g_eax+=20;MEM32(g_ebx)=g_eax;
  }
  g_edi=MEM32(g_esp+20);g_edx=MEM32(g_esp+48);g_eax=MEM32(g_esp+28);g_ecx+=4;g_edi+=64;g_edx>>=1;g_esi+=128;g_eax--;MEM32(g_esp+24)=g_ecx;MEM32(g_esp+20)=g_edi;MEM32(g_esp+48)=g_edx;MEM32(g_esp+28)=g_eax;
 }while(g_eax);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
