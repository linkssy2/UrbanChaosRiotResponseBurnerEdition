#include "migration_animation_asset_queries_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F2D30(void);
static int query_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int query_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int query_gate(unsigned depth,unsigned args){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_ASSET_QUERIES");enabled=v&&!strcmp(v,"1");}
 return enabled&&!(g_esp&3)&&g_esp>=0x10000+depth&&query_span(g_esp-depth,depth+args)&&query_span(0x51c178,1)&&!MEM8(0x51c178);
}
int uc_migration_animation_key_mask_count(void){
 if(!query_gate(8,12))return 0;uint32_t n=MEM32(g_esp+8),p=MEM32(g_esp+4);
 /* ponytail: bounded asset-sized masks; larger byte streams retain fallback. */
 if(n>1048576||(n&&(!query_span(p,n)||!query_apart(p,n,g_esp-8,20))))return 0;
 PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_eax=0;
 if(g_edi){
  PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);
  do{unsigned b=MEM8(g_esi);b=(b&0x55)+((b>>1)&0x55);b=(b&0x33)+((b>>2)&0x33);b=(b&15)+(b>>4);SET_LO8(g_ecx,b);g_edx=b;g_eax+=b;++g_esi;}while(--g_edi);
  POP32(g_esp,g_esi);
 }
 POP32(g_esp,g_edi);g_esp+=4;return 1;
}
static unsigned query_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
/* Last authored state at or before phase. Keep the original early stop and
 * four-record traversal boundaries; unsorted records are never reordered. */
int uc_migration_animation_asset_state(void){
 if(!query_gate(16,12)||!query_span(g_ecx,120)||!query_apart(g_ecx,120,g_esp-16,28))return 0;
 uint32_t count=MEM32(g_ecx+112),records=MEM32(g_ecx+116);
 if(count>65536||(count&&(!query_span(records,count*8)||!query_apart(records,count*8,g_esp-16,28))))return 0;
 uint32_t frame=g_seh_ebp;g_xmm0=XMM_SCALAR(MEMF(g_esp+8));PUSH32(g_esp,g_ebx);SET_LO16(g_ebx,MEM16(g_esp+8));PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_ecx+112);SET_LO8(g_eax,0);g_esi=0;
 if((int32_t)g_edi>=4){
  g_edx=MEM32(g_ecx+116)+2;
  do{
   for(unsigned j=0;j<4;j++){
    if(query_compare(g_xmm0.f[0],MEMF(g_edx+2+j*8))&1)goto state_done;
    if(MEM16(g_edx-2+j*8)==LO16(g_ebx))SET_LO8(g_eax,MEM16(g_edx+j*8)!=0);
   }
   g_esi+=4;frame=g_edi-3;g_edx+=32;
  }while((int32_t)g_esi<(int32_t)frame);
 }
 if((int32_t)g_esi<(int32_t)g_edi){
  g_ecx=MEM32(g_ecx+116)+g_esi*8;
  do{
   if(query_compare(g_xmm0.f[0],MEMF(g_ecx+4))&1)break;
   if(MEM16(g_ecx)==LO16(g_ebx))SET_LO8(g_eax,MEM16(g_ecx+2)!=0);
   ++g_esi;g_ecx+=8;
  }while((int32_t)g_esi<(int32_t)g_edi);
 }
state_done:
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=12;return 1;
}
int uc_migration_animation_controller_state(void){
 if(!query_gate(32,8)||!query_span(g_ecx,52)||!query_apart(g_ecx,52,g_esp-32,40))return 0;
 g_eax=MEM32(g_ecx+16);g_edx=MEM32(g_esp+4);g_ecx=MEM32(g_ecx+48);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x1f2371);sub_001F2D30();g_esp+=8;return 1;
}
