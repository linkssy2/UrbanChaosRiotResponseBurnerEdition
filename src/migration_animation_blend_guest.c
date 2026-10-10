#include "migration_animation_blend_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
extern void sub_001F4DA0(void);
extern void sub_001F4CA0(void);
extern void sub_001F4BA0(void);
extern void sub_002D51B0(void);
static int blend_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int blend_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int blend_gate(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_BLEND");enabled=v&&!strcmp(v,"1");}
 return enabled&&!(g_esp&3)&&g_esp>=0x10400&&blend_span(g_esp-1024,1044)&&blend_span(0x51c178,1)&&!MEM8(0x51c178)&&blend_span(0x44bf94,4)&&g_fp_top>=0&&g_fp_top<8;
}
static int blend_buffer(uint32_t p,unsigned n,unsigned args){return blend_span(p,n)&&blend_apart(p,n,g_esp-1024,1024+args)&&blend_apart(p,n,0x44bf94,4)&&blend_apart(p,n,0x51c178,1);}
static unsigned blend_compare(double a,double b){return(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;}
static int blend_bounded(uint32_t p,unsigned n,uint32_t limit){for(unsigned i=0;i<n;i+=4)if((MEM32(p+i)&0x7fffffffu)>limit)return 0;return 1;}
/* The ordered list owns contribution lifetime until the final head clear.
 * Shared quaternion interpolation remains a synchronous math boundary. */
int uc_migration_animation_blend_finish(void){
 if(!blend_gate()||!blend_buffer(g_ecx,48,4))return 0;
 uint32_t node=MEM32(g_ecx+44);
 if(node){
  uint32_t accumulated=MEM32(g_ecx+32);if(accumulated>0x3f800000u&&!((accumulated&0x7fffffffu)==0))return 0;
  if(!blend_bounded(g_ecx,16,0x41800000)||!blend_bounded(g_ecx+16,12,0x501502f9))return 0;
  unsigned count=0;
  do{
   if(++count>2048||!blend_buffer(node,40,4)||!blend_apart(node,40,g_ecx,48)||!blend_bounded(node,16,0x41800000)||!blend_bounded(node+16,12,0x501502f9))return 0;
   uint32_t weight=MEM32(node+28),factor=MEM32(node+32);if((weight>0x3f800000u&&(weight&0x7fffffffu))||(factor>0x41800000u&&(factor&0x7fffffffu)))return 0;
   node=MEM32(node+36);
  }while(node);
 }
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-0x44;g_xmm0=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_esi=MEM32(g_edi+44);g_xmm0.f[0]-=MEMF(g_edi+32);
 while(g_esi){
  g_xmm0.f[0]+=MEMF(g_esi+28);g_xmm1=XMM_SCALAR(MEMF(g_esi+28));SET_HI8(g_eax,blend_compare(g_xmm1.f[0],g_xmm0.f[0]));int interpolate=RECOMP_PARITY8(HI8(g_eax)&0x44);MEMF(g_esp+12)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+16));
  if(!interpolate){
   g_ebx=g_esi;g_eax=MEM32(g_ebx);g_ecx=MEM32(g_ebx+4);g_edx=MEM32(g_ebx+8);g_ebx=MEM32(g_ebx+12);MEM32(g_esp+0x3c)=g_ebx;MEMF(g_esp+0x40)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+20));MEMF(g_esp+0x44)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+24));
   g_ebx=g_edi;MEM32(g_ebx)=g_eax;g_eax=MEM32(g_esp+0x3c);MEM32(g_ebx+4)=g_ecx;MEM32(g_ebx+8)=g_edx;g_edx=MEM32(g_esp+0x40);MEM32(g_ebx+12)=g_eax;g_eax=MEM32(g_esp+0x44);g_ecx=g_edi+16;MEM32(g_ecx)=g_edx;MEM32(g_ecx+4)=g_eax;MEMF(g_esp+0x48)=g_xmm0.f[0];g_edx=MEM32(g_esp+0x48);MEM32(g_ecx+8)=g_edx;
   g_eax=MEM32(g_esi+32);PUSH32(g_esp,g_eax);g_ecx=g_edi;g_ebp=frame;PUSH32(g_esp,0x1f0efe);sub_001F4DA0();
  }else{
   g_ecx=g_esi;g_edx=MEM32(g_ecx);g_eax=MEM32(g_ecx+4);MEM32(g_esp+16)=g_edx;g_edx=MEM32(g_ecx+8);MEM32(g_esp+20)=g_eax;g_eax=MEM32(g_ecx+12);g_ecx=MEM32(g_esi+32);MEMF(g_esp+32)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+20));MEMF(g_esp+36)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esi+24));
   PUSH32(g_esp,g_ecx);g_ecx=g_esp+20;MEM32(g_esp+28)=g_edx;MEM32(g_esp+32)=g_eax;MEMF(g_esp+44)=g_xmm0.f[0];g_ebp=frame;PUSH32(g_esp,0x1f0f46);sub_001F4DA0();
   g_xmm0=XMM_SCALAR(MEMF(g_esi+28));g_xmm0.f[0]/=MEMF(g_esp+12);PUSH32(g_esp,g_ecx);g_edx=g_esp+20;MEMF(g_esp)=g_xmm0.f[0];PUSH32(g_esp,g_edx);g_ecx=g_edi;g_ebp=frame;PUSH32(g_esp,0x1f0f63);sub_001F4CA0();
  }
  g_esi=MEM32(g_esi+36);if(g_esi)g_xmm0=XMM_SCALAR(MEMF(g_esp+12));
 }
 g_xmm0=XMM_ZERO();MEMF(g_edi+32)=g_xmm0.f[0];MEM32(g_edi+44)=0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;RECOMP_TRACE_EXIT("sub_001F0E50",0x1f0e50);g_esp+=4;return 1;
}
int uc_migration_animation_blend_scale(void){
 if(!blend_gate()||!blend_buffer(g_ecx,28,8))return 0;
 g_xmm1=XMM_SCALAR(MEMF(g_esp+4));g_xmm2=XMM_SCALAR(MEMF(0x44bf94));SET_HI8(g_eax,blend_compare(g_xmm1.f[0],g_xmm2.f[0]));
 if(RECOMP_PARITY8(HI8(g_eax)&0x44)){
  g_xmm0=XMM_ZERO();SET_HI8(g_eax,blend_compare(g_xmm1.f[0],g_xmm0.f[0]));
  if(!RECOMP_PARITY8(HI8(g_eax)&0x44)){
   MEMF(g_ecx+8)=g_xmm0.f[0];MEMF(g_ecx+4)=g_xmm0.f[0];MEMF(g_ecx)=g_xmm0.f[0];MEMF(g_ecx+12)=g_xmm2.f[0];MEMF(g_ecx+20)=g_xmm0.f[0];MEMF(g_ecx+16)=g_xmm0.f[0];MEMF(g_ecx+24)=g_xmm0.f[0];
  }else for(unsigned offset=12;offset<=24;offset+=4){g_xmm0=XMM_SCALAR(MEMF(g_ecx+offset));g_xmm0.f[0]*=g_xmm1.f[0];MEMF(g_ecx+offset)=g_xmm0.f[0];}
 }
 g_recomp_zf=(HI8(g_eax)&0x44)==0;g_esp+=8;return 1;
}
int uc_migration_animation_blend_rotation(void){
 if(!blend_gate()||!blend_buffer(MEM32(g_esp+4),16,20)||!blend_buffer(MEM32(g_esp+8),16,20)||!blend_buffer(MEM32(g_esp+12),16,20))return 0;
 g_esp-=32;g_eax=MEM32(g_esp+40);g_ecx=MEM32(g_eax);g_edx=MEM32(g_eax+4);MEM32(g_esp)=g_ecx;g_ecx=MEM32(g_eax+8);MEM32(g_esp+4)=g_edx;g_edx=MEM32(g_eax+12);g_eax=MEM32(g_esp+48);PUSH32(g_esp,g_esi);MEM32(g_esp+12)=g_ecx;g_ecx=MEM32(g_esp+48);PUSH32(g_esp,g_eax);MEM32(g_esp+20)=g_edx;PUSH32(g_esp,g_ecx);g_edx=g_esp+28;PUSH32(g_esp,g_edx);g_ecx=g_esp+16;PUSH32(g_esp,0x1f4bda);sub_002D51B0();
 g_esi=MEM32(g_eax);g_ecx=MEM32(g_esp+40);g_edx=g_ecx;MEM32(g_edx)=g_esi;g_esi=MEM32(g_eax+4);MEM32(g_edx+4)=g_esi;g_esi=MEM32(g_eax+8);MEM32(g_edx+8)=g_esi;g_eax=MEM32(g_eax+12);MEM32(g_edx+12)=g_eax;g_eax=g_ecx;POP32(g_esp,g_esi);g_esp+=32;g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
int uc_migration_animation_blend_mix(void){
 if(!blend_gate()||!blend_buffer(g_ecx,28,12)||!blend_buffer(MEM32(g_esp+4),28,12))return 0;
 g_esp-=28;g_eax=MEM32(g_esp+36);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+40);PUSH32(g_esp,g_eax);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_ecx=g_esp+28;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x1f4cbc);sub_001F4BA0();
 g_ecx=MEM32(g_eax);g_xmm1=XMM_SCALAR(MEMF(g_esp+60));g_xmm0=XMM_SCALAR(MEMF(0x44bf94));g_edx=g_esi;MEM32(g_edx)=g_ecx;g_ecx=MEM32(g_eax+4);MEM32(g_edx+4)=g_ecx;g_ecx=MEM32(g_eax+8);MEM32(g_edx+8)=g_ecx;g_eax=MEM32(g_eax+12);MEM32(g_edx+12)=g_eax;
 g_xmm4=XMM_SCALAR(MEMF(g_edi+16));g_xmm5=XMM_SCALAR(MEMF(g_edi+20));g_xmm6=XMM_SCALAR(MEMF(g_edi+24));g_xmm2=XMM_SCALAR(MEMF(g_esi+16));g_xmm7=XMM_SCALAR(MEMF(g_esi+20));g_xmm3=XMM_SCALAR(MEMF(g_esi+24));g_eax=g_esi+16;
 g_xmm0.f[0]-=g_xmm1.f[0];g_xmm5.f[0]*=g_xmm1.f[0];g_xmm6.f[0]*=g_xmm1.f[0];g_xmm4.f[0]*=g_xmm1.f[0];g_xmm1=g_xmm0;g_xmm1.f[0]*=g_xmm2.f[0];g_xmm2=g_xmm0;g_xmm3.f[0]*=g_xmm0.f[0];g_xmm1.f[0]+=g_xmm4.f[0];MEMF(g_esp+24)=g_xmm1.f[0];g_ecx=MEM32(g_esp+24);g_xmm2.f[0]*=g_xmm7.f[0];g_xmm2.f[0]+=g_xmm5.f[0];MEMF(g_esp+28)=g_xmm2.f[0];g_edx=MEM32(g_esp+28);MEM32(g_esp+36)=g_ecx;g_xmm0=XMM_SCALAR(MEMF(g_esp+36));g_xmm3.f[0]+=g_xmm6.f[0];MEMF(g_esp+32)=g_xmm3.f[0];g_ecx=MEM32(g_esp+32);MEM32(g_esp+40)=g_edx;MEMF(g_esp+24)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+40));g_edx=MEM32(g_esp+24);MEM32(g_esp+44)=g_ecx;MEMF(g_esp+28)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+44));g_ecx=MEM32(g_esp+28);g_esp+=16;MEM32(g_eax)=g_edx;MEMF(g_esp+16)=g_xmm0.f[0];g_edx=MEM32(g_esp+16);POP32(g_esp,g_edi);MEM32(g_eax+4)=g_ecx;MEM32(g_eax+8)=g_edx;POP32(g_esp,g_esi);g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
