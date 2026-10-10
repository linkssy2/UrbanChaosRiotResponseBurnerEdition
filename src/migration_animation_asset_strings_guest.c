#include "migration_animation_asset_strings_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D631D(void),sub_002D589F(void),sub_002D8237(void);
static int asset_strings_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int asset_strings_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int asset_strings_length(uint32_t p,uint32_t *length){for(unsigned i=0;i<=4096;i++){if(!asset_strings_span(p+i,1))return 0;if(!MEM8(p+i)){*length=i;return 1;}}return 0;}
int uc_migration_animation_asset_strings(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_ASSET_STRINGS");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx,len=0,src=0;if(!enabled||(s&3)||s<0x10400||!asset_strings_span(s-1024,1032)||!asset_strings_span(p,16)||!asset_strings_apart(p,16,s-1024,1032)||!asset_strings_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry!=0x1f0320&&entry!=0x1f0330&&entry!=0x1f0340&&entry!=0x1f03d0)return 0;
 if(entry==0x1f0340){src=MEM32(s+4);if(!asset_strings_span(src,16)||!asset_strings_apart(src,16,s-1024,1032))return 0;if(MEM32(src+8)!=2&&MEM32(src+4)){if(MEM32(src+4)>4096||!asset_strings_length(MEM32(src),&len)||len>MEM32(src+4)||!asset_strings_apart(MEM32(src),len+1,s-1024,1032))return 0;}}
 if(entry==0x1f03d0){src=MEM32(s+4);if(!asset_strings_length(src,&len)||!asset_strings_apart(src,len+1,s-1024,1032)||!asset_strings_apart(src,len+1,p,16))return 0;}
 if(entry==0x1f0320){g_eax=p;g_ecx=0;MEM32(p+8)=0;MEM32(p)=0;MEM32(p+4)=0;MEM32(p+12)=0;g_recomp_zf=1;g_esp+=4;return 1;}
 if(entry==0x1f0330){int owned=MEM32(p+8)==1;if(owned){g_eax=MEM32(p);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f033e);sub_002D631D();POP32(g_esp,g_ecx);}g_recomp_zf=owned;g_esp+=4;return 1;}
 if(entry==0x1f0340){
  PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=p;
  if(MEM32(g_esi+8)!=2){g_eax=MEM32(g_esi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f0352);sub_002D631D();g_esp+=4;}
  g_ebx=MEM32(g_esp+12);g_eax=MEM32(g_ebx+8);
  if(g_eax==2){g_edx=MEM32(g_ebx);g_ecx=g_esi;MEM32(g_ecx)=g_edx;g_eax=MEM32(g_ebx+4);MEM32(g_ecx+4)=g_eax;g_edx=MEM32(g_ebx+8);MEM32(g_ecx+8)=g_edx;g_eax=MEM32(g_ebx+12);MEM32(g_ecx+12)=g_eax;}
  else{MEM32(g_esi+8)=g_eax;g_eax=MEM32(g_ebx+4);MEM32(g_esi+4)=g_eax;
   if(!g_eax){MEM32(g_esi)=0;MEM32(g_esi+12)=0;}
   else{++g_eax;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f03a1);sub_002D589F();g_edx=MEM32(g_ebx);g_esp+=4;g_edi=g_eax;
    do{SET_LO8(g_ecx,MEM8(g_edx));++g_edx;MEM8(g_edi)=LO8(g_ecx);++g_edi;}while(LO8(g_ecx));
    MEM32(g_esi)=g_eax;g_ecx=MEM32(g_ebx+12);POP32(g_esp,g_edi);MEM32(g_esi+12)=g_ecx;
   }
  }
  g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=8;return 1;
 }
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=src;g_eax=src;g_esi=p;g_edx=src+1;
 do{SET_LO8(g_ecx,MEM8(g_eax));++g_eax;}while(LO8(g_ecx));
 g_ecx=MEM32(g_esi+8);g_eax-=g_edx;MEM32(g_esi+4)=g_eax;MEM32(g_esi+12)=g_eax;
 if(g_ecx==1){
  ++g_eax;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x1f03fe);sub_002D589F();g_ebx=g_eax;g_eax=MEM32(g_esi+4);g_esp+=4;MEM32(g_esi)=g_ebx;MEM32(g_esp+20)=0;
  if(g_eax)do{g_eax=MEM32(g_esi+12)*31u;MEM32(g_esi+12)=g_eax;g_ecx=(uint32_t)(int32_t)SMEM8(g_edi);PUSH32(g_esp,g_ecx);frame=g_eax;PUSH32(g_esp,0x1f0428);sub_002D8237();g_edx=frame>>7;g_eax^=g_edx;g_eax^=frame;MEM32(g_esi+12)=g_eax;SET_LO8(g_eax,MEM8(g_edi));g_esp+=4;MEM8(g_ebx)=LO8(g_eax);g_eax=MEM32(g_esp+20);g_ecx=MEM32(g_esi+4);++g_ebx;++g_edi;++g_eax;MEM32(g_esp+20)=g_eax;}while(g_eax<g_ecx);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM8(g_ebx)=0;POP32(g_esp,g_ebx);
 }else{
  frame=0;MEM32(g_esi)=g_edi;
  if(g_eax)do{g_ecx=MEM32(g_esi+12)*31u;MEM32(g_esi+12)=g_ecx;g_eax=(uint32_t)(int32_t)SMEM8(g_edi);PUSH32(g_esp,g_eax);g_ebx=g_ecx;PUSH32(g_esp,0x1f0474);sub_002D8237();g_edx=g_ebx>>7;g_eax^=g_edx;g_eax^=g_ebx;g_esp+=4;++g_edi;MEM32(g_esi+12)=g_eax;g_eax=MEM32(g_esi+4);++frame;}while(frame<g_eax);
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);
 }
 g_esp+=8;return 1;
}
