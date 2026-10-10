#include "migration_animation_cache_reverse_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int cache_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int cache_separate(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int reverse_admit(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_CACHE_REVERSE");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;
 if(!enabled||(s&3)||s<0x1002c||!cache_span(s-44,52)||!cache_span(p,0x44)||!cache_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t cache=MEM32(s+4);if(!cache_span(cache,16))return 0;
 uint32_t n=MEM32(p+20),frames=MEM32(p+28),current=MEM32(cache),stride=(n+7)/8,bytes=(MEM32(p+64)&2)?6:4;
 /* ponytail: title-fixture caps; expand after proving larger asset lifetimes. */
 if(!n||n>256||frames<2||frames>4096||!current||current>=frames)return 0;
 uint32_t history=MEM32(cache+4),low=MEM32(cache+8),high=MEM32(cache+12),mask=MEM32(p+40),table=MEM32(p+44);
 const uint32_t writes[][2]={{s-44,52},{cache,16},{history,n*8},{low,n*bytes},{high,n*bytes}};
 const uint32_t reads[][2]={{p,0x44},{mask,frames*stride},{table,frames*4},{0x51c178,1}};
 for(unsigned i=0;i<5;i++){
  if(!cache_span(writes[i][0],writes[i][1]))return 0;
  for(unsigned j=0;j<i;j++)if(!cache_separate(writes[i][0],writes[i][1],writes[j][0],writes[j][1]))return 0;
  for(unsigned j=0;j<4;j++)if(!cache_span(reads[j][0],reads[j][1])||!cache_separate(writes[i][0],writes[i][1],reads[j][0],reads[j][1]))return 0;
 }
 unsigned remaining=0;uint8_t pending[256];
 for(unsigned i=0;i<n;i++){
  if(MEM32(history+i*8)>frames||MEM32(history+i*8+4)>frames)return 0;
  pending[i]=(uint8_t)(MEM32(history+i*8)>current-1);remaining+=pending[i];
 }
 /* Validate only source rows this reverse step will read. An unchanged cache
  * must not walk every keyframe in an immutable clip on each evaluation. */
 for(uint32_t f=current;remaining&&f;){
  --f;
  uint32_t keys=MEM32(table+f*4);if(!cache_span(keys,n*bytes))return 0;
  for(unsigned j=0;j<5;j++)if(!cache_separate(keys,n*bytes,writes[j][0],writes[j][1]))return 0;
  for(unsigned i=0;i<n;i++)if(pending[i]&&!(MEM8(mask+f*stride+i/8)&(1u<<(i&7)))){pending[i]=0;--remaining;}
 }
 return 1;
}

/* Reverse one interval: retain the old lower sample as the new upper sample
 * where its key lies ahead, then search backward for each missing lower key.
 * Packed 4/6-byte keys stay packed, and guest history/scratch writes stay exact. */
int uc_migration_animation_cache_reverse(void){
 if(!reverse_admit())return 0;
 uint32_t frame=g_seh_ebp;
 g_esp-=28;g_eax=MEM32(g_ecx+20)+7;g_edx=0;PUSH32(g_esp,g_ebx);g_edx=MEM32(g_esp+36);g_ebx=MEM32(g_edx);PUSH32(g_esp,frame);frame=g_eax;
 g_eax=MEM32(g_ecx+64)>>1;frame>>=3;SET_LO8(g_eax,LO8(g_eax)&1);--g_ebx;PUSH32(g_esp,g_esi);MEM32(g_edx)=g_ebx;PUSH32(g_esp,g_edi);
 g_edi=MEM32(g_ecx+20);g_ebx=g_esi=0;MEM32(g_esp+40)=frame;MEM8(g_esp+19)=LO8(g_eax);MEM32(g_esp+28)=g_ebx;MEM32(g_esp+48)=g_esi;
 do{
  g_edi=MEM32(g_edx+4);g_edi=MEM32(g_edi+g_esi*8);
  if((int32_t)g_edi>(int32_t)MEM32(g_edx)){
   frame=MEM32(g_edx+12);g_edi=MEM32(g_edx+8);
   if(LO8(g_eax)){
    g_eax=MEM32(g_esp+48);g_edi+=g_eax;frame+=g_eax;g_eax=MEM32(g_edi);MEM32(frame)=g_eax;SET_LO16(g_eax,MEM16(g_edi+4));MEM16(frame+4)=LO16(g_eax);SET_LO8(g_eax,MEM8(g_esp+19));
   }else{g_edi=MEM32(g_edi+g_esi*4);MEM32(frame+g_esi*4)=g_edi;}
   g_edi=MEM32(g_edx+4);frame=MEM32(g_edi+g_esi*8);g_edi+=g_esi*8;MEM32(g_edi+4)=frame;frame=MEM32(g_esp+40);++g_ebx;
  }
  MEM32(g_esp+48)+=6;g_edi=MEM32(g_ecx+20);++g_esi;
 }while((int32_t)g_esi<(int32_t)g_edi);
 MEM32(g_esp+28)=g_ebx;g_esi=MEM32(g_edx);g_edi=g_esi*frame+MEM32(g_ecx+40);MEM32(g_esp+20)=g_esi;
 while((int32_t)g_ebx>0&&(int32_t)g_esi>=0){
  g_eax=MEM32(g_ecx+44);frame=MEM32(g_esp+20);frame=MEM32(g_eax+frame*4);g_eax=MEM32(g_ecx+20);g_esi=0;
  MEM8(g_esp+48)=0;MEM32(g_esp+24)=g_esi;MEM32(g_esp+32)=frame;MEM32(g_esp+36)=g_esi;
  do{
   MEM8(g_esp+48)=(uint8_t)((int8_t)MEM8(g_esp+48)>>1);g_eax=MEM32(g_esp+24)-1;MEM32(g_esp+24)=g_eax;
   if((int32_t)g_eax<=0){SET_LO8(g_eax,MEM8(g_edi));MEM8(g_esp+48)=LO8(g_eax);++g_edi;MEM32(g_esp+24)=8;}
   if(!(MEM8(g_esp+48)&1)){
    g_eax=MEM32(g_edx+4);g_eax=MEM32(g_eax+g_esi*8);
    if((int32_t)g_eax>(int32_t)MEM32(g_edx)){
     SET_LO8(g_eax,MEM8(g_esp+19));unsigned wide=LO8(g_eax);g_eax=MEM32(g_edx+8);
     if(wide){g_eax+=MEM32(g_esp+36);g_ebx=MEM32(frame);MEM32(g_eax)=g_ebx;SET_LO16(g_ebx,MEM16(frame+4));frame=MEM32(g_esp+32);MEM16(g_eax+4)=LO16(g_ebx);}
     else{g_ebx=MEM32(frame);MEM32(g_eax+g_esi*4)=g_ebx;}
     g_ebx=MEM32(g_esp+20);g_eax=MEM32(g_edx+4);MEM32(g_eax+g_esi*8)=g_ebx;g_ebx=MEM32(g_esp+28)-1;MEM32(g_esp+28)=g_ebx;
    }
    SET_LO8(g_eax,MEM8(g_esp+19));frame+=LO8(g_eax)?6:4;MEM32(g_esp+32)=frame;
   }
   MEM32(g_esp+36)+=6;g_eax=MEM32(g_ecx+20);++g_esi;
  }while((int32_t)g_esi<(int32_t)g_eax);
  g_eax=0u-MEM32(g_esp+40);g_esi=MEM32(g_esp+20)-1;g_edi+=g_eax*2;MEM32(g_esp+20)=g_esi;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
