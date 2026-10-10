#include "hybrid_solver_pair_guest.h"
#include <stdlib.h>
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <string.h>
static int enabled(unsigned repeat){static int flags[2]={-1,-1};int *flag=&flags[repeat];if(*flag<0){const char*v=getenv(repeat?"UCRR_NATIVE_SOLVER_REPEAT":"UCRR_NATIVE_SOLVER_PAIR");*flag=v&&!strcmp(v,"1");}return *flag;}
static void*view(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram?(void*)XBOX_PTR(p):NULL;}
static int disjoint(uint32_t a,unsigned n,uint32_t b,unsigned m){return(uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static int bounded(const void*p,unsigned n){const unsigned char*b=p;for(unsigned i=0;i<n;i++){uint32_t u;memcpy(&u,b+i*4,4);if((u&0x7fffffffu)>0x47800000u)return 0;}return 1;}
static int finite_words(const void*p,unsigned n){const unsigned char*b=p;for(unsigned i=0;i<n;i++){uint32_t u;memcpy(&u,b+i*4,4);if((u&0x7f800000u)==0x7f800000u)return 0;}return 1;}
/* Exact initial or consecutive paired-contact math. Original row selection,
 * iteration and dispatch remain. All state is staged before publication.
 * The pinned title initializes worker mode to zero. Its guard qualifies that
 * configuration; it cannot synchronize arbitrary already-created workers. */
static int solve_pair(uint32_t stack,uint32_t frame,unsigned repeat){
 if(!enabled(repeat)||!view(0x51c178,1)||MEM8(0x51c178)!=0||stack!=g_esp||frame!=g_ebp||(unsigned)g_fp_top>7u||(_mm_getcsr()&~63u)!=0x1f80u||!view(stack+0x14,repeat?0xd8:0xd0)||!view(frame+8,4))return 0;
 uint32_t aa=g_edx,bb=MEM32(stack+0x18),cfg=MEM32(frame+8),cmd=g_ebx,jac=g_esi,imp=g_edi;
 if(!repeat&&g_ecx!=jac+0x50)return 0;
 const uint32_t spans[][2]={{stack+0x14,repeat?0xd8u:0xd0u},{frame+8,4},{aa+0x10,48},{bb+0x10,48},{cfg+4,8},{cmd,8},{jac,96},{imp,8},{0x44c174,4},{0x51c178,1}};
 for(unsigned i=0;i<10;i++){if(!view(spans[i][0],spans[i][1]))return 0;for(unsigned j=0;j<i;j++)if(!disjoint(spans[i][0],spans[i][1],spans[j][0],spans[j][1]))return 0;}
 if(MEM32(cmd)!=0x50803||MEM32(0x44c174)!=0||!bounded(view(aa+0x10,48),12)||!bounded(view(bb+0x10,48),12)||!bounded(view(cfg+4,8),2)||!bounded(view(jac,96),24)||!bounded(view(imp,8),2)||!bounded(view(cmd+4,4),1))return 0;
 RecompXmm a[3],b[3],row[6],x0,x1,x2,x3,x4;float impulses[2],factors[2],velocity[2];
 memcpy(a,view(aa+0x10,48),48);memcpy(b,view(bb+0x10,48),48);memcpy(row,view(jac,96),96);memcpy(impulses,view(imp,8),8);memcpy(factors,view(cfg+4,8),8);
 unsigned csr=_mm_getcsr();float coupling=MEMF(cmd+4);
 x4=row[0];x1=XMM_SUB(a[0],b[0]);x0=XMM_MUL(a[1],row[1]);x3=XMM_MUL(b[1],row[2]);x1=XMM_MUL(x1,x4);x0=XMM_ADD(x0,x3);x0=XMM_ADD(x0,x1);
 x1=XMM_SHUFFLE(x0,x0,0x55);x1.f[0]=x1.f[0]+x0.f[0];x2=XMM_SHUFFLE(x0,x0,0xaa);x2.f[0]=x2.f[0]+x1.f[0];velocity[0]=x2.f[0];
 x1=XMM_SUB(a[0],b[0]);x0=XMM_MUL(a[1],row[4]);x2=XMM_MUL(b[1],row[5]);x3=row[3];x1=XMM_MUL(x1,x3);x0=XMM_ADD(x0,x2);x0=XMM_ADD(x0,x1);
 x1=XMM_SHUFFLE(x0,x0,0x55);x1.f[0]=x1.f[0]+x0.f[0];x2=XMM_SHUFFLE(x0,x0,0xaa);x2.f[0]=x2.f[0]+x1.f[0];velocity[1]=x2.f[0];
 double r0d=(double)row[0].f[3]*factors[0];double product=(double)velocity[0]*factors[1];r0d=r0d-product;float r0=(float)r0d;
 double r1=(double)row[3].f[3]*factors[0];product=(double)velocity[1]*factors[1];r1=r1-product;
 double d0d=(double)row[2].f[3]*r0;product=r1*coupling;d0d=d0d+product;float d0=(float)d0d;
 double d1d=(double)row[5].f[3]*r1;double fp3=(double)r0*coupling;d1d=d1d+fp3;float d1=(float)d1d;
 double last;float single=0;unsigned active=0;int cmp=RECOMP_FCMP((double)d0,0.0);uint32_t eax;
 if(cmp>0&&cmp!=2){cmp=RECOMP_FCMP((double)d1,0.0);if(cmp>0&&cmp!=2){active=3;last=d0;}else{last=(double)r0*row[1].f[3];cmp=RECOMP_FCMP(last,0.0);if(cmp>0&&cmp!=2)active=1;}}
 else{last=r1*row[4].f[3];cmp=RECOMP_FCMP(last,0.0);if(cmp>0&&cmp!=2)active=2;else{last=(double)r0*row[1].f[3];cmp=RECOMP_FCMP(last,0.0);if(cmp>0&&cmp!=2)active=1;}}
 eax=(cfg&0xffff0000u)|(((g_fp_top+7u)&7u)<<11)|(cmp==2?0x4500u:cmp<0?0x100u:cmp>0?0:0x4000u);
 if(active){
  if(active!=3)single=(float)last;
  for(unsigned i=0;i<2;i++)if(active&(1u<<i)){
   float delta=active==3?(i?d1:d0):single;if(active==3)last=delta;
   x4=row[i*3];x1=XMM_SCALAR(delta);x0=XMM_MUL(XMM_SHUFFLE(x1,x1,0),a[2]);x1=XMM_MUL(XMM_SHUFFLE(x1,x1,0),b[2]);
   x2=XMM_MUL(XMM_SHUFFLE(x0,x0,0xff),x4);x3=XMM_MUL(XMM_SHUFFLE(x1,x1,0xff),x4);
   x4=XMM_ADD(a[0],x2);a[0]=x4;x2=XMM_SUB(b[0],x3);b[0]=x2;x0=XMM_MUL(x0,row[i*3+1]);x1=XMM_MUL(x1,row[i*3+2]);
   x2=XMM_ADD(a[1],x0);a[1]=x2;x0=XMM_ADD(b[1],x1);b[1]=x0;last=last+impulses[i];impulses[i]=(float)last;
  }
  eax=repeat?bb+0x20:bb;
 }
 if(!finite_words(a,8)||!finite_words(b,8)||!finite_words(impulses,2)||!finite_words(&x0,4)||!finite_words(&x1,4)||!finite_words(&x2,4)||!finite_words(&x3,4)||!finite_words(&x4,4)||!finite_words(velocity,2)||!finite_words(&r0,1)||!finite_words(&d0,1)||!finite_words(&d1,1)||!finite_words(&single,1)||!isfinite(last)||!isfinite(fp3)){_mm_setcsr(csr);return 0;}
 if(repeat){
  MEM32(stack+0x1c)=bb+0x20;MEMF(stack+0xe8)=velocity[0];MEMF(stack+0x60)=velocity[1];
  MEMF(stack+0x14)=r0;MEMF(stack+0x28)=d0;MEMF(stack+0x20)=d1;
  if(active==3){MEMF(stack+0xb0)=d0;MEMF(stack+0x68)=d1;MEM32(stack+0x14)=bb+0x30;}
  if(active&2)MEM32(stack+0x28)=jac+0x30;
  if(active&&active!=3)MEMF(stack+0x20)=single;
  g_ecx=bb+0x10;
 }else{
 MEMF(stack+0x14)=d1;MEMF(stack+0x1c)=r0;MEMF(stack+0x28)=d0;MEMF(stack+0x78)=velocity[0];MEMF(stack+0xe0)=velocity[1];
 if(active==3){MEMF(stack+0x50)=d0;MEMF(stack+0xa8)=d1;}else if(active)MEMF(stack+0x20)=single;
 }
 memcpy(view(aa+0x10,32),a,32);memcpy(view(bb+0x10,32),b,32);memcpy(view(imp,8),impulses,8);
 g_eax=eax;g_xmm0=x0;g_xmm1=x1;g_xmm2=x2;g_xmm3=x3;g_xmm4=x4;g_fp_cmp=cmp;
 g_fp_stack[(g_fp_top+7u)&7u]=last;g_fp_stack[(g_fp_top+6u)&7u]=d0;g_fp_stack[(g_fp_top+5u)&7u]=fp3;
 return 1;
}
int uc_hybrid_solver_pair(uint32_t stack,uint32_t frame){return solve_pair(stack,frame,0);}
int uc_hybrid_solver_repeat(uint32_t stack,uint32_t frame){return solve_pair(stack,frame,1);}
