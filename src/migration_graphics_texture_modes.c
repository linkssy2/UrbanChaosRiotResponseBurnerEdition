#include "migration_graphics_texture_modes.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void);
static int modes_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_TEXTURE_MODES");flag=v&&!strcmp(v,"1");}return flag;}
static int modes_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static uint32_t dimension_mode(uint32_t*f){
 if(*f&4)return 3;
 if(*f&0x40000000)return 2;
 *f&=0xf0;return *f==0x30?2:1;
}
static uint32_t automatic_mode(uint32_t*f,unsigned stage){
 if((int32_t)*f<0)return 0;
 if(stage){uint32_t t=MEM32(0x3f3d50+128*stage);if(t==25)return 6;if(t==26)return 7;}
 return dimension_mode(f);
}
static uint32_t explicit_mode(uint32_t mode,uint32_t*f){
 if((int32_t)*f<0&&mode!=4&&mode!=5&&mode!=10&&mode!=17)return 0;
 if(mode>=1&&mode<=3)return dimension_mode(f);
 if(mode==13||mode==14){*f=13+((*f&4)!=0);return *f;}
 return mode;
}
int uc_migration_graphics_texture_modes(uint32_t entry){
 if(!modes_enabled()||entry!=0x3ed100)return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10100||!modes_span(s-16,28)||!modes_span(0x51c178,1)||MEM8(0x51c178)||!modes_span(MEM32(s+4),0x794)||!modes_span(0x3f3d50,0x184))return 0;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);
 if(!MEM32(g_edi+0x784)){
  uint32_t packed=0;
  for(int stage=3;stage>=0;stage--){g_ecx=MEM32(g_edi+12+4*stage);g_edx=automatic_mode(&g_ecx,(unsigned)stage);packed=(packed<<5)|g_edx;}
  g_esi=packed;
 }else{
  g_esi=MEM32(g_edi+0x790);
  if(MEM32(g_edi+0x78c)){
   uint32_t packed=0;
   for(int stage=3;stage>=0;stage--){uint32_t format=MEM32(g_edi+12+4*stage);g_ecx=explicit_mode((g_esi>>(5*stage))&31,&format);packed=(packed<<5)|g_ecx;}
   g_edx=packed;g_esi=packed;
  }
 }
 g_eax=MEM32(g_edi);if(g_eax>=MEM32(g_edi+4)){PUSH32(g_esp,0x3ed469);sub_003E9810();}
 MEM32(g_eax)=0x41e70;MEM32(g_eax+4)=g_esi;g_eax+=8;MEM32(g_edi)=g_eax;
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;return 1;
}
