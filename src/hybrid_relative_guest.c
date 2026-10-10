#include "hybrid_relative_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>

static int active(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_RELATIVE_TRANSFORM");enabled=v&&!strcmp(v,"1");}return enabled;
}
static unsigned char*view(uint32_t p,unsigned n){return p>=0x10000u&&(uint64_t)p+n<=g_xbox_total_ram?(unsigned char*)XBOX_PTR(p):NULL;}
static int separate(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static int finite(const void*p,unsigned bytes){
 const unsigned char*s=p;for(unsigned i=0;i<bytes;i+=4){uint32_t b;memcpy(&b,s+i,4);if((b&0x7fffffffu)>=0x7f800000u)return 0;}return 1;
}
int uc_hybrid_relative_transform(void){
 if(!active()||(_mm_getcsr()&~63u)!=0x1f80u||!view(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t sp=g_esp,out=g_ecx;
 if((sp&3)||sp<0x10024u||!view(sp,12))return 0;
 uint32_t frame=sp-4,aligned=frame&~15u,low=aligned-16,a=MEM32(sp+4),b=MEM32(sp+8);
 if(!view(low,sp+12-low)||!view(out,64)||!view(a,64)||!view(b,64)||
    !separate(out,64,a,64)||!separate(out,64,b,64)||!separate(out,64,low,sp+12-low)||
    !separate(a,64,low,sp+12-low)||!separate(b,64,low,sp+12-low)||
    !separate(out,64,0x51c178,1)||!separate(low,sp+12-low,0x51c178,1)||
    !finite(view(a,64),64)||!finite(view(b,64),64))return 0;
 unsigned csr=_mm_getcsr();
 __m128 A=_mm_loadu_ps((const float*)view(a,16)),B=_mm_loadu_ps((const float*)view(a+16,16)),C=_mm_loadu_ps((const float*)view(a+32,16));
 __m128 lo=_mm_shuffle_ps(A,B,0x44),hi=_mm_shuffle_ps(A,B,0xee),cl=_mm_shuffle_ps(C,C,0x44),ch=_mm_shuffle_ps(C,C,0xee);
 __m128 x=_mm_shuffle_ps(lo,cl,0x88),y=_mm_shuffle_ps(lo,cl,0xdd),z=_mm_shuffle_ps(hi,ch,0x88);
 float result[4][4];
 for(unsigned i=0;i<3;i++){
  __m128 v=_mm_loadu_ps((const float*)view(b+i*16,16));
  __m128 px=_mm_mul_ps(x,_mm_shuffle_ps(v,v,0)),py=_mm_mul_ps(y,_mm_shuffle_ps(v,v,0x55)),pz=_mm_mul_ps(z,_mm_shuffle_ps(v,v,0xaa));
  _mm_storeu_ps(result[i],_mm_add_ps(_mm_add_ps(px,py),pz));
 }
 __m128 v=_mm_sub_ps(_mm_loadu_ps((const float*)view(b+48,16)),_mm_loadu_ps((const float*)view(a+48,16)));
 __m128 vx=_mm_shuffle_ps(v,v,0),vy=_mm_shuffle_ps(v,v,0x55),vz=_mm_shuffle_ps(v,v,0xaa);
 __m128 py=_mm_mul_ps(y,vy),pz=_mm_mul_ps(z,vz),answer=_mm_add_ps(_mm_add_ps(_mm_mul_ps(x,vx),py),pz);
 _mm_storeu_ps(result[3],answer);
 if(!finite(result,sizeof result)){_mm_setcsr(csr);return 0;}
 /* Exact source frame and final SIMD state; original dynamics remain owners. */
 MEM32(frame)=g_ebp;MEM32(aligned-4)=out;MEM32(aligned-8)=g_ebx;MEM32(aligned-12)=g_esi;MEM32(aligned-16)=g_edi;
 memcpy(view(out,64),result,64);
 _mm_storeu_ps(g_xmm0.f,v);_mm_storeu_ps(g_xmm1.f,answer);_mm_storeu_ps(g_xmm2.f,pz);_mm_storeu_ps(g_xmm3.f,vx);
 _mm_storeu_ps(g_xmm4.f,cl);_mm_storeu_ps(g_xmm5.f,py);_mm_storeu_ps(g_xmm6.f,vz);_mm_storeu_ps(g_xmm7.f,ch);
 g_eax=b+48;g_edx=a;g_ebp=frame;g_esp=sp+12;g_recomp_zf=1;return 1;
}
