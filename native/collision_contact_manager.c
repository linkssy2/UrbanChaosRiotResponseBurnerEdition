#include "collision_contact_manager.h"
static const uint32_t strides[3]={20,32,1};
static UCCollisionContactReference *reference(UCCollisionContactManager *m,unsigned i) {
 return i<2?m->body[i]:m->constraint_data;
}
UCCollisionStatus uc_collision_contact_manager_validate(UCCollisionContactManager *m,
 const UCCollisionContactManagerHooks *h,void *ctx) {
 if(!m)return UC_COLLISION_OK;
 if(m->active||m->kind!=UC_COLLISION_CONTACT_MANAGER||m->constraint_kind!=UC_COLLISION_CONTACT_CONSTRAINT||
  m->data_kind!=UC_COLLISION_CONTACT_DATA||m->allocation_size<sizeof(*m)||!h||!h->preflight||!h->destroy_reference||!h->release)return UC_COLLISION_ARGUMENT;
 if(m->arrays[0].count&&!h->remove_constraint)return UC_COLLISION_UNSUPPORTED;
 for(unsigned i=0;i<3;i++) {
  UCCollisionContactArray *a=m->arrays+i;uint32_t cap=a->capacity_flags&0x3fffffffu;
  if(a->count>cap||(!(a->capacity_flags&0x80000000u)&&((cap&&!a->data)||cap>UINT32_MAX/strides[i])))return UC_COLLISION_ARGUMENT;
  UCCollisionContactReference *r=reference(m,i);if(!r){if(i==2)return UC_COLLISION_ARGUMENT;continue;}
  if(!r->identity||!r->allocation_marker||!r->references||r->allocation_marker==r->references)return UC_COLLISION_ARGUMENT;
  for(unsigned j=0;j<3;j++){UCCollisionContactReference *q=reference(m,j);if(q&&q->identity==r->identity){
   if(q->references!=r->references||q->allocation_marker!=r->allocation_marker)return UC_COLLISION_ARGUMENT;}}
 }
 return h->preflight(ctx,m);
}
UCCollisionStatus uc_collision_contact_manager_destroy(UCCollisionContactManager *m,
 const UCCollisionContactManagerHooks *h,void *ctx,int *applied) {
 if(!applied)return UC_COLLISION_ARGUMENT;
 UCCollisionStatus status=uc_collision_contact_manager_validate(m,h,ctx);if(status)return status;
 if(!m){*applied=1;return UC_COLLISION_OK;}
 *applied=1;m->active=1;m->kind=UC_COLLISION_CONTACT_MANAGER;
 if(m->arrays[0].count)h->remove_constraint(ctx,m);
 m->constraint_kind=UC_COLLISION_CONTACT_CONSTRAINT;
 for(unsigned i=0;i<3;i++){
  UCCollisionContactReference *r=reference(m,i);
  if(r&&*r->allocation_marker){--*r->references;if(!*r->references)h->destroy_reference(ctx,r->identity);}
 }
 m->constraint_kind=UC_COLLISION_CONTACT_BASE;
 for(unsigned i=0;i<3;i++){
  UCCollisionContactArray *a=m->arrays+i;
  if(!(a->capacity_flags&0x80000000u))h->release(ctx,a->data,(size_t)(a->capacity_flags&0x3fffffffu)*strides[i],0x14);
 }
 m->data_kind=UC_COLLISION_CONTACT_BASE;m->kind=UC_COLLISION_CONTACT_BASE;
 h->release(ctx,m,m->allocation_size,0x1F);return UC_COLLISION_OK;
}
