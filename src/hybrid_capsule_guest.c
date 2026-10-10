#include "hybrid_capsule_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_CAPSULE_PREP");flag=v&&!strcmp(v,"1");}return flag;}
static uint32_t physical(uint32_t p){return p>=0x80000000u&&p<0x84000000u?p-0x80000000u:p;}
static unsigned char*view(uint32_t p,unsigned n){uint32_t a=physical(p);return p==a&&a>=0x10000u&&(uint64_t)a+n<=g_xbox_total_ram?(unsigned char*)XBOX_PTR(p):NULL;}
static int separate(uint32_t a,unsigned n,uint32_t b,unsigned m){return (uint64_t)physical(a)+n<=physical(b)||(uint64_t)physical(b)+m<=physical(a);}
static int bounded(const void*p,unsigned n){uint32_t bits[12];memcpy(bits,p,n*4);for(unsigned i=0;i<n;i++)if((bits[i]&0x7fffffffu)>0x47800000u)return 0;return 1;}
static int finite(RecompXmm v){for(unsigned i=0;i<4;i++)if((v.u[i]&0x7f800000u)==0x7f800000u)return 0;return 1;}
static RecompXmm cross(RecompXmm a,RecompXmm b){return XMM_SUB(XMM_MUL(XMM_SHUFFLE(a,a,0xc9),XMM_SHUFFLE(b,b,0xd2)),XMM_MUL(XMM_SHUFFLE(a,a,0xd2),XMM_SHUFFLE(b,b,0xc9)));}
static RecompXmm scale(RecompXmm a,float s){RecompXmm b=XMM_SCALAR(s);return XMM_MUL(a,XMM_SHUFFLE(b,b,0));}
int uc_hybrid_capsule_planes(uint32_t stack,uint32_t frame){
 if(!enabled()||(unsigned)g_fp_top>7u||stack!=g_esp||frame!=g_ebp||frame<0x10120u||stack!=(frame&~15u)-0x120u||
    !view(frame+4,24)||!view(stack+0x18,4)||!view(stack+0x50,0x70)||MEM32(frame+4)!=0x31ef67u||
    ((_mm_getcsr()&~63u)!=0x1f80u))return 0;
 uint32_t ends=MEM32(frame+8),triangle=MEM32(frame+0x10),cache=MEM32(frame+0x18);
 if(!view(ends,32)||!view(triangle,48)||!view(cache,20))return 0;
 /* Original stores scratch before later source reloads. Exclude physical
  * aliases with those stores before staging any arithmetic or publication. */
 const uint32_t inputs[][2]={{frame+4,24},{ends,32},{triangle,48},{cache,20}};
 for(unsigned i=0;i<4;i++)if(!separate(inputs[i][0],inputs[i][1],stack+0x18,4)||!separate(inputs[i][0],inputs[i][1],stack+0x50,0x70))return 0;
 RecompXmm t[3],p[2];float k[5];memcpy(t,view(triangle,48),48);memcpy(p,view(ends,32),32);memcpy(k,view(cache,20),20);
 if(!bounded(t,12)||!bounded(p,8)||!bounded(k,4))return 0;
 uint32_t area;memcpy(&area,k+4,4);if((area&0x7f800000u)==0x7f800000u)return 0;
 unsigned csr=_mm_getcsr();
 RecompXmm e0=XMM_SUB(t[2],t[1]),e1=XMM_SUB(t[0],t[2]),e2=XMM_SUB(t[1],t[0]);
 RecompXmm normal=scale(cross(e0,e1),k[3]);
 RecompXmm a=cross(scale(e0,k[0]),normal),b=cross(scale(e1,k[1]),normal),c=cross(scale(e2,k[2]),normal);
 /* These are the exact original transpose/store cuts, including W lanes. */
 RecompXmm columns[3];for(unsigned i=0;i<3;i++){columns[i].f[0]=a.f[i];columns[i].f[1]=b.f[i];columns[i].f[2]=c.f[i];columns[i].f[3]=normal.f[i];}
 double offset=-((double)k[4]*k[0]);RecompXmm offsets=XMM_ZERO();offsets.f[0]=(float)offset;
 RecompXmm distances[2],last_x,last_z,last_y;
 for(unsigned i=0;i<2;i++){
  RecompXmm relative=XMM_SUB(p[i],t[0]);last_x=XMM_SHUFFLE(relative,relative,0);
  last_y=XMM_MUL(columns[1],XMM_SHUFFLE(relative,relative,0x55));
  last_z=XMM_ADD(XMM_MUL(columns[2],XMM_SHUFFLE(relative,relative,0xaa)),offsets);
  distances[i]=XMM_ADD(XMM_ADD(XMM_MUL(columns[0],last_x),last_y),last_z);
 }
 if(!finite(normal)||!finite(columns[0])||!finite(columns[1])||!finite(columns[2])||!finite(offsets)||!finite(distances[0])||!finite(distances[1])){_mm_setcsr(csr);return 0;}
 /* No decline after this point. Preserve exact retired scratch and all CPU
  * state at 330681; arbitrary later manager callbacks remain valid. */
 memcpy(view(stack+0x18,4),k+2,4);memcpy(view(stack+0x50,16),&normal,16);
 memcpy(view(stack+0x60,32),distances,32);memcpy(view(stack+0x80,48),columns,48);memcpy(view(stack+0xb0,16),&offsets,16);
 g_eax=stack+0x80;g_ecx=0;g_edx=triangle;g_edi=ends;
 g_xmm0=distances[1];g_xmm1=normal;g_xmm2=offsets;g_xmm3=columns[2];g_xmm4=columns[1];g_xmm5=last_x;g_xmm6=last_z;g_xmm7=last_y;
 g_fp_stack[(g_fp_top+7u)&7u]=offset;
 return 1;
}
