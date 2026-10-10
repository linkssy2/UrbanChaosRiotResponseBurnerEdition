#include "scene_portal_clip.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
typedef struct Polygon {uint32_t count;float vertices[64][3];} Polygon;
static int values(const float *p,uint32_t n){
    if(!p)return 0;for(uint32_t i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>1e6f)return 0;return 1;
}
static int plane_values(const float *p,uint32_t n){
    for(uint32_t i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>1e9f)return 0;return 1;
}
static int mode(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
int uc_scene_portal_classify(const UCScenePortalLink *p,const float c[3],uint8_t *out){
    if(!p||!out||!p->vertex_count||!p->vertices||!values(p->vertices[0],3)||!values(p->normal,3)||!values(c,3)||!mode())return 1;
    const float *v=p->vertices[0],*n=p->normal;
    float x=v[0]-c[0],y=v[1]-c[1],z=v[2]-c[2];
    double d=(((double)c[2]-v[2])*n[2]+((double)c[1]-v[1])*n[1])+((double)c[0]-v[0])*n[0];
    *out=(uint8_t)((((n[2]*z+n[1]*y)+x*n[0])>0.f)|((fabs(d)<100.)<<1));return 0;
}
static float dot_negative(const float v[3],const UCScenePortalPlane *p){
    return 0.f-((p->normal[2]*v[2]+p->normal[1]*v[1])+v[0]*p->normal[0]);
}
static int clip_polygon(const Polygon *a,Polygon *b,const UCScenePortalPlane *p,int test){
    uint32_t count=0;float previous=dot_negative(a->vertices[0],p);int inside=previous-p->distance<0.f;
    for(uint32_t i=0;i<a->count;i++){
        const float *v=a->vertices[i],*w=a->vertices[(i+1)%a->count];
        float next=dot_negative(w,p);int next_inside=next-p->distance<0.f;
        if(inside){if(count==64)return -1;if(!test)memcpy(b->vertices[count],v,12);count++;}
        if(inside!=next_inside){
            if(count==64)return -1;
            if(!test){float ratio=(p->distance-previous)/(next-previous);for(unsigned j=0;j<3;j++)b->vertices[count][j]=(w[j]-v[j])*ratio+v[j];}
            count++;
        }
        previous=next;inside=next_inside;
    }
    if(!test)b->count=count;return count>=3;
}
int uc_scene_portal_clip(const UCScenePortalLink *link,const float camera[3],uint32_t count,
                         const UCScenePortalPlane *planes,UCScenePortalClipArena *arena,UCScenePortalClipBlock **out){
    if(!link||!out||!arena||!arena->data||((uintptr_t)arena->data&3u)||arena->used>arena->capacity||((arena->used|arena->capacity)&3u)||
       !arena->memory.allocate||!arena->memory.release||!mode()||!values(camera,3)||link->vertex_count<3||link->vertex_count>32||
       !link->vertices||count>32||(count&&!planes))return 1;
    for(uint32_t i=0;i<link->vertex_count;i++)if(!values(link->vertices[i],3))return 1;
    uint32_t kept=0;
    for(uint32_t i=0;i<count;i++){
        if(!plane_values(planes[i].normal,3)||!plane_values(&planes[i].distance,1))return 1;
        kept+=planes[i].kind!=0;if(kept>2)return 1;
    }
    Polygon polygons[2];polygons[0].count=link->vertex_count;memcpy(polygons[0].vertices,link->vertices,12*link->vertex_count);
    unsigned current=0;const UCScenePortalPlane *preserved[2];uint32_t retained=0;
    for(uint32_t i=0;i<count;i++){
        int test=planes[i].kind!=0;
        int result=clip_polygon(polygons+current,polygons+(1-current),planes+i,test);
        if(result<0)return 1;if(!result){*out=NULL;return 0;}
        if(test)preserved[retained++]=planes+i;else current=1-current;
    }
    Polygon *p=polygons+current;
    if(p->count+kept>64)return 1;
    uint32_t size=4+60*((p->count+kept<6)?6:p->count+kept);
    UCScenePortalClipBlock *b;
    if(size<=arena->capacity-arena->used){b=(UCScenePortalClipBlock*)(arena->data+arena->used);arena->used+=size;}
    else{b=arena->memory.allocate(arena->memory.owner,size);if(!b)return 2;}
    b->count=0;
    for(uint32_t i=0;i<p->count;i++){
        const float *v=p->vertices[i],*w=p->vertices[(i+1)%p->count];float a[3],z[3];
        for(unsigned j=0;j<3;j++){a[j]=v[j]-camera[j];z[j]=w[j]-camera[j];}
        if(a[0]==z[0]&&a[1]==z[1]&&a[2]==z[2])continue;
        UCScenePortalPlane *q=b->plane+i;
        q->normal[0]=z[2]*a[1]-a[2]*z[1];q->normal[1]=a[2]*z[0]-z[2]*a[0];q->normal[2]=z[1]*a[0]-a[1]*z[0];
        float *n=q->normal;
        if((n[0]*n[0]+n[2]*n[2])+n[1]*n[1]>0.0001f){
            double x=n[0],y=n[1],zv=n[2],inv=1.0/sqrt((x*x+zv*zv)+y*y);
            for(unsigned j=0;j<3;j++)n[j]=(float)((double)n[j]*inv);
        }
        q->kind=0;memcpy(q->point,v,12);q->distance=0.f-((v[2]*n[2]+n[1]*v[1])+n[0]*v[0]);b->count++;
    }
    for(uint32_t i=0;i<retained;i++)b->plane[b->count++]=*preserved[i];
    *out=b;return 0;
}
