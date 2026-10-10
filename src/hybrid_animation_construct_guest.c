#include "hybrid_animation_construct_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00011000(void);
extern void sub_001F11A0(void);
extern void sub_001F4C00(void);
extern void sub_002D57FB(void);
extern void sub_002D589F(void);
extern void sub_002D631D(void);
extern void sub_002D68FB(void);
static int construct_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_ANIMATION_CONSTRUCT");flag=v&&!strcmp(v,"1");}return flag;}
static int construct_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Same valid-object/allocation/callback lifetime contract as the source.
 * No prewalk, shadow arrays, allocator reordering or post-effect decline.
 * The worker flag qualifies configuration; it is not a synchronization lock. */
static int construct_admit(unsigned bytes){uint32_t s=g_esp,p=g_ecx;return !(s&3)&&s>=0x10400u&&construct_span(s-1024,1048)&&construct_span(p,bytes)&&((uint64_t)p+bytes<=s-1024||(uint64_t)s+24<=p)&&construct_span(0x51c138,12)&&construct_span(0x51c178,1)&&!MEM8(0x51c178);}
int uc_hybrid_animation_descriptor_init(void){
 if(!construct_enabled()||!construct_admit(24))return 0;
 uint32_t frame=g_seh_ebp;
 g_esp-=12;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+20);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);
 MEM32(g_esi)=0;MEM32(g_esi+16)=g_ebx;MEM8(g_esi+20)=0;PUSH32(g_esp,0x1f1000);sub_002D589F();
 MEM32(g_esi+4)=g_eax;g_eax=g_ebx*4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f1010);sub_002D589F();
 g_ecx=(g_ebx+g_ebx*2)<<2;PUSH32(g_esp,g_ecx);MEM32(g_esi+8)=g_eax;PUSH32(g_esp,0x1f101f);sub_002D589F();
 g_xmm1=XMM_ZERO();g_xmm0=XMM_SCALAR(MEMF(0x44bf94));MEM32(g_esi+12)=g_eax;g_esp+=12;g_eax=0;
 if((int32_t)g_ebx>=4){
  MEMF(g_esp+16)=g_xmm1.f[0];g_edi=MEM32(g_esp+16);MEMF(g_esp+20)=g_xmm1.f[0];g_edx=MEM32(g_esp+20);MEMF(g_esp+24)=g_xmm1.f[0];g_ecx=0;
  do{
   frame=MEM32(g_esi+4);MEM8(g_eax+frame)=1;frame=MEM32(g_esi+8);MEMF(frame+g_eax*4)=g_xmm0.f[0];frame=MEM32(g_esi+12);g_ebx=MEM32(g_esp+24);frame+=g_ecx;MEM32(frame)=g_edi;MEM32(frame+4)=g_edx;MEM32(frame+8)=g_ebx;
   frame=MEM32(g_esi+4);MEM8(g_eax+frame+1)=1;frame=MEM32(g_esi+8);MEMF(frame+g_eax*4+4)=g_xmm0.f[0];frame=MEM32(g_esi+12);frame=g_ecx+frame+12;MEM32(frame)=g_edi;MEM32(frame+4)=g_edx;MEM32(frame+8)=g_ebx;
   g_ebx=MEM32(g_esi+4);frame=MEM32(g_esp+24);MEM8(g_ebx+g_eax+2)=1;g_ebx=MEM32(g_esi+8);MEMF(g_ebx+g_eax*4+8)=g_xmm0.f[0];g_ebx=MEM32(g_esi+12);g_ebx=g_ecx+g_ebx+24;MEM32(g_ebx)=g_edi;MEM32(g_ebx+4)=g_edx;MEM32(g_ebx+8)=frame;
   g_ebx=MEM32(g_esi+4);MEM8(g_ebx+g_eax+3)=1;g_ebx=MEM32(g_esi+8);MEMF(g_ebx+g_eax*4+12)=g_xmm0.f[0];frame=MEM32(g_esi+12);g_ebx=g_ecx+36;frame+=g_ebx;g_ebx=MEM32(g_esp+24);MEM32(frame)=g_edi;MEM32(frame+4)=g_edx;MEM32(frame+8)=g_ebx;
   g_ebx=MEM32(g_esp+32);g_eax+=4;frame=g_ebx-3;g_ecx+=48;
  }while((int32_t)g_eax<(int32_t)frame);
 }
 if((int32_t)g_eax<(int32_t)g_ebx){
  g_ecx=g_eax+g_eax*2;MEMF(g_esp+20)=g_xmm1.f[0];g_edx=MEM32(g_esp+20);MEMF(g_esp+16)=g_xmm1.f[0];MEMF(g_esp+24)=g_xmm1.f[0];g_ecx<<=2;
  do{g_edi=MEM32(g_esi+4);frame=MEM32(g_esp+16);MEM8(g_eax+g_edi)=1;g_edi=MEM32(g_esi+8);MEMF(g_edi+g_eax*4)=g_xmm0.f[0];g_edi=MEM32(g_esi+12);g_edi+=g_ecx;MEM32(g_edi)=frame;frame=MEM32(g_esp+24);g_eax++;MEM32(g_edi+4)=g_edx;g_ecx+=12;MEM32(g_edi+8)=frame;}while((int32_t)g_eax<(int32_t)g_ebx);
 }
 g_eax=MEM32(0x51c138);
 if(g_eax){g_edx=MEM32(g_esi+16);if((int32_t)g_edx<=(int32_t)MEM32(0x51c13c))goto init_finish;}
 PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f1166);sub_002D631D();g_eax=MEM32(g_esi+16);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f116f);sub_002D589F();MEM32(0x51c138)=g_eax;g_ecx=MEM32(g_esi+16);g_esp+=8;MEM32(0x51c13c)=g_ecx;
