#include "hybrid_visibility_guest.h"
/* Structured six-plane classification with the complete AC730 return contract. */
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <string.h>
#include <stdlib.h>

static int span(uint32_t p,unsigned n){return p>=0x10000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static int bounded(uint32_t p,unsigned n){for(unsigned i=0;i<n;i++)if((MEM32(p+i*4)&0x7fffffffu)>0x53800000u)return 0;return 1;}
static float mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
static float add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
static float sub(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}

static int active(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_VISIBILITY");value=p&&!strcmp(p,"1");}return value;}
int uc_hybrid_visibility(void){
 if(!active()||!span(0x51c178,1)||MEM8(0x51c178))return 0;
 const uint32_t sp=g_esp;
 if((_mm_getcsr()&~63u)!=0x1f80u||sp<0x10014u||!span(sp-20,36))return 0;
 const uint32_t matrix=MEM32(sp+4),box=MEM32(sp+8),plane_address=MEM32(sp+12);
 if(!span(matrix,64)||!span(box,24)||!span(plane_address,16)||
    !apart(matrix,64,sp-20,36)||!apart(box,24,sp-20,36)||!apart(plane_address,16,sp-20,36)||
    !bounded(matrix,16)||!bounded(box,6)||!bounded(plane_address,4))return 0;
 float limits[6],m[16],p[4],r[8]={0};
 memcpy(limits,(const void*)XBOX_PTR(box),sizeof limits);memcpy(m,(const void*)XBOX_PTR(matrix),sizeof m);
 unsigned inside=1,result=0;
 for(unsigned plane=0;plane<6;plane++){
  for(unsigned row=0;row<4;row++){
   unsigned n=row*4;
   switch(plane){
   case 0:p[row]=MEMF(plane_address+row*4);break;
   case 1:p[row]=sub(m[n+3],m[n+2]);break;
   case 2:p[row]=row==1||row==2?add(m[n],m[n+3]):add(m[n+3],m[n]);break;
   case 3:p[row]=sub(m[n+3],m[n]);break;
   case 4:p[row]=row==1||row==2?add(m[n+1],m[n+3]):add(m[n+3],m[n+1]);break;
   default:p[row]=sub(m[n+3],m[n+1]);break;
   }
  }
  float limit=sub(0.0f,p[3]);
  if(plane==5)MEMF(sp+12)=limit;
  for(unsigned side=0;side<(inside?2u:1u);side++){
   float x=limits[(p[0]>0.0f) != (side!=0)?3:0];
   float y=limits[(p[1]>0.0f) != (side!=0)?4:1];
   float z=limits[(p[2]>0.0f) != (side!=0)?5:2];
   float zz=mul(z,p[2]),yy=mul(y,p[1]);
   float sum=add(zz,yy),xx=mul(x,p[0]);sum=add(sum,xx);
   if(!plane){r[0]=p[2];r[1]=sum;r[2]=limit;r[4]=yy;r[5]=p[0];r[6]=xx;r[7]=p[1];}
   else{
    r[4]=p[0];r[6]=p[1];r[7]=p[2];
    if(plane==5&&side){r[0]=sum;r[1]=limit;r[2]=xx;}
    else{r[0]=limit;r[1]=sum;r[2]=yy;r[5]=xx;}
   }
   if((double)limit>(double)sum){if(!side)goto finished;inside=0;}
  }
 }
 result=1+inside;
finished:
 /* Original local scratch and the overwritten bounds argument are observable. */
 MEMF(sp-20)=limits[2];MEMF(sp-16)=limits[5];MEMF(sp-12)=limits[0];
 MEMF(sp-8)=limits[3];MEMF(sp-4)=limits[1];MEMF(sp+8)=limits[4];
 g_xmm0=XMM_SCALAR(r[0]);g_xmm1=XMM_SCALAR(r[1]);g_xmm2=XMM_SCALAR(r[2]);g_xmm3=XMM_ZERO();
 g_xmm4=XMM_SCALAR(r[4]);g_xmm5=XMM_SCALAR(r[5]);g_xmm6=XMM_SCALAR(r[6]);g_xmm7=XMM_SCALAR(r[7]);
 g_ecx=(g_ecx&0xffffff00u)|inside;g_eax=result;g_esp=sp+4;g_recomp_zf=0;
 return 1;
}
