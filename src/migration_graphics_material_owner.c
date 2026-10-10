#include "migration_graphics_material_owner.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E7080(void),sub_0009C7A0(void),sub_0009C7E0(void),sub_0009C830(void),sub_0008EC20(void);
static int owner_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MATERIAL_OWNER");flag=v&&!strcmp(v,"1");}return flag;}
static int owner_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void owner_indirect(uint32_t table,unsigned slot,uint32_t pc,uint32_t saved){uint32_t target=MEM32(table+slot);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(target,saved);}
static void owner_device(unsigned slot,uint32_t pc){g_ecx=MEM32(0x519f60);g_eax=MEM32(g_ecx);owner_indirect(g_eax,slot,pc,g_esp);}
static void owner_result(unsigned slot,uint32_t pc){g_edx=MEM32(g_eax);g_ecx=g_eax;owner_indirect(g_edx,slot,pc,g_esp);}
static void owner_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void owner_store(uint32_t p){MEMF(p)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
static unsigned owner_compare(float x,float y){double a=x,b=y;return a!=a||b!=b?0x47:a<b?3:a>b?2:0x42;}
#define OWNER_CALL(fn,pc) do{g_ebp=frame;PUSH32(g_esp,pc);fn();}while(0)
/* Complete material transaction: select shaders, publish constants, dispatch
 * the original bytecode owner, and restore the exception chain and frame. */
int uc_migration_graphics_material_owner(uint32_t entry){
 if(!owner_enabled()||entry!=0x8ff30)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!owner_span(s-160,184)||!owner_span(g_ecx,80)||!owner_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7)return 0;
 if(!owner_span(MEM32(s+8),12))return 0;
 uint32_t frame=g_ebp,saved;int choose;unsigned flags;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=~15u;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x38c4de);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=72;
 g_eax=MEM32(frame+12);g_xmm0=XMM_SCALAR(MEMF(g_eax));g_xmm1=XMM_SCALAR(MEMF(g_eax+4));g_xmm2=XMM_SCALAR(MEMF(g_eax+8));PUSH32(g_esp,g_ebx);MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,g_esi);g_esi=g_ecx;choose=MEM8(g_esi+77)&2;MEMF(g_esp+32)=g_xmm1.f[0];MEMF(g_esp+36)=g_xmm2.f[0];MEMF(g_esp+40)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+28);PUSH32(g_esp,g_edi);XMM_STORE(g_esp+32,g_xmm0);
 if(choose){owner_device(0x60,0x8ff9f);owner_result(0x74,0x8ffa6);if(!LO8(g_eax))goto cleanup;}
 SET_LO16(g_eax,MEM16(g_esi+76));g_edi=1;choose=LO8(g_eax)&1;
 if(!choose){SET_LO8(g_ecx,MEM8(0x519f88));choose=LO8(g_ecx);if(!choose){SET_LO8(g_ecx,MEM8(0x519f89));choose=LO8(g_ecx);}}
 if(choose){g_ecx=0x40344;if((int8_t)LO8(g_eax)<0){g_edx=g_edi;OWNER_CALL(sub_003E7080,0x8ffdf);g_edx=g_edi;g_ecx=0x40348;MEM32(0x3f4098)=g_edi;OWNER_CALL(sub_003E7080,0x8fff1);MEM32(0x3f409c)=g_edi;}else{g_edx=0x302;OWNER_CALL(sub_003E7080,0x90003);g_edx=0x303;g_ecx=0x40348;MEM32(0x3f4098)=0x302;OWNER_CALL(sub_003E7080,0x9001c);MEM32(0x3f409c)=0x303;}}
 SET_LO16(g_eax,MEM16(g_esi+76));choose=0;
 if(LO8(g_eax)&0x30){SET_LO8(g_ecx,MEM8(g_esi+36));MEM8(g_esp+26)=1;choose=LO8(g_ecx)!=0;}
 if(!choose){SET_LO8(g_ecx,MEM8(g_esi+36));MEM8(g_esp+26)=0;choose=LO8(g_ecx)!=0;}
 if(choose&&!(LO8(g_eax)&0x20)){
  owner_device(0x30,0x90055);owner_result(0x30,0x9005c);owner_push(MEMF(0x44c174));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;SET_HI8(g_eax,g_fp_cmp==2?0x47:g_fp_cmp<0?3:g_fp_cmp>0?2:0x42);MEM8(g_esp+25)=RECOMP_PARITY8(HI8(g_eax)&0x44)?1:0;
 }else MEM8(g_esp+25)=0;
 SET_LO8(g_ecx,MEM8(0x519f88));g_ebx=MEM32(g_esi+40);MEM32(g_esp+28)=g_ebx;choose=LO8(g_ecx);if(!choose){SET_LO8(g_eax,MEM8(0x519f89));choose=LO8(g_eax);}
 if(choose){g_eax=MEM32(g_esi+28);if(g_eax){g_eax=MEM32(g_esi+32);g_eax=MEM32(g_eax+16);if(g_eax){g_ebx=g_eax;MEM32(g_esp+28)=g_ebx;MEM8(g_esp+26)=1;}}MEM8(g_esp+25)=0;}
 SET_LO8(g_eax,MEM8(g_esi+36));g_edi=MEM32(frame+16);choose=LO8(g_eax);if(!choose){choose=LO8(g_ecx);if(!choose){SET_LO8(g_eax,MEM8(0x519f89));choose=LO8(g_eax);}choose=choose&&g_ebx;}
 if(choose){g_edx=MEM32(g_ebx);g_ecx=g_ebx;owner_indirect(g_edx,0x24,0x900d3,g_esp);if(g_eax){g_eax=MEM32(g_edi);g_ecx=g_edi;owner_indirect(g_eax,0x14,0x900de,g_esp);g_edx=MEM32(g_eax);g_edi=MEM32(g_ebx);g_ecx=g_eax;owner_indirect(g_edx,12,0x900e7,g_esp);saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_ebx;owner_indirect(g_edi,32,0x900ed,saved);}}
 g_ecx=MEM32(frame+8);g_eax=MEM32(g_ecx);owner_indirect(g_eax,8,0x900f5,g_esp);
 SET_LO8(g_edx,MEM8(g_esi+44));g_eax=LO8(g_eax)!=0;g_ecx=MEM8(g_esi+76);g_ecx=(g_ecx>>1)&2;g_eax|=g_ecx;SET_LO8(g_ecx,MEM8(g_esp+25));g_edx=LO8(g_edx)?4:0;g_eax|=g_edx;SET_LO8(g_edx,MEM8(g_esp+26));g_ecx=LO8(g_ecx)?16:0;g_eax|=g_ecx;g_edx=LO8(g_edx)?8:0;g_eax|=g_edx;g_eax=MEM32(0x4eab70+g_eax*4);g_ecx=MEM32(g_eax);PUSH32(g_esp,g_ecx);OWNER_CALL(sub_0009C7A0,0x9013c);
 SET_LO8(g_eax,MEM8(g_esi+44));g_esp+=4;g_edi=0xffffffff;
 if(LO8(g_eax)){
  g_ecx=MEM32(g_esi+48);g_edx=MEM32(g_ecx);owner_indirect(g_edx,36,0x90155,g_esp);
  if(g_eax){g_ecx=MEM32(frame+16);g_eax=MEM32(g_ecx);owner_indirect(g_eax,16,0x90161,g_esp);g_ecx=MEM32(g_esi+48);g_ebx=MEM32(g_ecx);g_edx=MEM32(g_eax);g_ecx=g_eax;owner_indirect(g_edx,12,0x9016d,g_esp);g_ecx=MEM32(g_esi+48);saved=g_esp;PUSH32(g_esp,g_eax);owner_indirect(g_ebx,32,0x90174,saved);g_ebx=MEM32(g_esp+28);}
  g_ecx=MEM32(g_esi+48);g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,0);owner_indirect(g_eax,28,0x90182,saved);SET_LO8(g_eax,MEM8(g_esp+26));
  if(LO8(g_eax)){
   choose=MEM8(g_esi+76)&0x20;if(!choose){SET_LO8(g_eax,MEM8(0x519f88));choose=LO8(g_eax);}
   if(choose){g_ecx=MEM32(0x51a1e0);PUSH32(g_esp,g_ecx);goto select_shader;}
   SET_LO8(g_eax,MEM8(g_esp+25));if(LO8(g_eax)){g_ecx=MEM32(0x51a154);PUSH32(g_esp,g_ecx);}else{g_edx=MEM32(0x51a118);PUSH32(g_esp,g_edx);}goto select_texture_shader;
  }
  SET_LO8(g_eax,MEM8(g_esp+25));if(LO8(g_eax)){g_edx=MEM32(0x51a154);PUSH32(g_esp,g_edx);goto select_texture_shader;}
  g_ecx=MEM32(0x51a1b4);PUSH32(g_esp,g_ecx);OWNER_CALL(sub_0009C7E0,0x902f6);g_esp+=4;goto color_constants;
 }
 SET_LO8(g_eax,MEM8(g_esp+26));
 if(LO8(g_eax)){
  g_ecx=MEM32(g_esi+28);choose=g_ecx!=0;SET_LO8(g_eax,MEM8(0x519f89));MEM8(g_esp+27)=LO8(g_eax);
  if(choose&&LO8(g_eax)){g_edx=MEM32(g_esi+32);g_ecx=MEM32(g_edx+16);g_eax=MEM32(g_ecx);owner_indirect(g_eax,44,0x90329,g_esp);g_ecx=MEM32(0x519f60);if(g_eax!=MEM32(g_ecx+4))MEM8(g_esp+27)=0;}else{SET_LO8(g_edx,MEM8(0x519f88));MEM8(g_esp+27)=LO8(g_edx);}
  choose=MEM8(g_esi+76)&0x20;if(!choose){SET_LO8(g_eax,MEM8(g_esp+27));choose=LO8(g_eax);}
  if(choose){g_edx=MEM32(0x51a048);PUSH32(g_esp,g_edx);}else{SET_LO8(g_eax,MEM8(g_esp+25));if(LO8(g_eax)){g_eax=MEM32(0x51a1b0);PUSH32(g_esp,g_eax);}else{g_ecx=MEM32(0x51a220);PUSH32(g_esp,g_ecx);}}
  OWNER_CALL(sub_0009C7E0,0x90378);g_eax=MEM32(g_ebx);g_esp+=4;saved=g_esp;PUSH32(g_esp,0);g_ecx=g_ebx;owner_indirect(g_eax,28,0x90384,saved);g_edi=0;goto auxiliary_constants;
 }
 SET_LO8(g_eax,MEM8(g_esp+25));if(LO8(g_eax)){g_edx=MEM32(g_ebx);saved=g_esp;PUSH32(g_esp,0);g_ecx=g_ebx;owner_indirect(g_edx,28,0x9039c,saved);g_eax=MEM32(0x51a1b0);g_edi=0;PUSH32(g_esp,g_eax);goto select_shader;}
 g_ecx=MEM32(0x51a1d0);PUSH32(g_esp,g_ecx);OWNER_CALL(sub_0009C7E0,0x903b5);g_esp+=4;goto color_constants;
