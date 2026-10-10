#include "migration_particles_flare_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D57FB(void),sub_002D589F(void),sub_002D5CCD(void),sub_002D631D(void);
extern void sub_0021D0E0(void),sub_0021B030(void),sub_0021B070(void),sub_0021CB90(void),sub_0021B1A0(void);
extern void sub_00186910(void),sub_001869D0(void);
static int group_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_FLARE_GROUPS");f=p&&!strcmp(p,"1");}return f;}
static int group_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int group_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define GROUP_CALL(fn,pc) do{PUSH32(g_esp,pc);fn();}while(0)
#define GROUP_VCALL(target,pc,saved) do{uint32_t t_=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(t_,saved);}while(0)
static int group_text(uint32_t p,uint32_t s){
 /* ponytail: authored names up to4096 bytes; longer names retain the original. */
 for(unsigned i=0;i<4096;i++){if(!group_span(p+i,1)||!group_apart(p+i,1,s-128,148))return 0;if(!MEM8(p+i))return 1;}return 0;
}
static int group_lookup_ok(uint32_t p,uint32_t s){
 if(!group_span(MEM32(s+4),4)||!group_apart(MEM32(s+4),4,s-128,148))return 0;
 uint32_t node=MEM32(p+0xd0);if(!node)return 1;
 if(!group_text(MEM32(MEM32(s+4)),s))return 0;
 for(unsigned i=0;i<4096;i++){
  if(!group_span(node,16)||!group_apart(node,16,s-128,148)||!group_text(MEM32(node),s))return 0;
  node=MEM32(node+12);if(!node)return 1;
 }return 0;
}
static void find_group(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_ecx+0xd0);
 if(g_edi){g_eax=MEM32(g_esp+20);frame=MEM32(g_eax);do{
  g_esi=MEM32(g_edi);g_ecx=frame;
  for(;;){
   SET_LO8(g_eax,MEM8(g_ecx));SET_LO8(g_ebx,MEM8(g_esi));SET_LO8(g_edx,LO8(g_eax));
   if(LO8(g_eax)!=LO8(g_ebx)){g_ecx=LO8(g_eax)<LO8(g_ebx)?0xffffffffu:1;break;}
   if(!LO8(g_edx)){g_ecx=0;break;}
   SET_LO8(g_eax,MEM8(g_ecx+1));SET_LO8(g_ebx,MEM8(g_esi+1));SET_LO8(g_edx,LO8(g_eax));
   if(LO8(g_eax)!=LO8(g_ebx)){g_ecx=LO8(g_eax)<LO8(g_ebx)?0xffffffffu:1;break;}
   g_ecx+=2;g_esi+=2;if(!LO8(g_edx)){g_ecx=0;break;}
  }
  if(!g_ecx){g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=8;return;}
  g_edi=MEM32(g_edi+12);
 }while(g_edi);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=0;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=8;
}
static void renderer_lifetime(int construct){
 uint32_t saved=g_esp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x395e88);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;
 if(construct){g_eax=0;MEM32(g_esi)=0x472cd8;MEM8(g_esi+24)=LO8(g_eax);MEM8(g_esi+32)=LO8(g_eax);g_ecx=MEM32(0x5150b0);MEM32(g_esp+16)=g_eax;g_eax=MEM32(g_ecx);PUSH32(g_esp,g_esi);GROUP_VCALL(MEM32(g_eax+0x78),0x1869bb,saved);}
 else{MEM32(g_esi)=0x472cd8;g_ecx=MEM32(0x5150b0);g_edx=MEM32(g_esi+4);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edx);MEM32(g_esp+20)=0;GROUP_VCALL(MEM32(g_eax+0x7c),0x18680a,saved);}
 g_ecx=MEM32(g_esp+8);if(construct){MEM32(g_esi+4)=g_eax;g_eax=g_esi;}else MEM32(g_esi)=0x472cbc;
 POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void construct_group(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x395eb6);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_edi=MEM32(g_esp+40);g_eax=MEM32(g_edi);g_esi=g_ecx;PUSH32(g_esp,0x50);MEM32(g_esi)=g_eax;GROUP_CALL(sub_002D57FB,0x1869fb);g_esp+=4;MEM32(g_esp+16)=g_eax;MEM32(g_esp+28)=0;
 if(g_eax){g_ecx=MEM32(g_edi);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=g_eax;GROUP_CALL(sub_0021D0E0,0x186a1a);g_edi=g_eax;}else g_edi=0;
 g_ecx=g_edi;MEM32(g_esp+28)=0xffffffffu;GROUP_CALL(sub_0021B030,0x186a2f);MEM32(g_esi+8)=g_eax;if(!g_eax)MEM32(g_esi+8)=1;
 g_ecx=MEM32(g_esi+8)<<2;PUSH32(g_esp,g_ecx);GROUP_CALL(sub_002D589F,0x186a49);SET_LO8(g_ebx,MEM8(g_esp+48));g_esp+=4;MEM32(g_esi+4)=g_eax;MEM32(g_eax)=g_edi;
 if(LO8(g_ebx)){
  g_edx=MEM32(g_esi+4);g_eax=MEM32(g_edx);g_ecx=MEM32(g_eax);g_edx=MEM32(g_ecx);{uint32_t s=g_esp;GROUP_VCALL(MEM32(g_edx+44),0x186a65,s);}
  g_edx=MEM32(g_eax);{uint32_t s=g_esp;PUSH32(g_esp,1);g_ecx=g_eax;GROUP_VCALL(MEM32(g_edx+36),0x186a6e,s);}
  g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);g_ecx=MEM32(g_ecx);g_edx=MEM32(g_ecx);{uint32_t s=g_esp;GROUP_VCALL(MEM32(g_edx+44),0x186a7a,s);}
  g_edx=MEM32(g_eax);{uint32_t s=g_esp;PUSH32(g_esp,0x3e19999a);PUSH32(g_esp,0x3dcccccd);g_ecx=g_eax;GROUP_VCALL(MEM32(g_edx+84),0x186a8b,s);}
 }
 g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax);PUSH32(g_esp,1);PUSH32(g_esp,0);GROUP_CALL(sub_0021B070,0x186a99);
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);g_ecx=MEM32(g_edx);g_eax=MEM32(g_ecx);{uint32_t s=g_esp;PUSH32(g_esp,0);GROUP_VCALL(MEM32(g_eax+16),0x186aa7,s);}
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx);frame=MEM32(g_esp+36);g_ecx=MEM32(g_edx);g_eax=MEM32(g_ecx);frame+=16;{uint32_t s=g_esp;PUSH32(g_esp,frame);GROUP_VCALL(MEM32(g_eax+4),0x186abb,s);}
 g_ecx=MEM32(g_esi+4);g_ecx=MEM32(g_ecx);PUSH32(g_esp,0x3f800000);GROUP_CALL(sub_0021CB90,0x186aca);g_eax=MEM32(g_esi+8);g_edi=1;
 while((int32_t)g_edi<(int32_t)g_eax){
  PUSH32(g_esp,0x50);GROUP_CALL(sub_002D57FB,0x186ae7);g_esp+=4;MEM32(g_esp+44)=g_eax;MEM32(g_esp+28)=1;
  if(g_eax){g_edx=MEM32(g_esp+40);g_ecx=MEM32(g_edx);PUSH32(g_esp,0x458978);PUSH32(g_esp,g_ecx);g_ecx=g_eax;GROUP_CALL(sub_0021D0E0,0x186b0d);}else g_eax=0;
  g_ecx=MEM32(g_esi+4);MEM32(g_ecx+g_edi*4)=g_eax;g_edx=MEM32(g_esi+4);g_ecx=MEM32(g_edx+g_edi*4);PUSH32(g_esp,1);PUSH32(g_esp,g_edi);MEM32(g_esp+36)=0xffffffffu;GROUP_CALL(sub_0021B070,0x186b2d);
  g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_edi*4);g_ecx=MEM32(g_ecx);g_edx=MEM32(g_ecx);{uint32_t s=g_esp;PUSH32(g_esp,0);GROUP_VCALL(MEM32(g_edx+16),0x186b3c,s);}
  g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_edi*4);g_ecx=MEM32(g_ecx);g_edx=MEM32(g_ecx);{uint32_t s=g_esp;PUSH32(g_esp,frame);GROUP_VCALL(MEM32(g_edx+4),0x186b4a,s);}
  g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_edi*4);PUSH32(g_esp,0x3f800000);GROUP_CALL(sub_0021CB90,0x186b5a);
  if(LO8(g_ebx)){
   g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_ecx+g_edi*4);g_ecx=MEM32(g_edx);g_eax=MEM32(g_ecx);{uint32_t s=g_esp;GROUP_VCALL(MEM32(g_eax+44),0x186b6b,s);}
   g_edx=MEM32(g_eax);{uint32_t s=g_esp;PUSH32(g_esp,1);g_ecx=g_eax;GROUP_VCALL(MEM32(g_edx+36),0x186b74,s);}
   g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_eax+g_edi*4);g_ecx=MEM32(g_ecx);g_edx=MEM32(g_ecx);{uint32_t s=g_esp;GROUP_VCALL(MEM32(g_edx+44),0x186b81,s);}
   g_edx=MEM32(g_eax);{uint32_t s=g_esp;PUSH32(g_esp,0x3e19999a);PUSH32(g_esp,0x3dcccccd);g_ecx=g_eax;GROUP_VCALL(MEM32(g_edx+84),0x186b92,s);}
  }
  g_eax=MEM32(g_esi+8);++g_edi;
 }
 g_ecx=MEM32(g_esp+20);POP32(g_esp,g_edi);MEM32(g_esi+12)=0;g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=16;
}
static void append_group(void){
 g_eax=MEM32(0);PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x395ecb);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+24);PUSH32(g_esp,g_edi);g_esi=g_ecx;GROUP_CALL(sub_00186910,0x186be3);
 if(!g_eax){
  PUSH32(g_esp,16);GROUP_CALL(sub_002D57FB,0x186bee);g_esp+=4;MEM32(g_esp+24)=g_eax;MEM32(g_esp+16)=0;
  if(g_eax){g_ecx=0;SET_LO8(g_ecx,MEM8(g_esi+0xbc));PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);g_ecx=g_eax;GROUP_CALL(sub_001869D0,0x186c13);}else g_eax=0;
  g_edx=MEM32(g_esi+0xd0);MEM32(g_eax+12)=g_edx;MEM32(g_esi+0xd0)=g_eax;
 }
 g_ecx=MEM32(g_esp+8);POP32(g_esp,g_edi);MEM32(0)=g_ecx;POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void destroy_group(void){
 PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_eax=MEM32(g_ebx+8);PUSH32(g_esp,g_edi);g_edi=0;
 if((int32_t)g_eax>0){PUSH32(g_esp,g_esi);do{
  g_eax=MEM32(g_ebx+4);g_esi=MEM32(g_eax+g_edi*4);
  if(g_esi){g_ecx=g_esi;GROUP_CALL(sub_0021B1A0,0x187b51);PUSH32(g_esp,g_esi);GROUP_CALL(sub_002D5CCD,0x187b57);g_esp+=4;}
  g_eax=MEM32(g_ebx+8);++g_edi;
 }while((int32_t)g_edi<(int32_t)g_eax);POP32(g_esp,g_esi);}
 g_ecx=MEM32(g_ebx+4);PUSH32(g_esp,g_ecx);GROUP_CALL(sub_002D631D,0x187b6c);g_esp+=4;POP32(g_esp,g_edi);POP32(g_esp,g_ebx);g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_particles_flare(unsigned entry){
 if(!group_enabled())return 0;uint32_t s=g_esp,p=g_ecx;unsigned size;
 switch(entry){case 0x1867d0:case 0x186980:size=36;break;case 0x186910:case 0x186bc0:size=0xd8;break;case 0x1869d0:case 0x187b30:size=16;break;default:return 0;}
 if((s&3)||s<0x10080u||!group_span(s-128,148)||!group_span(p,size)||!group_apart(p,size,s-128,148)||!group_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x186910||entry==0x186bc0){if(!group_lookup_ok(p,s))return 0;}
 if(entry==0x1869d0&&(!group_span(MEM32(s+8),4)||!group_apart(MEM32(s+8),4,s-128,148)))return 0;
 if(entry==0x1867d0||entry==0x186980){if(!group_span(0x5150b0,4)||!group_span(MEM32(0x5150b0),4)||!group_span(MEM32(MEM32(0x5150b0)),128))return 0;}
 if(entry==0x187b30){int32_t n=(int32_t)MEM32(p+8);if(n>4096)return 0;if(n>0&&(!group_span(MEM32(p+4),(unsigned)n*4)||!group_apart(MEM32(p+4),(unsigned)n*4,s-128,148)))return 0;}
 /* Completing allocator/resource/virtual services retain their original valid
  * lifetime contract. Read live registers/counts after every callback. */
 switch(entry){case 0x1867d0:renderer_lifetime(0);break;case 0x186980:renderer_lifetime(1);break;case 0x186910:find_group();break;case 0x1869d0:construct_group();break;case 0x186bc0:append_group();break;case 0x187b30:destroy_group();break;}
 return 1;
}
#undef GROUP_CALL
#undef GROUP_VCALL
