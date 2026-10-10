#include "migration_animation_palette_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
/* Keep the guest destination NaN payload when both operands are NaNs.
 * C's commutative lowering otherwise changes payload selection in local SIMD
 * temporaries. Execute arithmetic first to retain floating exception flags. */
static RecompXmm palette_ordered_add(RecompXmm a,RecompXmm b){RecompXmm v=XMM_ADD(a,b);for(unsigned i=0;i<4;i++)if((a.u[i]&0x7fffffffu)>0x7f800000u&&(b.u[i]&0x7fffffffu)>0x7f800000u)v.u[i]=a.u[i]|0x400000u;return v;}
static RecompXmm palette_ordered_mul(RecompXmm a,RecompXmm b){RecompXmm v=XMM_MUL(a,b);for(unsigned i=0;i<4;i++)if((a.u[i]&0x7fffffffu)>0x7f800000u&&(b.u[i]&0x7fffffffu)>0x7f800000u)v.u[i]=a.u[i]|0x400000u;return v;}
static int palette_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int palette_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static RecompXmm palette_row(RecompXmm a,const RecompXmm b[4],RecompXmm*last_pair){
 RecompXmm x=palette_ordered_mul(b[0],XMM_SHUFFLE(a,a,0)),y=palette_ordered_mul(b[1],XMM_SHUFFLE(a,a,0x55));
 RecompXmm z=palette_ordered_mul(b[2],XMM_SHUFFLE(a,a,0xaa)),w=palette_ordered_mul(b[3],XMM_SHUFFLE(a,a,0xff));
 *last_pair=palette_ordered_add(z,w);return palette_ordered_add(palette_ordered_add(x,y),*last_pair);
}
int uc_migration_animation_palette(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_PALETTE");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,model=g_ecx;if(!enabled||(_mm_getcsr()&0x1f80)!=0x1f80||(s&3)||s<0x10200||!palette_span(s-512,516)||!palette_span(model,120)||!palette_span(0x44bf94,4)||!palette_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t asset=MEM32(model+104);if(!palette_span(asset,72)||!palette_apart(asset,72,s-512,516)||!palette_apart(model,120,s-512,516))return 0;
 uint32_t n=MEM32(asset+12),metadata=MEM32(asset+64),input=MEM32(model+112),out=MEM32(model+116);
 if(n>4096)return 0;
 if(n&&(!palette_span(metadata,n*160)||!palette_span(input,n*64)||!palette_span(out,n*48)||!palette_apart(out,n*48,model,120)||!palette_apart(out,n*48,asset,72)||!palette_apart(out,n*48,metadata,n*160)||!palette_apart(out,n*48,s-512,516)||!palette_apart(input,n*64,s-512,516)||!palette_apart(metadata,n*160,s-512,516)))return 0;
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-180;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=MEM32(g_ecx+104);g_xmm1=XMM_SCALAR(MEMF(g_eax+68));g_edx=MEM32(g_eax+12);g_xmm0=g_xmm1;MEMF(g_esp+16)=g_xmm0.f[0];g_xmm0=XMM_ZERO();
 const unsigned zeros[]={20,24,28,96,104,108,80,84,92,112,116,120};for(unsigned i=0;i<12;i++)MEMF(g_esp+zeros[i])=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_ebx=0;g_xmm2=g_xmm1;MEMF(g_esp+100)=g_xmm2.f[0];MEMF(g_esp+88)=g_xmm1.f[0];MEMF(g_esp+124)=g_xmm0.f[0];
 if((int32_t)g_edx>0){
  g_xmm4=XMM_MEM(g_esp+16);g_edx=0;g_edi=0;g_esi=0;
  do{
   g_eax=MEM32(g_eax+64);RecompXmm bind[4],scaled[4],world[4],result[4],pair=XMM_ZERO(),third_pair=XMM_ZERO();
   for(unsigned row=0;row<4;row++)bind[row]=XMM_MEM(g_eax+g_esi+96+row*16);
   /* Source saves bind rows in this order, then replaces them with world rows. */
   XMM_STORE(g_esp+64,bind[2]);XMM_STORE(g_esp+16,bind[0]);XMM_STORE(g_esp+32,bind[3]);XMM_STORE(g_esp+48,bind[1]);
   RecompXmm scale[4]={g_xmm4,XMM_MEM(g_esp+96),XMM_MEM(g_esp+80),XMM_MEM(g_esp+112)};
   for(unsigned row=0;row<4;row++)scaled[row]=palette_row(scale[row],bind,&pair);
   g_eax=MEM32(g_ecx+112);for(unsigned row=0;row<4;row++)world[row]=XMM_MEM(g_eax+g_edi+row*16);
   XMM_STORE(g_esp+16,world[1]);XMM_STORE(g_esp+32,world[0]);XMM_STORE(g_esp+64,world[3]);XMM_STORE(g_esp+48,world[2]);
   for(unsigned row=0;row<4;row++){result[row]=palette_row(scaled[row],world,&pair);if(row==2)third_pair=pair;}
   XMM_STORE(g_esp+144,result[1]);g_xmm0=result[0];g_xmm1=result[2];g_xmm3=result[3];g_xmm6=result[1];g_xmm7=third_pair;
   g_xmm5=g_xmm1;g_xmm2=g_xmm0;g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm3,0xee);g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm6,0x44);g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm6,0xee);g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm1,0x88);g_xmm5=XMM_SHUFFLE(g_xmm5,g_xmm3,0x44);g_xmm1=g_xmm2;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm5,0x88);
   g_eax=MEM32(g_ecx+116);XMM_STORE(g_eax+g_edx,g_xmm2);g_eax=MEM32(g_ecx+116);g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm5,0xdd);XMM_STORE(g_eax+g_edx+16,g_xmm1);g_eax=MEM32(g_ecx+116);XMM_STORE(g_eax+g_edx+32,g_xmm0);g_eax=MEM32(g_ecx+104);++g_ebx;g_esi+=160;g_edi+=64;g_edx+=48;
  }while((int32_t)g_ebx<(int32_t)MEM32(g_eax+12));
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=4;return 1;
}
