#include "scene_portal_data.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int fp(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
static int valid(const UCScenePortalData *p){
    if(!p||p->vertex_count>4096||(p->vertex_count>0&&!p->vertices))return 0;
    for(int32_t i=0;i<p->vertex_count;i++)for(unsigned j=0;j<3;j++)if(!isfinite(p->vertices[i][j]))return 0;
    return 1;
}
int uc_scene_portal_normal(UCScenePortalData *p){
    if(!fp()||!valid(p)||p->vertex_count<3)return 1;
    float a[3],b[3],n[3];
    for(unsigned j=0;j<3;j++){a[j]=p->vertices[0][j]-p->vertices[1][j];b[j]=p->vertices[2][j]-p->vertices[1][j];}
    n[0]=b[2]*a[1]-a[2]*b[1];n[1]=a[2]*b[0]-b[2]*a[0];n[2]=b[1]*a[0]-a[1]*b[0];
    double z=n[2],y=n[1],x=n[0],length=sqrt((z*z+y*y)+x*x);
    if(!(length>0)||!isfinite(length))return 1;
    float reciprocal=(float)(1.0/length);for(unsigned j=0;j<3;j++){n[j]=n[j]*reciprocal;if(!isfinite(n[j]))return 1;}
    memcpy(p->normal,n,sizeof n);return 0;
}
static uint8_t contains(const UCScenePortalData *a,const UCScenePortalData *b){
    for(int32_t i=0;i<a->vertex_count;i++){
        int32_t j;
        for(j=0;j<b->vertex_count;j++){
            double x=(double)a->vertices[i][0]-b->vertices[j][0];
            double y=(double)a->vertices[i][1]-b->vertices[j][1];
            double z=(double)a->vertices[i][2]-b->vertices[j][2];
            if((double)0.05f>sqrt((z*z+x*x)+y*y))break;
        }
        if(j==b->vertex_count||b->vertex_count<=0)return 0;
    }
    return 1;
}
int uc_scene_portal_contains(const UCScenePortalData *a,const UCScenePortalData *b,uint8_t *result){
    if(!result||!fp()||!valid(a)||!valid(b))return 1;
    *result=contains(a,b);return 0;
}
int uc_scene_portal_group(UCScenePortalData *p,uint32_t count,const UCScenePortalMemory *m){
    if(!fp()||count>4096||(count&&!p)||!m||!m->allocate||!m->release)return 1;
    for(uint32_t i=0;i<count;i++)if(!valid(p+i))return 1;
    uint8_t *marked=m->allocate(m->owner,count);if(count&&!marked)return 2;
    if(count)memset(marked,0,count);
    for(uint32_t i=0;i<count;i++)if(!marked[i])for(uint32_t j=i+1;j<count;j++)
        if(!marked[j]&&contains(p+i,p+j)){p[j].group=i;marked[j]=1;}
    m->release(m->owner,marked);return 0;
}
