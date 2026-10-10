/* Guest-layout counterpart to collision_contact_manager.c: whole default
 * zero-contact manager teardown. The original world and registered callbacks
 * retain authority; no shadow graph and no post-effect fallback exist. */
#include "hybrid_contact_manager_guest.h"
#include <stdlib.h>
#include <string.h>
#include "recomp_types.h"
#include <stdint.h>
#include "xbox_memory_layout.h"
static int owner_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_CONTACT_MANAGER");flag=v&&!strcmp(v,"1");}return flag;}
static int owner_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int owner_apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int owner_admit(void){
 uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10400u||!owner_span(s-1024,1040)||!owner_span(p,0x9c)||!owner_apart(p,0x9c,s-1024,1040)||!owner_span(0x51c178,1)||MEM8(0x51c178)||!owner_span(0x51e6f4,4))return 0;
 if(MEM32(s)!=0x2fbf9a||MEM32(s+4)!=1||MEM32(p)!=0x49abfc||MEM32(p+0x74)!=0x4990bc||MEM32(p+0x10)!=0x49b4ec||MEM16(p+4)<0x9c||MEM32(p+0x4c))return 0;
 uint32_t alloc=MEM32(0x51e6f4);if(!owner_span(alloc,4)||!owner_span(MEM32(alloc),24))return 0;
 const uint32_t offsets[3]={0x48,0x30,0x1c},strides[3]={20,32,1},ref_offsets[3]={0x84,0x88,0x80};
 uint32_t refs[3],ptrs[3],bytes[3];
 for(unsigned i=0;i<3;i++){
  uint32_t a=p+offsets[i],flags=MEM32(a+8),cap=flags&0x3fffffffu;refs[i]=MEM32(p+ref_offsets[i]);ptrs[i]=MEM32(a);bytes[i]=0;
  if(MEM32(a+4)>cap)return 0;
  if(!(flags&0x80000000u)){
   if(cap>UINT32_MAX/strides[i])return 0;bytes[i]=cap*strides[i];
   if(bytes[i]&&(!owner_span(ptrs[i],bytes[i])||!owner_apart(ptrs[i],bytes[i],p,0x9c)||!owner_apart(ptrs[i],bytes[i],s-1024,1040)))return 0;
   for(unsigned j=0;j<i;j++)if(bytes[i]&&bytes[j]&&!owner_apart(ptrs[i],bytes[i],ptrs[j],bytes[j]))return 0;
  }
  if(!refs[i]){if(i==2)return 0;continue;}
  if(!owner_span(refs[i],8)||!owner_apart(refs[i],8,s-1024,1040))return 0;
  if(refs[i]>=p&&refs[i]<p+0x9c){if(refs[i]!=p+0x10||MEM16(refs[i]+4))return 0;}
  else if(!owner_apart(refs[i],8,p,0x9c))return 0;
  if(MEM16(refs[i]+4)&&!owner_span(MEM32(refs[i]),4))return 0;
 }
 for(unsigned i=0;i<3;i++)if(refs[i]){
  unsigned occurrences=0;for(unsigned j=0;j<3;j++)occurrences+=refs[j]==refs[i];
  /* Initial aliases cannot become stale before their final release. Later
   * callback changes obey the same lifetime obligation as original code. */
  if(MEM16(refs[i]+4)&&MEM16(refs[i]+6)&&MEM16(refs[i]+6)<occurrences)return 0;
  for(unsigned j=0;j<3;j++)if(bytes[j]&&!owner_apart(refs[i],8,ptrs[j],bytes[j]))return 0;
 }
 return 1;
}
static void owner_constraint_destroy(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+0x10);MEM32(g_esi)=0x4990bc;
 if(g_ecx&&MEM16(g_ecx+4)){MEM16(g_ecx+6)=(uint16_t)(MEM16(g_ecx+6)-1);if(!MEM16(g_ecx+6)){
  g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2f0f48);RECOMP_ICALL_SAFE(target,saved);}}
 g_ecx=MEM32(g_esi+0x14);
 if(g_ecx&&MEM16(g_ecx+4)){MEM16(g_ecx+6)=(uint16_t)(MEM16(g_ecx+6)-1);if(!MEM16(g_ecx+6)){
  g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x2f0f67);RECOMP_ICALL_SAFE(target,saved);}}
 g_ecx=MEM32(g_esi+0xc);
 if(MEM16(g_ecx+4)){MEM16(g_ecx+6)=(uint16_t)(MEM16(g_ecx+6)-1);if(!MEM16(g_ecx+6)){
  g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2f0f82);RECOMP_ICALL_SAFE(target,saved);}}
 MEM32(g_esi)=0x4564ec;POP32(g_esp,g_esi);g_esp+=4;
}
static void owner_data_destroy(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;
 const unsigned offsets[3]={0x40,0x28,0x14},returns[3]={0x2fc727,0x2fc748,0x2fc766};
 for(unsigned i=0;i<3;i++){
  g_eax=MEM32(g_esi+offsets[i]);if((int32_t)g_eax<0)continue;
  g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;
  if(i==0){g_eax&=0x3fffffffu;g_eax+=g_eax*4;PUSH32(g_esp,0x14);g_eax<<=2;}
  else if(i==1){g_eax&=0x3fffffffu;PUSH32(g_esp,0x14);g_eax<<=5;}
  else{PUSH32(g_esp,0x14);g_eax&=0x3fffffffu;}
  PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+offsets[i]-8);PUSH32(g_esp,g_eax);
  uint32_t target=MEM32(g_edx+0x14);PUSH32(g_esp,returns[i]);RECOMP_ICALL_SAFE(target,saved);
 }
 MEM32(g_esi)=0x4564ec;POP32(g_esp,g_esi);g_esp+=4;
}
int uc_hybrid_contact_manager_destroy(void){
 if(!owner_enabled()||!owner_admit())return 0;
 PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x4c);MEM32(g_esi)=0x49abfc;
 /* Zero count is admitted before the first effect. Installed constraint
  * removal remains wholly original until its live owner is qualified. */
 g_ecx=g_esi+0x74;PUSH32(g_esp,0x2fc81a);owner_constraint_destroy();
 g_ecx=g_esi+0x10;PUSH32(g_esp,0x2fc822);owner_data_destroy();
 unsigned deleting=MEM8(g_esp+8)&1u;MEM32(g_esi)=0x4564ec;
 if(deleting){g_eax=MEM16(g_esi+4);g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;
  PUSH32(g_esp,0x1f);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_edx+0x14);PUSH32(g_esp,0x2fc842);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=!deleting;g_esp+=8;return 1;
}
