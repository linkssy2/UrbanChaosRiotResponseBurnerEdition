#include "collision_scenery_index.h"
#include "collision_bounds.h"
#include "collision_body.h"
#include "collision_sap_cast.h"
#include <math.h>
#include <stdlib.h>

typedef struct Entry { UCCollisionSceneryQuery query; UCCollisionSAPHandle handle; } Entry;
struct UCCollisionSceneryIndex {
    UCCollisionSAP *sap;
    Entry *entries;
    UCCollisionSAPPair *pairs;
    uint32_t capacity;
    float tolerance;
};
UCCollisionStatus uc_collision_scenery_index_create(const float lower[4],const float upper[4],
    float tolerance,uint32_t capacity,UCCollisionSceneryIndex **out) {
    if(!out||!isfinite(tolerance)||tolerance<0)return UC_COLLISION_ARGUMENT;
    UCCollisionSAP *sap=NULL;
    UCCollisionStatus status=uc_collision_sap_create(lower,upper,capacity,&sap);
    if(status)return status;
    UCCollisionSceneryIndex *index=calloc(1,sizeof(*index));
    if(!index){uc_collision_sap_free(sap);return UC_COLLISION_MEMORY;}
    index->sap=sap;index->capacity=capacity;index->tolerance=tolerance;
    index->entries=calloc(capacity,sizeof(*index->entries));
    index->pairs=malloc((size_t)capacity*sizeof(*index->pairs));
    if(!index->entries||!index->pairs){uc_collision_scenery_index_free(index);return UC_COLLISION_MEMORY;}
    for(uint32_t i=0;i<capacity;i++)index->entries[i].handle=(UCCollisionSAPHandle){index->entries+i,UC_COLLISION_SAP_DETACHED};
    *out=index;return UC_COLLISION_OK;
}
void uc_collision_scenery_index_free(UCCollisionSceneryIndex *index) {
    if(!index)return;
    /* SAP invalidates borrowed handles while the entries/bodies are still alive. */
    uc_collision_sap_free(index->sap);
    if(index->entries)for(uint32_t i=0;i<index->capacity;i++)uc_collision_scenery_query_close(&index->entries[i].query);
    free(index->entries);free(index->pairs);free(index);
}
UCCollisionStatus uc_collision_scenery_index_add(UCCollisionSceneryIndex *index,UCCollisionSceneryBody *body) {
    if(!index||!body)return UC_COLLISION_ARGUMENT;
    Entry *entry=NULL;
    for(uint32_t i=0;i<index->capacity;i++) {
        if(index->entries[i].query.body==body)return UC_COLLISION_ARGUMENT;
        if(!entry&&!index->entries[i].query.body)entry=index->entries+i;
    }
    if(!entry)return UC_COLLISION_LIMIT;
    const UCCollisionSceneryView *view=uc_collision_scenery_view(body);
    UCCollisionBounds bounds;
    UCCollisionStatus status=uc_collision_bounds(view->asset,view->transform,index->tolerance,&bounds);
    if(status)return status;
    status=uc_collision_scenery_query_open(body,&entry->query);if(status)return status;
    UCCollisionSAPPairs pairs={index->pairs,0,index->capacity};
    status=uc_collision_sap_add(index->sap,&entry->handle,bounds.minimum,bounds.maximum,&pairs);
    if(status)uc_collision_scenery_query_close(&entry->query);
    return status;
}
UCCollisionStatus uc_collision_scenery_index_remove(UCCollisionSceneryIndex *index,UCCollisionSceneryBody *body) {
    if(!index||!body)return UC_COLLISION_ARGUMENT;
    for(uint32_t i=0;i<index->capacity;i++)if(index->entries[i].query.body==body) {
        UCCollisionSAPPairs pairs={index->pairs,0,index->capacity};
        UCCollisionStatus status=uc_collision_sap_remove(index->sap,&index->entries[i].handle,&pairs);
        if(!status)uc_collision_scenery_query_close(&index->entries[i].query);
        return status;
    }
    return UC_COLLISION_ARGUMENT;
}
typedef struct Cast {
    const UCCollisionRay *ray;
    UCCollisionClosest closest;
    UCCollisionSceneryBody *selected;
} Cast;
static UCCollisionStatus candidate(void *context,const UCCollisionSAPHandle *handle,float *fraction) {
    Cast *cast=context;const Entry *entry=handle->identity;
    const UCCollisionSceneryView *view=uc_collision_scenery_view(entry->query.body);
    UCCollisionBodyFilter filter={view->collidable.shape_type,view->collidable.shape_user_data,
        view->collidable.filter_info,view->collidable_type,0,0,0};
    int accepted=uc_collision_body_filter(cast->ray->filter,&filter);
    if(accepted<0)return UC_COLLISION_UNSUPPORTED;
    if(accepted) {
        UCCollisionRay local;
        if(uc_collision_body_local(cast->ray->from,cast->ray,view->transform,&local)<0)return UC_COLLISION_ARGUMENT;
        UCCollisionStatus status=uc_collision_static_cast(&entry->query.shape,&local,&cast->closest);
        if(status)return status;
        if(cast->closest.root_body==view->collidable.identity)cast->selected=entry->query.body;
    }
    *fraction=cast->closest.limit;return UC_COLLISION_OK;
}
int uc_collision_scenery_index_cast(const UCCollisionSceneryIndex *index,const UCCollisionRay *ray,
    float impulse,UCCollisionResult *out) {
    if(!index||!ray||!out||!isfinite(impulse))return -1;
    Cast cast={0};cast.ray=ray;cast.closest.limit=cast.closest.hit.fraction=1;
    float fraction;
    if(uc_collision_sap_cast(uc_collision_sap_view(index->sap),ray->from,ray->to,candidate,&cast,&fraction))return -1;
    if(!cast.selected)return 0;
    const UCCollisionSceneryView *view=uc_collision_scenery_view(cast.selected);
    UCCollisionResultOwner owner={cast.selected,view->collidable_type,view->impulse_disabled,0,0};
    return uc_collision_result_convert(ray,&cast.closest,&owner,impulse,out);
}
