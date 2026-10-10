#include "scene_portal_move.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static float dot(const float*a,const float*b){return(a[2]*b[2]+a[1]*b[1])+a[0]*b[0];}
static void subtract(float*out,const float*a,const float*b){for(unsigned j=0;j<3;j++)out[j]=a[j]-b[j];}
static int finite3(const float*p){for(unsigned j=0;j<3;j++)if(!isfinite(p[j])||fabsf(p[j])>1e6f)return 0;return 1;}
static int contains(const UCScenePortalRoom*r,const float*p){
    for(unsigned j=0;j<3;j++)if(p[j]<r->minimum[j]||r->maximum[j]<p[j])return 0;
    return 1;
}
static int edge(const float*p,const float*a,const float*b,float*v,float*t){
    float d[3];subtract(d,p,a);subtract(v,b,a);float divisor=dot(v,v);
    if(!(divisor>0))return 1;*t=dot(v,d)/divisor;return !isfinite(*t);
}
/* A8960 projects onto the dominant-normal plane and stops at the first
 * failing edge. Its ordered edge walk is intentionally not a global minimum. */
static int polygon(const UCScenePortalLink*l,const float*p,float*out,int*inside){
    const float(*v)[3]=l->vertices;unsigned n=l->vertex_count;
    float x=fabsf(l->normal[0]),y=fabsf(l->normal[1]),z=fabsf(l->normal[2]);
    unsigned dominant=y>z?(x>y?0:1):(x>z?0:2);
    unsigned a=(dominant+1)%3,b=(dominant+2)%3,i=0,j=n-1;
    for(;i<n;j=i++){
        float side=((p[b]-v[j][b])*(v[i][a]-v[j][a])-(p[a]-v[j][a])*(v[i][b]-v[j][b]))*l->normal[dominant];
        if(side<0)break;
    }
    *inside=i==n;if(*inside){memcpy(out,p,12);return 0;}
    float direction[3],t;if(edge(p,v[j],v[i],direction,&t))return 1;
    if(t<0){
        if(i){memcpy(out,v[j],12);return 0;}
        while(j>=1&&t<0){i=j;--j;if(edge(p,v[j],v[i],direction,&t))return 1;}
    }else if(t>1){
        while(i+1<n&&t>1){j=i;++i;if(edge(p,v[j],v[i],direction,&t))return 1;}
    }
    if(t<0)memcpy(out,v[j],12);else if(t>1)memcpy(out,v[i],12);
    else for(unsigned k=0;k<3;k++)out[k]=v[j][k]+direction[k]*t;
    return !finite3(out);
}
static int crossing(const UCScenePortalGraph*g,UCScenePortalRoom*r,float*old,
                    const float*now,uint8_t*ambiguous,UCScenePortalRoom**next){
    *next=NULL;
    for(uint32_t i=0;i<r->count;i++){
        const UCScenePortalLink*l=r->links+i;if(!g->enabled[l->group])continue;
        float constant=0.f-dot(l->normal,l->vertices[0]);
        if(dot(l->normal,old)+constant>=0.f||0.f>dot(l->normal,now)+constant)continue;
        float delta[3],offset[3],point[3],nearest[3];subtract(delta,old,now);subtract(offset,l->vertices[0],now);
        float t=dot(l->normal,offset)/dot(l->normal,delta);if(!isfinite(t))return 1;
        for(unsigned j=0;j<3;j++)point[j]=now[j]+delta[j]*t;
        if(!finite3(point))return 1;
        int inside;if(polygon(l,point,nearest,&inside))return 1;
        subtract(offset,point,nearest);float distance=(offset[1]*offset[1]+offset[2]*offset[2])+offset[0]*offset[0];
        if(!inside&&distance>10000.f)continue;
        memcpy(old,point,12);
        if(l->neighbor_count>1)*ambiguous=1;
        else if(l->neighbor_count&&l->neighbors[0]!=r)*next=l->neighbors[0];
        return 0;
    }
    return 0;
}
int uc_scene_portal_move(const UCScenePortalGraph*g,const float matrix[16],UCScenePortalStart*s,uint8_t*ambiguity){
    if(!g||g->active!=1||!g->rooms||!g->room_count||g->room_count>4096||g->portal_count>4096||
       (g->portal_count&&!g->enabled)||!matrix||!s||!ambiguity||!s->rooms||
       s->capacity<g->room_count||s->capacity>4096||s->count>g->room_count||s->initialized>1||
       (!s->initialized&&s->count)||(_mm_getcsr()&0xE040u)||!finite3(matrix+12)||!finite3(s->position))return 1;
    for(uint32_t i=0;i<g->room_count;i++){
        const UCScenePortalRoom*r=g->rooms[i];if(!r||r->count>4096||(r->count&&!r->links)||!finite3(r->minimum)||!finite3(r->maximum))return 1;
        for(uint32_t j=0;j<r->count;j++){const UCScenePortalLink*l=r->links+j;
            if(l->group>=g->portal_count||l->vertex_count<3||l->vertex_count>4096||!l->vertices||
               l->neighbor_count>4096||(l->neighbor_count&&!l->neighbors)||!finite3(l->normal))return 1;
            for(uint32_t k=0;k<l->vertex_count;k++)if(!finite3(l->vertices[k]))return 1;
        }
    }
    for(uint32_t i=0;i<s->count;i++){uint32_t j=0;while(j<g->room_count&&s->rooms[i]!=g->rooms[j])++j;if(j==g->room_count)return 1;}
    float old[3],delta[3];memcpy(old,s->position,12);subtract(delta,matrix+12,old);
    uint8_t flag=*ambiguity;UCScenePortalRoom*selected[4096];uint32_t count=0;
    if(s->count){
        flag=0;
        if(dot(delta,delta)<10000.f){
            for(uint32_t i=0;i<s->count;i++){
                UCScenePortalRoom*next;if(crossing(g,s->rooms[i],old,matrix+12,&flag,&next))return 1;
                if(next){if(flag)break;if(contains(next,matrix+12)){selected[count++]=next;goto done;}}
            }
            if(!flag&&s->count==1&&contains(s->rooms[0],matrix+12))goto done;
        }
    }
    for(uint32_t i=0;i<g->room_count;i++)if(contains(g->rooms[i],matrix+12))selected[count++]=g->rooms[i];
    if(!count&&!s->count)selected[count++]=g->rooms[0];
done:
    if(count){memcpy(s->rooms,selected,(size_t)count*sizeof*selected);s->count=count;}
    memcpy(s->position,matrix+12,12);s->initialized=1;*ambiguity=flag;return 0;
}
