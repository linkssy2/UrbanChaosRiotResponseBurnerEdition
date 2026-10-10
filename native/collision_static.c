#include "collision_static.h"
#include <math.h>
#include <string.h>
UCCollisionStatus uc_collision_static_prepare(const UCCollisionAsset *asset,
    const UCCollisionBodyMetadata *body,const float axes[3][4],UCCollisionStatic *out) {
    if(!asset||!body||!body->identity||body->shape_type!=24||!axes||!out)return UC_COLLISION_ARGUMENT;
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++)if(!isfinite(axes[i][j]))return UC_COLLISION_ARGUMENT;
    UCCollisionStatic next={0};UCCollisionStatus status=uc_collision_mopp_prepare(asset,&next.mopp);
    if(status)return status;
    next.asset=asset;next.body=*body;memcpy(next.normal_axes,axes,sizeof(next.normal_axes));
    *out=next;return UC_COLLISION_OK;
}
typedef struct Cast {
    const UCCollisionStatic *shape;
    const UCCollisionRay *ray;
    UCCollisionClosest closest;
} Cast;
static UCCollisionStatus terminal(void *context,uint32_t key,float *fraction) {
    Cast *cast=(Cast*)context;const UCCollisionStatic *shape=cast->shape;
    uint32_t material;UCCollisionMeshChild child;UCCollisionTriangleHit hit;
    UCCollisionStatus status=uc_collision_mesh_material(shape->asset,key,&material);if(status)return status;
    /* Original storage-mesh virtual8=9D970 returns type16. Its pure material
     * getter is shared here by terminal filtering and accepted-hit metadata. */
    if(!uc_collision_terminal_filter(16,cast->ray->filter,material))return UC_COLLISION_OK;
    status=uc_collision_mesh_child(shape->asset,key,&child);if(status)return status;
    int accepted=uc_collision_triangle_cast_world(child.vertices,cast->ray->from,cast->ray->to,
        cast->closest.limit,shape->normal_axes,key,&hit);
    if(accepted<0)return UC_COLLISION_UNSUPPORTED;
    if(!accepted)return UC_COLLISION_OK;
    const UCCollisionBodyMetadata ancestry[2]={{&child,6,key,0,0},shape->body};
    UCCollisionHitRoute route;
    accepted=uc_collision_hit_route(&cast->closest,&hit,ancestry,2,&route);
    if(accepted<0)return UC_COLLISION_UNSUPPORTED;
    if(accepted){
        if(route.mode!=UC_COLLISION_MATERIAL_CHILD||route.material_body!=1||route.material_key!=key)
            return UC_COLLISION_UNSUPPORTED;
        accepted=uc_collision_hit_commit(&cast->closest,&route,material);
        if(accepted<0)return UC_COLLISION_UNSUPPORTED;
    }
    *fraction=cast->closest.limit;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_static_cast(const UCCollisionStatic *shape,
    const UCCollisionRay *ray,UCCollisionClosest *out) {
    if(!shape||!shape->asset||!shape->body.identity||shape->body.shape_type!=24||!ray||!out||
        !isfinite(out->limit)||out->limit<0||out->limit>1||
        !isfinite(out->hit.fraction)||out->hit.fraction<0||out->hit.fraction>1)return UC_COLLISION_ARGUMENT;
    Cast cast={shape,ray,*out};float fraction=out->limit;
    UCCollisionStatus status=uc_collision_mopp_cast(&shape->mopp,ray->from,ray->to,&fraction,terminal,&cast);
    if(!status)*out=cast.closest;return status;
}
