#include "collision_constraint_install.h"
#include <string.h>
static int reference_valid(const UCCollisionContactReference *r) {
 return r&&r->identity&&r->allocation_marker&&r->references&&r->allocation_marker!=r->references;
}
static void retain(UCCollisionContactReference *r){if(*r->allocation_marker)++*r->references;}
static int references_valid(UCCollisionContactReference *const *refs,unsigned count) {
 for(unsigned i=0;i<count;i++){
  if(!reference_valid(refs[i]))return 0;
  for(unsigned j=0;j<i;j++)if(refs[i]->identity==refs[j]->identity&&
   (refs[i]->allocation_marker!=refs[j]->allocation_marker||refs[i]->references!=refs[j]->references))return 0;
 }return 1;
}
UCCollisionStatus uc_collision_constraint_construct(UCCollisionConstraintDefinition *d,UCCollisionConstraint *c,
 UCCollisionConstraintEndpoint *a,UCCollisionConstraintEndpoint *b,UCCollisionConstraintData *data,uint32_t priority) {
 if(!d||!c||c->active||c->island||c->record||!a||!b||!a->storage||!b->storage||!data||!data->identity)return UC_COLLISION_ARGUMENT;
 UCCollisionContactReference *refs[]={c->reference,data->reference,a->reference,b->reference};
 if(!references_valid(refs,4)||data->reference->identity!=data->identity)return UC_COLLISION_ARGUMENT;
 *d=(UCCollisionConstraintDefinition){c,{a,b},data,(uint8_t)priority,0,{0,0}};
 *c->reference->references=1;c->island=NULL;c->record=NULL;
 retain(data->reference);retain(a->reference);retain(b->reference);return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_constraint_priority(UCCollisionConstraintDefinition *d,uint32_t priority) {
 if(!d||!d->constraint||d->constraint->active||
  (d->constraint->record&&d->constraint->record->constraint!=d->constraint))return UC_COLLISION_ARGUMENT;
 d->priority=(uint8_t)priority;if(d->constraint->record)d->constraint->record->priority=(uint8_t)priority;return UC_COLLISION_OK;
}
void uc_collision_constraint_default_runtime(uint32_t requested,uint32_t info[2]){(void)requested;info[0]=info[1]=0;}
void uc_collision_constraint_default_initialize(void *constraint,void *runtime,uint32_t bytes){(void)constraint;if(runtime)memset(runtime,0,bytes);}
static uint32_t signed_max(uint32_t a,uint32_t b){return (a^0x80000000u)>(b^0x80000000u)?a:b;}
void uc_collision_constraint_default_add(UCCollisionConstraintIsland *is,uint32_t *high,const uint32_t info[4]) {
 *high=signed_max(*high,info[0]);*high=signed_max(*high,info[1]);
 for(unsigned i=0;i<3;i++)*is->counter[i]+=info[i+1];
}
static int body_valid(UCCollisionConstraintBody *b) {
 return b&&!b->active&&b->capacity<=65536&&b->count<=b->capacity&&(!b->capacity||b->records)&&
  b->other_capacity<=65536&&b->other_count<=b->other_capacity&&(!b->other_capacity||b->other)&&
  b->runtime_count<=b->runtime_capacity&&(!b->runtime_capacity||b->runtime);
}
static int graph_valid(UCCollisionConstraintBody *b) {
 if(!body_valid(b))return 0;uint32_t bytes=0;
 for(uint32_t i=0;i<b->count;i++) {
  UCCollisionConstraintRecord *r=b->records+i;
  if(!r->constraint||r->constraint->active||r->constraint->record!=r||r->owner_side>1||r->body[r->owner_side]!=b||!r->data)return 0;
  UCCollisionConstraintBody *other=r->body[1-r->owner_side];
  if(!body_valid(other)||r->other_index>=other->other_count||other->other[r->other_index]!=r->constraint)return 0;
  if(r->runtime){if(!b->runtime||bytes>b->runtime_count||r->runtime_size>b->runtime_count-bytes||r->runtime!=b->runtime+bytes)return 0;bytes+=r->runtime_size;}
 }
 if(bytes!=b->runtime_count)return 0;
 for(uint32_t i=0;i<b->other_count;i++) {
  UCCollisionConstraint *c=b->other[i];
  if(!c||c->active||!c->record||c->record->constraint!=c||c->record->owner_side>1||
   c->record->body[1-c->record->owner_side]!=b||c->record->other_index!=i)return 0;
 }return 1;
}
static unsigned owner_side(const UCCollisionConstraintDefinition *d,const UCCollisionConstraintInstallBody s[2]) {
 if(s[0].fixed!=s[1].fixed)return s[0].fixed;
 uint32_t a=d->body[0]->storage->count,b=d->body[1]->storage->count;
 return a+b<8?a>=b:a<=b;
}
UCCollisionStatus uc_collision_constraint_install_validate(UCCollisionConstraintDefinition *d,const UCCollisionConstraintInstallHooks *h,void *ctx) {
 if(!d||!d->constraint||d->constraint->active||d->constraint->record||d->constraint->island||
  !d->body[0]||!d->body[1]||!d->data||!d->data->identity||!d->data->contact_count||!h||!h->preflight||!h->body)return UC_COLLISION_ARGUMENT;
 UCCollisionContactReference *refs[]={d->constraint->reference,d->data->reference,d->body[0]->reference,d->body[1]->reference};
 if(!references_valid(refs,4)||d->data->reference->identity!=d->data->identity)return UC_COLLISION_ARGUMENT;
 UCCollisionConstraintInstallBody s[2];
 for(unsigned i=0;i<2;i++) {
  if(!graph_valid(d->body[i]->storage))return UC_COLLISION_ARGUMENT;s[i]=h->body(ctx,d->body[i]);
  if(s[i].fixed>1||!s[i].island||!s[i].high_water||!s[i].island->dirty)return UC_COLLISION_ARGUMENT;
  for(unsigned j=0;j<3;j++)if(!s[i].island->counter[j])return UC_COLLISION_ARGUMENT;
 }
 if(d->body[0]->reference->identity==d->body[1]->reference->identity&&d->body[0]->storage!=d->body[1]->storage)return UC_COLLISION_ARGUMENT;
 if(s[0].island==s[1].island&&s[0].high_water!=s[1].high_water)return UC_COLLISION_ARGUMENT;
 if(d->body[0]->storage==d->body[1]->storage&&(d->body[0]->reference->identity!=d->body[1]->reference->identity||
  s[0].island!=s[1].island||s[0].high_water!=s[1].high_water||s[0].fixed!=s[1].fixed))return UC_COLLISION_ARGUMENT;
 if(!s[0].fixed&&!s[1].fixed&&s[0].island!=s[1].island)return UC_COLLISION_UNSUPPORTED;
 unsigned side=owner_side(d,s);UCCollisionConstraintBody *owner=d->body[side]->storage,*other=d->body[1-side]->storage;
 if(owner->count==owner->capacity||other->other_count==other->other_capacity)return UC_COLLISION_LIMIT;
 return h->preflight(ctx,d);
}
UCCollisionStatus uc_collision_constraint_install(UCCollisionConstraintDefinition *d,const UCCollisionConstraintInstallHooks *h,
 void *ctx,const UCCollisionConstraintRecord *scratch,int *applied) {
 if(!scratch||!applied)return UC_COLLISION_ARGUMENT;
 UCCollisionStatus status=uc_collision_constraint_install_validate(d,h,ctx);if(status)return status;
 UCCollisionConstraintInstallBody s[2]={h->body(ctx,d->body[0]),h->body(ctx,d->body[1])};unsigned side=owner_side(d,s);
 UCCollisionConstraintBody *owner=d->body[side]->storage,*other=d->body[1-side]->storage;UCCollisionConstraint *c=d->constraint;
 *applied=1;c->active=owner->active=other->active=1;retain(c->reference);c->island=s[side].island;
 uint32_t info[4];uc_collision_constraint_default_info(*d->data->contact_count,info);uc_collision_constraint_default_add(c->island,s[side].high_water,info);
 uint32_t index=0;while(index<owner->count&&owner->records[index].priority<=d->priority)++index;
 memmove(owner->records+index+1,owner->records+index,(owner->count-index)*sizeof(*owner->records));
 UCCollisionConstraintRecord *r=owner->records+index;memcpy(r,scratch,sizeof(*r));
 r->constraint=c;r->body[0]=d->body[0]->storage;r->body[1]=d->body[1]->storage;r->data=d->data->identity;r->priority=d->priority;r->owner_side=(uint8_t)side;++owner->count;
 for(uint32_t i=index;i<owner->count;i++)owner->records[i].constraint->record=owner->records+i;
 uint32_t runtime[2];uc_collision_constraint_default_runtime(d->runtime_requested,runtime);r->runtime_size=(uint16_t)runtime[0];r->runtime=NULL;
 uc_collision_constraint_default_initialize(c->reference->identity,r->runtime,r->runtime_size);
 r->other_index=(uint16_t)other->other_count;other->other[other->other_count++]=c;
 c->active=owner->active=other->active=0;return UC_COLLISION_OK;
}
