#include "hybrid_solver_guest.h"
#include <stdlib.h>
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <string.h>
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_SOLVER_CONTACT");flag=v&&!strcmp(v,"1");}return flag;}
static void*view(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram?(void*)XBOX_PTR(p):NULL;}
static int disjoint(uint32_t a,unsigned n,uint32_t b,unsigned m){return(uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static int bounded(const void*p,unsigned n){const unsigned char*b=p;for(unsigned i=0;i<n;i++){uint32_t u;memcpy(&u,b+i*4,4);if((u&0x7fffffffu)>0x47800000u)return 0;}return 1;}
static int finite_words(const void*p,unsigned n){const unsigned char*b=p;for(unsigned i=0;i<n;i++){uint32_t u;memcpy(&u,b+i*4,4);if((u&0x7f800000u)==0x7f800000u)return 0;}return 1;}
int uc_hybrid_solver_contact(uint32_t stack,uint32_t frame){
 if(!enabled()||!view(0x51c178,1)||MEM8(0x51c178)!=0||stack!=g_esp||frame!=g_ebp||(unsigned)g_fp_top>7u||(_mm_getcsr()&~63u)!=0x1f80u||
    !view(stack+0x18,0x44)||!view(frame+8,4))return 0;
 /* This mode guard states the pinned title startup contract; it is not a
  * synchronization mechanism for arbitrary already-created guest workers. */
 uint32_t aa=g_edx,bb=MEM32(stack+0x18),cfg=MEM32(frame+8),cmd=g_ebx,jac=g_esi,imp=g_edi;
 unsigned n=0;for(;n<64;n++){if(!view(cmd+n*4,5)||MEM8(cmd+n*4)!=2)return 0;if(MEM8(cmd+n*4+4)!=2){++n;break;}}
 if(n<4||(n==64&&MEM8(cmd+n*4)==2))return 0;
 const uint32_t spans[][2]={{stack+0x18,0x44},{frame+8,4},{aa+0x10,48},{bb+0x10,48},{cfg+4,8},{cmd,n*4+1},{jac,n*48},{imp,n*4},{0x44c174,4},{0x51c178,1}};
 for(unsigned i=0;i<10;i++){if(!view(spans[i][0],spans[i][1]))return 0;for(unsigned j=0;j<i;j++)if(!disjoint(spans[i][0],spans[i][1],spans[j][0],spans[j][1]))return 0;}
 if(MEM32(0x44c174)!=0||!bounded(view(aa+0x10,48),12)||!bounded(view(bb+0x10,48),12)||!bounded(view(cfg+4,8),2)||!bounded(view(jac,n*48),n*12)||!bounded(view(imp,n*4),n))return 0;
 RecompXmm a[3],b[3],x0,x1,x2,x3,x4;float impulses[64],velocity=0,delta_float=0;unsigned any=0;
 memcpy(a,view(aa+0x10,48),48);memcpy(b,view(bb+0x10,48),48);memcpy(impulses,view(imp,n*4),n*4);
 float factors[2];memcpy(factors,view(cfg+4,8),8);unsigned csr=_mm_getcsr();double last=0,product=0;int cmp=0;uint32_t eax=0;
 for(unsigned i=0;i<n;i++){
  RecompXmm row[3];memcpy(row,view(jac+i*48,48),48);
  x4=row[0];x1=XMM_SUB(a[0],b[0]);x0=XMM_MUL(a[1],row[1]);x3=XMM_MUL(b[1],row[2]);
  x1=XMM_MUL(x1,x4);x0=XMM_ADD(x0,x3);x0=XMM_ADD(x0,x1);
  x1=XMM_SHUFFLE(x0,x0,0x55);x2=XMM_SHUFFLE(x0,x0,0xaa);x1.f[0]=x1.f[0]+x0.f[0];x2.f[0]=x2.f[0]+x1.f[0];velocity=x2.f[0];
  last=(double)row[0].f[3]*factors[0];product=(double)velocity*factors[1];last=last-product;
  cmp=RECOMP_FCMP(last,0.0);eax=(cfg&0xffff0000u)|(((g_fp_top+7u)&7u)<<11)|(cmp==2?0x4500u:cmp<0?0x100u:cmp>0?0:0x4000u);
  if(cmp>0&&cmp!=2){
   last=last*row[1].f[3];delta_float=(float)last;any=1;
   x1=XMM_SCALAR(delta_float);x0=XMM_SHUFFLE(x1,x1,0);x0=XMM_MUL(x0,a[2]);x1=XMM_SHUFFLE(x1,x1,0);x1=XMM_MUL(x1,b[2]);
   x2=XMM_MUL(XMM_SHUFFLE(x0,x0,0xff),x4);x3=XMM_MUL(XMM_SHUFFLE(x1,x1,0xff),x4);
   x4=XMM_ADD(a[0],x2);a[0]=x4;x2=XMM_SUB(b[0],x3);b[0]=x2;
   x0=XMM_MUL(x0,row[1]);x1=XMM_MUL(x1,row[2]);x2=XMM_ADD(a[1],x0);a[1]=x2;x0=XMM_ADD(b[1],x1);b[1]=x0;
   last=last+impulses[i];impulses[i]=(float)last;eax=bb+0x20;
  }
 }
 if(!finite_words(a,8)||!finite_words(b,8)||!finite_words(impulses,n)||!finite_words(&x0,4)||!finite_words(&x1,4)||!finite_words(&x2,4)||!finite_words(&x3,4)||!finite_words(&x4,4)||!finite_words(&velocity,1)||!finite_words(&delta_float,1)||!isfinite(last)||!isfinite(product)){_mm_setcsr(csr);return 0;}
 /* No fallback after publication. The original row order remains sequential;
  * original iteration dispatch, other constraints and callbacks remain outside. */
 MEM32(stack+0x1c)=bb+0x20;MEMF(stack+0x58)=velocity;if(any)MEMF(stack+0x20)=delta_float;
 memcpy(view(aa+0x10,32),a,32);memcpy(view(bb+0x10,32),b,32);memcpy(view(imp,n*4),impulses,n*4);
 g_eax=(eax&0xffffff00u)|MEM8(cmd+n*4);g_ecx=bb+0x10;g_ebx=cmd+n*4;g_esi=jac+n*48;g_edi=imp+n*4;
 g_xmm0=x0;g_xmm1=x1;g_xmm2=x2;g_xmm3=x3;g_xmm4=x4;
 g_fp_cmp=cmp;g_fp_stack[(g_fp_top+7u)&7u]=last;g_fp_stack[(g_fp_top+6u)&7u]=product;
 return 1;
}
