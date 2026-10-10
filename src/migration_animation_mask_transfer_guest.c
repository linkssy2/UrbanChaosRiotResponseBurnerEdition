#include "migration_animation_mask_transfer_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int transfer_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int transfer_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int transfer_buffers(uint32_t descriptor,uint32_t holder,uint32_t root,uint32_t stack){
 if(!transfer_span(descriptor,20)||!transfer_span(holder,4)||!transfer_apart(descriptor,20,stack-64,80)||!transfer_apart(holder,4,stack-64,80))return 0;
 uint32_t n=MEM32(descriptor+16),src=MEM32(descriptor+4),dst=MEM32(holder);if(n>4096||(root!=0xffffffffu&&root>=n))return 0;
 if(!n)return 1;
 return transfer_span(src,n)&&transfer_span(dst,n)&&transfer_apart(src,n,stack-64,80)&&transfer_apart(dst,n,stack-64,80)&&transfer_apart(src,n,descriptor,20)&&transfer_apart(dst,n,descriptor,20)&&transfer_apart(src,n,holder,4)&&transfer_apart(dst,n,holder,4);
}
/* Overlapping buffers retain the source's forward byte order and root restore. */
static void transfer_run(void){
 uint32_t frame=g_seh_ebp;g_edx=MEM32(g_esp+4);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+20);g_esi=g_ecx;SET_LO8(g_ecx,0);
 if(g_edi!=0xffffffffu){g_eax=MEM32(g_edx+4);SET_LO8(g_ecx,MEM8(g_edi+g_eax));}
 g_ebx=MEM32(g_edx+16);g_eax=0;
 if((int32_t)g_ebx>0){PUSH32(g_esp,frame);do{g_ebx=MEM32(g_edx+4);SET_LO8(g_ebx,MEM8(g_eax+g_ebx));frame=MEM32(g_esi);MEM8(g_eax+frame)=LO8(g_ebx);g_ebx=MEM32(g_edx+4);MEM8(g_eax+g_ebx)=0;g_ebx=MEM32(g_edx+16);++g_eax;}while((int32_t)g_eax<(int32_t)g_ebx);POP32(g_esp,frame);}
 if(g_edi!=0xffffffffu){g_edx=MEM32(g_edx+4);MEM8(g_edi+g_edx)=LO8(g_ecx);}
 g_recomp_zf=g_edi==0xffffffffu;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;
}
int uc_migration_animation_mask_transfer(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_MASK_TRANSFER");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,obj=g_ecx;if(!enabled||(s&3)||s<0x10040||!transfer_span(s-64,80)||!transfer_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x1f0f90){if(!transfer_buffers(MEM32(s+4),obj,MEM32(s+8),s))return 0;transfer_run();return 1;}
 if(entry==0x1f1350){
  if(!transfer_span(obj,28)||!transfer_apart(obj,28,s-64,80)||!transfer_span(MEM32(obj+4),28))return 0;
  uint32_t asset=MEM32(obj+4);if(!transfer_buffers(obj+8,0x51c138,MEM32(asset+24),s))return 0;
  g_eax=asset;g_edx=MEM32(asset+24);g_ecx+=8;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);g_ecx=0x51c138;PUSH32(g_esp,0x1f1365);transfer_run();g_recomp_zf=g_ecx==0;g_esp+=4;return 1;
 }
 if(entry!=0x1f1370||!transfer_span(obj,28)||!transfer_apart(obj,28,s-64,80)||!transfer_span(0x51c138,4))return 0;
 uint32_t n=MEM32(obj+24),src=MEM32(0x51c138),dst=MEM32(obj+12);if(n>4096)return 0;
 if(n&&(!transfer_span(src,n)||!transfer_span(dst,n)||!transfer_apart(src,n,s-64,80)||!transfer_apart(dst,n,s-64,80)||!transfer_apart(dst,n,obj,28)||!transfer_apart(dst,n,0x51c138,4)))return 0;
 g_edx=n;g_eax=0;if((int32_t)n>0){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);do{g_esi=MEM32(0x51c138);g_edx=MEM32(g_ecx+12);SET_LO8(g_ebx,MEM8(g_esi+g_eax));MEM8(g_eax+g_edx)=LO8(g_ebx);g_edx=MEM32(g_ecx+24);++g_eax;}while((int32_t)g_eax<(int32_t)g_edx);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);}g_esp+=4;return 1;
}
