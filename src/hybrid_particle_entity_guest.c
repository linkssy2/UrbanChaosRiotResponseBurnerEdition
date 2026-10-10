#include "hybrid_particle_entity_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00218A60(void),sub_001FB780(void),sub_000BBC20(void),sub_001F8DC0(void),sub_001F92B0(void);
extern void sub_0020A140(void),sub_001FB9D0(void),sub_002D631D(void),sub_00218B30(void),sub_002D57FB(void);
extern void sub_001566D0(void),sub_001568D0(void),sub_002D5CCD(void),sub_001FBA40(void),sub_00218CE0(void);
extern void sub_001FB910(void),sub_00218C40(void),sub_00156920(void),sub_002084E0(void),sub_00210CA0(void);
extern void sub_001FB930(void),sub_001F8FC0(void),sub_00219530(void),sub_00219450(void),sub_002D7159(void);
extern void sub_00219740(void),sub_00218F40(void),sub_00208C50(void),sub_00219B60(void);
static int entity_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_ENTITY");f=p&&!strcmp(p,"1");}return f;}
static int entity_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int entity_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define ENTITY_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define ENTITY_FRAME_CALL(fn,pc) do{g_ebp=frame;ENTITY_CALL(fn,pc);}while(0)
#define ENTITY_VCALL(target,pc,saved) do{uint32_t t_=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(t_,saved);}while(0)
static void entity_construct(void){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x393466);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+12)=g_esi;ENTITY_CALL(sub_00218A60,0x1566f4);
 g_ebx=g_esi+0xd0;g_ecx=g_ebx;MEM32(g_esp+24)=0;MEM32(g_esi)=0x46c918;ENTITY_CALL(sub_001FB780,0x15670f);
 g_edi=MEM32(g_esp+32);PUSH32(g_esp,0);PUSH32(g_esp,0x4609f8);g_eax=g_esi+0xd4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x46c8fc);g_ecx=g_edi;MEM8(g_esp+40)=1;MEM32(g_esi+0xec)=0;ENTITY_CALL(sub_000BBC20,0x15673c);
 g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x46c8e8);PUSH32(g_esp,1);g_ecx=g_edi;MEM32(g_esi+0xd8)=g_eax;ENTITY_CALL(sub_001F8DC0,0x156755);
 g_edx=MEM32(g_esp+32);g_eax=LO8(g_eax)?0xffffffffu:0;g_ecx=g_edi;g_eax&=g_edx;g_edx=g_esp+32;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x46c8d8);PUSH32(g_esp,3);MEM32(g_esi+0xdc)=g_eax;ENTITY_CALL(sub_001F8DC0,0x156778);
 if(!LO8(g_eax))SET_LO8(g_eax,1);else{g_eax=MEM32(g_esp+32)&255u;SET_LO8(g_eax,g_eax!=0);}
 g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);g_eax=ZX8(LO8(g_eax));PUSH32(g_esp,0x46c8c8);PUSH32(g_esp,9);g_ecx=g_edi;MEM32(g_esi+0xe8)=g_eax;MEM32(g_esp+44)=0;ENTITY_CALL(sub_001F8DC0,0x1567b0);
 g_edx=MEM32(g_esp+32);PUSH32(g_esp,0);PUSH32(g_esp,0x458f10);g_eax=g_esi+0xe4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x46c8c0);g_ecx=g_edi;MEM32(g_esi+0xec)=g_edx;ENTITY_CALL(sub_000BBC20,0x1567d4);
 g_ecx=g_esp+32;PUSH32(g_esp,0x46c880);PUSH32(g_esp,g_ecx);MEM32(g_esi+0xe0)=g_eax;ENTITY_CALL(sub_001F92B0,0x1567e9);
 g_esp+=8;PUSH32(g_esp,0x2000);g_ecx=g_esi;ENTITY_CALL(sub_0020A140,0x1567f8);PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ecx=g_ebx;ENTITY_CALL(sub_001FB9D0,0x156802);
 g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void entity_continue(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff0u;g_esp-=0x1f4;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0xd4);int attached=g_eax==1;PUSH32(g_esp,g_edi);
 if((int32_t)g_eax>1)goto final_yield;
 if(attached){
  g_eax=MEM32(g_esi+0xd8);g_ecx=MEM32(g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x51c4dc);ENTITY_FRAME_CALL(sub_002084E0,0x15695a);g_ecx=g_eax;ENTITY_FRAME_CALL(sub_00210CA0,0x156961);g_ebx=g_eax;
  if(g_ebx==0xffffffffu){g_edi=g_esi+0xd0;do{g_ecx=g_edi;ENTITY_FRAME_CALL(sub_001FB930,0x156977);g_edx=MEM32(g_esi+0xd8);g_eax=MEM32(g_edx);g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_eax);ENTITY_FRAME_CALL(sub_002084E0,0x15698b);g_ecx=g_eax;ENTITY_FRAME_CALL(sub_00210CA0,0x156992);g_ebx=g_eax;}while(g_ebx==0xffffffffu);}
  PUSH32(g_esp,g_ebx);ENTITY_FRAME_CALL(sub_001F8FC0,0x15699f);g_edi=g_eax;g_esp+=4;MEM32(g_esp+20)=0;
  if(g_edi){
   g_eax=MEM32(g_esi+0xdc);
   if(!g_eax){
    g_eax=MEM32(g_esi+0xe8);
    if(g_eax){PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);ENTITY_FRAME_CALL(sub_00219530,0x1569d0);g_esp+=8;}
    else{g_edx=MEM32(g_edi);g_ebx=MEM32(g_edi+84);g_ecx=g_edi;{uint32_t s=g_esp;ENTITY_VCALL(MEM32(g_edx+44),0x1569e2,s);}PUSH32(g_esp,g_eax);ENTITY_FRAME_CALL(sub_00219450,0x1569e8);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);ENTITY_FRAME_CALL(sub_00218F40,0x156a69);g_esp+=12;}
   }else{
    PUSH32(g_esp,g_eax);PUSH32(g_esp,0x46c9ac);g_eax=g_esp+104;PUSH32(g_esp,63);PUSH32(g_esp,g_eax);ENTITY_FRAME_CALL(sub_002D7159,0x1569fe);
    g_eax=MEM32(g_esi+0xe8);g_esp+=16;
    if(g_eax){PUSH32(g_esp,0);g_ecx=g_esp+100;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);ENTITY_FRAME_CALL(sub_00219740,0x156a18);g_esp+=12;}
    else{
     g_edx=MEM32(g_edi);g_ecx=g_edi;{uint32_t s=g_esp;ENTITY_VCALL(MEM32(g_edx+44),0x156a21,s);}
     g_xmm0=XMM_MEM(g_eax);XMM_STORE(g_esp+32,g_xmm0);g_xmm0=XMM_MEM(g_eax+16);XMM_STORE(g_esp+48,g_xmm0);g_xmm0=XMM_MEM(g_eax+32);g_ecx=g_esp+32;
     {uint32_t s=g_esp;PUSH32(g_esp,g_ecx);XMM_STORE(g_esp+68,g_xmm0);g_xmm0=XMM_MEM(g_eax+48);g_eax=MEM32(g_edi);g_edx=g_esp+100;PUSH32(g_esp,g_edx);g_ecx=g_edi;XMM_STORE(g_esp+88,g_xmm0);ENTITY_VCALL(MEM32(g_eax+12),0x156a55,s);}
     g_edi=MEM32(g_edi+84);g_eax=g_esp+32;PUSH32(g_esp,g_eax);ENTITY_FRAME_CALL(sub_00219450,0x156a62);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);ENTITY_FRAME_CALL(sub_00218F40,0x156a69);g_esp+=12;
    }
   }
   MEM32(g_esp+20)=g_eax;
  }
  g_eax=MEM32(g_esi+0xe4);g_ecx=MEM32(0x51c4dc);
 }else{
  g_eax=MEM32(g_esi+84);PUSH32(g_esp,0);PUSH32(g_esp,g_eax);ENTITY_FRAME_CALL(sub_00219530,0x156b4a);g_ecx=MEM32(0x51c4dc);g_edi=g_eax;g_eax=MEM32(g_esi+0xe4);g_esp+=8;MEM32(g_esp+24)=g_edi;
 }
 if((int32_t)g_eax>0){
  ENTITY_FRAME_CALL(sub_00208C50,attached?0x156a89:0x156b6c);g_ecx=MEM32(0x51c4dc);g_edi=g_eax;ENTITY_FRAME_CALL(sub_00208C50,attached?0x156a96:0x156b79);g_ecx=MEM32(0x51c4dc);g_ebx=g_eax;ENTITY_FRAME_CALL(sub_00208C50,attached?0x156aa3:0x156b86);
  g_edx=MEM32(g_edi);g_ecx=g_esi+0xec;{uint32_t s=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_esp+164;MEM32(g_esp+32)=g_eax;g_eax=MEM32(g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_edi;MEM32(g_esp+(attached?32:28))=g_eax;ENTITY_VCALL(MEM32(g_edx+28),attached?0x156ac3:0x156ba6,s);}
  g_edx=MEM32(g_esp+(attached?20:24));{uint32_t s=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);g_edx=MEM32(g_ebx);g_eax=g_esi+0xec;PUSH32(g_esp,g_eax);g_ecx=g_ebx;ENTITY_VCALL(MEM32(g_edx+32),attached?0x156ad7:0x156bba,s);}
  PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+0xe0);g_ecx=MEM32(g_eax);PUSH32(g_esp,g_ecx);ENTITY_FRAME_CALL(sub_00219B60,attached?0x156ae6:0x156bc9);
  g_ecx=MEM32(g_esp+44);g_esp+=8;{uint32_t s=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+(attached?36:32));ENTITY_VCALL(MEM32(g_eax+20),attached?0x156af5:0x156bd8,s);}
  if(attached){g_edx=MEM32(g_esi);g_ecx=g_esi;uint32_t s=g_esp;ENTITY_VCALL(MEM32(g_edx+36),0x156afc,s);}
  g_ecx=g_esi+0xd0;ENTITY_FRAME_CALL(sub_001FB930,attached?0x156b07:0x156be3);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;return;
 }
 if(attached){
  int zero=g_eax==0;ENTITY_FRAME_CALL(sub_00208C50,0x156b13);g_edx=MEM32(g_eax);g_ecx=g_esi+0xec;{uint32_t s=g_esp;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+24);PUSH32(g_esp,g_ecx);g_ecx=g_eax;ENTITY_VCALL(MEM32(g_edx+16),0x156b26,s);}
  g_edx=MEM32(g_esi);g_ecx=g_esi;{uint32_t s=g_esp;ENTITY_VCALL(MEM32(g_edx+36),0x156b2d,s);}g_ecx=g_esi+0xd0;ENTITY_FRAME_CALL(sub_001FB930,0x156b38);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=zero;g_esp+=4;return;
 }
 ENTITY_FRAME_CALL(sub_00208C50,0x156bef);g_edx=MEM32(g_eax);g_ecx=g_esi+0xec;{uint32_t s=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);g_ecx=g_eax;ENTITY_VCALL(MEM32(g_edx+16),0x156bfe,s);}
