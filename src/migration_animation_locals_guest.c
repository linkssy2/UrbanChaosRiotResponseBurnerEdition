#include "migration_animation_locals_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F4E30(void),sub_002D3F00(void);
static int local_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int local_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int local_gate(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_LOCALS");enabled=v&&!strcmp(v,"1");}
 return enabled&&!(g_esp&3)&&g_esp>=0x10400&&local_span(g_esp-1024,1036)&&local_span(0x51c178,1)&&!MEM8(0x51c178);
}
int uc_migration_animation_pose_matrix(void){
 if(!local_gate())return 0;uint32_t out=MEM32(g_esp+4);
 if(out&&(!local_span(out,64)||!local_span(g_ecx,28)||!local_apart(out,64,g_esp-1024,1036)||!local_apart(g_ecx,28,g_esp-1024,1036)))return 0;
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39b9a1);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=16;g_eax=g_ecx;g_ecx=MEM32(g_esp+32);MEM32(g_esp)=g_ecx;MEM32(g_esp+24)=0;
 if(g_ecx){
  g_xmm0=XMM_SCALAR(MEMF(g_esp+36));g_xmm1=XMM_SCALAR(MEMF(g_eax+16));g_xmm1.f[0]*=g_xmm0.f[0];MEMF(g_esp+4)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_eax+20));g_xmm1.f[0]*=g_xmm0.f[0];g_edx=g_esp+4;MEMF(g_esp+8)=g_xmm1.f[0];g_xmm1=XMM_SCALAR(MEMF(g_eax+24));PUSH32(g_esp,g_edx);g_xmm1.f[0]*=g_xmm0.f[0];PUSH32(g_esp,g_eax);MEMF(g_esp+20)=g_xmm1.f[0];PUSH32(g_esp,0x1f4e9b);sub_002D3F00();
 }
 g_ecx=MEM32(g_esp+16);MEM32(0)=g_ecx;g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
/* Complete local-pose traversal, including root-only and sparse masks that the
 * prior staged prefix declined. Shared quaternion-to-matrix math stays a call. */
int uc_migration_animation_locals(void){
 if(!local_gate()||!local_span(g_ecx,40))return 0;
 uint32_t p=g_ecx,s=g_esp,n=MEM32(p+36),out=MEM32(s+4),bytes=n?n*64:64;
 if(n>4096||!local_span(out,bytes)||!local_apart(out,bytes,s-1024,1036)||!local_apart(p,40,s-1024,1036)||!local_apart(p,40,out,bytes)||!local_span(0x44bf80,24)||!local_apart(0x44bf80,24,out,bytes))return 0;
 if(n>1){const uint32_t at[]={MEM32(p+4),MEM32(p+12),MEM32(p+16),MEM32(p+20),MEM32(p+32)};const unsigned sizes[]={28,n,n*4,n*12,n*48};for(unsigned i=0;i<5;i++)if(!local_span(at[i],sizes[i])||!local_apart(at[i],sizes[i],out,bytes)||!local_apart(at[i],sizes[i],s-1024,1036))return 0;}
 uint32_t frame=g_seh_ebp;g_edx=MEM32(g_esp+4);g_xmm0=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_eax=0;g_ecx=16;g_edi=g_edx;
 for(unsigned i=0;i<g_ecx;i++)MEM32(g_edi+i*4)=g_eax;g_edi+=g_ecx*4;g_ecx=0;SET_LO8(g_eax,MEM8(g_esi+28));
 if(LO8(g_eax)){g_xmm1=XMM_SCALAR(MEMF(0x44bf80));MEMF(g_edx)=g_xmm1.f[0];MEMF(g_edx+24)=g_xmm1.f[0];}else{MEMF(g_edx)=g_xmm0.f[0];MEMF(g_edx+24)=g_xmm0.f[0];}
 MEMF(g_edx+36)=g_xmm0.f[0];MEMF(g_edx+60)=g_xmm0.f[0];g_eax=MEM32(g_esi+36);g_ebx=1;
 if((int32_t)g_eax>1){
  PUSH32(g_esp,frame);frame=12;MEM32(g_esp+20)=48;g_edi=g_edx+116;
  do{
   g_eax=MEM32(g_esi+12);
   if(MEM8(g_ebx+g_eax)){
    g_ecx=MEM32(g_esi+16);g_edx=MEM32(g_ecx+g_ebx*4);g_ecx=MEM32(g_esi+32);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+24);g_eax=g_edi-52;PUSH32(g_esp,g_eax);g_ecx+=g_edx;PUSH32(g_esp,0x1f1792);sub_001F4E30();g_ecx=MEM32(g_esi+4);
    if(g_ebx!=MEM32(g_ecx+24)){g_edx=MEM32(g_esi+20);g_xmm0=XMM_SCALAR(MEMF(g_edx+frame));g_xmm0.f[0]+=MEMF(g_edi-4);MEMF(g_edi-4)=g_xmm0.f[0];g_eax=MEM32(g_esi+20);g_xmm0=XMM_SCALAR(MEMF(g_eax+frame+4));g_xmm0.f[0]+=MEMF(g_edi);MEMF(g_edi)=g_xmm0.f[0];g_ecx=MEM32(g_esi+20);g_xmm0=XMM_SCALAR(MEMF(g_ecx+frame+8));g_xmm0.f[0]+=MEMF(g_edi+4);MEMF(g_edi+4)=g_xmm0.f[0];}
   }
   MEM32(g_esp+20)+=48;g_eax=MEM32(g_esi+36);++g_ebx;frame+=12;g_edi+=64;
  }while((int32_t)g_ebx<(int32_t)g_eax);
  POP32(g_esp,frame);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=8;return 1;
}
