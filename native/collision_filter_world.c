#include "collision_filter_world.h"
static int valid(const UCCollisionFilterWorld *w,const int *applied) {
    return w&&applied&&!w->active&&w->filter&&*w->filter&&w->query_filter&&
        w->busy&&w->pending_requests&&w->drain_blocked&&w->hooks.preflight&&
        w->hooks.retain&&w->hooks.release&&w->hooks.query_facet&&w->hooks.list&&
        w->hooks.rigid&&w->hooks.phantom&&w->hooks.enqueue&&w->hooks.drain;
}
static UCCollisionStatus refresh(UCCollisionFilterWorld *w,uint32_t child,int *applied) {
    UCCollisionStatus status;
    if(*w->busy) {
        status=w->hooks.enqueue(w->context,0,(uint8_t)child);
        if(status==UC_COLLISION_OK){++*w->pending_requests;*applied=0;}
        return status;
    }
    *w->drain_blocked=1;
    for(unsigned group=0;group<2;group++) {
        UCCollisionFilterWorldList islands;w->hooks.list(w->context,group,NULL,&islands);
        for(uint32_t i=0;i<islands.count;i++) {
            const void *island=islands.data[i];UCCollisionFilterWorldList bodies;
            w->hooks.list(w->context,3,island,&bodies);
            for(uint32_t j=0;j<bodies.count;j++) {
                w->hooks.rigid(w->context,bodies.data[j],0,child);
                w->hooks.list(w->context,3,island,&bodies);
            }
            w->hooks.list(w->context,group,NULL,&islands);
        }
    }
    UCCollisionFilterWorldList phantoms;w->hooks.list(w->context,2,NULL,&phantoms);
    for(uint32_t i=0;i<phantoms.count;i++) {
        w->hooks.phantom(w->context,phantoms.data[i],child);
        w->hooks.list(w->context,2,NULL,&phantoms);
    }
    *w->drain_blocked=0;*applied=1;w->active=0;
    return !*w->busy&&*w->pending_requests?w->hooks.drain(w->context):UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_filter_world_refresh(UCCollisionFilterWorld *w,
    uint32_t mode,uint32_t child,int *applied) {
    UCCollisionStatus status;
    if(!valid(w,applied))return UC_COLLISION_ARGUMENT;
    if(mode)return UC_COLLISION_UNSUPPORTED;
    if(!*w->busy&&(status=w->hooks.preflight(w->context,0,NULL,1,child))!=UC_COLLISION_OK)return status;
    w->active=1;status=refresh(w,child,applied);w->active=0;return status;
}
UCCollisionStatus uc_collision_filter_world_set(UCCollisionFilterWorld *w,void *filter,
    uint32_t enable,uint32_t mode,uint32_t child,int *applied) {
    UCCollisionStatus status;uint8_t update=(uint8_t)enable;
    if(!valid(w,applied))return UC_COLLISION_ARGUMENT;
    if(!filter||(update&&mode))return UC_COLLISION_UNSUPPORTED;
    if((status=w->hooks.preflight(w->context,1,filter,update,child))!=UC_COLLISION_OK)return status;
    w->active=1;w->hooks.retain(w->context,filter);
    w->hooks.release(w->context,*w->filter);
    *w->filter=filter;*w->query_filter=w->hooks.query_facet(w->context,filter);*applied=1;
    if(update){int refresh_applied;status=refresh(w,child,&refresh_applied);}
    w->active=0;return status;
}
