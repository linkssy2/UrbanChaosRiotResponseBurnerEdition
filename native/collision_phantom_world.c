#include "collision_phantom_world.h"
#include <stdlib.h>
#include <string.h>
UCCollisionStatus uc_collision_phantom_world_init(UCCollisionPhantomWorld *out,const void *identity,
    const UCCollisionPhantomGates *g,const UCCollisionPhantomWorldHooks *h,void *context,
    uint32_t capacity,uint32_t pairs) {
    if(!out||out->identity||!identity||!g||!g->busy||!g->phantom_busy||!g->pending_requests||!g->drain_blocked||
       !h||!h->preflight||!h->member||!h->prepare_add||!h->retain||!h->release||!h->bounds||!h->broadphase||
       !h->pairs||!h->world_notify||!h->local_notify||!h->enqueue||!h->drain||!capacity||capacity>65535||pairs>1048576)
        return UC_COLLISION_ARGUMENT;
    UCCollisionPhantomWorld w={0};w.members=calloc(capacity,sizeof(*w.members));
    w.scratch_bounds=calloc(capacity,sizeof(*w.scratch_bounds));
    w.scratch_pairs.data=pairs?calloc(pairs,sizeof(*w.scratch_pairs.data)):NULL;
    if(!w.members||!w.scratch_bounds||(pairs&&!w.scratch_pairs.data)) {
        free(w.members);free(w.scratch_bounds);free(w.scratch_pairs.data);return UC_COLLISION_MEMORY;
    }
    w.identity=identity;w.context=context;w.hooks=*h;w.gates=*g;w.capacity=capacity;
    w.scratch_pairs.capacity=pairs;*out=w;return UC_COLLISION_OK;
}
static uint32_t find(const UCCollisionPhantomWorld *w,const UCCollisionPhantom *p) {
    uint32_t i=0;while(i<w->count&&w->members[i]!=p)++i;return i;
}
static void notify(UCCollisionPhantomWorld *w,int add,UCCollisionPhantom *p) {
    w->hooks.world_notify(w->context,add,p);w->hooks.local_notify(w->context,add,p);
}
static void broad(UCCollisionPhantomWorld *w,unsigned op,UCCollisionPhantom *const *p,uint32_t n) {
    w->scratch_pairs.count=0;
    w->hooks.broadphase(w->context,op,p,n,(op&1)?w->scratch_bounds:NULL,&w->scratch_pairs);
    if(op>=15||w->scratch_pairs.count)w->hooks.pairs(w->context,op&1,&w->scratch_pairs);
}
UCCollisionStatus uc_collision_phantom_world_request(UCCollisionPhantomWorld *w,unsigned op,
    UCCollisionPhantom *const *p,uint32_t n,int *applied) {
    if(!w||!w->identity||!applied||op<13||op>16||n>65535||(n&&!p)||(op<15&&n!=1))return UC_COLLISION_ARGUMENT;
    for(uint32_t i=0;i<n;++i)if(!p[i])return UC_COLLISION_ARGUMENT;
    if((uint32_t)(*w->gates.busy+*w->gates.phantom_busy)!=0) {
        UCCollisionStatus s=w->hooks.enqueue(w->context,op,p,(uint16_t)n);
        if(s)return s;
        ++*w->gates.pending_requests;*applied=0;return UC_COLLISION_OK;
    }
    if(w->active)return UC_COLLISION_UNSUPPORTED;
    int add=(int)(op&1);
    if(add&&n>w->capacity-w->count)return UC_COLLISION_LIMIT;
    for(uint32_t i=0;i<n;++i) {
        UCCollisionPhantomMember *m=w->hooks.member(w->context,p[i]);
        if(!m||(add?m->world!=NULL:m->world!=w->identity)||
            (add?find(w,p[i])!=w->count:find(w,p[i])==w->count))return UC_COLLISION_ARGUMENT;
        for(uint32_t j=0;j<i;++j)if(p[i]==p[j])return UC_COLLISION_ARGUMENT;
    }
    UCCollisionStatus s=w->hooks.preflight(w->context,op,p,n);if(s)return s;
    w->active=1;++*w->gates.busy;
    if(add) {
        for(uint32_t i=0;i<n;++i) {
            w->hooks.prepare_add(w->context,p[i]);
            w->hooks.member(w->context,p[i])->world=w->identity;
            if(op==15)w->hooks.bounds(w->context,p[i],w->scratch_bounds+i);
            w->hooks.retain(w->context,p[i]);w->members[w->count++]=p[i];
            if(op==15)notify(w,1,p[i]);
        }
        if(op==13)w->hooks.bounds(w->context,p[0],w->scratch_bounds);
        broad(w,op,p,n);
        if(op==13)notify(w,1,p[0]);
    } else {
        for(uint32_t i=0;i<n;++i)notify(w,0,p[i]);
        broad(w,op,p,n);
        for(uint32_t i=0;i<n;++i) {
            UCCollisionPhantomMember *m=w->hooks.member(w->context,p[i]);
            if(op==16)m->world=NULL;
            uint32_t index=find(w,p[i]);w->members[index]=w->members[--w->count];
            if(op==14)m->world=NULL;
            w->hooks.release(w->context,p[i]);
        }
    }
    --*w->gates.busy;w->active=0;*applied=1;
    if(!*w->gates.busy&&*w->gates.pending_requests&&!*w->gates.drain_blocked)return w->hooks.drain(w->context);
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_world_dispose(UCCollisionPhantomWorld *w) {
    if(!w||!w->identity)return UC_COLLISION_ARGUMENT;
    if(w->count||w->active)return UC_COLLISION_UNSUPPORTED;
    free(w->members);free(w->scratch_bounds);free(w->scratch_pairs.data);memset(w,0,sizeof(*w));return UC_COLLISION_OK;
}
