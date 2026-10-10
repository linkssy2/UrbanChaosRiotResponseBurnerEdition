#include "order_target.h"
#include "point_light_receiver.h"
#include <float.h>
#include <math.h>
#include <string.h>
static int finite(const float*v,unsigned n){for(unsigned i=0;i<n;++i)if(!isfinite(v[i]))return 0;return 1;}
static int slab(const float*p,const float*d,const UCModelBounds*b,float*out){
 float near=0,far=FLT_MAX;
 for(unsigned i=0;i<3;++i){if(d[i]>-1.e-8f&&d[i]<1.e-8f){if(b->minimum[i]>p[i]||p[i]>b->maximum[i])return 0;}
  else {float a=b->minimum[i]-p[i],z=b->maximum[i]-p[i],inverse=1.f/d[i];a*=inverse;z*=inverse;
   float low=z>a?a:z;if(low>near)near=low;if(z>a)a=z;if(!(a>far))far=a;if(near>far)return 0;}}
 *out=near;return 1;
}
int uc_order_target_select(const float*from,const float*direction,const UCOrderTarget*t,uint32_t n,uint32_t*ordinal,float*distance){
 if(!from||!direction||!ordinal||!distance||(n&&!t)||n>65536||!finite(from,3)||!finite(direction,3))return -1;
 uint32_t selected=UINT32_MAX;float closest=FLT_MAX;
 for(uint32_t k=0;k<n;++k){const UCOrderTarget*v=t+k;if(v->flags&0x100)continue;
  UCModelBounds b=v->bounds;float inverse[16],p[3],d[3];
  if(!finite((const float*)&b,12)||!uc_point_light_inverse(v->world,inverse))return -1;
  for(unsigned i=0;i<3;++i)if(b.minimum[i]>b.maximum[i])return -1;
  double size=sqrt(((double)b.extent[2]*b.extent[2]+(double)b.extent[1]*b.extent[1])+(double)b.extent[0]*b.extent[0]);
  if(size<50)for(unsigned i=0;i<3;++i){float lo=b.center[i]-25,hi=b.center[i]+25;if(b.minimum[i]>lo)b.minimum[i]=lo;if(hi>b.maximum[i])b.maximum[i]=hi;}
  for(unsigned i=0;i<3;++i)p[i]=(inverse[i]*from[0]+inverse[4+i]*from[1])+(inverse[8+i]*from[2]+inverse[12+i]);
  d[0]=(direction[1]*inverse[4]+direction[2]*inverse[8])+direction[0]*inverse[0];
  d[1]=(direction[2]*inverse[9]+direction[0]*inverse[1])+direction[1]*inverse[5];
  d[2]=(direction[0]*inverse[2]+direction[1]*inverse[6])+direction[2]*inverse[10];
  if(!finite(p,3)||!finite(d,3))return -1;float hit;
  if(slab(p,d,&b,&hit)&&(selected==UINT32_MAX||closest>hit)){selected=k;closest=hit;}
 }
 *ordinal=selected;*distance=closest;return selected!=UINT32_MAX;
}
int uc_order_target_ray(const float*m,float*from,float*to,float*direction){
 if(!m||!from||!to||!direction||!finite(m,16))return 1;float end[3],delta[3];
 for(unsigned i=0;i<3;++i){end[i]=(m[i]*0.f+m[4+i]*0.f)+(m[8+i]*10000.f+m[12+i]);delta[i]=end[i]-m[12+i];}
 double length=sqrt(((double)delta[2]*delta[2]+(double)delta[1]*delta[1])+(double)delta[0]*delta[0]);
 if(!(length>0)||!isfinite(length))return 1;float inverse=(float)(1./length);
 for(unsigned i=0;i<3;++i){from[i]=m[12+i];to[i]=end[i];direction[i]=delta[i]*inverse;}return 0;
}

int uc_order_target_resolve(const float*from,const float*to,const float*hit,uint8_t candidate,uint8_t same,float distance,float*point){
 if(!from||!to||!point||!finite(from,3)||!finite(to,3)||(hit&&!finite(hit,3))||candidate>1||same>1||
    (same&&(!hit||!candidate))||(candidate&&(!isfinite(distance)||distance<0)))return -1;
 float limit=FLT_MAX;
 if(hit){double x=(double)hit[0]-from[0],y=(double)hit[1]-from[1],z=(double)hit[2]-from[2];
  limit=(float)(sqrt((z*z+y*y)+x*x)+10.);}
 memcpy(point,hit?hit:to,12);return candidate&&(limit>distance||same);
}
