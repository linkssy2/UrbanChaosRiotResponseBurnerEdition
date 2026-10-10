#include "migration_animation_masks_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int mask_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int mask_separate(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int mask_admit(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_MASKS");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,a=g_ecx;
 if(!enabled||(s&3)||s<0x10010||!mask_span(s-16,24)||!mask_span(a,8)||!mask_separate(a,8,s-16,24)||!mask_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t b=MEM32(s+4);if(!mask_span(b,8)||!mask_separate(b,8,s-16,24))return 0;
 const uint32_t masks[]={a,b};
 for(unsigned i=0;i<2;i++){
  uint32_t count=MEM32(masks[i]+4);
  if((int32_t)count<0){
   count=0u-count;
   /* ponytail: fixture bounds, not an original bone limit. */
   if(count<2||count>128)return 0;
   uint32_t data=MEM32(masks[i]);
   if(!mask_span(data,count*4)||!mask_separate(data,count*4,s-16,24)||!mask_separate(data,count*4,a,8)||!mask_separate(data,count*4,b,8))return 0;
  }
 }
 return 1;
}
/* True when the right mask has no bit outside the left mask. Inline masks
 * borrow their two words; negative descriptors point to an allocated array. */
int uc_migration_animation_mask_subset(void){
 if(!mask_admit())return 0;
 uint32_t frame=g_seh_ebp;unsigned result;
 g_eax=MEM32(g_ecx+4);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if((int32_t)g_eax<0){
  g_edi=MEM32(g_esp+16);g_esi=MEM32(g_edi+4);
  if((int32_t)g_esi<0){
   g_eax=0u-g_eax;g_edx=0;PUSH32(g_esp,frame);g_ebx=0u-g_esi;MEM32(g_esp+20)=g_ebx;
   do{
    g_ebx=MEM32(g_esp+20);if((int32_t)g_edx>=(int32_t)g_ebx)break;
    g_ebx=MEM32(g_ecx);g_ebx=MEM32(g_ebx+g_edx*4);frame=MEM32(g_edi);g_ebx=~g_ebx;
    if(MEM32(frame+g_edx*4)&g_ebx){result=0;goto array_done;}
    ++g_edx;
   }while((int32_t)g_edx<(int32_t)g_eax);
   g_esi=0u-g_esi;
   if((int32_t)g_edx<(int32_t)g_esi){
    g_eax=MEM32(g_edi)+g_edx*4;
    do{if(MEM32(g_eax)){result=0;goto array_done;}++g_edx;g_eax+=4;}while((int32_t)g_edx<(int32_t)g_esi);
   }
   result=1;
array_done:
   POP32(g_esp,frame);g_recomp_zf=result?(g_edx==g_esi):1;
  }else{
   g_ecx=MEM32(g_ecx);g_eax=MEM32(g_ecx+4);g_ecx=MEM32(g_ecx);g_ebx=MEM32(g_edi);
   g_eax=(~g_eax)&g_esi;g_ecx=(~g_ecx)&g_ebx;g_eax|=g_ecx;g_eax=g_eax?0:1;result=g_eax;g_recomp_zf=g_eax==0;
  }
 }else{
  g_esi=MEM32(g_esp+16);g_edx=MEM32(g_esi+4);g_eax=~g_eax;
  if((int32_t)g_edx<0){
   g_esi=MEM32(g_esi);g_edi=MEM32(g_esi+4);g_ebx=MEM32(g_esi);g_edi&=g_eax;g_eax=(~MEM32(g_ecx))&g_ebx;g_edi|=g_eax;
   if(g_edi){result=0;goto subset_fail;}
   g_eax=2;g_edx=0u-g_edx;
   if((int32_t)g_edx>(int32_t)g_eax){
    g_ecx=g_esi+8;
    do{if(MEM32(g_ecx)){result=0;goto subset_fail;}++g_eax;g_ecx+=4;}while((int32_t)g_eax<(int32_t)g_edx);
   }
   result=1;
  }else{
   g_ecx=MEM32(g_ecx);g_edi=MEM32(g_esi);g_eax&=g_edx;g_ecx=(~g_ecx)&g_edi;g_eax|=g_ecx;g_eax=g_eax?0:1;result=g_eax;g_recomp_zf=g_eax==0;
  }
 }
 goto subset_return;
subset_fail:g_recomp_zf=1;
subset_return:
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);SET_LO8(g_eax,result);POP32(g_esp,g_ebx);g_esp+=8;return 1;
}

int uc_migration_animation_mask_intersect(void){
 if(!mask_admit())return 0;
 g_eax=g_ecx;g_edx=MEM32(g_eax+4);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if((int32_t)g_edx<0){
  g_esi=MEM32(g_esp+12);g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_ebx);
  if((int32_t)g_ecx<0){
   g_edx=0u-g_edx;g_ecx=0;
   do{
    g_edx=0u-MEM32(g_esi+4);if((int32_t)g_ecx>=(int32_t)g_edx)break;
    g_edx=MEM32(g_eax);g_edi=MEM32(g_esi);g_ebx=MEM32(g_edx+g_ecx*4);g_edi=MEM32(g_edi+g_ecx*4);g_edx+=g_ecx*4;g_ebx&=g_edi;MEM32(g_edx)=g_ebx;
    g_edx=MEM32(g_eax+4);++g_ecx;g_edx=0u-g_edx;
   }while((int32_t)g_ecx<(int32_t)g_edx);
   /* Original 1F2757 compares against the negative descriptor. It deliberately
    * leaves the longer left tail unchanged in the heap/heap branch. */
  }else{
   g_ecx=MEM32(g_eax);g_edi=MEM32(g_ecx);g_edx=MEM32(g_esi);g_edi&=g_edx;MEM32(g_ecx)=g_edi;
   g_ecx=MEM32(g_eax);g_edx=MEM32(g_esi+4);MEM32(g_ecx+4)&=g_edx;g_edx=MEM32(g_eax+4);g_ecx=2;g_edx=0u-g_edx;
   if((int32_t)g_edx>(int32_t)g_ecx){
    do{g_edx=MEM32(g_eax);MEM32(g_edx+g_ecx*4)=0;g_edx=MEM32(g_eax+4);++g_ecx;g_edx=0u-g_edx;}while((int32_t)g_ecx<(int32_t)g_edx);
    g_recomp_zf=g_ecx==g_edx;
   }
  }
  POP32(g_esp,g_ebx);
 }else{
  g_ecx=MEM32(g_esp+12);g_esi=MEM32(g_ecx+4);g_edi=MEM32(g_eax);unsigned right_array=(int32_t)g_esi<0;g_esi=MEM32(g_ecx);
  if(right_array){g_esi=MEM32(g_esi);g_edi&=g_esi;MEM32(g_eax)=g_edi;g_ecx=MEM32(g_ecx);g_ecx=MEM32(g_ecx+4);g_ecx&=g_edx;POP32(g_esp,g_edi);MEM32(g_eax+4)=g_ecx;POP32(g_esp,g_esi);g_recomp_zf=g_ecx==0;g_esp+=8;return 1;}
  g_edi&=g_esi;MEM32(g_eax)=g_edi;g_edx&=MEM32(g_ecx+4);POP32(g_esp,g_edi);MEM32(g_eax+4)=g_edx;POP32(g_esp,g_esi);g_recomp_zf=g_edx==0;g_esp+=8;return 1;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;return 1;
}