final_yield:
 g_ecx=g_esi+0xd0;ENTITY_FRAME_CALL(sub_001FB930,0x156c09);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=4;
}
int uc_hybrid_particle_entity(uint32_t entry){
 if(!entity_enabled())return 0;
 switch(entry){case 0x1566d0:case 0x156820:case 0x156850:case 0x1568b0:case 0x1568d0:case 0x156920:case 0x156c10:case 0x120c60:case 0x15dfc0:break;default:return 0;}
 uint32_t s=g_esp,p=g_ecx,frame=g_seh_ebp;
 if((s&3)||s<0x11000u||!entity_span(s-4096,4112)||!entity_span(0x51c178,1)||MEM8(0x51c178)||!entity_apart(s-4096,4112,0x51c178,1))return 0;
 if(entry!=0x156850&&(!entity_span(p,0xf0)||!entity_apart(p,0xf0,s-4096,4112)))return 0;
 if((entry==0x120c60||entry==0x15dfc0)&&MEM32(p)!=0x46c918)return 0;
 /* Original completing service/lifetime contract; no decline after effects. */
 if(entry==0x1566d0){entity_construct();return 1;}if(entry==0x156920){entity_continue();return 1;}
 if(entry==0x156c10){g_seh_ebp=frame;sub_00156920();return 1;}
 if(entry==0x15dfc0){SET_LO8(g_eax,1);g_esp+=12;return 1;}
 if(entry==0x120c60){PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=g_esi+0xd0;ENTITY_CALL(sub_001FB910,0x120c6e);g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_00218C40();return 1;}
 if(entry==0x156820){PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0xe0);PUSH32(g_esp,g_eax);ENTITY_CALL(sub_002D631D,0x15682f);g_ecx=MEM32(g_esi+0xd8);PUSH32(g_esp,g_ecx);ENTITY_CALL(sub_002D631D,0x15683b);g_esp+=8;MEM32(g_esi+0xd8)=0;g_ecx=g_esi;POP32(g_esp,g_esi);g_seh_ebp=frame;sub_00218B30();return 1;}
 if(entry==0x156850){PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39347b);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0xf0);ENTITY_CALL(sub_002D57FB,0x156870);g_esp+=4;MEM32(g_esp)=g_eax;MEM32(g_esp+12)=0;if(g_eax){g_ecx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=g_eax;ENTITY_CALL(sub_001566D0,0x15688e);}else g_eax=0;g_ecx=MEM32(g_esp+4);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;}
 if(entry==0x1568b0){PUSH32(g_esp,g_esi);g_esi=g_ecx;ENTITY_CALL(sub_001568D0,0x1568b8);if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);ENTITY_CALL(sub_002D5CCD,0x1568c5);g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;}
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x393498);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;g_ecx=g_esi+0xd0;MEM32(g_esp+16)=0;ENTITY_CALL(sub_001FBA40,0x156900);g_ecx=g_esi;MEM32(g_esp+16)=0xffffffffu;ENTITY_CALL(sub_00218CE0,0x15690f);g_ecx=MEM32(g_esp+8);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
#undef ENTITY_VCALL
#undef ENTITY_FRAME_CALL
#undef ENTITY_CALL
