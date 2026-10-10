#include "migration_animation_cache_seek_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int seek_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int seek_separate(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int seek_admit(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_CACHE_SEEK");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;
 if(!enabled||(s&3)||s<0x10028||!seek_span(s-40,52)||!seek_span(p,0x44)||!seek_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t cache=MEM32(s+4);if(!seek_span(cache,16))return 0;
 uint32_t n=MEM32(p+20),frames=MEM32(p+28),key=MEM32(s+8),stride=(n+7)/8,bytes=(MEM32(p+64)&2)?6:4;
 /* ponytail: title-fixture caps; expand after proving larger asset lifetimes. */
 if(!n||n>256||frames<2||frames>4096||key>=frames)return 0;
 uint32_t history=MEM32(cache+4),low=MEM32(cache+8),high=MEM32(cache+12),mask=MEM32(p+40),table=MEM32(p+44);
 const uint32_t writes[][2]={{s-40,52},{cache,16},{history,n*8},{low,n*bytes},{high,n*bytes}};
 const uint32_t reads[][2]={{p,0x44},{mask,frames*stride},{table,frames*4},{0x51c178,1}};
 for(unsigned i=0;i<5;i++){
  if(!seek_span(writes[i][0],writes[i][1]))return 0;
  for(unsigned j=0;j<i;j++)if(!seek_separate(writes[i][0],writes[i][1],writes[j][0],writes[j][1]))return 0;
  for(unsigned j=0;j<4;j++)if(!seek_span(reads[j][0],reads[j][1])||!seek_separate(writes[i][0],writes[i][1],reads[j][0],reads[j][1]))return 0;
 }
 unsigned remaining=0;uint8_t pending[256];
 for(unsigned i=0;i<n;i++){uint32_t lower=MEM32(history+i*8),upper=MEM32(history+i*8+4);if(lower>frames||upper>frames)return 0;pending[i]=(uint8_t)(lower>key||upper<=key);remaining+=pending[i];}
 for(uint32_t f=key+1;remaining&&f;){
  --f;uint32_t keys=MEM32(table+f*4);if(!seek_span(keys,n*bytes))return 0;
  for(unsigned j=0;j<5;j++)if(!seek_separate(keys,n*bytes,writes[j][0],writes[j][1]))return 0;
  for(unsigned i=0;i<n;i++)if(pending[i]&&!(MEM8(mask+f*stride+i/8)&(1u<<(i&7)))){pending[i]=0;--remaining;}
 }
 if(key+1<frames){uint32_t keys=MEM32(table+(key+1)*4);if(!seek_span(keys,n*bytes))return 0;for(unsigned j=0;j<5;j++)if(!seek_separate(keys,n*bytes,writes[j][0],writes[j][1]))return 0;}
 return 1;
}

/* Seek preserves tracks whose existing interval already contains the target.
 * Rebuild only the invalid histories: search backward for lower payloads, use
 * the next row for upper payloads, then search forward for missing bounds.
 * Omitted next-row keys copy the current lower payload even on a warm seek. */
