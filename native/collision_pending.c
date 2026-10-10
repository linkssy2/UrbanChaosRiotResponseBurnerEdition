#include "collision_pending.h"
#include <stdlib.h>
#define NONE UINT32_MAX
typedef struct Operation {void *body;uint32_t activation,next;unsigned code;} Operation;
struct UCCollisionPending {
    const void *world;void *context;UCCollisionPendingHooks hooks;
    uint32_t capacity,head,tail,free_head,queued,used,depth,budget;
    Operation *operations;
};
UCCollisionStatus uc_collision_pending_create(const void *world,uint32_t capacity,
    const UCCollisionPendingHooks *h,void *context,UCCollisionPending **out) {
    if(!world||!capacity||capacity>65536||!h||!h->retain||!h->release||!h->current_world||!h->perform||!out)return UC_COLLISION_ARGUMENT;
    UCCollisionPending *q=(UCCollisionPending*)calloc(1,sizeof(*q)+(size_t)capacity*sizeof(Operation));
    if(!q)return UC_COLLISION_MEMORY;
    q->operations=(Operation*)(q+1);q->world=world;q->context=context;q->hooks=*h;q->capacity=capacity;q->head=q->tail=NONE;
    for(uint32_t i=0;i<capacity;i++)q->operations[i].next=i+1<capacity?i+1:NONE;
    *out=q;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_pending_enqueue(UCCollisionPending *q,unsigned code,void *body,uint32_t activation) {
    if(!q||!body)return UC_COLLISION_ARGUMENT;
    if(code!=1&&code!=2)return UC_COLLISION_UNSUPPORTED;
    if(q->free_head==NONE)return UC_COLLISION_LIMIT;
    uint32_t i=q->free_head;Operation *op=&q->operations[i];q->free_head=op->next;
    *op=(Operation){body,activation,NONE,code};
    if(q->tail!=NONE)q->operations[q->tail].next=i;else q->head=i;
    q->tail=i;++q->queued;++q->used;q->hooks.retain(q->context,body);return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_pending_drain(UCCollisionPending *q,uint32_t budget) {
    if(!q||!budget)return UC_COLLISION_ARGUMENT;
    if(q->depth>=64)return UC_COLLISION_LIMIT;
    if(!q->depth)q->budget=budget;
    uint32_t head=q->head,tail=q->tail,count=q->queued;q->head=q->tail=NONE;q->queued=0;++q->depth;
    UCCollisionStatus status=UC_COLLISION_OK;
    while(head!=NONE){
        if(!q->budget){status=UC_COLLISION_LIMIT;break;}--q->budget;
        Operation op=q->operations[head];const void *world=q->hooks.current_world(q->context,op.body);
        if((op.code==1&&!world)||(op.code==2&&world==q->world)){
            status=q->hooks.perform(q->context,op.code,op.body,op.code==1?op.activation:0);if(status)break;
        }
        q->hooks.release(q->context,op.body);
        uint32_t old=head;head=op.next;--count;--q->used;
        q->operations[old].next=q->free_head;q->free_head=old;
        if(q->head!=NONE){status=uc_collision_pending_drain(q,budget);if(status)break;}
    }
    /* Retain unconsumed older work behind any newer callback work on failure. */
    if(head!=NONE){if(q->tail!=NONE)q->operations[q->tail].next=head;else q->head=head;q->tail=tail;q->queued+=count;}
    --q->depth;return status;
}
uint32_t uc_collision_pending_queued(const UCCollisionPending *q){return q?q->queued:0;}
uint32_t uc_collision_pending_depth(const UCCollisionPending *q){return q?q->depth:0;}
UCCollisionStatus uc_collision_pending_free(UCCollisionPending *q){
    if(!q)return UC_COLLISION_ARGUMENT;
    if(q->used||q->depth)return UC_COLLISION_UNSUPPORTED;
    free(q);return UC_COLLISION_OK;
}
