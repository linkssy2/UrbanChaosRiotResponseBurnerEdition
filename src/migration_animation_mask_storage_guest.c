#include "migration_animation_mask_storage_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void);
extern void sub_002D5CCD(void);
extern void sub_001F2450(void);
extern void sub_001F2710(void);
static int storage_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int storage_separate(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int storage_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_MASK_STORAGE");flag=v&&!strcmp(v,"1");}return flag;}
static int storage_base(unsigned below,unsigned above){return storage_enabled()&&!(g_esp&3)&&g_esp>=0x10000u+below&&storage_span(g_esp-below,below+above)&&storage_span(0x51c178,1)&&!MEM8(0x51c178);}
static int storage_mask(uint32_t p,unsigned below,unsigned above){
 if(!storage_span(p,8)||!storage_separate(p,8,g_esp-below,below+above))return 0;
 int32_t descriptor=SMEM32(p+4);
 if(descriptor<0){
  uint32_t words=0u-(uint32_t)descriptor,data=MEM32(p);
  /* ponytail: retain the qualified 128-word mask ceiling. */
  if(words<2||words>128||!storage_span(data,words*4)||!storage_separate(data,words*4,p,8)||!storage_separate(data,words*4,g_esp-below,below+above))return 0;
 }
 return 1;
}
/* Allocator/free remain synchronous services boundaries. Their valid returned
 * storage and callback ABI are inherited; there is no replay after commitment. */
int uc_migration_animation_mask_copy(void){
 if(!storage_base(16,8)||!storage_span(g_ecx,8)||!storage_separate(g_ecx,8,g_esp-16,24)||!storage_mask(MEM32(g_esp+4),16,8))return 0;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_eax=MEM32(g_edi+4);g_esi=g_ecx;
 if((int32_t)g_eax<0){
  MEM32(g_esi+4)=g_eax;g_eax=(0u-g_eax)<<2;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f246d);sub_002D57FB();
  g_ecx=MEM32(g_esi+4);MEM32(g_esi)=g_eax;g_ecx=0u-g_ecx;g_esp+=4;g_eax=0;
  if((int32_t)g_ecx>0){do{g_edx=MEM32(g_edi);g_ecx=MEM32(g_esi);g_edx=MEM32(g_edx+g_eax*4);MEM32(g_ecx+g_eax*4)=g_edx;g_ecx=MEM32(g_esi+4);++g_eax;g_ecx=0u-g_ecx;}while((int32_t)g_eax<(int32_t)g_ecx);g_recomp_zf=g_eax==g_ecx;}
 }else{g_edx=MEM32(g_edi);MEM32(g_esi)=g_edx;g_eax=MEM32(g_edi+4);MEM32(g_esi+4)=g_eax;}
 POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;return 1;
}
int uc_migration_animation_mask_init(void){
 if(!storage_base(12,8)||!storage_span(g_ecx,8)||!storage_separate(g_ecx,8,g_esp-12,20)||SMEM32(g_esp+4)>4096)return 0;
 g_eax=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_esi=g_ecx;
 if((int32_t)g_eax>63){
  g_eax=(g_eax+31)>>5;g_edx=0;g_eax=0u-g_eax;MEM32(g_esi+4)=g_eax;g_eax=(0u-g_eax)<<2;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f24d8);sub_002D57FB();
  g_ecx=MEM32(g_esi+4);MEM32(g_esi)=g_eax;g_ecx=0u-g_ecx;g_esp+=4;g_eax=0;
  if((int32_t)g_ecx>0){do{g_edx=MEM32(g_esi);MEM32(g_edx+g_eax*4)=0;g_ecx=MEM32(g_esi+4);++g_eax;g_ecx=0u-g_ecx;}while((int32_t)g_eax<(int32_t)g_ecx);g_recomp_zf=g_eax==g_ecx;}
 }else{MEM32(g_esi+4)=0;MEM32(g_esi)=0;}
 POP32(g_esp,g_esi);g_esp+=8;return 1;
}
static int storage_bit(unsigned above){
 if(!storage_base(8,above)||!storage_mask(g_ecx,8,above))return 0;
 uint32_t bit=MEM32(g_esp+4),desc=MEM32(g_ecx+4),bits=(int32_t)desc<0?(0u-desc)*32:64;
 return bit<bits;
}
int uc_migration_animation_mask_set(void){
 if(!storage_bit(12))return 0;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+4);
 if((int32_t)g_eax<0){g_eax=MEM32(g_esp+8);g_ecx=g_eax;g_edx=MEM32(g_esi);g_eax>>=5;g_esi=MEM32(g_edx+g_eax*4);g_eax=g_edx+g_eax*4;g_ecx&=31;g_edx=(~(1u<<g_ecx))&g_esi;g_esi=MEM8(g_esp+12);g_esi<<=g_ecx;g_edx|=g_esi;MEM32(g_eax)=g_edx;g_recomp_zf=g_edx==0;}
 else{
  g_ecx=MEM32(g_esp+8);
  if(g_ecx<32){g_edx=MEM32(g_esi);g_eax=(~(1u<<g_ecx))&g_edx;g_edx=MEM8(g_esp+12);g_edx<<=g_ecx;g_eax|=g_edx;MEM32(g_esi)=g_eax;g_recomp_zf=g_eax==0;}
  else{g_ecx&=31;g_edx=(~(1u<<g_ecx))&g_eax;g_eax=MEM8(g_esp+12);g_eax<<=g_ecx;g_edx|=g_eax;MEM32(g_esi+4)=g_edx;g_recomp_zf=g_edx==0;}
 }
 POP32(g_esp,g_esi);g_esp+=12;return 1;
}
int uc_migration_animation_mask_get(void){
 if(!storage_bit(8))return 0;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_edx=MEM32(g_esi+4);
 if((int32_t)g_edx<0){g_eax=MEM32(g_esp+8);g_ecx=g_eax;g_edx=0;PUSH32(g_esp,g_edi);g_ecx&=31;g_edi=1u<<g_ecx;g_ecx=MEM32(g_esi);g_eax>>=5;unsigned present=(MEM32(g_ecx+g_eax*4)&g_edi)!=0;POP32(g_esp,g_edi);SET_LO8(g_eax,present);g_recomp_zf=!present;}
 else{g_ecx=MEM32(g_esp+8);g_eax=1;if(g_ecx<32){g_eax<<=g_ecx;g_ecx=MEM32(g_esi);g_eax&=g_ecx;}else{g_ecx&=31;g_eax<<=g_ecx;g_eax&=g_edx;}unsigned present=g_eax!=0;SET_LO8(g_eax,present);g_recomp_zf=!present;}
 POP32(g_esp,g_esi);g_esp+=8;return 1;
}
int uc_migration_animation_mask_common(void){
 if(!storage_base(64,16)||!storage_span(MEM32(g_esp+4),8)||!storage_separate(MEM32(g_esp+4),8,g_esp-64,80)||!storage_mask(MEM32(g_esp+8),64,16)||!storage_mask(MEM32(g_esp+12),64,16))return 0;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b8b1);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=12;g_eax=MEM32(g_esp+32);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_ecx=g_esp+12;MEM32(g_esp+8)=0;PUSH32(g_esp,0x1f281f);sub_001F2450();
 g_ecx=MEM32(g_esp+40);PUSH32(g_esp,g_ecx);g_ecx=g_esp+12;MEM32(g_esp+28)=1;PUSH32(g_esp,0x1f2835);sub_001F2710();
 g_esi=MEM32(g_esp+32);g_edx=g_esp+8;PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,0x1f2845);sub_001F2450();
 g_eax=MEM32(g_esp+12);MEM32(g_esp+4)=1;MEM8(g_esp+24)=0;if((int32_t)g_eax<0){g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f2864);sub_002D5CCD();g_esp+=4;}
 g_ecx=MEM32(g_esp+16);g_eax=g_esi;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
