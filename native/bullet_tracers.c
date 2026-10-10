#include "bullet_tracers.h"
#include <math.h>
#include <string.h>
static int finite_n(const float*p,unsigned n){if(!p)return 0;for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
void uc_bullet_tracers_init(UCBulletTracers*s){if(s)s->count=0;}
int uc_bullet_tracers_step(UCBulletTracers*s,float delta){
 if(!s||s->count>16||!isfinite(delta))return 1;if(!(delta>0))return 0;
 for(unsigned i=0;i<s->count;i++){UCBulletTracer*t=s->items+i;t->rgba[3]*=.75f;t->remaining-=500.f;
  if(t->remaining<=0||t->rgba[3]<.1f){--s->count;if(i!=s->count)s->items[i]=s->items[s->count];}}
 return 0;
}
int uc_bullet_tracers_append(UCBulletTracers*s,const float*a,const float*b,const float*color,float width,const UCBulletTracersServices*h){
 if(!s||s->count>16||!finite_n(a,3)||!finite_n(b,3)||!finite_n(color,4)||!isfinite(width)||!h||!h->listener||!h->flyby)return 1;
 if(a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2])return 0;
 double x=(double)a[0]-b[0],y=(double)a[1]-b[1],z=(double)a[2]-b[2];float distance=(float)sqrt((x*x+z*z)+y*y);
 if(!isfinite(distance)||distance<=0)return 1;
 if(s->count<16){UCBulletTracer*t=s->items+s->count++;memcpy(t->from,a,12);memcpy(t->to,b,12);memcpy(t->rgba,color,16);t->width=width;t->remaining=t->total=distance;}
 float camera[3];int present=h->listener(h->owner,camera);if(present<0)return 2;if(!present)return 0;if(!finite_n(camera,3))return 2;
 float toward[3],d[3],square[3];for(unsigned i=0;i<3;i++){toward[i]=camera[i]-a[i];d[i]=b[i]-a[i];square[i]=d[i]*d[i];}
 if(160000.f>((toward[0]*toward[0]+toward[2]*toward[2])+toward[1]*toward[1]))return 0;
 float qa=(square[0]+square[1])+square[2];
 float qb=(((a[1]-camera[1])*d[1])*2.f+((a[0]-camera[0])*d[0])*2.f)+((a[2]-camera[2])*d[2])*2.f;
 float qc=(((0.f-a[0]*camera[0])-a[1]*camera[1])-a[2]*camera[2])*2.f;
 qc+=a[2]*a[2];qc+=a[1]*a[1];qc+=a[0]*a[0];qc+=camera[2]*camera[2];qc+=camera[1]*camera[1];qc+=camera[0]*camera[0];qc-=160000.f;
 float discriminant=qb*qb-(qc*qa)*4.f;if(discriminant<0)return 0;
 float root=(float)sqrt(discriminant),denominator=qa*2.f;
 float high=(root-qb)/denominator,lo=((0.f-qb)-root)/denominator;
 if(high>1||lo>1)return 0;
 float t=((toward[2]*d[2]+toward[1]*d[1])+toward[0]*d[0])*(1.f/qa),nearby[3];
 for(unsigned i=0;i<3;i++)nearby[i]=(d[i]*t+a[i])-camera[i];
 float gain=(float)(1.-sqrt(((double)nearby[0]*nearby[0]+(double)nearby[2]*nearby[2])+(double)nearby[1]*nearby[1])*(double).0025f);
 double advance=800./sqrt(((double)square[0]+square[1])+square[2]);float start[3],end[3];
 float first=lo>high?high:lo;for(unsigned i=0;i<3;i++){start[i]=first*d[i]+a[i];end[i]=(float)((double)d[i]*(advance+first)+a[i]);}
 if(!(gain>.1f))return 0;float velocity[3];for(unsigned i=0;i<3;i++)velocity[i]=end[i]-start[i];
 float inv=(float)(1./sqrt(((double)velocity[2]*velocity[2]+(double)velocity[1]*velocity[1])+(double)velocity[0]*velocity[0]));
 for(unsigned i=0;i<3;i++)velocity[i]=(velocity[i]*inv)*1600.f;
 if(!finite_n(start,3)||!finite_n(velocity,3))return 2;return h->flyby(h->owner,start,velocity)?2:0;
}
int uc_bullet_tracers_vertices(const UCBulletTracers*s,const float*f,UCBulletTracerVertex*out,uint32_t cap){
 if(!s||s->count>16||!finite_n(f,3)||cap<s->count*3||(s->count&&!out))return 1;
 for(unsigned i=0;i<s->count;i++){const UCBulletTracer*t=s->items+i;float d[3],v[3],tail[3];float advance=1.f-t->remaining/t->total;
  for(unsigned k=0;k<3;k++){d[k]=t->to[k]-t->from[k];tail[k]=d[k]*advance+t->from[k];}
  v[0]=d[1]*f[2]-d[2]*f[1];v[1]=d[2]*f[0]-d[0]*f[2];v[2]=d[0]*f[1]-d[1]*f[0];
  double n=((double)v[2]*v[2]+(double)v[1]*v[1])+(double)v[0]*v[0];if(!(n>0)||!isfinite(n))return 1;
  float inv=(float)(1./sqrt(n)),grow=(t->remaining*.0005f)*5.f+1.f;
  for(unsigned k=0;k<3;k++)v[k]=(((v[k]*inv)*t->width)*.5f)*grow;
  UCBulletTracerVertex*p=out+i*3;for(unsigned k=0;k<3;k++){p[0].position[k]=tail[k];p[1].position[k]=t->to[k]+v[k];p[2].position[k]=t->to[k]-v[k];}
  const float uv[3][2]={{1,.5f},{0,0},{0,1}};for(unsigned k=0;k<3;k++){memcpy(p[k].uv,uv[k],8);memcpy(p[k].rgba,t->rgba,16);}
 }return 0;
}
