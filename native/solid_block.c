#include "solid_block.h"
#include "collision_box_axes.h"
#include <math.h>
#include <fenv.h>
#include <string.h>
#include <xmmintrin.h>
int uc_solid_block_cast(const UCSolidBlockGeometry *b,const float from[3],const float to[3],float limit,UCCollisionTriangleHit *out){
 if(!b||!from||!to||!out)return -1;
 double axes[3][3];if(uc_collision_box_axes(b->rotation,axes))return -1;
 for(unsigned i=0;i<3;i++)if(!isfinite(b->center[i])||!isfinite(from[i])||!isfinite(to[i]))return -1;
 float a[3],z[3];for(unsigned i=0;i<3;i++){
  double x=0,y=0;for(unsigned k=0;k<3;k++){x+=((double)from[k]-b->center[k])*axes[i][k];y+=((double)to[k]-b->center[k])*axes[i][k];}
  a[i]=(float)x;z[i]=(float)y;
 }
 UCCollisionTriangleHit hit;int status=uc_solid_block_cast_local(b->half,b->radius,a,z,limit,&hit);if(status!=1)return status;
 float normal[3];for(unsigned k=0;k<3;k++){double n=0;for(unsigned i=0;i<3;i++)n+=hit.normal[i]*axes[i][k];normal[k]=(float)n;}
 memcpy(hit.normal,normal,sizeof normal);*out=hit;return 1;
}
int uc_solid_block_cast_local(const float half[3],float radius,const float from[3],const float to[3],float limit,UCCollisionTriangleHit *out){
 if(!half||!from||!to||!out||!isfinite(radius)||radius<0||!isfinite(limit)||limit<0||limit>1||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return -1;
 float h[3];unsigned high_from=0,high_to=0,low_from=0,low_to=0;
 for(unsigned i=0;i<3;i++){
  if(!isfinite(half[i])||half[i]<0||!isfinite(from[i])||!isfinite(to[i])||!isfinite(half[i]+radius))return -1;
  h[i]=half[i]+radius;high_from|=(unsigned)(from[i]>=h[i])<<i;high_to|=(unsigned)(to[i]>=h[i])<<i;
  low_from|=(unsigned)(from[i]<=-h[i])<<i;low_to|=(unsigned)(to[i]<=-h[i])<<i;
 }
 if((high_from&high_to)||(low_from&low_to)||!(high_from|low_from))return 0;
 float enter=0,leave=limit;UCCollisionTriangleHit result={{0,0,0,0},UINT32_MAX,0};
 for(unsigned side=0;side<2;side++){
  unsigned first=side?low_from:high_from,mask=first|(side?low_to:high_to);double sign=side?-1.:1.;
  for(int i=2;i>=0;i--)if(mask&(1u<<i)){
   double a=sign*from[i]-h[i],b=sign*to[i]-h[i],fraction=a/(a-b);
   if(!isfinite(fraction))return -1;
   if(first&(1u<<i)){if(fraction>=enter){enter=(float)fraction;memset(result.normal,0,sizeof result.normal);result.normal[i]=(float)sign;}}
   else if(leave>=fraction)leave=(float)fraction;
   if(leave<enter)return 0;
  }
 }
 result.fraction=enter;*out=result;return 1;
}
static float squared(__m128 v){v=_mm_mul_ps(v,v);v=_mm_add_ps(v,_mm_movehl_ps(v,v));return _mm_cvtss_f32(_mm_add_ss(v,_mm_shuffle_ps(v,v,1)));}
static float inverse(float n){float r=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(n)));return(.5f*r)*(3.f-(r*n)*r);}
static __m128 cross(__m128 a,__m128 b){return _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(a,a,0xC9),_mm_shuffle_ps(b,b,0xD2)),_mm_mul_ps(_mm_shuffle_ps(a,a,0xD2),_mm_shuffle_ps(b,b,0xC9)));}
int uc_solid_block_geometry(const float *m,float radius,UCSolidBlockGeometry*out){
 if(!m||!out||!isfinite(radius)||radius<0||radius>1e4f||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
 for(unsigned i=0;i<16;i++)if(!isfinite(m[i])||fabsf(m[i])>1e6f)return 1;
 if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)return 1;
 __m128 x=_mm_loadu_ps(m),y=_mm_loadu_ps(m+4),z=_mm_loadu_ps(m+8);
 float nx=squared(x),ny=squared(y),nz=squared(z);if(nx<1e-12f||ny<1e-12f||nz<1e-12f)return 1;
 UCSolidBlockGeometry g;g.radius=radius;g.half[0]=(nx*inverse(nx))*.5f;g.half[1]=(nz*inverse(nz))*.5f;g.half[2]=(ny*inverse(ny))*.5f;
 float position[4];_mm_storeu_ps(position,_mm_add_ps(_mm_add_ps(_mm_mul_ps(x,_mm_setzero_ps()),_mm_mul_ps(y,_mm_set1_ps(50.f))),_mm_add_ps(_mm_mul_ps(z,_mm_setzero_ps()),_mm_loadu_ps(m+12))));
 g.center[0]=position[0]*.01f;g.center[1]=position[2]*.01f;g.center[2]=position[1]*.01f;
 x=_mm_mul_ps(x,_mm_set1_ps(inverse(nx)));y=_mm_mul_ps(y,_mm_set1_ps(inverse(ny)));z=cross(x,y);y=cross(z,x);
 if(squared(z)<1e-12f)return 1;
 /*2D4640, then row1/row2 swap and full2A2CA0 constant-matrix product.
  * Keep zero products/addition order instead of a direct component permutation. */
 float rows[12],a[12];_mm_storeu_ps(rows,x);_mm_storeu_ps(rows+4,z);_mm_storeu_ps(rows+8,y);
 const float axes[4][4]={{1,0,0,0},{0,0,1,0},{0,1,0,0},{0,0,0,1}};
 for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)a[r*4+c]=(axes[0][c]*rows[r*4]+axes[1][c]*rows[r*4+1])+(axes[2][c]*rows[r*4+2]+axes[3][c]*rows[r*4+3]);
 double trace=((double)a[5]+a[0])+a[10];
 if(trace>0){double s=sqrt(trace+1),v=.5/s;g.rotation[0]=(float)(((double)a[6]-a[9])*v);g.rotation[1]=(float)(((double)a[8]-a[2])*v);g.rotation[2]=(float)(((double)a[1]-a[4])*v);g.rotation[3]=(float)(s*.5);}
 else{unsigned i=a[5]>a[0]?1u:0u;if(a[10]>a[i*5])i=2;unsigned j=(i+1)%3,k=(j+1)%3;
  double s=sqrt(((double)a[i*5]-((double)a[k*5]+a[j*5]))+1),v=.5/s;
  g.rotation[i]=(float)(s*.5);g.rotation[3]=(float)(((double)a[j*4+k]-a[k*4+j])*v);
  g.rotation[j]=(float)(((double)a[j*4+i]+a[i*4+j])*v);g.rotation[k]=(float)(((double)a[k*4+i]+a[i*4+k])*v);
 }
 for(unsigned i=0;i<4;i++)if(!isfinite(g.rotation[i]))return 1;
 *out=g;return 0;
}
int uc_solid_block_construct(UCSolidBlock*s,void*owner,int(*base)(void*)){
 if(!s||!base)return 1;if(base(owner))return 2;s->body=NULL;s->state=0;return 0;
}
int uc_solid_block_update(UCSolidBlock*s,const uint32_t*flags,const UCSolidBlockServices*h){
 if(!s||!flags||!h||!h->prepare||!h->add||!h->remove||!h->actor_update||((s->state==1||s->state==2)&&!s->body))return 1;
 if(s->state==0){if(h->prepare(h->owner,s))return 2;s->state=1;}
 else if(s->state==1&&!(*flags&0x10000000)){if(h->remove(h->owner,s->body))return 2;s->state=2;}
 else if(s->state==2&&(*flags&0x10000000)){if(h->add(h->owner,s->body))return 2;s->state=1;}
 return h->actor_update(h->owner)?2:0;
}
