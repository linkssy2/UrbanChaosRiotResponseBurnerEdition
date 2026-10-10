#include "collision_world_ops.h"
#include <string.h>
static void retain(void *p,void *body){UCCollisionWorldOps *w=p;w->hooks.retain(w->context,body);}
static void release(void *p,void *body){UCCollisionWorldOps *w=p;w->hooks.release(w->context,body);}
static const void *current_world(void *p,void *body){UCCollisionWorldOps *w=p;return w->hooks.current_world(w->context,body);}
static UCCollisionStatus perform(void *p,unsigned op,void *body,uint32_t activation) {
    UCCollisionWorldOps *w=p;int applied=0;
    UCCollisionStatus status=uc_collision_world_ops_request(w,op,body,activation,&applied);
    /* A nested tail drain may fail after THIS action committed. The queue must
     * consume this item once; retain only the failed/newer pending work. */
    if(status&&applied){if(!w->drain_error)w->drain_error=status;return UC_COLLISION_OK;}
    return status;
}
UCCollisionStatus uc_collision_world_ops_init(UCCollisionWorldOps *out,const void *identity,
    const UCCollisionWorldHooks *h,void *context,int32_t kind,uint32_t capacity,uint32_t work) {
    if(!out||out->pending||!identity||!h||!work||!h->preflight||!h->prepare_add||!h->retain||!h->release||
        !h->current_world||!h->member_add||!h->member_remove||!h->broad_add||!h->broad_remove||
        !h->world_notify||!h->body_notify||(kind>=4&&!h->simulation_remove))return UC_COLLISION_ARGUMENT;
    UCCollisionPendingHooks hooks={retain,release,current_world,perform};UCCollisionPending *pending=NULL;
    UCCollisionStatus status=uc_collision_pending_create(identity,capacity,&hooks,out,&pending);if(status)return status;
    UCCollisionWorldOps next={0};next.identity=identity;next.context=context;next.hooks=*h;
    next.pending=pending;next.work_limit=work;next.simulation_kind=kind;next.changes_allowed=1;*out=next;
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_world_ops_drain(UCCollisionWorldOps *w) {
    if(!w||!w->pending)return UC_COLLISION_ARGUMENT;
    if(w->busy||w->drain_blocked)return UC_COLLISION_UNSUPPORTED;
    if(!uc_collision_pending_depth(w->pending))w->drain_error=UC_COLLISION_OK;
    w->pending_requests=0;
    UCCollisionStatus status=uc_collision_pending_drain(w->pending,w->work_limit);
    return status?status:w->drain_error;
}
UCCollisionStatus uc_collision_world_ops_request(UCCollisionWorldOps *w,unsigned op,
    void *body,uint32_t activation,int *applied) {
    if(!w||!w->pending||!body||!applied)return UC_COLLISION_ARGUMENT;
    if(op!=1&&op!=2)return UC_COLLISION_UNSUPPORTED;
    if(w->busy) {
        if(w->pending_requests==UINT32_MAX)return UC_COLLISION_LIMIT;
        UCCollisionStatus status=uc_collision_pending_enqueue(w->pending,op,body,activation);
        if(status)return status;
        ++w->pending_requests;*applied=0;return UC_COLLISION_OK;
    }
    UCCollisionStatus status=w->hooks.preflight(w->context,op,body,activation);if(status)return status;
    if(op==1) {
        w->hooks.prepare_add(w->context,body);
        w->changes_allowed=0;
        w->hooks.retain(w->context,body);
        w->hooks.member_add(w->context,body,activation);
        ++w->busy;w->changes_allowed=1;
        w->hooks.broad_add(w->context,body);
        w->hooks.world_notify(w->context,op,body);
        w->hooks.body_notify(w->context,op,body);
        --w->busy;
    }else {
        w->busy=1;
        w->hooks.broad_remove(w->context,body);
        if(w->simulation_kind>=4)w->hooks.simulation_remove(w->context,body);
        w->hooks.world_notify(w->context,op,body);
        w->hooks.body_notify(w->context,op,body);
        w->changes_allowed=0;
        if(w->hooks.current_world(w->context,body)) {
            w->hooks.member_remove(w->context,body);
            w->hooks.release(w->context,body);
        }
        --w->busy;w->changes_allowed=1;
    }
    *applied=1;
    return !w->busy&&w->pending_requests&&!w->drain_blocked?uc_collision_world_ops_drain(w):UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_world_ops_dispose(UCCollisionWorldOps *w) {
    if(!w||!w->pending)return UC_COLLISION_ARGUMENT;
    if(w->busy)return UC_COLLISION_UNSUPPORTED;
    UCCollisionStatus status=uc_collision_pending_free(w->pending);
    if(!status)memset(w,0,sizeof(*w));return status;
}
