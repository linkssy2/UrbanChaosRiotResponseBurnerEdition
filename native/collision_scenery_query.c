#include "collision_scenery_query.h"
#include "collision_body.h"
#include <string.h>
UCCollisionStatus uc_collision_scenery_query_open(UCCollisionSceneryBody *body,UCCollisionSceneryQuery *out) {
    if(!body||!out||out->body)return UC_COLLISION_ARGUMENT;
    const UCCollisionSceneryView *view=uc_collision_scenery_view(body);
    UCCollisionSceneryQuery next={0};
    UCCollisionStatus status=uc_collision_static_prepare(view->asset,&view->collidable,view->transform,&next.shape);
    if(status)return status;
    status=uc_collision_scenery_retain(body);if(status)return status;
    next.body=body;*out=next;return UC_COLLISION_OK;
}
void uc_collision_scenery_query_close(UCCollisionSceneryQuery *query) {
    if(query){uc_collision_scenery_release(query->body);memset(query,0,sizeof(*query));}
}
int uc_collision_scenery_query_cast(const UCCollisionSceneryQuery *query,const UCCollisionRay *ray,
    float impulse,UCCollisionResult *out) {
    if(!query||!query->body||!ray||!out)return -1;
    const UCCollisionSceneryView *view=uc_collision_scenery_view(query->body);
    UCCollisionBodyFilter filter={view->collidable.shape_type,view->collidable.shape_user_data,
        view->collidable.filter_info,view->collidable_type,0,0,0};
    int accepted=uc_collision_body_filter(ray->filter,&filter);if(accepted<=0)return accepted;
    UCCollisionRay local;
    if(uc_collision_body_local(ray->from,ray,view->transform,&local)<0)return -1;
    UCCollisionClosest closest={0};closest.limit=closest.hit.fraction=1;
    if(uc_collision_static_cast(&query->shape,&local,&closest)!=UC_COLLISION_OK)return -1;
    UCCollisionResultOwner owner={query->body,view->collidable_type,view->impulse_disabled,0,0};
    return uc_collision_result_convert(ray,&closest,&owner,impulse,out);
}
