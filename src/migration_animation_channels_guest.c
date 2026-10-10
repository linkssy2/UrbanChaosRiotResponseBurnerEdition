#include "migration_animation_channels_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
static int channels_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int channels_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* The second contribution reads after the first store, including aliases. */
static void channels_value(uint32_t base,uint32_t first,uint32_t second,uint32_t out,int reversed){
 g_xmm0=XMM_SCALAR(MEMF(base));g_xmm4=XMM_SCALAR(MEMF(first));
 g_xmm0.f[0]*=g_xmm1.f[0];g_xmm4.f[0]*=g_xmm2.f[0];g_xmm0.f[0]+=g_xmm4.f[0];MEMF(out)=g_xmm0.f[0];
 if(second){if(reversed){g_xmm4=g_xmm3;g_xmm4.f[0]*=MEMF(second);}else{g_xmm4=XMM_SCALAR(MEMF(second));g_xmm4.f[0]*=g_xmm3.f[0];}g_xmm4.f[0]+=g_xmm0.f[0];MEMF(out)=g_xmm4.f[0];}
}
int uc_migration_animation_channels(uint32_t entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_CHANNELS");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx;if(!enabled||entry!=0x1f1800||(s&3)||s<0x10040||!channels_span(s-32,56)||!channels_span(p,28)||!channels_span(0x51c178,1)||MEM8(0x51c178)||(_mm_getcsr()&0x1f80)!=0x1f80)return 0;
 uint32_t n=MEM32(p+24),out=MEM32(p+16),base=MEM32(s+4),first=MEM32(s+8),second=MEM32(s+12);
 if(n>4096||!channels_apart(p,28,s-32,56)||!channels_span(0x44bf94,4))return 0;
 uint32_t buffers[4]={out,base,first,second};
 for(unsigned j=0;j<4;j++)if(j!=3||second){if(!channels_span(buffers[j],n*4)||!channels_apart(buffers[j],n*4,s-32,56)||!channels_apart(buffers[j],n*4,p,28))return 0;}
 /* Exceptional input payloads retain the translated baseline until qualified. */
 for(unsigned j=1;j<4;j++)if(j!=3||second)for(unsigned i=0;i<n;i++)if((MEM32(buffers[j]+i*4)&0x7f800000u)==0x7f800000u)return 0;
 if((MEM32(s+16)&0x7f800000u)==0x7f800000u||(MEM32(s+20)&0x7f800000u)==0x7f800000u||(MEM32(0x44bf94)&0x7f800000u)==0x7f800000u)return 0;
 uint32_t frame=g_seh_ebp;
 g_esp-=16;g_xmm2=XMM_SCALAR(MEMF(s+16));g_xmm3=XMM_SCALAR(MEMF(s+20));g_xmm1=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,g_ebx);g_ebx=n;PUSH32(g_esp,frame);frame=out;g_ecx=second;PUSH32(g_esp,g_esi);g_esi=first;PUSH32(g_esp,g_edi);g_edi=0;
 g_xmm0=g_xmm2;g_xmm0.f[0]+=g_xmm3.f[0];MEM32(g_esp+24)=n;g_xmm1.f[0]-=g_xmm0.f[0];MEM32(g_esp+28)=out;
 if(n>=4){
  MEM32(g_esp+52)=base-first;MEM32(g_esp+16)=second-first;MEM32(g_esp+44)=out-first;MEM32(g_esp+20)=second-out;MEM32(g_esp+40)=base-second;MEM32(g_esp+48)=second+12;g_edx=out+8;g_eax=first+4;
  for(;g_edi+3<n;g_edi+=4){
   for(unsigned lane=0;lane<4;lane++){uint32_t offset=(g_edi+lane)*4;channels_value(base+offset,first+offset,second?second+offset:0,out+offset,lane==2);}
   MEM32(g_esp+48)+=16;g_eax+=16;g_edx+=16;
  }
 }
 if(g_edi<n){g_edx=second-first;frame=out-first;g_eax=first+g_edi*4;g_esi=base-first;MEM32(g_esp+52)=g_esi;g_ebx=n-g_edi;
  do{channels_value(g_esi+g_eax,g_eax,second?g_edx+g_eax:0,frame+g_eax,0);g_eax+=4;}while(--g_ebx);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=24;return 1;
}
