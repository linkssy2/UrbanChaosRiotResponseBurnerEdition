#include "character_collision.h"
#include <math.h>
#include <string.h>
static int finite_values(const float*p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
int uc_character_collision_pose(const float*a,const float*b,float*out){
 if(!a||!b||!out||!finite_values(a,16)||!finite_values(b,16))return 1;
 float result[16];
 for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++){
  float p0=b[c]*a[r*4],p1=b[4+c]*a[r*4+1],p2=b[8+c]*a[r*4+2],p3=b[12+c]*a[r*4+3];
  result[r*4+c]=(p0+p1)+(p2+p3);
 }
 if(!finite_values(result,16))return 1;
 for(unsigned r=0;r<3;r++){
  const float*p=result+r*4;double x=p[0],y=p[1],z=p[2],w=p[3];
  double length=sqrt(((x*x+y*y)+z*z)+w*w);if(!(length>0)||!isfinite(length))return 1;
  double reciprocal=1.0/length;for(unsigned c=0;c<4;c++)result[r*4+c]=(float)(result[r*4+c]*reciprocal);
 }
 memcpy(out,result,16);memcpy(out+4,result+8,16);memcpy(out+8,result+4,16);memcpy(out+12,result+12,16);return 0;
}
static double dot(const double*a,const double*b){return (a[0]*b[0]+a[1]*b[1])+a[2]*b[2];}
static void subtract(const float*a,const float*b,double*out){for(unsigned j=0;j<3;j++)out[j]=(double)a[j]-b[j];}
static void sphere(const double*start,const double*delta,double radius,double maximum,double*best){
 double aa=dot(delta,delta),bb=dot(start,delta),cc=dot(start,start)-radius*radius;
 double discriminant=bb*bb-aa*cc;if(discriminant<0)return;
 double t=(-bb-sqrt(discriminant))/aa;if(t>=0&&t<maximum&&t<*best)*best=t;
}
int uc_character_capsule_cast(const UCCharacterCapsule*s,const float*from,const float*to,float limit,UCCollisionTriangleHit*out){
 if(!s||!from||!to||!out||!finite_values(s->a,3)||!finite_values(s->b,3)||!finite_values(from,3)||!finite_values(to,3)||
    !isfinite(s->radius)||s->radius<=0||!isfinite(limit)||limit<0||limit>1)return -1;
 double axis[3],delta[3],start[3];subtract(s->b,s->a,axis);subtract(to,from,delta);subtract(from,s->a,start);
 double axis2=dot(axis,axis),delta2=dot(delta,delta),along=dot(start,axis),radius=s->radius;
 if(!(delta2>0)||limit==0)return 0;
 double fraction=axis2>0?along/axis2:0;if(fraction<0)fraction=0;if(fraction>1)fraction=1;
 double radial[3];for(unsigned j=0;j<3;j++)radial[j]=start[j]-fraction*axis[j];
 if(dot(radial,radial)<=radius*radius)return 0;
 double best=limit;
 if(axis2>0){
  double heading=dot(axis,delta),cross=dot(start,delta),aa=axis2*delta2-heading*heading;
  double bb=axis2*cross-along*heading,cc=axis2*(dot(start,start)-radius*radius)-along*along;
  double discriminant=bb*bb-aa*cc;
  if(aa>0&&discriminant>=0){double t=(-bb-sqrt(discriminant))/aa,y=along+t*heading;
   if(t>=0&&t<best&&y>=0&&y<=axis2)best=t;}
 }
 sphere(start,delta,radius,limit,&best);double end_start[3];subtract(from,s->b,end_start);sphere(end_start,delta,radius,limit,&best);
 if(!(best<limit))return 0;
 double point[3];for(unsigned j=0;j<3;j++)point[j]=start[j]+best*delta[j];
 fraction=axis2>0?dot(point,axis)/axis2:0;if(fraction<0)fraction=0;if(fraction>1)fraction=1;
 for(unsigned j=0;j<3;j++)radial[j]=point[j]-fraction*axis[j];double length=sqrt(dot(radial,radial));if(!(length>0))return -1;
 UCCollisionTriangleHit hit={0};hit.fraction=(float)best;hit.shape_key=UINT32_MAX;
 for(unsigned j=0;j<3;j++)hit.normal[j]=(float)(radial[j]/length);*out=hit;return 1;
}
