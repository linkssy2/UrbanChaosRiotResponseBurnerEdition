/* Original rigid-body -> model hierarchy writeback, including unmapped bones. */
#include "migration_physics_ragdoll_writeback.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002D4360(void);void sub_002B4D80(void);
#define SPLAT(x,i) XMM_SHUFFLE(x,x,i)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-768||(uint64_t)g_esp+32<=p);}
static void local_axis(void){
 g_xmm0=XMM_MEM(0x51da80);g_xmm2=SPLAT(g_xmm0,255);g_xmm1=XMM_MEM(g_esp+0x150);g_xmm3=XMM_MUL(g_xmm1,g_xmm2);g_xmm4=SPLAT(g_xmm0,170);g_xmm2=XMM_MEM(g_esp+0x140);g_xmm5=XMM_ADD(XMM_MUL(g_xmm2,g_xmm4),g_xmm3);g_xmm4=SPLAT(g_xmm0,85);g_xmm3=XMM_MEM(g_esp+0x130);g_xmm6=XMM_MUL(g_xmm3,g_xmm4);g_xmm0=SPLAT(g_xmm0,0);g_xmm4=XMM_MEM(g_esp+0x120);g_xmm7=XMM_MUL(g_xmm4,g_xmm0);g_xmm0=XMM_ADD(XMM_ADD(g_xmm7,g_xmm6),g_xmm5);XMM_STORE(g_esp+0xe0,g_xmm0);
 for(unsigned row=1;row<3;row++){g_xmm0=XMM_MEM(0x51da80+row*16);g_xmm5=SPLAT(g_xmm0,255);g_xmm6=XMM_MUL(g_xmm1,g_xmm5);g_xmm5=SPLAT(g_xmm0,170);g_xmm7=XMM_ADD(XMM_MUL(g_xmm2,g_xmm5),g_xmm6);g_xmm5=SPLAT(g_xmm0,85);g_xmm6=XMM_MUL(g_xmm3,g_xmm5);g_xmm5=SPLAT(g_xmm0,0);g_xmm0=XMM_ADD(XMM_ADD(XMM_MUL(g_xmm4,g_xmm5),g_xmm6),g_xmm7);XMM_STORE(g_esp+0xe0+row*16,g_xmm0);}
 g_ecx=MEM32(g_esi+32);g_eax=MEM32(g_ecx);g_xmm0=XMM_MEM(0x51dab0);g_xmm5=SPLAT(g_xmm0,255);g_xmm1=XMM_MUL(g_xmm1,g_xmm5);g_xmm5=SPLAT(g_xmm0,170);g_xmm2=XMM_ADD(XMM_MUL(g_xmm2,g_xmm5),g_xmm1);g_xmm1=SPLAT(g_xmm0,85);g_xmm3=XMM_MUL(g_xmm3,g_xmm1);g_xmm1=SPLAT(g_xmm0,0);g_xmm4=XMM_ADD(XMM_MUL(g_xmm4,g_xmm1),g_xmm3);g_xmm0=XMM_ADD(g_xmm4,g_xmm2);XMM_STORE(g_esp+0x110,g_xmm0);
}
static void physical_product(void){
 g_xmm0=XMM_MEM(g_esp+32);g_xmm3=XMM_MEM(g_esp+0x110);g_xmm5=XMM_MEM(g_esp+0x100);g_xmm1=SPLAT(g_xmm0,255);g_xmm2=XMM_MUL(g_xmm3,g_xmm1);g_xmm1=SPLAT(g_xmm0,170);g_xmm4=XMM_ADD(XMM_MUL(g_xmm5,g_xmm1),g_xmm2);g_xmm2=g_xmm4;g_xmm1=SPLAT(g_xmm0,85);g_xmm1=XMM_MUL(XMM_MEM(g_esp+0xf0),g_xmm1);g_xmm0=SPLAT(g_xmm0,0);g_xmm4=XMM_MEM(g_esp+0xe0);g_xmm6=XMM_MUL(g_xmm4,g_xmm0);g_xmm1=XMM_ADD(g_xmm6,g_xmm1);g_xmm0=XMM_MEM(g_esp+48);g_xmm1=XMM_ADD(g_xmm1,g_xmm2);
 g_xmm2=SPLAT(g_xmm0,255);g_xmm6=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=SPLAT(g_xmm0,170);g_xmm7=XMM_ADD(XMM_MUL(g_xmm5,g_xmm2),g_xmm6);g_xmm2=SPLAT(g_xmm0,85);g_xmm6=XMM_MUL(XMM_MEM(g_esp+0xf0),g_xmm2);g_xmm2=SPLAT(g_xmm0,0);g_xmm2=XMM_MUL(g_xmm4,g_xmm2);g_xmm0=XMM_MEM(g_esp+64);g_xmm2=XMM_ADD(g_xmm2,g_xmm6);g_xmm6=SPLAT(g_xmm0,255);g_xmm3=XMM_MUL(g_xmm3,g_xmm6);g_xmm2=XMM_ADD(g_xmm2,g_xmm7);
 g_xmm6=SPLAT(g_xmm0,170);g_ecx=MEM32(g_esi+32);g_xmm7=XMM_ADD(XMM_MUL(g_xmm5,g_xmm6),g_xmm3);g_xmm3=SPLAT(g_xmm0,85);g_xmm6=XMM_MUL(XMM_MEM(g_esp+0xf0),g_xmm3);g_xmm3=SPLAT(g_xmm0,0);g_xmm3=XMM_MUL(g_xmm4,g_xmm3);g_xmm0=XMM_MEM(g_esp+80);g_xmm3=XMM_ADD(XMM_ADD(g_xmm3,g_xmm6),g_xmm7);
 g_xmm6=SPLAT(g_xmm0,255);g_xmm7=XMM_MUL(XMM_MEM(g_esp+0x110),g_xmm6);g_xmm6=SPLAT(g_xmm0,170);g_xmm5=XMM_MUL(g_xmm5,g_xmm6);g_xmm6=SPLAT(g_xmm0,85);g_xmm5=XMM_ADD(g_xmm5,g_xmm7);g_xmm7=XMM_MUL(XMM_MEM(g_esp+0xf0),g_xmm6);g_xmm6=SPLAT(g_xmm0,0);g_xmm4=XMM_MUL(g_xmm4,g_xmm6);g_xmm0=XMM_ADD(XMM_ADD(g_xmm4,g_xmm7),g_xmm5);
}
static void hierarchy_product(void){
 g_xmm0=XMM_MEM(g_eax);g_xmm1=XMM_MEM(g_eax+16);g_xmm4=SPLAT(g_xmm0,255);g_xmm5=XMM_MUL(XMM_MEM(g_esp+0xd0),g_xmm4);g_xmm2=XMM_MEM(g_eax+32);g_xmm4=SPLAT(g_xmm0,170);g_xmm6=XMM_ADD(XMM_MUL(XMM_MEM(g_esp+0xc0),g_xmm4),g_xmm5);g_xmm4=SPLAT(g_xmm0,85);g_xmm5=XMM_MUL(XMM_MEM(g_esp+0xb0),g_xmm4);g_xmm4=SPLAT(g_xmm0,0);g_xmm0=XMM_MEM(g_esp+0xa0);g_xmm7=XMM_MUL(g_xmm0,g_xmm4);g_xmm4=XMM_ADD(g_xmm7,g_xmm5);g_xmm5=XMM_MEM(g_esp+0xd0);g_xmm4=XMM_ADD(g_xmm4,g_xmm6);g_xmm6=XMM_MEM(g_esp+0xc0);XMM_STORE(g_esp+0x1a0,g_xmm4);
 g_xmm4=SPLAT(g_xmm1,255);g_xmm5=XMM_MUL(g_xmm5,g_xmm4);g_xmm4=SPLAT(g_xmm1,170);g_xmm3=XMM_MEM(g_eax+48);g_xmm6=XMM_MUL(g_xmm6,g_xmm4);g_xmm4=SPLAT(g_xmm1,85);g_xmm6=XMM_ADD(g_xmm6,g_xmm5);g_xmm5=g_xmm4;g_xmm4=XMM_MEM(g_esp+0xb0);g_xmm7=XMM_MUL(g_xmm4,g_xmm5);g_xmm5=SPLAT(g_xmm1,0);g_xmm1=XMM_MUL(g_xmm0,g_xmm5);g_xmm5=XMM_MEM(g_esp+0xd0);g_xmm1=XMM_ADD(XMM_ADD(g_xmm1,g_xmm7),g_xmm6);XMM_STORE(g_esp+0x1b0,g_xmm1);
 g_xmm1=SPLAT(g_xmm2,255);g_xmm5=XMM_MUL(g_xmm5,g_xmm1);g_xmm6=SPLAT(g_xmm2,170);g_xmm1=XMM_MEM(g_esp+0xc0);g_xmm7=XMM_ADD(XMM_MUL(g_xmm1,g_xmm6),g_xmm5);g_xmm5=SPLAT(g_xmm2,85);g_xmm6=XMM_MUL(g_xmm4,g_xmm5);g_xmm5=SPLAT(g_xmm2,0);g_xmm2=XMM_ADD(XMM_MUL(g_xmm0,g_xmm5),g_xmm6);
 g_xmm6=SPLAT(g_xmm3,255);g_xmm5=XMM_MUL(XMM_MEM(g_esp+0xd0),g_xmm6);g_xmm6=SPLAT(g_xmm3,170);g_xmm1=XMM_ADD(XMM_MUL(g_xmm1,g_xmm6),g_xmm5);g_xmm5=SPLAT(g_xmm3,85);g_xmm4=XMM_MUL(g_xmm4,g_xmm5);g_xmm5=SPLAT(g_xmm3,0);g_xmm0=XMM_ADD(XMM_ADD(XMM_MUL(g_xmm0,g_xmm5),g_xmm4),g_xmm1);g_xmm1=XMM_MEM(g_esp+0x1a0);XMM_STORE(g_esp+0x160,g_xmm1);g_xmm1=XMM_MEM(g_esp+0x1b0);g_xmm2=XMM_ADD(g_xmm2,g_xmm7);XMM_STORE(g_esp+0x170,g_xmm1);XMM_STORE(g_esp+0x180,g_xmm2);XMM_STORE(g_esp+0x190,g_xmm0);
}
static void writeback(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-468;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;for(unsigned row=0;row<3;row++){g_xmm0=XMM_MEM(g_esi+48+row*16);XMM_STORE(g_esp+284+row*16,g_xmm0);}g_xmm0=XMM_MEM(g_esi+96);PUSH32(g_esp,g_edi);g_ecx=g_esp+288;XMM_STORE(g_esp+336,g_xmm0);g_ebp=frame;PUSH32(g_esp,0x2a55ed);sub_002D4360();local_axis();uint32_t saved=g_esp,target=MEM32(g_eax+32);PUSH32(g_esp,0x2a574b);RECOMP_ICALL_SAFE(target,saved);g_edi=1;MEM32(g_esp+24)=g_eax;
 while((int32_t)g_edi<(int32_t)g_eax){
  g_ecx=MEM32(g_esi+16);g_edx=MEM32(g_ecx+g_edi*4);PUSH32(g_esp,8);MEM32(g_esp+32)=g_edx;g_ebp=frame;PUSH32(g_esp,0x2a5771);sub_002B4D80();g_esp+=4;int special=g_edi==g_eax;g_eax=MEM32(g_esp+28);SET_LO8(g_ebx,special);int physical=g_eax!=0xffffffff;if(physical&&special){SET_LO8(g_ecx,MEM8(g_esi+152));physical=!(uint8_t)g_ecx;}
  if(physical){
   g_ecx=MEM32(g_esi+112);g_edx=MEM32(g_ecx+g_eax*4);g_eax=MEM32(g_edx+80);for(unsigned row=0;row<4;row++){g_xmm0=XMM_MEM(g_eax+16+row*16);XMM_STORE(g_esp+96+row*16,g_xmm0);}
   for(unsigned row=0;row<3;row++){for(unsigned col=0;col<3;col++)MEM32(g_esp+32+row*16+col*4)=MEM32(g_esp+96+row*16+col*4);MEM32(g_esp+44+row*16)=0;}
   g_xmm0=XMM_SCALAR(MEMF(0x44c178));for(unsigned col=0;col<3;col++){g_xmm1=XMM_SCALAR(MEMF(g_esp+144+col*4));g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+80+col*4)=g_xmm1.f[0];}MEM32(g_esp+92)=MEM32(0x44bf94);g_eax+=16;physical_product();saved=g_esp;PUSH32(g_esp,1);g_edx=g_esp+36;PUSH32(g_esp,g_edx);XMM_STORE(g_esp+40,g_xmm1);XMM_STORE(g_esp+56,g_xmm2);XMM_STORE(g_esp+72,g_xmm3);XMM_STORE(g_esp+88,g_xmm0);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edi);target=MEM32(g_eax+12);PUSH32(g_esp,0x2a5a13);RECOMP_ICALL_SAFE(target,saved);
  }else{
   g_ecx=MEM32(g_esi+32);g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,g_edi);target=MEM32(g_eax+20);PUSH32(g_esp,0x2a5a21);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+32);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,1);PUSH32(g_esp,g_eax);target=MEM32(g_edx+8);PUSH32(g_esp,0x2a5a2c);RECOMP_ICALL_SAFE(target,saved);
   g_xmm0=XMM_MEM(g_eax);g_ecx=MEM32(g_esi+32);XMM_STORE(g_esp+160,g_xmm0);for(unsigned row=1;row<3;row++){g_xmm0=XMM_MEM(g_eax+row*16);XMM_STORE(g_esp+160+row*16,g_xmm0);}g_xmm0=XMM_MEM(g_eax+48);g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_edi);XMM_STORE(g_esp+216,g_xmm0);target=MEM32(g_eax+8);PUSH32(g_esp,0x2a5a66);RECOMP_ICALL_SAFE(target,saved);hierarchy_product();
   if((uint8_t)g_ebx){SET_LO8(g_eax,MEM8(g_esi+152));if((uint8_t)g_eax){g_xmm0=XMM_SCALAR(MEMF(0x456b00));g_xmm0=SPLAT(g_xmm0,0);g_eax=g_esp+352;g_ecx=3;do{g_xmm1=XMM_MUL(XMM_MEM(g_eax),g_xmm0);XMM_STORE(g_eax,g_xmm1);g_eax+=16;--g_ecx;}while(g_ecx);}}
   g_ecx=MEM32(g_esi+32);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,1);g_eax=g_esp+356;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);target=MEM32(g_edx+12);PUSH32(g_esp,0x2a5c32);RECOMP_ICALL_SAFE(target,saved);
  }
  g_eax=MEM32(g_esp+24);++g_edi;
 }
 g_ecx=MEM32(MEM32(g_esi+28));g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+48);PUSH32(g_esp,0x2a5c49);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_eax);saved=g_esp;PUSH32(g_esp,0);g_ecx=g_eax;target=MEM32(g_edx+36);PUSH32(g_esp,0x2a5c52);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=4;
}
int uc_migration_physics_ragdoll_writeback(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_RAGDOLL_WRITEBACK");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2a55a0||!span(g_esp-768,800)||!safe(g_ecx,156)||!span(0x51da80,64)||!span(0x456b00,4))return 0;uint32_t p=g_ecx,model=MEM32(p+32),owner=MEM32(p+28),n=MEM32(p+20);if(n>65535||!safe(model,4)||!safe(MEM32(model),36)||!safe(owner,4)||!safe(MEM32(owner),4)||!safe(MEM32(MEM32(owner)),52)||(n&&!safe(MEM32(p+16),n*4)))return 0;uint32_t count=MEM32(p+116);if(count>65535||(count&&!safe(MEM32(p+112),count*4)))return 0;for(unsigned i=0;i<count;i++){uint32_t b=MEM32(MEM32(p+112)+i*4);if(!safe(b,84)||!safe(MEM32(b+80),80))return 0;}writeback();return 1;
}
#undef SPLAT
