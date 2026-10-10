#include "hybrid_particle_material_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E7080(void);
extern void sub_003E7450(void);
static int material_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_MATERIAL");flag=v&&!strcmp(v,"1");}return flag;}
static int material_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int material_admit(void){
 uint32_t s=g_esp;
 return material_enabled()&&!(s&3)&&s>=0x10400u&&material_span(s-1024,1040)&&
  material_span(0x3f3d98,0x600)&&material_span(0x51c178,1)&&!MEM8(0x51c178)&&
  ((uint64_t)s+16<=0x3f3d98u||s-1024>=0x3f4398u)&&
  ((uint64_t)s+16<=0x51c178u||s-1024>=0x51c179u);
}
static void material_emit(uint32_t method,uint32_t value,uint32_t ret){
 g_ecx=method;g_edx=value;PUSH32(g_esp,ret);sub_003E7080();
}
int uc_hybrid_particle_material_setup(void){
 if(!material_admit())return 0;
 uint32_t mode=MEM32(g_esp+8),target=0;
 if(mode<=6){
  if(!material_span(0x99c18,28)||!((uint64_t)g_esp+16<=0x99c18u||g_esp-1024>=0x99c34u))return 0;
  target=MEM32(0x99c18+mode*4);
  switch(target){case 0x999dd:case 0x99a06:case 0x99a48:case 0x99a8a:case 0x99ae2:case 0x99af9:case 0x99b63:break;default:return 0;}
 }
 /* Follow the live source table, including valid redirected entries. The source
  * argument at +4 is unused. No rejection after these saved-register writes. */
 g_eax=mode;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=0;g_esi=1;
 switch(target){
 case 0x999dd:
  material_emit(0x40304,0,0x999e9);g_edx=0;g_ecx=0x40300;MEM32(0x3f408c)=g_edi;
  PUSH32(g_esp,0x999fb);sub_003E7080();MEM32(0x3f4090)=g_edi;break;
 case 0x99a06:case 0x99a48:
  material_emit(0x40304,g_esi,target==0x99a06?0x99a12:0x99a54);
  g_edx=0;g_ecx=0x40300;MEM32(0x3f408c)=g_esi;
  PUSH32(g_esp,target==0x99a06?0x99a24:0x99a66);sub_003E7080();
  g_edx=target==0x99a06?0x300:0x301;g_ecx=0x40344;MEM32(0x3f4090)=g_edi;
  PUSH32(g_esp,target==0x99a06?0x99a39:0x99a7b);sub_003E7080();
  MEM32(0x3f4098)=target==0x99a06?0x300:0x301;
  if(target==0x99a48)goto subtract;goto additive;
 case 0x99a8a:case 0x99af9:
  material_emit(0x40304,g_esi,target==0x99a8a?0x99a96:0x99b05);
  g_edx=g_esi;g_ecx=0x40300;MEM32(0x3f408c)=g_esi;
  PUSH32(g_esp,target==0x99a8a?0x99aa8:0x99b17);sub_003E7080();
  g_edx=0x302;g_ecx=0x40344;MEM32(0x3f4090)=g_esi;
  PUSH32(g_esp,target==0x99a8a?0x99abd:0x99b2c);sub_003E7080();
  if(target==0x99af9){MEM32(0x3f4098)=0x302;goto additive;}
  g_edx=0x303;g_ecx=0x40348;MEM32(0x3f4098)=0x302;
  PUSH32(g_esp,0x99ad6);sub_003E7080();MEM32(0x3f409c)=0x303;goto add_operation;
 case 0x99ae2:material_emit(0x40304,0,0x99aee);MEM32(0x3f408c)=g_edi;break;
 case 0x99b63:
  material_emit(0x40300,g_esi,0x99b6f);g_edx=g_esi;g_ecx=0x40304;MEM32(0x3f4090)=g_esi;
  PUSH32(g_esp,0x99b81);sub_003E7080();g_edx=0x302;g_ecx=0x40344;MEM32(0x3f408c)=g_esi;
  PUSH32(g_esp,0x99b96);sub_003E7080();MEM32(0x3f4098)=0x302;
 subtract:
  material_emit(0x40348,g_esi,0x99bac);g_ecx=0x40350;g_edx=0x800b;MEM32(0x3f409c)=g_esi;
  PUSH32(g_esp,0x99bc1);sub_003E7080();MEM32(0x3f40c8)=0x800b;break;
 additive:
  material_emit(0x40348,g_esi,0x99b42);MEM32(0x3f409c)=g_esi;
 add_operation:
  material_emit(0x40350,0x8006,0x99b57);MEM32(0x3f40c8)=0x8006;break;
 default:break;
 }
 PUSH32(g_esp,g_edi);PUSH32(g_esp,0x99bd1);sub_003E7450();
 material_emit(0x4033c,0x206,0x99be0);g_edx=g_esi;g_ecx=0x40340;MEM32(0x3f4088)=0x206;
 PUSH32(g_esp,0x99bf6);sub_003E7080();g_eax=MEM32(0x3f3d98);g_eax|=g_esi;
 POP32(g_esp,g_edi);MEM32(0x3f4094)=g_esi;MEM32(0x3f3d98)=g_eax;MEM32(0x3f3dcc)=4;
 POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=12;return 1;
}
int uc_hybrid_particle_material_cleanup(void){
 if(!material_admit())return 0;
 g_edx=MEM32(0x3f3d98);g_edx|=0x2000;PUSH32(g_esp,g_esi);MEM32(0x3f3d98)=g_edx;
 g_esi=0;g_edx=0;g_ecx=0x40304;MEM32(0x3f4110)=g_esi;PUSH32(g_esp,0x99c67);sub_003E7080();
 g_edx=1;g_ecx=0x40344;MEM32(0x3f408c)=g_esi;PUSH32(g_esp,0x99c7c);sub_003E7080();
 g_edx=0;g_ecx=0x40348;MEM32(0x3f4098)=1;PUSH32(g_esp,0x99c92);sub_003E7080();
 g_edx=0x8006;g_ecx=0x40350;MEM32(0x3f409c)=g_esi;PUSH32(g_esp,0x99ca7);sub_003E7080();
 PUSH32(g_esp,0x900);MEM32(0x3f40c8)=0x8006;PUSH32(g_esp,0x99cbb);sub_003E7450();
 material_emit(0x40300,0,0x99cc7);g_eax=MEM32(0x3f3d98);g_eax|=1;
 MEM32(0x3f4090)=g_esi;MEM32(0x3f3dcc)=g_esi;MEM32(0x3f3d98)=g_eax;
 POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;return 1;
}
