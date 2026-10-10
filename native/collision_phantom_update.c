#include "collision_phantom_update.h"
#include <stdlib.h>
#include <string.h>
UCCollisionStatus uc_collision_phantom_update_init(UCCollisionPhantomUpdate *out,const void *world,
    const UCCollisionPhantomGates *g,const UCCollisionPhantomUpdateHooks *h,void *context,uint32_t capacity) {
    if(!out||out->world||!world||!g||!g->busy||!g->pending_requests||!g->drain_blocked||
       !h||!h->preflight||!h->member||!h->selected||!h->query||!h->other||!h->filter||
       !h->refresh||!h->child_update||!h->enqueue||!h->drain||capacity>1048576)return UC_COLLISION_ARGUMENT;
    UCCollisionPhantomUpdate u={0};u.scratch.data=capacity?calloc(capacity,sizeof(*u.scratch.data)):NULL;
    if(capacity&&!u.scratch.data)return UC_COLLISION_MEMORY;
    u.world=world;u.gates=*g;u.hooks=*h;u.context=context;u.scratch.capacity=capacity;*out=u;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_update_request(UCCollisionPhantomUpdate *u,
    UCCollisionPhantom *phantom,uint32_t mode,int *applied) {
    if(!u||!u->world||!phantom||!applied)return UC_COLLISION_ARGUMENT;
    if(*u->gates.busy) {
        UCCollisionStatus s=u->hooks.enqueue(u->context,phantom,(uint8_t)mode);if(s)return s;
        ++*u->gates.pending_requests;*applied=0;return UC_COLLISION_OK;
    }
    if(u->active)return UC_COLLISION_UNSUPPORTED;
    const UCCollisionPhantomMember *member=u->hooks.member(u->context,phantom);
    const UCCollisionPhantomPairBody *selected=u->hooks.selected(u->context,phantom);
    if(!member||member->world!=u->world||member->broadphase.index==UC_COLLISION_SAP_DETACHED||
       !selected||selected->owner!=phantom||selected->type!=2||!selected->identity)return UC_COLLISION_ARGUMENT;
    UCCollisionStatus s=u->hooks.preflight(u->context,phantom,mode);if(s)return s;
    u->active=1;*u->gates.busy=1;u->scratch.count=0;
    u->hooks.query(u->context,phantom,&u->scratch);
    for(uint32_t i=0;i<u->scratch.count;++i) {
        const UCCollisionSAPHandle *handle=u->scratch.data[i].second;
        if(handle==&member->broadphase)continue;
        const UCCollisionPhantomPairBody *other=u->hooks.other(u->context,handle);
        UCCollisionPhantomPairFilter filter=u->hooks.filter(u->context);
        u->hooks.refresh(u->context,filter,selected,other);
    }
    if(mode)u->hooks.child_update(u->context,phantom);
    --*u->gates.busy;u->active=0;*applied=1;
    if(!*u->gates.busy&&*u->gates.pending_requests&&!*u->gates.drain_blocked)return u->hooks.drain(u->context);
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_update_dispose(UCCollisionPhantomUpdate *u) {
    if(!u||!u->world)return UC_COLLISION_ARGUMENT;if(u->active)return UC_COLLISION_UNSUPPORTED;
    free(u->scratch.data);memset(u,0,sizeof(*u));return UC_COLLISION_OK;
}
