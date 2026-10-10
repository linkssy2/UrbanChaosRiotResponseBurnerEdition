#include "hybrid_chainsaw_sparks_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002088D0(void),sub_0020A140(void),sub_001CB270(void),sub_001F9010(void);
extern void sub_002D5CCD(void),sub_001F9140(void),sub_001CB400(void),sub_001CBBC0(void);
extern void sub_0020A480(void),sub_002D57FB(void),sub_001CB880(void),sub_0020BDC0(void);
extern void sub_001F9120(void),sub_001CB800(void),sub_001F8DC0(void),sub_000643E0(void);
extern void sub_0004A840(void),sub_00208C50(void),sub_00219530(void),sub_002D37B0(void);
static int chainsaw_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_CHAINSAW_SPARKS");f=p&&!strcmp(p,"1");}return f;}
static int chainsaw_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int chainsaw_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define CS_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define CS_VCALL(target,pc,saved) do{uint32_t t_=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(t_,saved);}while(0)

/* All table/owner reads stay live across retained callbacks. No owned native
 * graph or failure path exists after the first original side effect. */
static void chainsaw_construct(uint32_t frame){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x399bce);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x24;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+24)=g_esi;CS_CALL(sub_0020BDC0,0x1cb8a7);
 g_ebx=0;MEM32(g_esp+0x3c)=g_ebx;MEM32(g_esi)=0x47c000;g_ecx=g_esi+0x80;MEM32(g_ecx)=0x47bff0;
 MEM32(g_ecx+8)=g_ebx;MEM32(g_ecx+12)=g_ebx;MEM32(g_ecx+16)=g_ebx;MEM32(g_ecx+20)=g_ebx;MEM32(g_esi+0xc4)=g_ebx;MEM32(g_ecx+20)=g_esi;
 g_eax=MEM32(g_esi+0x54);PUSH32(g_esp,g_eax);MEM8(g_esp+0x40)=2;CS_CALL(sub_001F9120,0x1cb8e2);
 g_ecx=MEM32(0x4f9d5c);g_edx=g_esp+0x1c;MEM32(g_esp+0x1c)=g_ecx;g_edi=g_esi+0x84;PUSH32(g_esp,g_edx);g_ecx=g_edi;MEM32(g_esp+0x24)=0x1cb300;MEM32(g_esp+0x28)=g_ebx;CS_CALL(sub_001CB800,0x1cb90a);
 g_eax=MEM32(0x4fa024);g_ecx=g_esp+0x1c;PUSH32(g_esp,g_ecx);g_ecx=g_edi;MEM32(g_esp+0x20)=g_eax;MEM32(g_esp+0x24)=0x1cb320;MEM32(g_esp+0x28)=g_ebx;CS_CALL(sub_001CB800,0x1cb92b);
 g_xmm0=XMM_SCALAR(MEMF(g_esi+16));g_edi=MEM32(g_esp+0x44);MEMF(g_esp+0x1c)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+20));g_eax=MEM32(g_esp+0x1c);MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+24));g_ecx=MEM32(g_esp+0x20);g_edx=g_esi+0xa0;
 MEM32(g_edx)=g_eax;MEM32(g_edx+4)=g_ecx;g_ecx=g_esp+0x44;PUSH32(g_esp,g_ecx);MEMF(g_esp+0x28)=g_xmm0.f[0];g_eax=MEM32(g_esp+0x28);PUSH32(g_esp,0x457ff8);PUSH32(g_esp,1);g_ecx=g_edi;MEM32(g_edx+8)=g_eax;CS_CALL(sub_001F8DC0,0x1cb97d);
 g_ecx=MEM32(g_esp+0x44);int found=LO8(g_eax)!=0;SET_LO8(g_eax,(uint32_t)(-(int32_t)LO8(g_eax)));g_edx=g_esp+0x44;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x457ff0);PUSH32(g_esp,2);g_eax=found?0xffffffffu:0;g_eax&=g_ecx;g_ecx=g_edi;MEM32(g_esi+0xc0)=g_eax;CS_CALL(sub_001F8DC0,0x1cb9a0);
 found=LO8(g_eax)!=0;g_xmm0=XMM_SCALAR(MEMF(0x44bf18));if(found)g_xmm0=XMM_SCALAR(MEMF(g_esp+0x44));
 g_eax=g_esp+0x44;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x457fe8);PUSH32(g_esp,2);g_ecx=g_edi;MEMF(g_esi+0xb8)=g_xmm0.f[0];CS_CALL(sub_001F8DC0,0x1cb9cd);
 found=LO8(g_eax)!=0;g_xmm0=XMM_SCALAR(MEMF(0x44bf18));if(found)g_xmm0=XMM_SCALAR(MEMF(g_esp+0x44));
 g_ecx=g_esp+0x28;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x457fdc);PUSH32(g_esp,4);g_ecx=g_edi;MEMF(g_esi+0xbc)=g_xmm0.f[0];CS_CALL(sub_001F8DC0,0x1cb9fa);
 if(!LO8(g_eax)){g_xmm0=XMM_ZERO();MEMF(g_esp+0x1c)=g_xmm0.f[0];MEMF(g_esp+0x20)=g_xmm0.f[0];}
 else{g_xmm0=XMM_SCALAR(MEMF(g_esp+0x28));MEMF(g_esp+0x1c)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x2c));MEMF(g_esp+0x20)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+0x30));}
 g_eax=MEM32(g_esp+0x1c);g_ecx=MEM32(g_esp+0x20);g_edx=g_esi+0xac;MEM32(g_edx)=g_eax;MEM32(g_edx+4)=g_ecx;g_ecx=g_esp+0x44;PUSH32(g_esp,g_ecx);MEMF(g_esp+0x28)=g_xmm0.f[0];g_eax=MEM32(g_esp+0x28);PUSH32(g_esp,0x457fd0);PUSH32(g_esp,3);g_ecx=g_edi;MEM32(g_edx+8)=g_eax;CS_CALL(sub_001F8DC0,0x1cba60);
 if(LO8(g_eax)){g_eax=MEM32(g_esp+0x44)&0xff;SET_LO8(g_eax,g_eax!=0);}
 g_ecx=MEM32(g_esi+0xc0);MEM8(g_esi+0x98)=LO8(g_eax);g_xmm0=XMM_SCALAR(MEMF(g_ecx==9||((int32_t)g_ecx>=14&&(int32_t)g_ecx<=18)?0x44c1f8:0x44c5fc));
 frame=0x4fc160;MEMF(g_esi+0x9c)=g_xmm0.f[0];MEM32(g_esi+0xc4)=g_ebx;MEM32(g_esp+0x44)=frame;g_eax=0;
 while(MEM32(frame)!=g_ecx){g_eax+=12;frame+=12;if(g_eax>=0x174)break;}
 if(g_eax>=0x174){MEM32(g_esp+0x44)=frame;}
 else{
  g_edi=MEM32(frame+8);MEM32(g_esp+0x44)=frame;int allow=1;
  if(g_edi==0x4fc2e8){CS_CALL(sub_000643E0,0x1cbae5);SET_LO8(g_ecx,MEM8(g_eax+0x32));allow=LO8(g_ecx)!=0;}
  if(allow){
   CS_CALL(sub_0004A840,0x1cbaf1);g_edx=MEM32(g_eax);g_ecx=g_eax;{uint32_t saved=g_esp;CS_VCALL(MEM32(g_edx+32),0x1cbaf8,saved);}
   if(LO8(g_eax)&&g_edi==0x4fc2e8)g_edi=0x4fc2f0;
   if(MEM32(g_edi)!=g_ebx){
    g_ebx=g_edi;
    do{
     g_ecx=MEM32(0x51c4dc);CS_CALL(sub_00208C50,0x1cbb1b);frame=g_eax;g_eax=MEM32(g_esi+0x54);g_ecx=MEM32(frame);PUSH32(g_esp,g_ebx);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);MEM32(g_esp+0x20)=g_ecx;CS_CALL(sub_00219530,0x1cbb30);g_esp+=8;
     {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x1c);g_ecx=frame;CS_VCALL(MEM32(g_eax+16),0x1cbb3d,saved);}
     g_eax=MEM32(g_edi+4);g_edi+=4;g_ebx=g_edi;
    }while(g_eax);
    frame=MEM32(g_esp+0x44);g_ebx=0;
   }
  }
  g_edi=MEM32(frame+4);g_eax=g_edi;
  if(MEM32(g_edi)!=g_ebx){do{g_ecx=MEM32(g_eax+4);g_eax+=4;}while(g_ecx!=g_ebx);}
  g_eax=(uint32_t)((int32_t)(g_eax-g_edi)>>2);
  if(g_eax==g_ebx)g_eax=0;
  else{PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);CS_CALL(sub_002D37B0,0x1cbb76);g_eax=MEM32(g_edi+g_eax*4);g_esp+=8;}
  g_ecx=g_esi;MEM32(g_esi+0xc4)=g_eax;CS_CALL(sub_001CB270,0x1cbb89);
 }
 g_ecx=MEM32(g_esp+0x34);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp+=0x30;g_recomp_zf=g_esp==0;g_esp+=8;
}

