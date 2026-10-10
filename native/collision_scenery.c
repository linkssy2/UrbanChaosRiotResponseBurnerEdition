#include "collision_scenery.h"
#include <stdlib.h>
#include <limits.h>
struct UCCollisionSceneryBody {
    UCCollisionSceneryView view;
    UCCollisionSceneryResource resource;
    uint32_t references;
};
UCCollisionStatus uc_collision_scenery_retain(UCCollisionSceneryBody *body) {
    if(!body||!body->references)return UC_COLLISION_ARGUMENT;
    if(body->references==UINT32_MAX)return UC_COLLISION_LIMIT;
    ++body->references;return UC_COLLISION_OK;
}
void uc_collision_scenery_release(UCCollisionSceneryBody *body) {
    if(body&&!--body->references){body->resource.release(body->resource.context);free(body);}
}
const UCCollisionSceneryView *uc_collision_scenery_view(const UCCollisionSceneryBody *body) {
    return body?&body->view:NULL;
}
uint32_t uc_collision_scenery_references(const UCCollisionSceneryBody *body) {
    return body?body->references:0;
}
static int accepted(UCCollisionSceneryWorldResult result) {
    return result==UC_SCENERY_APPLIED||result==UC_SCENERY_QUEUED;
}
UCCollisionStatus uc_collision_scenery_remove(UCCollisionScenery *scene,const UCCollisionSceneryWorld *world) {
    if(!scene||!scene->body||!world||!world->remove)return UC_COLLISION_ARGUMENT;
    if(!accepted(world->remove(world->context,scene->body)))return UC_COLLISION_UNSUPPORTED;
    uc_collision_scenery_release(scene->body);scene->body=NULL;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_scenery_replace(UCCollisionScenery *scene,
    const UCCollisionSceneryResource *resource,const UCCollisionSceneryWorld *world) {
    if(!scene||!resource||!resource->asset||!resource->retain||!resource->release||
        !world||!world->add||!world->remove)return UC_COLLISION_ARGUMENT;
    if(scene->body){UCCollisionStatus status=uc_collision_scenery_remove(scene,world);if(status)return status;}
    UCCollisionSceneryBody *body=(UCCollisionSceneryBody *)calloc(1,sizeof(*body));
    if(!body)return UC_COLLISION_MEMORY;
    body->resource=*resource;body->references=1;body->view.asset=resource->asset;
    body->view.collidable.identity=&body->view.collidable;
    body->view.collidable.shape_type=24;body->view.collidable.shape_key=UINT32_MAX;
    body->view.collidable.shape_user_data=resource->asset->shape_user_data;
    body->view.collidable.filter_info=0x02F00000;
    for(unsigned i=0;i<3;++i)body->view.transform[i][i]=1;
    body->view.collidable_type=1;body->view.impulse_disabled=1;
    resource->retain(resource->context);
    scene->body=body;
    return accepted(world->add(world->context,body,1))?UC_COLLISION_OK:UC_COLLISION_UNSUPPORTED;
}
