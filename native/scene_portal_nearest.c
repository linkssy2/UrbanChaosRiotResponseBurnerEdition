#include "scene_portal_nearest.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int position(const float *p){
    return p&&isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2])&&
        fabsf(p[0])<=1e15f&&fabsf(p[1])<=1e15f&&fabsf(p[2])<=1e15f;
}
int uc_scene_portal_nearest(UCScenePortalGraph *g,const float point[3],uint32_t *out){
    if(!g||!out)return 1;
    if(!g->active){*out=UINT32_MAX;return 0;}
    if(g->active!=1||g->room_count>4096||g->portal_count>4096||!g->rooms||!g->seen||
       !position(point)||(_mm_getcsr()&0xE040u))return 1;
    uint32_t count=0;
    for(uint32_t i=0;i<g->room_count;i++){
        const UCScenePortalRoom *r=g->rooms[i];
        if(!r||r->count>4096||(r->count&&!r->links))return 1;
        count+=r->count;if(count>65536)return 1;
        for(uint32_t j=0;j<r->count;j++){
            const UCScenePortalLink *l=r->links+j;
            if(l->group>=g->portal_count||l->vertex_count>4096||(l->vertex_count&&!l->vertices))return 1;
            for(uint32_t k=0;k<l->vertex_count;k++)if(!position(l->vertices[k]))return 1;
        }
    }
    memset(g->seen,0,g->portal_count);uint32_t group=UINT32_MAX;float closest=0;
    for(uint32_t i=0;i<g->room_count;i++)for(uint32_t j=0;j<g->rooms[i]->count;j++){
        const UCScenePortalLink *l=g->rooms[i]->links+j;
        if(g->seen[l->group])continue;
        g->seen[l->group]=1;
        for(uint32_t k=0;k<l->vertex_count;k++){
            float dx=point[0]-l->vertices[k][0],dy=point[1]-l->vertices[k][1],dz=point[2]-l->vertices[k][2];
            float d=(dz*dz+dy*dy)+dx*dx;
            if(group==UINT32_MAX||closest>d){group=l->group;closest=d;}
        }
    }
    memset(g->seen,0,g->portal_count);*out=group;return 0;
}
