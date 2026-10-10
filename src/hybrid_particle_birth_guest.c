#include "hybrid_particle_birth_guest.h"
#include <stdlib.h>
#include "recomp_types.h"
#include <math.h>
#include <string.h>
#include "xbox_memory_layout.h"
static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_BIRTH");flag=v&&!strcmp(v,"1");}return flag;}
typedef struct{uint32_t a,n;} Span;
static int view(uint32_t a,uint32_t n){return a>=0x10000u&&a<0x80000000u&&(uint64_t)a+n<=g_xbox_total_ram;}
static int overlap(Span a,Span b){return (uint64_t)a.a+a.n>b.a&&(uint64_t)b.a+b.n>a.a;}
static int bounded(uint32_t a,unsigned n){for(unsigned i=0;i<n;i++)if((MEM32(a+4*i)&0x7fffffffu)>0x47800000u)return 0;return 1;}
static int finite_float(float x){uint32_t b;memcpy(&b,&x,4);return (b&0x7f800000u)!=0x7f800000u;}
/* Three draws share the original stream. Only cells written earlier in this
 * private suffix can differ from guest memory; no persistent RNG is created. */
typedef struct{uint32_t first,second,at[3],word[3],last_second;unsigned count;} Draws;
static uint32_t read_word(const Draws*d,uint32_t i){for(unsigned j=d->count;j;j--)if(d->at[j-1]==i)return d->word[j-1];return MEM32(0x51dd10+4*i);}
static uint32_t next_word(Draws*d){
 d->first=d->first==249?0:d->first+1;d->second=d->second==249?0:d->second+1;
 d->last_second=read_word(d,d->second);uint32_t w=read_word(d,d->first)^d->last_second;
 d->at[d->count]=d->first;d->word[d->count++]=w;return w;
}
static double unit(uint32_t w){int32_t v;memcpy(&v,&w,4);return (double)v*(double)MEMF(0x492d20);}
/* Exact synchronous suffix; worker0 qualifies the pinned title mode, not a lock.
 * Earlier original random draws, CRT calls and particle stores are retained. */