int uc_migration_animation_cache_seek(void){
 if(!seek_admit())return 0;
 uint32_t frame=g_seh_ebp;
 g_esp-=24;g_eax=MEM32(g_ecx+20)+7;g_edx=0;g_edx=MEM32(g_esp+28);g_eax>>=3;MEM32(g_esp+20)=g_eax;g_eax=MEM32(g_ecx+64)>>1;SET_LO8(g_eax,LO8(g_eax)&1);
 PUSH32(g_esp,g_ebx);MEM8(g_esp+7)=LO8(g_eax);g_eax=MEM32(g_esp+36);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);MEM32(g_edx)=g_eax;g_eax=MEM32(g_ecx+20);g_esi=0;PUSH32(g_esp,g_edi);MEM8(g_esp+18)=0;MEM32(g_esp+28)=g_esi;
 do{
  g_eax=MEM32(g_edx+4);frame=MEM32(g_edx);g_edi=g_eax+g_esi*8;
  if((int32_t)MEM32(g_edi)>(int32_t)frame||(int32_t)MEM32(g_edi+4)<=(int32_t)frame){++frame;MEM32(g_edi)=frame;g_eax=MEM32(g_edx+4);g_edi=MEM32(g_edx);MEM32(g_eax+g_esi*8+4)=g_edi;++MEM32(g_esp+28);}
  g_eax=MEM32(g_ecx+20);++g_esi;
 }while((int32_t)g_esi<(int32_t)g_eax);
 g_edi=MEM32(g_edx);g_ebx=MEM32(g_esp+28);frame=MEM32(g_ecx+40);g_esi=g_edi*MEM32(g_esp+36)+frame;g_eax=g_edi;MEM32(g_esp+48)=g_ebx;MEM32(g_esp+20)=g_eax;
 while((int32_t)MEM32(g_esp+48)>0&&(int32_t)g_eax>=0){
  g_edi=MEM32(g_ecx+44);frame=MEM32(g_edi+g_eax*4);g_edi=g_ebx=0;MEM8(g_esp+18)=0;MEM32(g_esp+44)=frame;MEM32(g_esp+32)=g_edi;
  do{
   SET_LO8(g_eax,MEM8(g_esp+18));SET_LO8(g_eax,(uint8_t)((int8_t)LO8(g_eax)>>1));--g_ebx;MEM8(g_esp+18)=LO8(g_eax);MEM32(g_esp+24)=g_ebx;
   if((int32_t)g_ebx<=0){SET_LO8(g_eax,MEM8(g_esi));g_ebx=8;MEM8(g_esp+18)=LO8(g_eax);++g_esi;MEM32(g_esp+24)=g_ebx;}
   if(!(MEM8(g_esp+18)&1)){
    g_eax=MEM32(g_edx+4);g_eax=MEM32(g_eax+g_edi*8);
    if((int32_t)g_eax>(int32_t)MEM32(g_edx)){
     SET_LO8(g_eax,MEM8(g_esp+19));unsigned wide=LO8(g_eax);g_eax=MEM32(g_edx+8);
     if(wide){g_eax+=MEM32(g_esp+32);g_ebx=MEM32(frame);MEM32(g_eax)=g_ebx;SET_LO16(g_ebx,MEM16(frame+4));frame=MEM32(g_esp+44);MEM16(g_eax+4)=LO16(g_ebx);}
     else{g_ebx=MEM32(frame);MEM32(g_eax+g_edi*4)=g_ebx;}
     g_eax=MEM32(g_edx+4);g_ebx=MEM32(g_esp+20);MEM32(g_eax+g_edi*8)=g_ebx;g_eax=MEM32(g_esp+48);g_ebx=MEM32(g_esp+24);--g_eax;MEM32(g_esp+48)=g_eax;
    }
    SET_LO8(g_eax,MEM8(g_esp+19));frame+=LO8(g_eax)?6:4;MEM32(g_esp+44)=frame;
   }
   MEM32(g_esp+32)+=6;g_eax=MEM32(g_ecx+20);++g_edi;
  }while((int32_t)g_edi<(int32_t)g_eax);
  g_eax=MEM32(g_esp+20);g_edi=0u-MEM32(g_esp+36);g_esi+=g_edi*2;g_edi=MEM32(g_esp+48);--g_eax;MEM32(g_esp+20)=g_eax;
 }
 g_ebx=MEM32(g_edx);g_eax=MEM32(g_ecx+28);++g_ebx;MEM32(g_esp+20)=g_ebx;
 if((int32_t)g_ebx>=(int32_t)g_eax){
  g_esi=MEM32(g_ecx+20);g_eax=0;
  do{g_esi=MEM32(g_edx+4);g_edi=MEM32(g_ecx+28);MEM32(g_esi+g_eax*8+4)=g_edi;g_esi=MEM32(g_ecx+20);++g_eax;}while((int32_t)g_eax<(int32_t)g_esi);
 }else{
  g_esi=MEM32(g_ecx+40);g_eax=MEM32(g_esp+28);g_edi=g_ebx*MEM32(g_esp+36)+g_esi;g_esi=MEM32(g_ecx+44);g_esi=MEM32(g_esi+g_ebx*4);frame=0;MEM32(g_esp+44)=g_esi;g_esi=0;MEM32(g_esp+48)=g_eax;MEM32(g_esp+24)=frame;MEM32(g_esp+36)=frame;
  do{
   MEM8(g_esp+18)=(uint8_t)((int8_t)MEM8(g_esp+18)>>1);g_eax=MEM32(g_esp+24)-1;MEM32(g_esp+24)=g_eax;
   if((int32_t)g_eax<=0){SET_LO8(g_eax,MEM8(g_edi));MEM8(g_esp+18)=LO8(g_eax);++g_edi;MEM32(g_esp+24)=8;}
   if(!(MEM8(g_esp+18)&1)){
    g_eax=MEM32(g_edx+4);g_eax=MEM32(g_eax+g_esi*8+4);
    if((int32_t)g_eax<=(int32_t)MEM32(g_edx)){
     SET_LO8(g_eax,MEM8(g_esp+19));g_ebx=MEM32(g_esp+44);unsigned wide=LO8(g_eax);g_eax=MEM32(g_edx+12);
     if(wide){g_eax+=frame;frame=MEM32(g_ebx);MEM32(g_eax)=frame;SET_LO16(g_ebx,MEM16(g_ebx+4));frame=MEM32(g_esp+36);MEM16(g_eax+4)=LO16(g_ebx);}
     else{g_ebx=MEM32(g_ebx);MEM32(g_eax+g_esi*4)=g_ebx;}
     g_eax=MEM32(g_edx+4);g_ebx=MEM32(g_esp+20);MEM32(g_eax+g_esi*8+4)=g_ebx;--MEM32(g_esp+48);
    }
    SET_LO8(g_eax,MEM8(g_esp+19));unsigned wide=LO8(g_eax);g_eax=MEM32(g_esp+44)+(wide?6:4);MEM32(g_esp+44)=g_eax;
   }else{
    SET_LO8(g_eax,MEM8(g_esp+19));g_ebx=MEM32(g_edx+12);unsigned wide=LO8(g_eax);g_eax=MEM32(g_edx+8);
    if(wide){g_eax+=frame;g_ebx+=frame;frame=MEM32(g_eax);MEM32(g_ebx)=frame;SET_LO16(g_eax,MEM16(g_eax+4));frame=MEM32(g_esp+36);MEM16(g_ebx+4)=LO16(g_eax);}
    else{g_eax=MEM32(g_eax+g_esi*4);MEM32(g_ebx+g_esi*4)=g_eax;}
    g_ebx=MEM32(g_esp+20);
   }
   g_eax=MEM32(g_ecx+20);++g_esi;frame+=6;MEM32(g_esp+36)=frame;
  }while((int32_t)g_esi<(int32_t)g_eax);
  g_eax=MEM32(g_esp+48);++g_ebx;
  while((int32_t)g_eax>0&&(int32_t)g_ebx<(int32_t)MEM32(g_ecx+28)){
   frame=g_esi=0;MEM32(g_esp+44)=frame;
   do{
    SET_LO8(g_eax,MEM8(g_esp+18));SET_LO8(g_eax,(uint8_t)((int8_t)LO8(g_eax)>>1));--g_esi;MEM8(g_esp+18)=LO8(g_eax);
    if((int32_t)g_esi<=0){SET_LO8(g_eax,MEM8(g_edi));MEM8(g_esp+18)=LO8(g_eax);++g_edi;g_esi=8;}
    if(!(MEM8(g_esp+18)&1)){g_eax=MEM32(g_edx+4)+frame*8+4;frame=MEM32(g_eax);if((int32_t)frame<=(int32_t)MEM32(g_edx)){MEM32(g_eax)=g_ebx;--MEM32(g_esp+48);}}
    frame=MEM32(g_esp+44);g_eax=MEM32(g_ecx+20);++frame;MEM32(g_esp+44)=frame;
   }while((int32_t)frame<(int32_t)g_eax);
   g_eax=MEM32(g_esp+48);++g_ebx;
  }
  if((int32_t)g_eax>0){
   g_eax=MEM32(g_ecx+20);g_esi=0;
   do{g_eax=MEM32(g_edx+4);g_edi=MEM32(g_eax+g_esi*8+4);g_eax+=g_esi*8+4;if((int32_t)g_edi<=(int32_t)MEM32(g_edx)){g_edi=MEM32(g_ecx+28);MEM32(g_eax)=g_edi;}g_eax=MEM32(g_ecx+20);++g_esi;}while((int32_t)g_esi<(int32_t)g_eax);
  }
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
