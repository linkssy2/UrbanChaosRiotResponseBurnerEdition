#include "migration_animation_world_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
/* Keep the guest destination NaN payload when both operands are NaNs.
 * C's commutative lowering otherwise changes payload selection in local SIMD
 * temporaries. Execute arithmetic first to retain floating exception flags. */
static RecompXmm world_ordered_add(RecompXmm a,RecompXmm b){RecompXmm v=XMM_ADD(a,b);for(unsigned i=0;i<4;i++)if((a.u[i]&0x7fffffffu)>0x7f800000u&&(b.u[i]&0x7fffffffu)>0x7f800000u)v.u[i]=a.u[i]|0x400000u;return v;}
static RecompXmm world_ordered_mul(RecompXmm a,RecompXmm b){RecompXmm v=XMM_MUL(a,b);for(unsigned i=0;i<4;i++)if((a.u[i]&0x7fffffffu)>0x7f800000u&&(b.u[i]&0x7fffffffu)>0x7f800000u)v.u[i]=a.u[i]|0x400000u;return v;}
/* Pinned MSVC /O2 /Ob1 baseline: the first linked row lowers to
 * scalar MULSS and commutes lanes1..3; later rows use destination-first MULPS.
 * Preserve that observed NaN payload behavior without changing finite math.
 * See migration_animation_linked_machine_audit.txt, linked_original05d5..084b. */
static RecompXmm world_product_mul(RecompXmm a,RecompXmm b,unsigned row,unsigned scratch){
 RecompXmm v=world_ordered_mul(a,b);
 if(scratch==16&&!row)for(unsigned i=1;i<4;i++)if((a.u[i]&0x7fffffffu)>0x7f800000u&&(b.u[i]&0x7fffffffu)>0x7f800000u)v.u[i]=b.u[i]|0x400000u;
 return v;
}
static int world_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int world_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int world_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_WORLD");enabled=v&&!strcmp(v,"1");}return enabled&&(_mm_getcsr()&0x1f80)==0x1f80&&world_span(0x51c178,1)&&!MEM8(0x51c178);}
/* Read complete operands before writing the result, as the original does.
 * Preserve its two stack rows and final SIMD scratch as well as the matrix. */
