#include "scene_portal_start.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
int uc_scene_portal_start(const UCScenePortalGraph *g,const float matrix[16],UCScenePortalStart *s){
    if(!g||g->active!=1||!g->rooms||!g->room_count||g->room_count>4096||!matrix||!s||
       s->initialized||s->count||!s->rooms||s->capacity<g->room_count||s->capacity>4096||(_mm_getcsr()&0xE040u))return 1;
    for(unsigned j=0;j<3;j++)if(!isfinite(matrix[12+j])||s->position[j]!=0.f)return 1;
    for(uint32_t i=0;i<g->room_count;i++){
        const UCScenePortalRoom *r=g->rooms[i];if(!r)return 1;
        for(unsigned j=0;j<3;j++)if(!isfinite(r->minimum[j])||!isfinite(r->maximum[j]))return 1;
    }
    memcpy(s->position,matrix+12,sizeof s->position);
    for(uint32_t i=0;i<g->room_count;i++){
        UCScenePortalRoom *r=g->rooms[i];unsigned j=0;
        for(;j<3;j++)if(s->position[j]<r->minimum[j]||r->maximum[j]<s->position[j])break;
        if(j==3)s->rooms[s->count++]=r;
    }
    if(!s->count)s->rooms[s->count++]=g->rooms[0];
    s->initialized=1;return 0;
}
