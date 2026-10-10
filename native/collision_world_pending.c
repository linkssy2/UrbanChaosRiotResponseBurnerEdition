#include "collision_world_pending.h"
#include <stdlib.h>
#include <string.h>
typedef struct Job {unsigned code;uint32_t count,activation;void **objects;} Job;
struct UCCollisionWorldPending {
    UCCollisionPending *queue;UCCollisionWorldPendingHooks hooks;void *context;
};
static void retain(void *context,void *object){
    UCCollisionWorldPending *q=context;Job *j=object;
    for(uint32_t i=0;i<j->count;i++)q->hooks.retain(q->context,j->objects[i]);
}
static void release(void *context,void *object){
    UCCollisionWorldPending *q=context;Job *j=object;
    for(uint32_t i=0;i<j->count;i++)q->hooks.release(q->context,j->objects[i]);
    free(j);
}
static const void *world(void *context,void *object){
    UCCollisionWorldPending *q=context;Job *j=object;
    return j->code==15||j->code==16||j->code==20?NULL:q->hooks.current_world(q->context,j->objects[0]);
}
static UCCollisionStatus perform(void *context,unsigned code,void *object,uint32_t activation){
    UCCollisionWorldPending *q=context;Job *j=object;(void)code;(void)activation;
    return q->hooks.perform(q->context,j->code,j->objects,j->count,j->activation);
}
UCCollisionStatus uc_collision_world_pending_create(const void *world_id,uint32_t capacity,
    const UCCollisionWorldPendingHooks *h,void *context,UCCollisionWorldPending **out){
    if(!world_id||!capacity||capacity>65536||!h||!h->retain||!h->release||!h->current_world||!h->perform||!out)return UC_COLLISION_ARGUMENT;
    UCCollisionWorldPending *q=calloc(1,sizeof(*q));if(!q)return UC_COLLISION_MEMORY;
    q->hooks=*h;q->context=context;
    UCCollisionPendingHooks hooks={retain,release,world,perform};
    UCCollisionStatus s=uc_collision_pending_create(world_id,capacity,&hooks,q,&q->queue);
    if(s){free(q);return s;}*out=q;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_world_pending_enqueue(UCCollisionWorldPending *q,unsigned code,
    void *const *objects,uint32_t count,uint32_t activation){
    if(!q)return UC_COLLISION_ARGUMENT;
    if(code!=1&&code!=2&&(code<13||code>16)&&(code<18||code>20))return UC_COLLISION_UNSUPPORTED;
    if(count>65535||(code==20?count!=0:code!=15&&code!=16&&count!=1)||(count&&!objects))return UC_COLLISION_ARGUMENT;
    if((code==19&&activation>255)||((code==18||code==20)&&activation>65535))return UC_COLLISION_ARGUMENT;
    for(uint32_t i=0;i<count;i++)if(!objects[i])return UC_COLLISION_ARGUMENT;
    Job *j=malloc(sizeof(*j)+(size_t)count*sizeof(*objects));if(!j)return UC_COLLISION_MEMORY;
    j->code=code;j->count=count;j->activation=code==1||code>=18?activation:0;j->objects=(void**)(j+1);
    if(count)memcpy(j->objects,objects,(size_t)count*sizeof(*objects));
    UCCollisionStatus s=uc_collision_pending_enqueue(q->queue,(code==2||code==14||code==18||code==19)?2:1,j,0);
    if(s)free(j);return s;
}
UCCollisionStatus uc_collision_world_pending_drain(UCCollisionWorldPending *q,uint32_t budget){return q?uc_collision_pending_drain(q->queue,budget):UC_COLLISION_ARGUMENT;}
uint32_t uc_collision_world_pending_queued(const UCCollisionWorldPending *q){return q?uc_collision_pending_queued(q->queue):0;}
uint32_t uc_collision_world_pending_depth(const UCCollisionWorldPending *q){return q?uc_collision_pending_depth(q->queue):0;}
UCCollisionStatus uc_collision_world_pending_free(UCCollisionWorldPending *q){
    if(!q)return UC_COLLISION_ARGUMENT;
    UCCollisionStatus s=uc_collision_pending_free(q->queue);if(!s)free(q);return s;
}
