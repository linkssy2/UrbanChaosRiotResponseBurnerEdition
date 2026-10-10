#include "scene_portal_load.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef struct Reader {const unsigned char *p;size_t size,at;} Reader;
static int read(Reader*r,void*out,size_t n){
    if(n>r->size-r->at)return 0;if(out)memcpy(out,r->p+r->at,n);r->at+=n;return 1;
}
static int count(Reader*r,uint32_t*out,uint32_t limit){return read(r,out,4)&&*out<=limit;}
static int floats(Reader*r,float*out,uint32_t n,float limit){
    if(!read(r,out,4*n))return 0;for(uint32_t i=0;i<n;i++)if(!isfinite(out[i])||fabsf(out[i])>limit)return 0;return 1;
}
int uc_scene_portal_load(UCScenePortalGraph*g,const void*data,size_t size,
                        const UCScenePortalMemory*m,size_t*consumed,uint8_t*active){
    if(!g||g->active||g->rooms||g->seen||g->enabled||g->room_count||g->portal_count||!data||size>16*1024*1024||
       !m||!m->allocate||!m->release||!consumed||!active)return 1;
    Reader r={data,size,0};uint32_t present=0,rn=0,pn=0,links=0;int status=1;
    UCSceneRoomInput *rooms=NULL;UCScenePortalInput *portals=NULL;UCScenePortalData *groups=NULL;
    if(!count(&r,&present,1))return 1;if(!present){*consumed=r.at;*active=0;return 0;}
    if(!count(&r,&rn,256))goto done;
    rooms=calloc(rn?rn:1,sizeof*rooms);if(!rooms){status=2;goto done;}
    for(uint32_t i=0;i<rn;i++){
        UCSceneRoomInput*q=rooms+i;uint32_t skip=0;
        if(!floats(&r,q->bounds,6,1e15f)||!count(&r,&skip,4096)||!read(&r,NULL,4*skip)||!count(&r,&q->count,4096-links))goto done;
        links+=q->count;uint32_t*ids=malloc(4*(q->count?q->count:1));float(*normals)[3]=malloc(12*(q->count?q->count:1));
        q->portal_indices=ids;q->normals=(const float(*)[3])normals;if(!ids||!normals){status=2;goto done;}
        for(uint32_t j=0;j<q->count;j++){
            if(!count(&r,ids+j,255)||!floats(&r,normals[j],3,1e6f))goto done;
            for(unsigned k=0;k<3;k++)normals[j][k]=0.f-normals[j][k];
        }
    }
    if(!count(&r,&pn,256))goto done;
    portals=calloc(pn?pn:1,sizeof*portals);groups=calloc(pn?pn:1,sizeof*groups);if(!portals||!groups){status=2;goto done;}
    for(uint32_t i=0;i<pn;i++){
        UCScenePortalInput*q=portals+i;float(*v)[3]=malloc(48);q->data.vertices=(const float(*)[3])v;if(!v){status=2;goto done;}
        q->data.group=i;q->data.vertex_count=4;
        if(!floats(&r,v[0],12,1e6f)||uc_scene_portal_normal(&q->data)||!count(&r,&q->neighbor_count,256))goto done;
        uint32_t*ids=malloc(4*(q->neighbor_count?q->neighbor_count:1));q->neighbors=ids;if(!ids){status=2;goto done;}
        for(uint32_t j=0;j<q->neighbor_count;j++)if(!count(&r,ids+j,INT32_MAX))goto done;
        groups[i]=q->data;
    }
    status=uc_scene_portal_group(groups,pn,m);if(status)goto done;
    for(uint32_t i=0;i<pn;i++)portals[i].data=groups[i];
    if(rn>1&&pn){status=uc_scene_portal_graph_begin(g,rooms,rn,portals,pn,m);if(status)goto done;}
    *consumed=r.at;*active=(uint8_t)(rn>1&&pn);status=0;
done:
    if(portals)for(uint32_t i=0;i<pn;i++){free((void*)portals[i].neighbors);free((void*)portals[i].data.vertices);}
    if(rooms)for(uint32_t i=0;i<rn;i++){free((void*)rooms[i].portal_indices);free((void*)rooms[i].normals);}
    free(groups);free(portals);free(rooms);return status;
}