static void world_product(uint32_t local,uint32_t parent,uint32_t output,unsigned scratch){
 RecompXmm b0=XMM_MEM(parent),b1=XMM_MEM(parent+16),b2=XMM_MEM(parent+32),b3=XMM_MEM(parent+48);
 RecompXmm result[4],a3=XMM_ZERO(),b3part=XMM_ZERO(),c3part=XMM_ZERO(),ab3=XMM_ZERO(),cd2=XMM_ZERO();
 for(unsigned row=0;row<4;row++){
  RecompXmm a=XMM_MEM(local+row*16);
  RecompXmm p0=world_product_mul(b0,XMM_SHUFFLE(a,a,0),row,scratch);
  RecompXmm p1=world_product_mul(b1,XMM_SHUFFLE(a,a,0x55),row,scratch);
  RecompXmm p2=world_product_mul(b2,XMM_SHUFFLE(a,a,0xaa),row,scratch);
  RecompXmm p3=world_product_mul(b3,XMM_SHUFFLE(a,a,0xff),row,scratch);
  RecompXmm ab=world_ordered_add(p0,p1),cd=world_ordered_add(p2,p3);result[row]=world_ordered_add(ab,cd);
  if(row<2)XMM_STORE(g_esp+scratch+row*16,result[row]);
  if(row==2)cd2=cd;
  if(row==3){a3=a;b3part=p1;c3part=cd;ab3=ab;}
 }
 g_xmm0=result[3];g_xmm1=result[1];g_xmm2=c3part;g_xmm3=b3part;g_xmm4=ab3;g_xmm5=result[2];g_xmm6=XMM_SHUFFLE(a3,a3,0xaa);g_xmm7=cd2;
 for(unsigned row=0;row<4;row++)XMM_STORE(output+row*16,result[row]);
}
int uc_migration_animation_world(void){
 uint32_t s=g_esp,model=g_ecx;if(!world_enabled()||(s&3)||s<0x10100||!world_span(s-256,268)||!world_span(model,112))return 0;
 uint32_t asset=MEM32(model+104);if(!world_span(asset,68))return 0;
 uint32_t n=MEM32(asset+12),out=MEM32(s+4),root=MEM32(s+8),local=MEM32(model+108),metadata=MEM32(asset+64),bytes=(n?n:1)*64;
 if(n>4096||!world_span(out,bytes)||!world_span(local,bytes)||!world_span(root,64)||!world_apart(out,bytes,s-256,268)||!world_apart(local,bytes,s-256,268)||!world_apart(root,64,s-256,268)||!world_apart(model,112,out,bytes)||!world_apart(asset,68,out,bytes)||!world_apart(model,112,s-256,268)||!world_apart(asset,68,s-256,268))return 0;
 if(n>1){if(!world_span(metadata,n*160)||!world_apart(metadata,n*160,out,bytes)||!world_apart(metadata,n*160,s-256,268))return 0;for(unsigned i=1;i<n;i++)if(MEM16(metadata+i*160+22)>=n)return 0;}
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-84;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 g_eax=MEM32(g_ecx+108);g_edx=MEM32(frame+12);g_edi=MEM32(frame+8);world_product(g_eax,g_edx,g_edi,32);g_eax=MEM32(g_ecx+104);g_esi=MEM32(g_eax+12);g_edx=1;MEM32(g_esp+24)=g_edx;
 if((int32_t)g_esi>1){
  g_edx=0xffffffe0u-g_edi;g_ebx=160;g_esi=g_edi+96;MEM32(g_esp+28)=g_edx;
  do{
   g_eax=MEM32(g_eax+64);g_eax=(uint32_t)MEM16(g_eax+g_ebx+22)*64+g_edi;g_edx+=g_esi;g_edx+=MEM32(g_ecx+108);world_product(g_edx,g_eax,g_esi-32,32);g_edx=MEM32(g_esp+24);g_eax=MEM32(g_ecx+104);++g_edx;g_ebx+=160;g_esi+=64;MEM32(g_esp+24)=g_edx;
   if((int32_t)g_edx>=(int32_t)MEM32(g_eax+12))break;g_edx=MEM32(g_esp+28);
  }while(1);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=12;return 1;
}
static uint32_t linked_frame(void){uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-68;g_edx=MEM32(frame+16);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(frame+12);g_ebx=g_ecx;return frame;}
int uc_migration_animation_linked(void){
 uint32_t s=g_esp,model=g_ecx;if(!world_enabled()||(s&3)||!world_span(s,16)||!world_span(model,112))return 0;
 uint32_t asset=MEM32(model+104);if(!world_span(asset,68))return 0;
 uint32_t n=MEM32(asset+12),start=MEM32(s+8),out=MEM32(s+4),root=MEM32(s+12),local=MEM32(model+108),meta=MEM32(asset+64);
 if(!n||n>4096||start>=n||s<0x10000+n*112)return 0;
 uint32_t low=s-n*112,stackbytes=n*112+16,bytes=n*64;
 if(!world_span(low,stackbytes)||!world_span(out,bytes)||!world_span(local,bytes)||!world_span(root,64)||!world_span(meta,n*160)||!world_apart(out,bytes,low,stackbytes)||!world_apart(out,bytes,model,112)||!world_apart(out,bytes,asset,68)||!world_apart(out,bytes,meta,n*160)||!world_apart(out,bytes,local,bytes)||!world_apart(out,bytes,root,64)||!world_apart(model,112,low,stackbytes)||!world_apart(asset,68,low,stackbytes)||!world_apart(local,bytes,low,stackbytes)||!world_apart(root,64,low,stackbytes)||!world_apart(meta,n*160,low,stackbytes))return 0;
 uint32_t pending[4096];unsigned char seen[4096]={0};unsigned used=1;pending[0]=start;
 while(used){uint32_t bone=pending[--used];if(bone>=n||seen[bone])return 0;seen[bone]=1;uint32_t child=MEM16(meta+bone*160+24),sibling=MEM16(meta+bone*160+26);if(used+!!child+!!sibling>4096)return 0;if(child)pending[used++]=child;if(sibling)pending[used++]=sibling;}
 /* Native continuation stack preserves sibling-call guest frames without
  * making host recursion depth depend on authored skeleton depth. */
 uint32_t frame=linked_frame();unsigned depth=0;int resumed=0;
 for(;;){
  if(!resumed){
   g_eax=MEM32(g_ebx+108);g_ecx=g_edi*64;g_eax+=g_ecx;g_esi=g_ecx+MEM32(frame+8);world_product(g_eax,g_edx,g_esi,16);g_eax=MEM32(frame+8);g_ecx=MEM32(g_ebx+104);g_ecx=MEM32(g_ecx+64);g_edi*=160;SET_LO16(g_ecx,MEM16(g_ecx+g_edi+26));
   if(LO16(g_ecx)){PUSH32(g_esp,g_edx);g_edx=LO16(g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_ecx=g_ebx;g_ebp=frame;PUSH32(g_esp,0xb8fb7);pending[depth++]=frame;frame=linked_frame();continue;}
  }
  resumed=0;g_eax=MEM32(g_ebx+104);g_ecx=MEM32(g_eax+64);SET_LO16(g_edi,MEM16(g_ecx+g_edi+24));
  if(LO16(g_edi)){g_edx=g_esi;g_edi=LO16(g_edi);continue;}
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=16;
  if(!depth)return 1;frame=pending[--depth];resumed=1;
 }
}