init_finish:
 g_eax=MEM32(0x51c140)+1;POP32(g_esp,g_edi);MEM32(0x51c140)=g_eax;g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
int uc_hybrid_animation_descriptor_copy(void){
 if(!construct_enabled()||!construct_admit(24))return 0;
 uint32_t frame=g_seh_ebp;
 g_eax=MEM32(0);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b7eb);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edi=MEM32(g_esp+32);g_eax=MEM32(g_edi);g_esi=g_ecx;SET_LO8(g_ecx,MEM8(g_esp+36));MEM32(g_esi)=g_eax;g_eax=MEM32(g_edi+16);MEM32(g_esi+16)=g_eax;
 if(LO8(g_ecx)){
  g_ecx=MEM32(g_edi+4);MEM32(g_esi+4)=g_ecx;g_edx=MEM32(g_edi+8);g_eax=0;MEM32(g_edi+4)=g_eax;MEM32(g_esi+8)=g_edx;g_ecx=MEM32(g_edi+12);MEM32(g_edi+8)=g_eax;MEM32(g_esi+12)=g_ecx;MEM32(g_edi+12)=g_eax;
 }else{
  PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f11f9);sub_002D589F();g_edx=MEM32(g_esi+16)<<2;PUSH32(g_esp,g_edx);MEM32(g_esi+4)=g_eax;PUSH32(g_esp,0x1f1208);sub_002D589F();
  frame=MEM32(g_esi+16);MEM32(g_esi+8)=g_eax;g_eax=(frame+frame*2)<<2;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f121b);sub_002D589F();g_ebx=g_eax;g_esp+=12;MEM32(g_esp+36)=g_ebx;MEM32(g_esp+24)=0;
  if(g_ebx){PUSH32(g_esp,0x1f4b10);PUSH32(g_esp,frame);PUSH32(g_esp,12);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x1f123e);sub_00011000();}else g_ebx=0;
  g_ecx=MEM32(g_esi+16);g_eax=0;MEM32(g_esp+24)=0xffffffffu;MEM32(g_esi+12)=g_ebx;
  if((int32_t)g_ecx>0){g_ecx=0;do{
   g_edx=MEM32(g_edi+4);SET_LO8(g_edx,MEM8(g_eax+g_edx));g_ebx=MEM32(g_esi+4);MEM8(g_eax+g_ebx)=LO8(g_edx);g_edx=MEM32(g_edi+8);g_edx=MEM32(g_edx+g_eax*4);g_ebx=MEM32(g_esi+8);MEM32(g_ebx+g_eax*4)=g_edx;
   g_edx=MEM32(g_edi+12);g_ebx=MEM32(g_esi+12);g_edx+=g_ecx;frame=MEM32(g_edx);g_ebx+=g_ecx;MEM32(g_ebx)=frame;frame=MEM32(g_edx+4);MEM32(g_ebx+4)=frame;g_edx=MEM32(g_edx+8);MEM32(g_ebx+8)=g_edx;g_edx=MEM32(g_esi+16);g_eax++;g_ecx+=12;
  }while((int32_t)g_eax<(int32_t)g_edx);}
 }
 SET_LO8(g_eax,MEM8(g_edi+20));MEM8(g_esi+20)=LO8(g_eax);g_eax=MEM32(0x51c138);
 if(g_eax){g_ecx=MEM32(g_esi+16);if((int32_t)g_ecx<=(int32_t)MEM32(0x51c13c))goto copy_finish;}
 PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f12bd);sub_002D631D();g_edx=MEM32(g_esi+16);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f12c6);sub_002D589F();MEM32(0x51c138)=g_eax;g_eax=MEM32(g_esi+16);g_esp+=8;MEM32(0x51c13c)=g_eax;