select_texture_shader:
 OWNER_CALL(sub_0009C7E0,0x902d4);g_eax=MEM32(g_ebx);g_esp+=4;saved=g_esp;PUSH32(g_esp,1);g_ecx=g_ebx;owner_indirect(g_eax,28,0x902e0,saved);g_edi=1;goto auxiliary_constants;
select_shader:
 OWNER_CALL(sub_0009C7E0,0x901c8);g_esp+=4;
auxiliary_constants:
 SET_LO8(g_eax,MEM8(g_esp+25));if(LO8(g_eax)){
  if(!(MEM8(0x519f90)&1)){MEM32(0x519f90)|=1;g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);MEM32(g_esp+92)=0;owner_indirect(g_edx,48,0x901f6,g_esp);owner_result(48,0x901fd);owner_store(0x519f8c);MEM32(g_esp+92)=0xffffffff;}
  g_xmm0=XMM_ZERO();g_xmm1=XMM_SCALAR(MEMF(0x44bf94));g_eax=g_esp+48;PUSH32(g_esp,g_eax);MEMF(g_esp+52)=g_xmm0.f[0];MEMF(g_esp+60)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(0x519f8c));PUSH32(g_esp,1);MEMF(g_esp+60)=g_xmm1.f[0];MEMF(g_esp+68)=g_xmm0.f[0];OWNER_CALL(sub_0009C830,0x90242);g_esp+=8;
 }
