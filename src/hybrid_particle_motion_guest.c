/* Exact native loop; original emitter callbacks, bounds and return remain. */
#include "hybrid_particle_motion_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <immintrin.h>
#include <string.h>
#include <stdlib.h>

static unsigned char *view(uint32_t p,unsigned n){return p>=0x10000u&&(uint64_t)p+n<=g_xbox_total_ram?(unsigned char*)XBOX_PTR(p):NULL;}
static int apart(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)a+n<=b||(uint64_t)b+m<=a;}
static int finite_bytes(const void *p,unsigned n){const unsigned char *s=p;for(unsigned i=0;i<n;i+=4){uint32_t u;memcpy(&u,s+i,4);if((u&0x7fffffffu)>=0x7f800000u)return 0;}return 1;}

static int active(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_MOTION");value=p&&!strcmp(p,"1");}return value;}
int uc_hybrid_particle_motion(void){
 if(!active())return 0;
 const uint32_t object=g_ecx,sp=g_esp,frame=g_ebp;
 if((_mm_getcsr()&~63u)!=0x1f80u||!view(0x51c178,1)||MEM8(0x51c178)||
    !view(object,0x94)||frame<0x10090u||sp!=(frame&~15u)-0x80u||
    !view(sp,frame+40-sp)||g_esi||g_edx||MEM32(sp+0x14)||MEM32(sp+0x18))return 0;
 const uint32_t count=MEM32(object+0x90),data=MEM32(object+4),block=MEM32(object+0x14);
 if(count<8||count>1024||!view(data,count*32)||!view(block,0x8c)||
    !apart(data,count*32,object,0x94)||!apart(data,count*32,block,0x8c)||
    !apart(data,count*32,sp,frame+40-sp)||!apart(object,0x94,sp,frame+40-sp)||
    !apart(block,0x8c,sp,frame+40-sp)||!apart(data,count*32,0x51c178,1)||
    !apart(sp,frame+40-sp,0x51c178,1)||!view(0x51e4a0,16)||
    !apart(data,count*32,0x51e4a0,16)||!apart(sp,frame+40-sp,0x51e4a0,16))return 0;
 for(unsigned i=0;i<4;i++)if(MEM32(0x51e4a0+i*4)!=0x80000000u)return 0;
 if(!finite_bytes(g_xmm6.f,16)||!finite_bytes(g_xmm7.f,16)||
    !finite_bytes(g_xmm5.f,4)||!finite_bytes(view(frame+8,4),4)||
    !finite_bytes(view(frame+0x1c,4),4)||!finite_bytes(view(block+0x88,4),4))return 0;
 /* Integer inspection does not raise floating-point status on a declined call. */
 const unsigned char *input=view(data,count*32);
 const uint32_t step=MEM32(frame+0x10);
 for(unsigned i=0;i<count;i++){
  uint16_t age;memcpy(&age,input+i*32+28,2);
  if((uint16_t)(age+step)>=age&&!finite_bytes(input+i*32,32))return 0;
 }
 unsigned csr=_mm_getcsr(),live=0;uint16_t last_seed=0;
 unsigned char result[1024*32];
 __m128 x0=_mm_loadu_ps(g_xmm0.f),x1=_mm_loadu_ps(g_xmm1.f),x2=_mm_loadu_ps(g_xmm2.f);
 __m128 low=_mm_loadu_ps(g_xmm3.f),high=_mm_loadu_ps(g_xmm4.f);
 const __m128 gravity=_mm_loadu_ps(g_xmm6.f),wind=_mm_loadu_ps(g_xmm7.f);
 const __m128 sign=_mm_castsi128_ps(_mm_set1_epi32((int)0x80000000u));
 const __m128 drag=_mm_set1_ps(MEMF(frame+0x1c)),delta=_mm_set1_ps(MEMF(frame+8));
 uint32_t last_ebx=g_ebx;
 for(unsigned i=0;i<count;i++){
  uint16_t age,seed;memcpy(&age,input+i*32+28,2);memcpy(&seed,input+i*32+30,2);
  const uint16_t next=(uint16_t)(age+step);last_ebx=age;
  if(next<age)continue;
  __m128 old=_mm_loadu_ps((const float*)(input+i*32+16));
  __m128 resistance=_mm_mul_ps(_mm_xor_ps(old,sign),drag);
  x1=_mm_add_ps(_mm_add_ps(old,gravity),resistance);
  x2=_mm_mul_ps(x1,delta);
  x0=_mm_add_ps(_mm_add_ps(_mm_loadu_ps((const float*)(input+i*32)),x2),wind);
  x2=_mm_move_ss(x2,_mm_set_ss((float)next));
  x2=_mm_mul_ss(x2,_mm_set_ss(g_xmm5.f[0]));
  x2=_mm_add_ss(x2,_mm_set_ss(MEMF(block+0x88)));
  low=_mm_min_ps(low,x0);high=_mm_max_ps(high,x0);
  _mm_storeu_ps((float*)(result+live*32),x0);_mm_storeu_ps((float*)(result+live*32+16),x1);
  /* Preserve the retired fourth-lane work before publishing packed age/seed. */
  if(!finite_bytes(result+live*32,32)||!finite_bytes(&x2,16)){_mm_setcsr(csr);return 0;}
  memcpy(result+live*32+12,&x2,4);memcpy(result+live*32+28,&next,2);memcpy(result+live*32+30,&seed,2);
  ++live;last_seed=seed;last_ebx=block;
 }
 memcpy(view(data,live*32),result,live*32);
 MEM32(sp+0x14)=live;MEM32(sp+0x18)=count;if(live)MEM16(sp+0x1c)=last_seed;
 _mm_storeu_ps((float*)view(sp+0x40,16),low);_mm_storeu_ps((float*)view(sp+0x30,16),high);
 _mm_storeu_ps(g_xmm0.f,x0);_mm_storeu_ps(g_xmm1.f,x1);_mm_storeu_ps(g_xmm2.f,x2);
 _mm_storeu_ps(g_xmm3.f,low);_mm_storeu_ps(g_xmm4.f,high);
 g_eax=count;g_edi=count;g_esi=count*32;g_edx=live*32;g_ebx=last_ebx;
 return 1;
}
