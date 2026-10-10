#include "migration_particles_flare_recipe_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00186910(void),sub_00208C50(void),sub_00219450(void),sub_0021B0B0(void),sub_0021FC20(void),sub_002D37B0(void),sub_002D3780(void);
static int recipe_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_FLARE_RECIPE");f=p&&!strcmp(p,"1");}return f;}
static int recipe_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int recipe_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define RECIPE_CALL(fn,pc) do{g_ebp=frame;PUSH32(g_esp,pc);fn();}while(0)
typedef struct{uint32_t descriptor,slot,layout,manager_pc,source_pc,submit_pc;}FlareEmission;
/* These are the original visual recipes and their retired call-stack locations. */
static const FlareEmission muzzle[]={
 {0x472e20,12,0,0x186da2,0x186dbf,0x186dca},
 {0x472dec,36,1,0x186dd5,0x186df2,0x186dfd},
 {0x472dc4,44,2,0x186e08,0x186e25,0x186e30},
 {0x472d94,40,0,0x186e49,0x186e66,0x186f3b}};
static const FlareEmission common[]={
 {0x472d38,24,2,0x186ead,0x186eca,0x186ed5},
 {0x472d10,28,0,0x186ee0,0x186efd,0x186f08},
 {0x472dc4,20,1,0x186f13,0x186f30,0x186f3b}};
