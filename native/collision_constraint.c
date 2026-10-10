#include "collision_constraint.h"
#include <string.h>
void uc_collision_constraint_default_info(uint32_t n,uint32_t out[4]) {
 uint32_t extra=n>=2&&n<0x80000000u;
 out[0]=48*n+128;out[1]=48*(n+2)+32*extra;
 out[2]=4*n+44+4*extra;out[3]=n+2+extra;
}
void uc_collision_constraint_default_subtract(UCCollisionConstraintIsland *is,const uint32_t info[4]) {
 for(unsigned i=0;i<3;i++)*is->counter[i]-=info[i+1];
}
static int body_valid(const UCCollisionConstraintBody *b) {
 return b&&!b->active&&b->capacity<=65536&&b->count<=b->capacity&&(!b->capacity||b->records)&&
  b->other_capacity<=65536&&b->other_count<=b->other_capacity&&(!b->other_capacity||b->other)&&
  b->runtime_count<=b->runtime_capacity&&(!b->runtime_capacity||b->runtime);
}
UCCollisionStatus uc_collision_constraint_validate(UCCollisionConstraint *c,const UCCollisionConstraintHooks *h,void *ctx) {
 if(!c||c->active||!c->record||!c->island||!c->island->dirty||!c->reference||!c->reference->identity||
  !c->reference->allocation_marker||!c->reference->references||c->reference->allocation_marker==c->reference->references||
  !h||!h->preflight||!h->info||!h->remove_info||!h->destroy_reference)return UC_COLLISION_ARGUMENT;
 UCCollisionConstraintRecord *record=c->record;if(record->owner_side>1)return UC_COLLISION_ARGUMENT;
 UCCollisionConstraintBody *owner=record->body[record->owner_side],*other=record->body[1-record->owner_side];
 if(!body_valid(owner)||!body_valid(other)||!owner->count)return UC_COLLISION_ARGUMENT;
 uintptr_t offset=(uintptr_t)record-(uintptr_t)owner->records;
 if(offset%sizeof(*record)||offset/sizeof(*record)>=owner->count||record->constraint!=c)return UC_COLLISION_ARGUMENT;
 uint32_t bytes=0;
 for(uint32_t i=0;i<owner->count;i++) {
  UCCollisionConstraintRecord *r=owner->records+i;
  if(!r->constraint||r->constraint->active||r->constraint->record!=r||r->owner_side>1||
   r->body[r->owner_side]!=owner||!r->data)return UC_COLLISION_ARGUMENT;
  UCCollisionConstraintBody *b=r->body[1-r->owner_side];
  if(!body_valid(b)||r->other_index>=b->other_count||b->other[r->other_index]!=r->constraint)return UC_COLLISION_ARGUMENT;
  if(r->runtime) {
   if(!owner->runtime||bytes>owner->runtime_count||r->runtime_size>owner->runtime_count-bytes||
    r->runtime!=owner->runtime+bytes)return UC_COLLISION_ARGUMENT;
   bytes+=r->runtime_size;
  }
 }
 if(bytes!=owner->runtime_count)return UC_COLLISION_ARGUMENT;
 for(uint32_t i=0;i<other->other_count;i++) {
  UCCollisionConstraint *q=other->other[i];
  if(!q||!q->record||q->record->owner_side>1||q->record->constraint!=q||
   q->record->body[1-q->record->owner_side]!=other||q->record->other_index!=i)return UC_COLLISION_ARGUMENT;
 }
 return h->preflight(ctx,c);
}
UCCollisionStatus uc_collision_constraint_remove(UCCollisionConstraint *c,const UCCollisionConstraintHooks *h,void *ctx,int *applied) {
 if(!applied)return UC_COLLISION_ARGUMENT;
 UCCollisionStatus status=uc_collision_constraint_validate(c,h,ctx);if(status)return status;
 UCCollisionConstraintRecord *r=c->record;UCCollisionConstraintBody *owner=r->body[r->owner_side],*other=r->body[1-r->owner_side];
 *applied=1;c->active=owner->active=other->active=1;*c->island->dirty=1;
 uint32_t info[4];h->info(ctx,r->data,info);h->remove_info(ctx,c->island,c,info);c->island=NULL;
 UCCollisionConstraint *tail=other->other[other->other_count-1];
 other->other[r->other_index]=tail;--other->other_count;tail->record->other_index=r->other_index;
 uint16_t removed=0;
 if(r->runtime) {
  removed=r->runtime_size;size_t offset=(size_t)(r->runtime-owner->runtime);
  memmove(r->runtime,r->runtime+removed,owner->runtime_count-offset-removed);
  owner->runtime_count-=removed;r->runtime=NULL;
 }
 c->record=NULL;uint32_t index=(uint32_t)(r-owner->records);
 for(uint32_t i=index;i+1<owner->count;i++) {
  memcpy(owner->records+i,owner->records+i+1,sizeof(*r));
  UCCollisionConstraintRecord *m=owner->records+i;m->constraint->record=m;if(m->runtime)m->runtime-=removed;
 }
 --owner->count;c->active=owner->active=other->active=0;
 UCCollisionContactReference *ref=c->reference;
 if(*ref->allocation_marker){--*ref->references;if(!*ref->references)h->destroy_reference(ctx,ref->identity);}
 return UC_COLLISION_OK;
}
