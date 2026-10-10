#include "scene_portal_graph.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int floats(const float *p,uint32_t n){
    for(uint32_t i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>1e15f)return 0;
    return 1;
}
static void *storage(const UCScenePortalMemory *m,size_t size){
    void *p=m->allocate(m->owner,size);if(p&&size)memset(p,0,size);return p;
}
static void release(UCScenePortalGraph *g,int complete){
    for(uint32_t i=0;i<g->room_count;i++)if(g->rooms&&g->rooms[i]){
        UCScenePortalRoom *r=g->rooms[i];
        if(r->links)for(uint32_t j=0;j<r->count;j++){
            if(complete||r->links[j].vertices)g->memory.release(g->memory.owner,r->links[j].vertices);
            if(complete||r->links[j].neighbors)g->memory.release(g->memory.owner,r->links[j].neighbors);
        }
        if(complete||r->links)g->memory.release(g->memory.owner,r->links);
        g->memory.release(g->memory.owner,r);
    }
    if(complete||g->seen)g->memory.release(g->memory.owner,g->seen);
    if(complete||g->enabled)g->memory.release(g->memory.owner,g->enabled);
    if(complete||g->rooms)g->memory.release(g->memory.owner,g->rooms);
    if(complete)g->memory.release(g->memory.owner,NULL); /* empty 17C5E0 scratch */
}
int uc_scene_portal_graph_begin(UCScenePortalGraph *out,UCSceneRoomInput *input,uint32_t rn,
                               const UCScenePortalInput *portals,uint32_t pn,const UCScenePortalMemory *memory){
    if(!out||out->active||out->rooms||out->seen||out->enabled||out->room_count||out->portal_count||
       !memory||!memory->allocate||!memory->release||rn>4096||pn>4096||(rn&&!input)||(pn&&!portals)||
       (_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    uint32_t links=0;
    for(uint32_t i=0;i<pn;i++){
        const UCScenePortalInput *p=portals+i;
        if(p->data.vertex_count<0||p->data.vertex_count>4096||p->neighbor_count>4096||
           (p->data.vertex_count&&!p->data.vertices)||(p->neighbor_count&&!p->neighbors)||!floats(p->data.normal,3))return 1;
        for(int32_t j=0;j<p->data.vertex_count;j++)if(!floats(p->data.vertices[j],3))return 1;
        for(uint32_t j=0;j<p->neighbor_count;j++)if(p->neighbors[j]>INT32_MAX)return 1;
    }
    for(uint32_t i=0;i<rn;i++){
        const UCSceneRoomInput *r=input+i;
        if(r->count>4096||(r->count&&(!r->portal_indices||!r->normals))||!floats(r->bounds,6))return 1;
        links+=r->count;if(links>65536)return 1;
        for(uint32_t j=0;j<r->count;j++)if(r->portal_indices[j]>=pn||!floats(r->normals[j],3))return 1;
    }
    UCScenePortalGraph g={0};g.memory=*memory;
    g.rooms=storage(memory,(size_t)rn*sizeof*g.rooms);if(!g.rooms)goto fail;
    g.portal_count=pn;
    g.seen=storage(memory,pn);if(!g.seen)goto fail;
    g.enabled=storage(memory,pn);if(!g.enabled)goto fail;
    if(pn)memset(g.enabled,1,pn);
    g.room_count=rn;
    for(uint32_t i=0;i<rn;i++){
        UCScenePortalRoom *r=g.rooms[i]=storage(memory,sizeof*r);if(!r)goto fail;
        r->count=input[i].count;
        if(r->count){r->links=storage(memory,(size_t)r->count*sizeof*r->links);if(!r->links)goto fail;}
    }
    for(uint32_t i=0;i<rn;i++)for(uint32_t j=0;j<g.rooms[i]->count;j++){
        UCScenePortalLink *l=g.rooms[i]->links+j;
        const UCScenePortalInput *p=portals+input[i].portal_indices[j];
        l->neighbor_count=p->neighbor_count;
        l->neighbors=storage(memory,(size_t)p->neighbor_count*sizeof*l->neighbors);if(!l->neighbors)goto fail;
        l->group=p->data.group;uint32_t n=0;
        for(uint32_t k=0;k<p->neighbor_count;k++){uint32_t v=p->neighbors[k];if(v!=i&&v<rn)l->neighbors[n++]=g.rooms[v];}
        l->neighbor_count=n;l->vertex_count=(uint32_t)p->data.vertex_count;
        l->vertices=storage(memory,(size_t)l->vertex_count*sizeof*l->vertices);if(!l->vertices)goto fail;
        memcpy(l->normal,input[i].normals[j],sizeof l->normal);
        float dot=(l->normal[2]*p->data.normal[2]+l->normal[1]*p->data.normal[1])+p->data.normal[0]*l->normal[0];
        for(uint32_t k=0;k<l->vertex_count;k++)memcpy(l->vertices[k],p->data.vertices[dot>0?l->vertex_count-1-k:k],sizeof*l->vertices);
        for(uint32_t k=0;k<l->vertex_count;k++)for(unsigned a=0;a<3;a++){
            float v=l->vertices[k][a];if(input[i].bounds[a]>v)input[i].bounds[a]=v-1.f;
            if(v>input[i].bounds[3+a])input[i].bounds[3+a]=v+1.f;
        }
    }
    for(uint32_t i=0;i<rn;i++)for(unsigned a=0;a<3;a++){
        UCScenePortalRoom *r=g.rooms[i];r->minimum[a]=input[i].bounds[a]-10.f;r->maximum[a]=input[i].bounds[3+a]+10.f;
        r->center[a]=(r->maximum[a]+r->minimum[a])*.5f;r->half[a]=(r->maximum[a]-r->minimum[a])*.5f;
    }
    g.active=1;*out=g;return 0;
fail:release(&g,0);return 2;
}
int uc_scene_portal_graph_end(UCScenePortalGraph *g){
    if(!g||g->active!=1||!g->memory.release)return 1;
    release(g,1);memset(g,0,sizeof*g);return 0;
}