static const FlareEmission extra={0x472d60,32,1,0x186e7a,0x186e97,0x186ea2};
static void recipe_emit(uint32_t frame,const FlareEmission*r){
 g_ecx=MEM32(0x51c4dc);RECIPE_CALL(sub_00208C50,r->manager_pc);
 if(r->layout==2){g_ecx=g_esp+r->slot;PUSH32(g_esp,g_ecx);g_edx=g_esp+84;MEM32(g_esp+r->slot+4)=r->descriptor;g_ebx=MEM32(g_eax);PUSH32(g_esp,g_edx);MEM32(g_esp+24)=g_eax;}
 else{
  MEM32(g_esp+r->slot)=r->descriptor;g_ebx=MEM32(g_eax);
  if(r->layout==0){MEM32(g_esp+16)=g_eax;g_eax=g_esp+r->slot;PUSH32(g_esp,g_eax);g_ecx=g_esp+84;PUSH32(g_esp,g_ecx);}
  else{g_edx=g_esp+r->slot;MEM32(g_esp+16)=g_eax;PUSH32(g_esp,g_edx);g_eax=g_esp+84;PUSH32(g_esp,g_eax);}
 }
 RECIPE_CALL(sub_00219450,r->source_pc);g_ecx=MEM32(g_esp+24);g_esp+=4;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_ebx+16);PUSH32(g_esp,r->submit_pc);RECOMP_ICALL_SAFE(target,saved);}
}
static void flare_recipe(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-132;
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+8);PUSH32(g_esp,g_esi);g_esi=g_ecx;SET_LO8(g_eax,MEM8(g_esi+0xbc));MEM32(g_esi+0xcc)=1;MEM8(g_esi+0xb8)=0;
 int alternate=LO8(g_eax)!=0;g_eax=MEM32(g_ebx);PUSH32(g_esp,g_edi);
 uint32_t name,pc;unsigned layout,mode=1;
 if(alternate){
  g_eax-=3;uint32_t route=0x186cb9;if(g_eax<=19){g_eax=MEM8(0x1870c8+g_eax);route=MEM32(0x1870bc+4*g_eax);}
  if(route==0x186c87){name=0x472f68;layout=0;pc=0x186c9b;}
  else if(route==0x186ca0){name=0x472f00;layout=1;pc=0x186cb4;}
  else{name=0x472f68;layout=2;pc=0x186ccd;}
 }else if(g_eax==4){
  SET_LO8(g_eax,MEM8(g_ebx+4));if(LO8(g_eax)){name=0x472e48;layout=2;pc=0x186d28;mode=2;}else{name=0x472ea8;layout=0;pc=0x186d48;mode=0;}
 }else if((int32_t)g_eax>19&&(int32_t)g_eax<=22){name=0x472f00;layout=0;pc=0x186cf5;}
 else{name=0x472ea8;layout=1;pc=0x186d0b;}
 if(layout==0){g_ecx=g_esp+12;PUSH32(g_esp,g_ecx);}else if(layout==1){g_edx=g_esp+12;PUSH32(g_esp,g_edx);}else{g_eax=g_esp+12;PUSH32(g_esp,g_eax);}
 g_ecx=g_esi;MEM32(g_esp+16)=name;RECIPE_CALL(sub_00186910,pc);if(mode!=1)MEM32(g_esi+0xcc)=mode;
 /* The authored basis is copied in its original column order. */
 g_xmm0=XMM_MEM(g_esi+48);XMM_STORE(g_esp+80,g_xmm0);g_xmm0=XMM_MEM(g_esi+32);g_edi=g_eax;g_eax=MEM32(g_ebx);XMM_STORE(g_esp+96,g_xmm0);g_xmm0=XMM_MEM(g_esi+16);g_eax-=3;XMM_STORE(g_esp+112,g_xmm0);g_xmm0=XMM_MEM(g_esi+64);XMM_STORE(g_esp+128,g_xmm0);
 uint32_t route=0x186ea2;if(g_eax<=22){g_edx=MEM8(0x1870ec+g_eax);route=MEM32(0x1870dc+4*g_edx);}
 if(route==0x186d97){
  for(unsigned i=0;i<3;i++)recipe_emit(frame,&muzzle[i]);g_ebx=MEM32(frame+8);SET_LO8(g_eax,MEM8(g_ebx+4));
  if(LO8(g_eax)){recipe_emit(frame,&muzzle[3]);g_ebx=MEM32(frame+8);}
 }else if(route!=0x186f3e){
  if(route==0x186e6f)recipe_emit(frame,&extra);
  for(unsigned i=0;i<3;i++)recipe_emit(frame,&common[i]);g_ebx=MEM32(frame+8);
 }
 /* Select one model using the same shared integer/float RNG stream. */
 g_ecx=0;if(g_edi){
  MEM32(g_esi+0xd4)=g_edi;g_eax=MEM32(g_edi+8);
  if((int32_t)g_eax>1){PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);RECIPE_CALL(sub_002D37B0,0x186f5d);g_esp+=8;MEM32(g_esi+0xc4)=g_eax;}else MEM32(g_esi+0xc4)=g_ecx;
  g_eax=MEM32(g_ebx)-3;route=0x186fc3;if(g_eax<=22){g_ecx=MEM8(0x18710c+g_eax);route=MEM32(0x187104+4*g_ecx);}
  if(route==0x186f86){
   PUSH32(g_esp,0);PUSH32(g_esp,0);RECIPE_CALL(sub_002D3780,0x186f8f);MEMF(g_esp+68)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_xmm0=XMM_ZERO();
   for(unsigned i=0;i<3;i++)MEMF(g_esp+56+4*i)=g_xmm0.f[0];g_edx=g_esp+56;
  }else{
   PUSH32(g_esp,0x3f800000);PUSH32(g_esp,0x3f666666);RECIPE_CALL(sub_002D3780,0x186fd2);MEMF(g_esp+84)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;g_xmm0=XMM_SCALAR(MEMF(0x44bf94));
   for(unsigned i=0;i<3;i++)MEMF(g_esp+72+4*i)=g_xmm0.f[0];g_edx=g_esp+72;
  }
  g_eax=MEM32(g_esi+0xc4);g_ecx=MEM32(g_edi+4);g_ecx=MEM32(g_ecx+4*g_eax);g_esp+=8;PUSH32(g_esp,g_edx);RECIPE_CALL(sub_0021B0B0,0x186fc1);
 }else MEM32(g_esi+0xd4)=g_ecx;
 /* Optional renderer effect and its original lifetime. */
 g_eax=MEM32(g_ebx)-3;int z=g_eax==22;route=0x1870ab;if(g_eax<=22){g_edx=MEM8(0x187134+g_eax);route=MEM32(0x187124+4*g_edx);}
 /* The translated ABI publishes CMP_EQ after these TESTs: both operands
  * contain the same byte, so its final ZF contract is one on either branch. */
 if(route==0x187018){SET_LO8(g_eax,MEM8(g_esi+0xbc));z=1;if(!LO8(g_eax)){
  g_eax=g_esp+20;PUSH32(g_esp,g_eax);MEM32(g_esp+24)=0x472cf4;RECIPE_CALL(sub_0021FC20,0x187038);g_xmm0=XMM_SCALAR(MEMF(0x44fe3c));g_esp+=4;MEM32(g_esi+0xa0)=g_eax;MEMF(g_esi+0xb4)=g_xmm0.f[0];MEM8(g_esi+0xb8)=1;
 }}else if(route==0x187042){
  g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);MEM32(g_esp+32)=0x472cf4;RECIPE_CALL(sub_0021FC20,0x187054);g_xmm0=XMM_SCALAR(MEMF(0x44c798));MEM32(g_esi+0xa0)=g_eax;SET_LO8(g_eax,MEM8(g_esi+0xbc));g_esp+=4;z=1;if(!LO8(g_eax))g_xmm0=XMM_SCALAR(MEMF(0x44f964));MEMF(g_esi+0xb4)=g_xmm0.f[0];MEM8(g_esi+0xb8)=1;
 }else if(route==0x187079){
  g_edx=g_esp+24;PUSH32(g_esp,g_edx);MEM32(g_esp+28)=0x472cf4;RECIPE_CALL(sub_0021FC20,0x18708b);g_xmm0=XMM_SCALAR(MEMF(0x44d964));g_esp+=4;MEM32(g_esi+0xa0)=g_eax;MEMF(g_esi+0xb4)=g_xmm0.f[0];MEM8(g_esi+0xb8)=1;
 }
 POP32(g_esp,g_edi);MEM8(g_esi+0xc8)=1;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=z;g_esp+=12;
}
int uc_migration_particles_flare_recipe(unsigned entry){
 if(entry!=0x186c40||!recipe_enabled())return 0;uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10200||!recipe_span(s-512,528)||!recipe_span(p,216)||!recipe_apart(p,216,s-512,528)||!recipe_span(MEM32(s+4),8)||!recipe_apart(MEM32(s+4),8,s-512,528)||!recipe_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
 if(!recipe_span(0x1870bc,0x90)||!recipe_span(0x51c4dc,4)||!recipe_apart(p,216,0x1870bc,0x90)||!recipe_apart(s-512,528,0x1870bc,0x90))return 0;
 static const uint32_t tables[]={0x1870bc,0x1870dc,0x187104,0x187124},bytes[]={0x1870c8,0x1870ec,0x18710c,0x187134},sizes[]={20,23,23,23},counts[]={3,4,2,4};
 static const uint32_t targets[4][4]={{0x186ca0,0x186c87,0x186cb9,0},{0x186f3e,0x186d97,0x186e6f,0x186ea2},{0x186f86,0x186fc3,0,0},{0x187042,0x187018,0x187079,0x1870ab}};
 for(unsigned t=0;t<4;t++){for(unsigned i=0;i<counts[t];i++)if(MEM32(tables[t]+4*i)!=targets[t][i])return 0;for(unsigned i=0;i<sizes[t];i++)if(MEM8(bytes[t]+i)>=counts[t])return 0;}
 flare_recipe();return 1;
}
#undef RECIPE_CALL