copy_finish:
 g_eax=MEM32(0x51c140)+1;g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);MEM32(0x51c140)=g_eax;g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
int uc_hybrid_animation_descriptor_free(void){
 if(!construct_enabled()||!construct_admit(24))return 0;
 g_eax=MEM32(0x51c140)-1;PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(0x51c140)=g_eax;
 if((int32_t)g_eax<=0){g_eax=MEM32(0x51c138);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f131d);sub_002D631D();g_esp+=4;MEM32(0x51c138)=0;}
 g_ecx=MEM32(g_esi+12);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f1333);sub_002D631D();g_edx=MEM32(g_esi+8);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f133c);sub_002D631D();g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f1345);sub_002D631D();g_esp+=12;POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
int uc_hybrid_animation_construct(void){
 if(!construct_enabled()||!construct_admit(0x74))return 0;
 uint32_t frame=g_ebp;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b816);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=120;
 g_edx=MEM32(frame+12);g_eax=MEM32(frame+8);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=g_ecx;g_ecx=MEM32(frame+20);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ecx);g_esi=0;PUSH32(g_esp,g_edx);g_ecx=g_ebx+8;MEM32(g_esp+36)=g_ebx;MEM32(g_ebx)=g_esi;MEM32(g_ebx+4)=g_eax;
 g_ebp=frame;PUSH32(g_esp,0x1f13e1);sub_001F11A0();
 g_eax=MEM32(g_ebx+4);g_ecx=MEM32(g_eax+4);g_xmm0=XMM_SCALAR(MEMF(0x44bf94));MEM32(g_ebx+36)=g_ecx;g_eax=0;g_ecx=16;g_edi=g_esp+64;
 for(uint32_t i=0;i<g_ecx;i++)MEM32(g_edi+i*4)=g_eax;g_edi+=g_ecx*4;g_ecx=0;
 SET_LO8(g_eax,MEM8(g_ebx+28));MEM32(g_esp+140)=g_esi;MEM32(g_ebx+40)=g_esi;MEM32(g_ebx+44)=g_esi;MEMF(g_esp+100)=g_xmm0.f[0];MEMF(g_esp+124)=g_xmm0.f[0];if(LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(0x44bf80));
 g_edi=MEM32(g_ebx+36);g_edx=((g_edi+g_edi*2)<<4)+16;PUSH32(g_esp,g_edx);MEMF(g_esp+68)=g_xmm0.f[0];MEMF(g_esp+92)=g_xmm0.f[0];g_ebp=frame;PUSH32(g_esp,0x1f1445);sub_002D589F();
 g_esp+=4;MEM32(g_esp+24)=g_eax;MEM8(g_esp+140)=1;
 if(g_eax!=g_esi){PUSH32(g_esp,0x11030);PUSH32(g_esp,0x1f0e30);PUSH32(g_esp,g_edi);g_esi=g_eax+16;PUSH32(g_esp,48);PUSH32(g_esp,g_esi);MEM32(g_eax)=g_edi;g_ebp=frame;PUSH32(g_esp,0x1f1470);sub_002D68FB();}
 g_eax=MEM32(frame+16);MEM8(g_esp+140)=0;MEM32(g_ebx+32)=g_esi;
 if(g_eax){
  g_eax=MEM32(g_ebx+16);g_ecx=MEM32(g_eax);PUSH32(g_esp,g_ecx);g_edx=g_esp+68;PUSH32(g_esp,g_edx);g_ecx=g_esp+40;g_ebp=frame;PUSH32(g_esp,0x1f149a);sub_001F4C00();
  g_ecx=MEM32(g_ebx+32);g_edx=g_eax;g_edi=MEM32(g_edx);g_esi=g_ecx;MEM32(g_esi)=g_edi;g_edi=MEM32(g_edx+4);MEM32(g_esi+4)=g_edi;g_edi=MEM32(g_edx+8);MEM32(g_esi+8)=g_edi;g_edx=MEM32(g_edx+12);g_eax+=16;MEM32(g_esi+12)=g_edx;g_edx=MEM32(g_eax);g_ecx+=16;MEM32(g_ecx)=g_edx;g_edx=MEM32(g_eax+4);MEM32(g_ecx+4)=g_edx;g_eax=MEM32(g_eax+8);MEM32(g_ecx+8)=g_eax;
  g_ecx=MEM32(g_ebx+36);g_eax=1;MEM32(g_esp+12)=g_eax;
  if((int32_t)g_ecx>(int32_t)g_eax){
   g_ecx=MEM32(frame+16)+64;MEM32(g_esp+24)=48;MEM32(g_esp+20)=12;MEM32(g_esp+16)=g_ecx;
   do{
    g_edx=MEM32(g_ebx+20);g_esi=MEM32(g_esp+16);g_eax=MEM32(g_esp+20)+g_edx;g_edx=MEM32(g_esp+12);g_ecx=16;g_edi=g_esp+64;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;
    g_xmm0=XMM_SCALAR(MEMF(g_esp+112));g_xmm0.f[0]-=MEMF(g_eax);g_ecx=MEM32(g_ebx+16);MEMF(g_esp+112)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+116));g_xmm0.f[0]-=MEMF(g_eax+4);MEMF(g_esp+116)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+120));g_xmm0.f[0]-=MEMF(g_eax+8);MEMF(g_esp+120)=g_xmm0.f[0];
    g_eax=MEM32(g_ecx+g_edx*4);PUSH32(g_esp,g_eax);g_ecx=g_esp+68;PUSH32(g_esp,g_ecx);g_ecx=g_esp+40;g_ebp=frame;PUSH32(g_esp,0x1f1563);sub_001F4C00();
    g_ecx=MEM32(g_ebx+32)+MEM32(g_esp+24);g_edx=g_eax;g_edi=MEM32(g_edx);g_esi=g_ecx;MEM32(g_esi)=g_edi;g_edi=MEM32(g_edx+4);MEM32(g_esi+4)=g_edi;g_edi=MEM32(g_edx+8);MEM32(g_esi+8)=g_edi;g_edx=MEM32(g_edx+12);g_edi=MEM32(g_esp+16);MEM32(g_esi+12)=g_edx;g_esi=MEM32(g_esp+20);g_eax+=16;g_edx=MEM32(g_eax);g_ecx+=16;MEM32(g_ecx)=g_edx;g_edx=MEM32(g_eax+4);MEM32(g_ecx+4)=g_edx;g_eax=MEM32(g_eax+8);g_edx=MEM32(g_esp+24);MEM32(g_ecx+8)=g_eax;g_eax=MEM32(g_esp+12);g_ecx=MEM32(g_ebx+36);g_eax++;g_edi+=64;g_esi+=12;g_edx+=48;MEM32(g_esp+12)=g_eax;MEM32(g_esp+16)=g_edi;MEM32(g_esp+20)=g_esi;MEM32(g_esp+24)=g_edx;
   }while((int32_t)g_eax<(int32_t)g_ecx);
  }
 }
 PUSH32(g_esp,68);g_ebp=frame;PUSH32(g_esp,0x1f15d6);sub_002D57FB();g_esp+=4;if(g_eax)MEM32(g_eax)=0;else g_eax=0;
 g_ecx=MEM32(g_esp+132);POP32(g_esp,g_edi);MEM32(g_ebx+112)=g_eax;POP32(g_esp,g_esi);g_eax=g_ebx;MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=20;return 1;
}
