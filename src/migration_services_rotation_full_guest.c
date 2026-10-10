#include "migration_services_rotation_full_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_services_rotation_full(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_ROTATION_FULL");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x2d3dc0)return 0;
 uint32_t s=g_esp,d=g_ecx;
 if(!span(s-80,100)||!span(d,64))return 0;
 uint32_t pos=MEM32(s+16);
 if(!span(pos,16)||((uint64_t)d<s+20u&&(uint64_t)d+64>s-80u)||((uint64_t)pos<s+20u&&(uint64_t)pos+16>s-80u))return 0;
 uint32_t frame=s-4,work=(frame&~15u)-48;unsigned t=g_fp_top&7u;
 MEM32(frame)=g_ebp;g_ebp=frame;MEM32(work+44)=0;
 /* Preserve the original double trig / float matrix boundary. Combining
  * these products into a higher precision formula changes the guest result. */
 double x=MEMF(s+4),y=MEMF(s+8),z=MEMF(s+12);
 double sx=sin(x);MEMF(work+24)=(float)sx;
 double sy=sin(y);MEMF(work+20)=(float)sy;
 MEMD(work+8)=z;double sz=sin(z);MEMF(work+4)=(float)sz;
 double cx=cos(x);MEMF(work+28)=(float)cx;
 double cy=cos(y);MEMF(work)=(float)cy;
 double cz=cos(MEMD(work+8));MEMF(work+8)=(float)cz;
 g_xmm5=XMM_SCALAR(MEMF(work+24));g_xmm4=XMM_SCALAR(MEMF(work+20));g_xmm2=XMM_SCALAR(MEMF(work+4));
 g_xmm6=XMM_SCALAR(MEMF(work+28));g_xmm3=XMM_SCALAR(MEMF(work+8));g_ecx=MEM32(work+20);MEM32(work+40)=g_ecx;
 g_xmm1=XMM_SCALAR(g_xmm3.f[0]*g_xmm4.f[0]);
 g_fp_stack[(t+7)&7]=cz;g_fp_stack[(t+7)&7]*=MEMF(work);MEMF(work+32)=(float)g_fp_stack[(t+7)&7];g_xmm1.f[0]*=g_xmm5.f[0];
 g_fp_stack[(t+7)&7]=MEMF(work);g_fp_stack[(t+7)&7]*=MEMF(work+4);g_fp_stack[(t+7)&7]=-g_fp_stack[(t+7)&7];MEMF(work+36)=(float)g_fp_stack[(t+7)&7];
 g_xmm0=XMM_MEM(work+32);XMM_STORE(d,g_xmm0);
 g_fp_stack[(t+7)&7]=MEMF(work);g_fp_stack[(t+7)&7]*=MEMF(work+24);
 g_xmm0=g_xmm6;g_xmm0.f[0]*=g_xmm2.f[0];
 g_xmm1.f[0]+=g_xmm0.f[0];MEMF(work+32)=g_xmm1.f[0];
 g_xmm1=XMM_SCALAR(g_xmm3.f[0]*g_xmm6.f[0]);
 g_xmm6=g_xmm2;g_xmm6.f[0]*=g_xmm4.f[0];g_xmm7=g_xmm1;g_xmm6.f[0]*=g_xmm5.f[0];g_xmm7.f[0]-=g_xmm6.f[0];
 g_xmm6=XMM_ZERO();g_fp_stack[(t+7)&7]=-g_fp_stack[(t+7)&7];g_xmm0.f[0]*=g_xmm4.f[0];
 MEMF(work+40)=(float)g_fp_stack[(t+7)&7];MEMF(work+36)=g_xmm7.f[0];MEM32(work+44)=0;g_xmm7=XMM_MEM(work+32);
 g_xmm3.f[0]*=g_xmm5.f[0];g_xmm0.f[0]+=g_xmm3.f[0];MEMF(work+36)=g_xmm0.f[0];
 g_xmm0=XMM_SCALAR(MEMF(work));g_xmm0.f[0]*=MEMF(work+28);g_xmm2.f[0]*=g_xmm5.f[0];g_xmm1.f[0]*=g_xmm4.f[0];g_xmm2.f[0]-=g_xmm1.f[0];
 XMM_STORE(d+16,g_xmm7);MEMF(work+32)=g_xmm2.f[0];MEMF(work+40)=g_xmm0.f[0];MEM32(work+44)=0;
 g_xmm0=XMM_MEM(work+32);XMM_STORE(d+32,g_xmm0);g_edx=MEM32(s+16);g_xmm0=XMM_MEM(g_edx);XMM_STORE(d+48,g_xmm0);
 g_fp_stack[(t+6)&7]=cx;g_fp_stack[(t+5)&7]=sz;
 g_eax=d;g_recomp_zf=s==0;g_esp=s+20;return 1;
}