int uc_hybrid_particle_birth(unsigned s,unsigned f){
 if(!enabled())return 0;
 unsigned csr=_mm_getcsr();uint32_t q=s+8,d=g_ebx,m=g_edi,p=g_esi;unsigned t=(unsigned)g_fp_top;
 if((csr&~63u)!=0x1f80u||t>7||g_esp!=s||g_ebp!=f||!view(f,28)||!view(q-12,92)||
    q!=(f&~15u)-0x50u||MEM32(f+4)!=0x23919c||MEM32(f+8)!=d||MEM32(f+12)!=m||MEM32(f+24)!=p||
    !view(d,0x2c)||!view(m,48)||!view(p,32)||!view(0x51c178,1)||MEM8(0x51c178)||
    !view(0x51dd10,1000)||!view(0x51e0f8,4)||!view(0x504670,4)||
    !view(0x492d20,4)||!view(0x44c53c,4)||!view(0x44c174,4)||
    MEM32(0x492d20)!=0x30000000u||MEM32(0x44c53c)!=0x3d088889u||MEM32(0x44c174)!=0)return 0;
 Draws draws={0};draws.first=MEM32(0x51e0f8);draws.second=MEM32(0x504670);
 if(draws.first>=250||draws.second>=250)return 0;
 /* Read-only inputs may alias each other. Only publication destinations must
  * exclude reads and other writes; the fixed RNG/literal regions are mutually
  * disjoint by address. The first index immediately follows its1000B table. */
 Span scratch={q-12,92},particle={p,32};
 Span inputs[]={{f,28},{d+0x24,8},{m,48}};
 Span fixed[]={{0x51dd10,1004},{0x504670,4},{0x51c178,1},{0x492d20,4},{0x44c53c,4},{0x44c174,4}};
 if(overlap(scratch,particle))return 0;
 for(unsigned i=0;i<3;i++)if(overlap(inputs[i],scratch)||overlap(inputs[i],particle)||overlap(inputs[i],fixed[0])||overlap(inputs[i],fixed[1]))return 0;
 for(unsigned i=0;i<6;i++)if(overlap(fixed[i],scratch)||overlap(fixed[i],particle))return 0;
 uint64_t azimuth_bits;memcpy(&azimuth_bits,&g_fp_stack[t],8);
 if((azimuth_bits&0x7fffffffffffffffull)>0x40f0000000000000ull||!bounded(q+0x18,1)||!bounded(d+0x24,2)||!bounded(m,12)||!bounded(p,3))return 0;
 double azimuth=g_fp_stack[t],angle=(double)MEMF(q+0x18);
 float x=(float)cos(angle);double sine=sin(angle),dy=cos(azimuth)*sine,dz=sin(azimuth)*sine;
 float direction[4]={x,(float)dy,(float)dz,0};
 __m128 row2=_mm_loadu_ps((const float*)XBOX_PTR(m+32)),row1=_mm_loadu_ps((const float*)XBOX_PTR(m+16));
 row2=_mm_mul_ps(row2,_mm_set1_ps(direction[2]));row1=_mm_mul_ps(row1,_mm_set1_ps(direction[1]));
 __m128 transformed=_mm_mul_ps(_mm_loadu_ps((const float*)XBOX_PTR(m)),_mm_set1_ps(direction[0]));
 transformed=_mm_add_ps(transformed,row1);transformed=_mm_add_ps(transformed,row2);
 float vector[4];_mm_storeu_ps(vector,transformed);
 float force_mix=(float)unit(next_word(&draws));float force=MEMF(d+0x28)-MEMF(d+0x24);
 force=force*force_mix;force=force+MEMF(d+0x24);
 float velocity[3],delta[3],position[3];
 for(unsigned i=0;i<3;i++)velocity[i]=vector[i]*force;
 for(unsigned i=0;i<3;i++)delta[i]=velocity[i]*MEMF(0x44c53c);
 uint32_t offset_word=next_word(&draws);double offset_double=unit(offset_word);float offset=(float)offset_double;
 for(unsigned i=0;i<3;i++){position[i]=delta[i]*offset;position[i]=position[i]+MEMF(p+4*i);}
 uint32_t seed_word=next_word(&draws);
 for(unsigned i=0;i<4;i++)if(!finite_float(vector[i])){_mm_setcsr(csr);return 0;}
 for(unsigned i=0;i<3;i++)if(!finite_float(position[i])||!finite_float(velocity[i])||!finite_float(delta[i])){_mm_setcsr(csr);return 0;}
 /* All admission and arithmetic are complete before guest publication. */
 MEM32(q-12)=0x2d3766;MEM32(q-8)=offset_word;MEM32(q-4)=0x239a9b;
 MEMF(q+0x1c)=offset;memcpy((void*)XBOX_PTR(q+0x20),direction,16);memcpy((void*)XBOX_PTR(q+0x34),delta,12);memcpy((void*)XBOX_PTR(q+0x40),vector,16);
 memcpy((void*)XBOX_PTR(p),position,12);memcpy((void*)XBOX_PTR(p+0x10),velocity,12);MEM16(p+0x1c)=0;
 for(unsigned i=0;i<draws.count;i++)MEM32(0x51dd10+4*draws.at[i])=draws.word[i];
 MEM32(0x51e0f8)=draws.first;MEM32(0x504670)=draws.second;
 g_eax=g_edx=seed_word;g_ecx=draws.last_second;g_esp=q;g_ebp=f;g_recomp_zf=seed_word==0;
 g_xmm0=XMM_SCALAR(position[0]);g_xmm1=XMM_SCALAR(position[1]);g_xmm2=XMM_SCALAR(position[2]);g_xmm3=XMM_SCALAR(offset);
 g_fp_stack[(t+6)&7]=dy;g_fp_stack[(t+7)&7]=dz;g_fp_stack[t]=offset_double;g_fp_top=(t+1)&7;
 return 1;
}
