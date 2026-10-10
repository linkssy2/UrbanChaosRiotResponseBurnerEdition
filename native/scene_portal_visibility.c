#include "scene_portal_visibility.h"
#include <math.h>
#include <string.h>
typedef struct Traversal {
    UCScenePortalGraph *graph;UCScenePortalVisibility *visible;
    const float *point;uint32_t remaining;
} Traversal;
static int room_index(const UCScenePortalGraph*g,const UCScenePortalRoom*r){
    for(uint32_t i=0;i<g->room_count;i++)if(g->rooms[i]==r)return (int)i;return -1;
}
static int visit(Traversal *,UCScenePortalRoom *,uint32_t,const UCScenePortalPlane *);
static int portal(Traversal*t,UCScenePortalLink*p,uint32_t n,const UCScenePortalPlane*planes){
    UCScenePortalVisibility*v=t->visible;uint8_t classification;int error=0;
    if(t->graph->seen[p->group])return 0;t->graph->seen[p->group]=1;
    if(uc_scene_portal_classify(p,t->point,&classification)){error=2;goto done;}
    if(!(classification&1))goto done;
    if(classification&2){
        for(uint32_t i=0;i<p->neighbor_count&&!error;i++)error=visit(t,p->neighbors[i],n,planes);
    }else{
        UCScenePortalClipArena arena=v->scratch;uintptr_t base=(uintptr_t)arena.data,current=(uintptr_t)v->cursor;
        if(current>=base&&current<=base+arena.capacity)arena.used=(uint32_t)(current-base);
        else arena.capacity=arena.used=0;
        UCScenePortalClipBlock *block=NULL;
        if(uc_scene_portal_clip(p,t->point,n,planes,&arena,&block)){error=2;goto done;}
        if(!block)goto done;
        int heap=(uintptr_t)block<base||(uintptr_t)block>base+v->scratch.capacity;
        if(!heap)v->cursor=arena.data+arena.used;
        for(uint32_t i=0;i<p->neighbor_count&&!error;i++)error=visit(t,p->neighbors[i],block->count,block->plane);
        uintptr_t rewind=(uintptr_t)block;
        if(heap)v->scratch.memory.release(v->scratch.memory.owner,block);
        v->cursor=(void*)rewind;
    }
done:
    t->graph->seen[p->group]=0;return error;
}
static int visit(Traversal*t,UCScenePortalRoom*r,uint32_t n,const UCScenePortalPlane*p){
    if(!t->remaining)return 2;t->remaining--;
    if(r->flags[0])return 0;r->flags[0]=1;
    if(!r->flags[1]){t->visible->rooms[t->visible->count++]=r;r->flags[1]=1;}
    int error=0;
    for(uint32_t i=0;i<r->count&&!error;i++){
        UCScenePortalLink*link=r->links+i;
        if(!t->graph->seen[link->group]&&t->graph->enabled[link->group])error=portal(t,link,n,p);
    }
    r->flags[0]=0;return error;
}
int uc_scene_portal_visibility(UCScenePortalGraph*g,const UCScenePortalStart*s,UCScenePortalPlanes*p,
                              const float a[16],const float b[16],const float c[16],UCScenePortalVisibility*v,uint32_t budget){
    if(!g||!s||!p||!v||v->failed||!budget||!g->active||!g->room_count||g->room_count>256||g->portal_count>256||!g->rooms||
       (g->portal_count&&(!g->seen||!g->enabled))||!s->initialized||s->count>s->capacity||s->count>g->room_count||(s->count&&!s->rooms)||
       !v->rooms||v->capacity<g->room_count||v->count>g->room_count||!v->scratch.data||((uintptr_t)v->scratch.data&3u)||
       (v->scratch.capacity&3u)||!v->scratch.memory.allocate||!v->scratch.memory.release)return 1;
    for(unsigned j=0;j<3;j++)if(!isfinite(s->position[j])||fabsf(s->position[j])>1e6f)return 1;
    for(uint32_t i=0;i<s->count;i++)if(room_index(g,s->rooms[i])<0)return 1;
    for(uint32_t i=0;i<v->count;i++)if(room_index(g,v->rooms[i])<0)return 1;
    uint32_t links=0;
    for(uint32_t i=0;i<g->room_count;i++){
        UCScenePortalRoom*r=g->rooms[i];if(!r||r->count>4096-links||(r->count&&!r->links))return 1;links+=r->count;
        for(uint32_t j=0;j<i;j++)if(g->rooms[j]==r)return 1;
        for(uint32_t j=0;j<r->count;j++){
            UCScenePortalLink*l=r->links+j;if(l->group>=g->portal_count||l->vertex_count<3||l->vertex_count>32||!l->vertices||l->neighbor_count>256||(l->neighbor_count&&!l->neighbors))return 1;
            for(uint32_t k=0;k<l->neighbor_count;k++)if(room_index(g,l->neighbors[k])<0)return 1;
            for(uint32_t k=0;k<l->vertex_count;k++)for(unsigned q=0;q<3;q++)if(!isfinite(l->vertices[k][q])||fabsf(l->vertices[k][q])>1e6f)return 1;
            for(unsigned q=0;q<3;q++)if(!isfinite(l->normal[q])||fabsf(l->normal[q])>1e6f)return 1;
        }
    }
    UCScenePortalPlanes next=*p;if(uc_scene_portal_planes_transform(&next,a,b,c))return 1;
    for(uint32_t i=0;i<v->count;i++)v->rooms[i]->flags[0]=v->rooms[i]->flags[1]=0;
    v->count=0;*p=next;v->cursor=v->scratch.data;
    Traversal t={g,v,s->position,budget};
    for(uint32_t i=0;i<s->count;i++){
        for(uint32_t j=0;j<v->count;j++)v->rooms[j]->flags[0]=0;
        if(visit(&t,s->rooms[i],p->count,p->plane)){v->failed=1;return 2;}
    }
    return 0;
}
