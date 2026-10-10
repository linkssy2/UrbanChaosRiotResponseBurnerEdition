#include "migration_graphics_material_init.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D589F(void),sub_002D631D(void),sub_0022B280(void),sub_00090A40(void),sub_0009D130(void),sub_0009D090(void);
static int init_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MATERIAL_INIT");flag=v&&!strcmp(v,"1");}return flag;}
static int init_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void init_indirect(uint32_t table,unsigned offset,uint32_t pc,uint32_t saved){uint32_t target=MEM32(table+offset);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
#define INIT_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
/* Preserve the stream's ordered reads and the original ownership transitions.
 * Allocator, stream and resource callbacks are completing guest services. */
int uc_migration_graphics_material_init(uint32_t entry){
 if(!init_enabled()||entry!=0x90ad0)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10200||!init_span(s-400,416)||!init_span(g_ecx,80)||!init_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t frame=g_seh_ebp,saved;g_esp-=296;saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edi);init_indirect(g_eax,164,0x90aea,saved);
 g_edx=MEM32(g_eax);g_ecx=g_eax;init_indirect(g_edx,28,0x90af1,g_esp);g_eax=MEM32(g_eax+16);frame=MEM32(g_esp+316);MEM32(0x519f84)=g_eax;g_edx=MEM32(frame);saved=g_esp;PUSH32(g_esp,14);g_eax=g_esp+32;PUSH32(g_esp,g_eax);g_ecx=frame;init_indirect(g_edx,4,0x90b0f,saved);
 SET_LO8(g_eax,MEM8(g_esp+35));g_edi=0;MEM32(g_esp+24)=g_edi;
 if(LO8(g_eax)){
  g_ecx=(uint32_t)LO8(g_eax)<<2;PUSH32(g_esp,g_ecx);INIT_CALL(sub_002D589F,0x90b2d);MEM32(g_esp+28)=g_eax;SET_LO8(g_eax,MEM8(g_esp+39));g_esp+=4;
  if(LO8(g_eax)){
   g_ebx=MEM32(g_esp+320);
   do{
    g_edx=MEM32(frame);saved=g_esp;PUSH32(g_esp,1);g_eax=g_esp+27;PUSH32(g_esp,g_eax);g_ecx=frame;init_indirect(g_edx,4,0x90b56,saved);
    g_edx=MEM32(frame);saved=g_esp;PUSH32(g_esp,2);g_eax=g_esp+20;PUSH32(g_esp,g_eax);g_ecx=frame;init_indirect(g_edx,4,0x90b65,saved);
    g_eax=MEM16(g_esp+16);g_edx=MEM32(frame);saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esp+52;PUSH32(g_esp,g_ecx);g_ecx=frame;init_indirect(g_edx,4,0x90b78,saved);
    g_edx=MEM16(g_esp+16);SET_LO8(g_eax,MEM8(g_esp+23));MEM8(g_esp+g_edx+48)=0;
    if(LO8(g_eax)){g_eax=g_esp+48;PUSH32(g_esp,g_eax);g_ecx=g_ebx;INIT_CALL(sub_0022B280,0x90b96);g_ecx=MEM32(g_esp+24);if(g_eax)g_eax-=4;else g_eax=0;MEM32(g_ecx+g_edi*4)=g_eax;}
    else{
     g_edx=MEM32(g_esi+56);g_ecx=g_esi+52;int resize=!g_edx;if(g_edx){g_eax=MEM32(g_ecx+8);g_eax=(uint32_t)((int32_t)(g_eax-g_edx)>>2);resize=g_eax<=g_edi;}
     if(resize){PUSH32(g_esp,0);g_edx=g_edi+1;PUSH32(g_esp,g_edx);INIT_CALL(sub_00090A40,0x90bd2);}
     if(!(MEM8(0x519fb0)&1)){g_eax=MEM32(0x519fb0);g_xmm0=XMM_SCALAR(MEMF(0x44c1f8));g_eax|=1;MEM32(0x519fb0)=g_eax;for(unsigned i=0;i<4;i++)MEMF(0x519fa0+i*4)=g_xmm0.f[0];}
     g_eax=MEM32(g_esi+56);MEM32(g_eax+g_edi*4)=0x519fa0;
    }
    g_ecx=MEM8(g_esp+35);g_edi++;
   }while(g_edi<g_ecx);
  }
 }
 g_edx=MEM8(g_esp+34);g_eax=MEM8(g_esp+38);g_xmm0=XMM_SCALAR(MEMF(0x44c23c));SET_LO16(g_ecx,MEM16(g_esp+32));g_xmm1.f[0]=(float)(int32_t)g_edx;g_edx=MEM8(g_esp+40);g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esi+68)=g_xmm1.f[0];g_xmm1.f[0]=(float)(int32_t)g_eax;g_eax=MEM16(g_esp+28);g_xmm1.f[0]*=g_xmm0.f[0];PUSH32(g_esp,g_eax);MEMF(g_esi+72)=g_xmm1.f[0];MEM16(g_esi+76)=LO16(g_ecx);MEM32(g_esi+16)=g_edx;MEM32(g_esi+8)=g_eax;INIT_CALL(sub_002D589F,0x90c73);
 g_ecx=MEM32(g_esi+8);g_esp+=4;saved=g_esp;PUSH32(g_esp,g_ecx);MEM32(g_esi+20)=g_eax;g_edx=MEM32(frame);PUSH32(g_esp,g_eax);g_ecx=frame;init_indirect(g_edx,4,0x90c86,saved);
 g_eax=MEM16(g_esp+30);PUSH32(g_esp,g_eax);MEM32(g_esi+12)=g_eax;INIT_CALL(sub_002D589F,0x90c94);g_ecx=MEM32(g_esi+12);g_esp+=4;saved=g_esp;PUSH32(g_esp,g_ecx);MEM32(g_esi+24)=g_eax;g_edx=MEM32(frame);PUSH32(g_esp,g_eax);g_ecx=frame;init_indirect(g_edx,4,0x90ca7,saved);
 g_edi=MEM8(g_esp+39);g_edx=g_edi<<5;PUSH32(g_esp,g_edx);MEM32(g_esi+28)=g_edi;INIT_CALL(sub_002D589F,0x90cba);g_esp+=4;
 if(g_eax){g_edi--;if((int32_t)g_edi>=0){g_ecx=g_eax+16;g_edx=g_edi+1;do{g_edi=0;g_ebx=g_ecx;MEM32(g_ebx)=g_edi;MEM32(g_ebx+4)=g_edi;MEM32(g_ebx+8)=g_edi;g_ecx+=32;g_edx--;MEM32(g_ebx+12)=g_edi;}while(g_edx);}}
 else g_eax=0;
 MEM32(g_esi+32)=g_eax;SET_LO8(g_eax,MEM8(g_esp+39));g_edi=0;MEM32(g_esp+44)=g_edi;
 if(LO8(g_eax))do{
  g_ecx=MEM32(g_esi+32);g_eax=MEM32(frame);g_ecx+=g_edi;saved=g_esp;PUSH32(g_esp,16);PUSH32(g_esp,g_ecx);g_ecx=frame;init_indirect(g_eax,4,0x90d10,saved);
  g_edx=MEM32(g_esi+32);SET_LO8(g_ecx,MEM8(g_edi+g_edx));g_eax=0;
  if(LO8(g_ecx)){g_ecx=g_edi+16;MEM32(g_esp+16)=g_ecx;do{g_ecx=MEM32(g_esi+32);g_ebx=MEM32(g_esp+24);g_edx=g_edi+g_ecx;g_edx=MEM8(g_edx+g_eax+1);g_ebx=MEM32(g_ebx+g_edx*4);g_edx=MEM32(g_esp+16);MEM32(g_edx+g_ecx)=g_ebx;g_ecx=MEM32(g_esi+32);g_edx+=4;MEM32(g_esp+16)=g_edx;g_edx=MEM8(g_edi+g_ecx);g_eax++;}while((int32_t)g_eax<(int32_t)g_edx);}
  g_eax=MEM32(g_esi+32);g_ecx=MEM16(g_edi+g_eax+8);PUSH32(g_esp,g_ecx);INIT_CALL(sub_002D589F,0x90d5d);g_edx=MEM32(g_esi+32);g_ebx=g_eax;g_eax=MEM16(g_edi+g_edx+10);PUSH32(g_esp,g_eax);INIT_CALL(sub_002D589F,0x90d6d);
  g_edx=MEM32(frame);MEM32(g_esp+24)=g_eax;g_eax=MEM32(g_esi+32);g_ecx=MEM16(g_edi+g_eax+8);g_esp+=8;saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ecx=frame;init_indirect(g_edx,4,0x90d86,saved);
  g_eax=MEM32(g_esi+32);g_ecx=MEM16(g_edi+g_eax+10);g_eax=MEM32(g_esp+16);g_edx=MEM32(frame);saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_ecx=frame;init_indirect(g_edx,4,0x90d9c,saved);
  g_ecx=MEM32(g_esp+16);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_ecx);INIT_CALL(sub_0009D130,0x90da7);g_edx=MEM32(g_esi+32);MEM32(g_edi+g_edx+8)=g_eax;g_eax=MEM32(g_esp+24);PUSH32(g_esp,g_eax);INIT_CALL(sub_002D631D,0x90db8);PUSH32(g_esp,g_ebx);INIT_CALL(sub_002D631D,0x90dbe);
  g_ecx=MEM32(g_esi+32);g_edx=MEM16(g_edi+g_ecx+12);PUSH32(g_esp,g_edx);INIT_CALL(sub_002D589F,0x90dcc);g_ecx=MEM32(g_esi+32);g_edx=MEM16(g_edi+g_ecx+12);g_esp+=20;g_ebx=g_eax;g_eax=MEM32(frame);saved=g_esp;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ebx);g_ecx=frame;init_indirect(g_eax,4,0x90de3,saved);
  g_eax=g_ebx+4;PUSH32(g_esp,g_eax);INIT_CALL(sub_0009D090,0x90dec);g_ecx=MEM32(g_esi+32);PUSH32(g_esp,g_ebx);MEM32(g_edi+g_ecx+12)=g_eax;INIT_CALL(sub_002D631D,0x90df9);g_eax=MEM32(g_esp+52);g_edx=MEM8(g_esp+47);g_esp+=8;g_eax++;g_edi+=32;MEM32(g_esp+44)=g_eax;
 }while((int32_t)g_eax<(int32_t)g_edx);
 SET_LO8(g_eax,MEM8(g_esp+36));g_ecx=MEM32(g_esp+24);if(LO8(g_eax)!=255){g_eax=LO8(g_eax);g_edx=MEM32(g_ecx+g_eax*4);MEM32(g_esi+40)=g_edx;MEM8(g_esi+36)=1;}else MEM8(g_esi+36)=0;
 SET_LO8(g_eax,MEM8(g_esp+37));if(LO8(g_eax)!=255){g_eax=LO8(g_eax);g_edx=MEM32(g_ecx+g_eax*4);MEM32(g_esi+48)=g_edx;MEM8(g_esi+44)=1;}else MEM8(g_esi+44)=0;
 g_eax=MEM16(g_esi+76);if((LO8(g_eax)&1)&&!(LO8(g_eax)&0x60)){g_eax|=0x142;MEM16(g_esi+76)=LO16(g_eax);}g_eax=MEM16(g_esi+76);if((LO8(g_eax)&2)&&(LO8(g_eax)&1)){g_eax|=0x140;MEM16(g_esi+76)=LO16(g_eax);}
 PUSH32(g_esp,g_ecx);INIT_CALL(sub_002D631D,0x90e83);g_esp+=4;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=296;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
#undef INIT_CALL
