#include "scene_portal_planes.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int valid(const float *p,uint32_t n){
    if(!p)return 0;
    for(uint32_t i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>1e12f)return 0;
    return 1;
}
static int normalize(float v[3],int direction){
    double x=v[0],y=v[1],z=v[2],length=sqrt(direction?(x*x+y*y)+z*z:(z*z+x*x)+y*y);
    if(!(length>0)||!isfinite(length))return 1;
    double reciprocal=1.0/length;
    for(unsigned j=0;j<3;j++){v[j]=(float)((double)v[j]*reciprocal);if(!isfinite(v[j]))return 1;}
    return 0;
}
int uc_scene_portal_planes_initialize(UCScenePortalPlanes *p){
    static const float n[5][3]={{1,0,0},{-1,0,0},{0,-1,0},{0,1,0},{0,0,1}};
    if(!p)return 1;p->count=5;
    for(unsigned i=0;i<5;i++){p->plane[i].kind=i==4;memcpy(p->plane[i].local_normal,n[i],12);memset(p->plane[i].local_point,0,12);}
    return 0;
}
int uc_scene_portal_planes_transform(UCScenePortalPlanes *p,const float a[16],const float b[16],const float c[16]){
    if(!p||p->count>5||!valid(a,16)||!valid(b,16)||!valid(c,16)||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    UCScenePortalPlanes q=*p;
    for(uint32_t i=0;i<q.count;i++){
        UCScenePortalPlane *v=q.plane+i;const float *n=v->local_normal,*t=v->local_point;
        if(!valid(n,3)||!valid(t,3))return 1;
        v->normal[0]=((c[1]*n[1]+n[2]*c[2])+n[0]*c[0])+c[3];
        v->normal[1]=((c[6]*n[2]+c[4]*n[0])+c[5]*n[1])+c[7];
        v->normal[2]=((c[9]*n[1]+c[10]*n[2])+c[8]*n[0])+c[11];
        if(normalize(v->normal,0))return 1;
        v->point[0]=(((t[2]*c[2]+c[1]*t[1])+t[0]*c[0])+c[3])+a[12];
        v->point[1]=(((c[4]*t[0]+c[6]*t[2])+t[1]*c[5])+c[7])+a[13];
        v->point[2]=(((t[2]*c[10]+c[8]*t[0])+c[9]*t[1])+c[11])+a[14];
        v->distance=0.f-((v->point[2]*v->normal[2]+v->point[1]*v->normal[1])+v->normal[0]*v->point[0]);
        v->local_distance=0.f-((t[2]*n[2]+n[0]*t[0])+t[1]*n[1]);
        if(!isfinite(v->distance)||!isfinite(v->local_distance))return 1;
    }
    q.direction[0]=((b[2]*100.f+b[1]*0.f)+b[0]*0.f)+b[3];
    q.direction[1]=((b[6]*100.f+b[5]*0.f)+b[4]*0.f)+b[7];
    q.direction[2]=((b[10]*100.f+b[9]*0.f)+b[8]*0.f)+b[11];
    if(normalize(q.direction,1))return 1;
    *p=q;return 0;
}