int uc_hybrid_chainsaw_sparks(unsigned entry){
 if(!chainsaw_enabled())return 0;
 switch(entry){case 0x1cb270:case 0x1cb300:case 0x1cb320:case 0x1cb370:case 0x1cb3c0:case 0x1cb400:case 0x1cb440:case 0x1cb880:case 0x1cbba0:case 0x1cbbc0:case 0x1cbc40:break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx,frame=g_seh_ebp;unsigned size=(entry==0x1cb400||entry==0x1cb440)?24:0xd0;
 if((s&3)||s<0x12000u||!chainsaw_span(s-8192,8224)||!chainsaw_span(0x51c178,1)||MEM8(0x51c178)||!chainsaw_apart(s-8192,8224,0x51c178,1))return 0;
 if(entry!=0x1cbc40&&(!chainsaw_span(p,size)||!chainsaw_apart(p,size,s-8192,8224)||!chainsaw_apart(p,size,0x51c178,1)))return 0;
 if(entry==0x1cb370&&(unsigned)g_fp_top>7)return 0;
 if(entry==0x1cb880){chainsaw_construct(frame);return 1;}
 if(entry==0x1cbc40){
  PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x399c0b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xd0);CS_CALL(sub_002D57FB,0x1cbc60);g_esp+=4;MEM32(g_esp)=g_eax;MEM32(g_esp+12)=0;
  if(g_eax){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;CS_CALL(sub_001CB880,0x1cbc7e);}else g_eax=0;
  g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
 }
 if(entry==0x1cb270){
  g_eax=MEM32(g_ecx+0xc4);g_edx=g_ecx+0xc4;PUSH32(g_esp,g_esi);
  if(g_eax){g_xmm0=XMM_SCALAR(MEMF(g_ecx+0xbc));g_xmm1=XMM_SCALAR(MEMF(g_ecx+0xb8));PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0x4000);g_esp-=8;g_eax=g_esp;MEMF(g_eax)=g_xmm1.f[0];MEMF(g_eax+4)=g_xmm0.f[0];g_esi=MEM32(g_ecx+0xa0);g_eax=g_ecx+0xac;PUSH32(g_esp,g_eax);g_esp-=12;g_eax=g_esp;MEM32(g_eax)=g_esi;g_esi=MEM32(g_ecx+0xa4);MEM32(g_eax+4)=g_esi;g_esi=MEM32(g_ecx+0xa8);MEM32(g_eax+8)=g_esi;g_esi=MEM32(g_ecx+0x40);g_esp-=12;g_eax=g_esp;MEM32(g_eax)=g_esi;g_esi=MEM32(g_ecx+0x44);MEM32(g_eax+4)=g_esi;g_ecx=MEM32(g_ecx+0x48);MEM32(g_eax+8)=g_ecx;g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_edx);CS_CALL(sub_002088D0,0x1cb2f0);}
  POP32(g_esp,g_esi);g_esp+=4;return 1;
 }
 if(entry==0x1cb300){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x20);g_esi=g_ecx;CS_CALL(sub_0020A140,0x1cb30a);PUSH32(g_esp,1);g_ecx=g_esi;CS_CALL(sub_0020A140,0x1cb313);POP32(g_esp,g_esi);g_esp+=12;return 1;}
 if(entry==0x1cb320){
  g_eax=MEM32(g_esp+4);for(unsigned i=0;i<4;i++){g_xmm0=XMM_MEM(g_eax+i*16);XMM_STORE(g_ecx+16+i*16,g_xmm0);}g_eax+=64;PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax);g_edx=g_ecx+0xac;MEM32(g_edx)=g_esi;g_esi=MEM32(g_eax+4);MEM32(g_edx+4)=g_esi;g_eax=MEM32(g_eax+8);MEM32(g_edx+8)=g_eax;CS_CALL(sub_001CB270,0x1cb362);POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=12;return 1;
 }
 if(entry==0x1cb370){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;SET_LO8(g_eax,MEM8(g_esi+0x98));
  if(LO8(g_eax)){
   g_xmm0=XMM_SCALAR(MEMF(g_esi+0x9c));double a=g_xmm0.f[0],b=MEMF(0x44c174);unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;
   if(flags&0x41u){PUSH32(g_esp,0x20);g_ecx=g_esi;CS_CALL(sub_0020A140,0x1cb3aa);g_eax=MEM32(g_esi);g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;RECOMP_ITAIL(MEM32(g_eax+0x24));return 1;}
   CS_CALL(sub_001F9010,0x1cb393);g_fp_stack[g_fp_top]=MEMF(g_esi+0x9c)-g_fp_stack[g_fp_top];MEMF(g_esi+0x9c)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
  }
  POP32(g_esp,g_esi);g_esp+=4;return 1;
 }
 if(entry==0x1cb3c0){
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_eax=g_ecx;g_edi=0x4583c8;g_ecx=8;g_edx=0;int same=0;
  while(g_ecx){uint8_t a=MEM8(g_esi),b=MEM8(g_edi);same=a==b;g_esi++;g_edi++;g_ecx--;if(!same)break;}
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);
  if(same){g_eax=MEM32(g_eax+0xc0);g_ecx=MEM32(g_esp+8);MEM32(g_ecx+4)=g_eax;MEM32(g_ecx)=1;SET_LO8(g_eax,1);g_recomp_zf=1;}else{SET_LO8(g_eax,0);g_recomp_zf=1;}
  g_esp+=12;return 1;
 }
 if(entry==0x1cb400){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi)=0x47bff0;g_eax=MEM32(g_esi+8);if(g_eax){PUSH32(g_esp,g_eax);CS_CALL(sub_002D5CCD,0x1cb416);g_esp+=4;}
  MEM32(g_esi+8)=0;MEM32(g_esi+12)=0;MEM32(g_esi+16)=0;g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_001F9140();return 1;
 }
 if(entry==0x1cb440||entry==0x1cbba0){
  PUSH32(g_esp,g_esi);g_esi=g_ecx;if(entry==0x1cb440)CS_CALL(sub_001CB400,0x1cb448);else CS_CALL(sub_001CBBC0,0x1cbba8);
  if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);if(entry==0x1cb440)CS_CALL(sub_002D5CCD,0x1cb455);else CS_CALL(sub_002D5CCD,0x1cbbb5);g_esp+=4;}
  g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
 }
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x399be8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;MEM32(g_esp+12)=g_edi;g_esi=g_edi+0x80;g_ebx=0;MEM32(g_esi)=0x47bff0;g_eax=MEM32(g_esi+8);MEM32(g_esp+24)=g_ebx;
 if(g_eax!=g_ebx){PUSH32(g_esp,g_eax);CS_CALL(sub_002D5CCD,0x1cbbfe);g_esp+=4;}
 g_ecx=g_esi;MEM32(g_esi+8)=g_ebx;MEM32(g_esi+12)=g_ebx;MEM32(g_esi+16)=g_ebx;CS_CALL(sub_001F9140,0x1cbc11);g_ecx=g_edi;MEM32(g_esp+24)=0xffffffffu;CS_CALL(sub_0020A480,0x1cbc20);
 g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
#undef CS_CALL
#undef CS_VCALL
