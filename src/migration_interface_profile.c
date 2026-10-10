#include "migration_interface_profile.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
static int profile_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_PROFILE");v=s&&!strcmp(s,"1");}return v;}
static int profile_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
int migration_interface_profile(unsigned op){
 if(!profile_enabled()||op>9)return 0;
 uint32_t s=g_esp,m=g_ecx;
 if((s&3)||s<0x10008||!profile_span(s-8,20))return 0;
 if(op>=6){
  if(op==8||op==9){g_eax=op==8?0x5152b0:0x515280;g_esp+=4;return 1;}
  if(!profile_span(0x515278,4))return 0;
  uint32_t owner=MEM32(0x515278);
  if(op==6){g_ecx=owner;g_eax=owner+12+MEM32(s+4)*0x19c;}
  else{if(!profile_span((uint64_t)owner+8,4))return 0;g_eax=MEM32(owner+8);}
  g_esp+=4;return 1;
 }
 /* Spills must not overwrite the profile while aggregating its live bytes. */
 if(!profile_span(m,0x110)||!((uint64_t)m+0x110<=s-8||(uint64_t)s+12<=m))return 0;
 if(op==1||op==4){
  uint32_t index=MEM32(s+4),bonus=MEM32(s+8);
  uint32_t offset=index==UINT32_MAX?0xe0+bonus*8:0x88+index*8;
  if(op==4&&((index==UINT32_MAX&&bonus>=6)||(index!=UINT32_MAX&&index>=11)))return 0;
  g_eax=op==1?m+offset:MEM32(m+offset);g_recomp_zf=index==UINT32_MAX;g_esp+=12;return 1;
 }
 if(op==3){uint32_t index=MEM32(s+4);if(index>=49)return 0;g_eax=index;SET_LO8(g_eax,MEM8(m+0x50+index));g_esp+=8;return 1;}
 if(op==0){
  g_edx=MEM32(m+0x14);g_eax=0;PUSH32(g_esp,g_esi);
  for(g_ecx=0;g_ecx<11;g_ecx++)g_eax+=(g_edx>>g_ecx)&1;
  POP32(g_esp,g_esi);
 }else if(op==2){
  g_eax=g_edx=0;PUSH32(g_esp,g_ebx);
  for(;g_edx<49;g_edx++)g_eax+=MEM8(g_ecx+0x50+g_edx)!=0;
  POP32(g_esp,g_ebx);
 }else{
  PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=0;g_ecx+=0x8e;
  for(g_edx=17;g_edx;g_edx--,g_ecx+=8)g_eax+=MEM8(g_ecx-2)+MEM8(g_ecx-1)+MEM8(g_ecx)+MEM8(g_ecx+1);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);
 }
 g_recomp_zf=1;g_esp+=4;return 1;
}
