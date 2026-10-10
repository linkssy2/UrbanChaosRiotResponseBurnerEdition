#include "collision_agent_remove.h"

static int island(const UCCollisionAgentIsland *i) {
    if(!i||!i->store||!i->world||!i->dirty)return 0;
    const UCCollisionAgentStore *s=i->store;
    if(s->count>65535||s->count>(s->capacity_flags&0x3fffffffu)||
       (s->count&&(!s->blocks||!s->block_size)))return 0;
    for(uint32_t n=0;n<s->count;++n)
        if(!s->blocks[n]||(uintptr_t)s->blocks[n]>UINTPTR_MAX-s->block_size)return 0;
    return 1;
}
UCCollisionStatus uc_collision_agent_remove_owner(const void *address,
    UCCollisionAgentIsland *first,UCCollisionAgentIsland *second,UCCollisionAgentIsland **out) {
    if(!address||!out||!island(first)||!island(second))return UC_COLLISION_ARGUMENT;
    UCCollisionAgentIsland *scan=first->store->count<=second->store->count?first:second;
    uintptr_t p=(uintptr_t)address;int found=0;
    for(uint32_t i=0;i<scan->store->count&&!found;++i) {
        uintptr_t start=(uintptr_t)scan->store->blocks[i];
        found=p>=start&&p-start<scan->store->block_size;
    }
    *out=found?scan:(scan==first?second:first);return UC_COLLISION_OK;
}
typedef struct Bridge {const UCCollisionAgentRemoveHooks *hooks;void *context;const void *dispatcher;} Bridge;
static void *allocate(void *p,size_t n,uint32_t cls){Bridge *b=p;return b->hooks->allocate(b->context,n,cls);}
static void release(void *p,void *memory,size_t n,uint32_t cls){Bridge *b=p;b->hooks->release(b->context,memory,n,cls);}
static void destroy(void *p,UCCollisionAgent *agent,void *state,void *manager){Bridge *b=p;b->hooks->destroy(b->context,b->dispatcher,agent,state,manager);}
UCCollisionStatus uc_collision_agent_remove(UCCollisionAgent *a,
    const UCCollisionAgentRemoveHooks *h,void *ctx,int *applied) {
    if(!a||!h||!h->body||!h->dispatcher||!h->preflight||!h->allocate||!h->release||
       !h->destroy||!h->finish||!applied||!a->side[0]||!a->side[1]||!a->manager)return UC_COLLISION_ARGUMENT;
    UCCollisionAgentRemoveBody body[2]={h->body(ctx,a->side[0]),h->body(ctx,a->side[1])};
    if(!island(body[0].island)||!island(body[1].island))return UC_COLLISION_ARGUMENT;
    UCCollisionAgentIsland *chosen;
    if(body[0].island==body[1].island)chosen=body[0].island;
    else if(body[0].fixed)chosen=body[1].island;
    else if(body[1].fixed)chosen=body[0].island;
    else {UCCollisionStatus s=uc_collision_agent_remove_owner(a,body[0].island,body[1].island,&chosen);if(s)return s;}
    UCCollisionStatus s=uc_collision_agent_store_validate(chosen->store);if(s)return s;
    int found=0;UCCollisionAgentStore *store=chosen->store;
    for(uint32_t b=0;b<store->count&&!found;++b) {
        uintptr_t start=(uintptr_t)store->blocks[b],p=(uintptr_t)a;
        uint32_t used=b+1==store->count?store->used_bytes:store->block_size;
        found=p>=start&&p-start<used&&(p-start)%store->entry_size==0;
    }
    if(!found)return UC_COLLISION_ARGUMENT;
    const void *dispatcher=h->dispatcher(ctx,chosen->world);void *manager=a->manager;
    if(!dispatcher)return UC_COLLISION_ARGUMENT;
    s=h->preflight(ctx,dispatcher,manager);if(s)return s;
    *applied=1;
    if(body[0].island==body[1].island)*body[0].island->dirty=1;
    else if(!body[0].fixed&&!body[1].fixed) {
        *body[0].island->dirty=1;*h->body(ctx,a->side[1]).island->dirty=1;
    }
    Bridge bridge={h,ctx,dispatcher};UCCollisionAgentStoreHooks inner={allocate,release,destroy};int inner_applied=0;
    s=uc_collision_agent_store_remove(store,a,&inner,&bridge,&inner_applied);
    if(s)return s;
    h->finish(ctx,manager);return UC_COLLISION_OK;
}