color_constants:
 g_xmm0=XMM_SCALAR(MEMF(g_esi+72));g_xmm3=XMM_ZERO();flags=owner_compare(g_xmm0.f[0],g_xmm3.f[0]);SET_HI8(g_eax,flags);
 if(RECOMP_PARITY8(HI8(g_eax)&0x44)){
  g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);owner_indirect(g_edx,64,0x90265,g_esp);g_xmm0=XMM_SCALAR(MEMF(g_esi+72));g_edx=MEM32(g_eax);g_ecx=g_eax;for(unsigned i=0;i<4;i++)MEMF(g_esp+48+i*4)=g_xmm0.f[0];owner_indirect(g_edx,16,0x90289,g_esp);owner_store(g_esp+28);g_xmm0=XMM_SCALAR(MEMF(g_esp+28));g_xmm1=XMM_MEM(g_esp+48);g_xmm2=XMM_MEM(0x4eac00);g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);g_xmm1=XMM_DIV(g_xmm1,g_xmm0);g_xmm0=XMM_MEM(g_esp+32);g_xmm0=XMM_MUL(g_xmm0,g_xmm2);g_eax=g_esp+64;g_xmm0=XMM_ADD(g_xmm0,g_xmm1);XMM_STORE(g_esp+64,g_xmm0);PUSH32(g_esp,g_eax);
 }else{
  SET_LO8(g_eax,MEM8(0x519f89));g_xmm0=XMM_MEM(g_esp+32);XMM_STORE(g_esp+32,g_xmm0);
  if(LO8(g_eax)){g_ecx=MEM32(0x51a180);g_xmm0=XMM_MEM(0x4eac00);PUSH32(g_esp,g_ecx);XMM_STORE(g_esp+36,g_xmm0);OWNER_CALL(sub_0009C7E0,0x903e8);g_esp+=4;}
  else{SET_LO8(g_eax,MEM8(0x519f88));if(LO8(g_eax)){
   g_xmm1=XMM_SCALAR(MEMF(0x4eac0c));g_xmm2=XMM_SCALAR(MEMF(0x44bf94));
   for(unsigned i=0;i<3;i++){
    if(i==2){g_xmm1.f[0]+=MEMF(g_esp+40);g_xmm0=g_xmm1;}else{g_xmm0=g_xmm1;g_xmm0.f[0]+=MEMF(g_esp+32+i*4);}
    if(owner_compare(g_xmm2.f[0],g_xmm0.f[0])&0x41)g_xmm0=g_xmm2;else if(owner_compare(g_xmm0.f[0],g_xmm3.f[0])&0x41)g_xmm0=g_xmm3;
    MEMF(g_esp+32+i*4)=g_xmm0.f[0];
   }
  }}
  g_edx=g_esp+32;PUSH32(g_esp,g_edx);
 }
 PUSH32(g_esp,2);OWNER_CALL(sub_0009C830,0x90475);g_esp+=8;
 if((int32_t)g_edi>=0){SET_LO8(g_eax,MEM8(g_esp+26));if(LO8(g_eax)){SET_LO8(g_eax,MEM8(0x519f89));if(!LO8(g_eax)){g_ecx=g_edi;g_eax=1u<<(LO8(g_ecx)&31);MEM32(0x3f3d98)|=g_eax;g_ecx=g_edi<<7;MEM32(g_ecx+0x3f3dcc)=4;}}}
 g_ecx=MEM32(0x519f60);g_edx=MEM32(g_ecx);owner_indirect(g_edx,80,0x904b6,g_esp);owner_result(48,0x904bd);g_eax=MEM32(g_esi+8);choose=!g_eax;if(choose){g_eax=MEM32(g_esi+12);choose=g_eax!=0;}
 if(choose){g_eax=MEM32(frame+8);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);}else{g_ecx=MEM32(frame+16);g_edx=MEM32(frame+8);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,1);PUSH32(g_esp,g_edx);}
 g_ecx=g_esi;OWNER_CALL(sub_0008EC20,0x904ea);
 if((int32_t)g_edi>=0){SET_LO8(g_eax,MEM8(g_esp+26));if(LO8(g_eax)){g_ecx=g_edi;g_eax=1u<<(LO8(g_ecx)&31);g_ecx=MEM32(0x3f3d98);g_ecx|=g_eax;g_edi<<=7;MEM32(0x3f3d98)=g_ecx;MEM32(g_edi+0x3f3dcc)=0;}}
cleanup:
 g_ecx=MEM32(g_esp+84);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=20;return 1;
}
#undef OWNER_CALL
